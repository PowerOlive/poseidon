// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2015, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "session.hpp"
#include <iomanip>
#include "exception.hpp"
#include "../http/session.hpp"
#include "../optional_map.hpp"
#include "../singletons/job_dispatcher.hpp"
#include "../singletons/websocket_adaptor_depository.hpp"
#include "../log.hpp"
#include "../random.hpp"
#include "../endian.hpp"
#include "../job_base.hpp"
#include "../profiler.hpp"

namespace Poseidon {

namespace WebSocket {
	namespace {
		class RequestJob : public JobBase {
		private:
			const boost::weak_ptr<const AdaptorCallback> m_adaptor;
			const boost::weak_ptr<Session> m_session;
			const OpCode m_opcode;
			const StreamBuffer m_payload;

		public:
			RequestJob(boost::weak_ptr<const AdaptorCallback> m_adaptor,
				boost::weak_ptr<Session> session, OpCode opcode, StreamBuffer payload)
				: m_adaptor(STD_MOVE(m_adaptor))
				, m_session(STD_MOVE(session)), m_opcode(opcode), m_payload(STD_MOVE(payload))
			{
			}

		protected:
			boost::weak_ptr<const void> getCategory() const OVERRIDE {
				return m_session;
			}
			void perform() const OVERRIDE {
				PROFILE_ME;

				const AUTO(session, m_session.lock());
				if(!session){
					return;
				}

				try {
					const AUTO(adaptor, m_adaptor.lock());
					if(!adaptor){
						LOG_POSEIDON_WARNING("Adaptor expired: category = ", session->getCategory(), ", URI = ", session->getUri());
						DEBUG_THROW(Exception, ST_GOING_AWAY, SharedNts::observe("Adaptor expired"));
					}

					LOG_POSEIDON_DEBUG("Dispatching packet: URI = ", session->getUri(), ", payload size = ", m_payload.size());
					(*adaptor)(session, m_opcode, m_payload);
					session->setTimeout(WebSocketAdaptorDepository::getKeepAliveTimeout());
				} catch(TryAgainLater &){
					throw;
				} catch(Exception &e){
					LOG_POSEIDON(Logger::SP_MAJOR | Logger::LV_INFO, "WebSocket::Exception thrown in adaptor, URI = ", session->getUri(),
						", statusCode = ", e.statusCode(), ", what = ", e.what());
					try {
						session->shutdown(e.statusCode(), StreamBuffer(e.what()));
					} catch(...){
						session->forceShutdown();
					}
					throw;
				} catch(...){
					LOG_POSEIDON(Logger::SP_MAJOR | Logger::LV_INFO, "Forwarding exception... URI = ", session->getUri());
					try {
						session->shutdown(ST_INTERNAL_ERROR);
					} catch(...){
						session->forceShutdown();
					}
					throw;
				}
			}
		};

		bool isValidUtf8String(const std::string &str){
			PROFILE_ME;

			boost::uint32_t codePoint;
			for(AUTO(it, str.begin()); it != str.end(); ++it){
				codePoint = static_cast<unsigned char>(*it);
				if((codePoint & 0x80) == 0){
					continue;
				}
				const AUTO(bytes, static_cast<unsigned>(__builtin_clz((~codePoint | 1) & 0xFF)) -
					(sizeof(unsigned) - sizeof(unsigned char)) * CHAR_BIT);
				if(bytes - 2 > 2){ // 2, 3, 4
					LOG_POSEIDON_WARNING("Invalid UTF-8 leading byte: bytes = ", bytes);
					return false;
				}
				codePoint &= (0xFFu >> bytes);
				for(unsigned i = 1; i < bytes; ++i){
					++it;
					if(it == str.end()){
						LOG_POSEIDON_WARNING("String is truncated.");
						return false;
					}
					const unsigned trailing = static_cast<unsigned char>(*it);
					if((trailing & 0xC0u) != 0x80u){
						LOG_POSEIDON_WARNING("Invalid UTF-8 trailing byte: trailing = 0x",
							std::hex, std::setw(2), std::setfill('0'), trailing);
						return false;
					}
				}
				if(codePoint > 0x10FFFFu){
					LOG_POSEIDON_WARNING("Invalid UTF-8 code point: codePoint = 0x",
						std::hex, std::setw(6), std::setfill('0'), codePoint);
					return false;
				}
				if(codePoint - 0xD800u < 0x800u){
					LOG_POSEIDON_WARNING("UTF-8 code point is reserved for UTF-16: codePoint = 0x",
						std::hex, std::setw(4), std::setfill('0'), codePoint);
					return false;
				}
			}
			return true;
		}
	}

	Session::Session(const boost::shared_ptr<Http::Session> &parent, boost::weak_ptr<const AdaptorCallback> adaptor)
		: Http::UpgradedSessionBase(parent)
		, m_adaptor(STD_MOVE(adaptor))
		, m_state(S_OPCODE), m_fin(false), m_opcode(OP_INVALID_OPCODE), m_payloadLen(0), m_payloadMask(0)
	{
	}

	void Session::onInitContents(const void * /* data */ , std::size_t /* size */ ){
	}
	void Session::onReadAvail(const void *data, std::size_t size){
		PROFILE_ME;

		try {
			const AUTO(maxRequestLength, WebSocketAdaptorDepository::getMaxRequestLength());

			m_payload.put(data, size);

			for(;;){
				switch(m_state){
					int ch;
					boost::uint16_t temp16;
					boost::uint32_t temp32;
					boost::uint64_t temp64;
					std::size_t remaining;

				case S_OPCODE:
					ch = m_payload.get();
					if(ch == -1){
						goto _exitFor;
					}
					if(ch & (OP_FL_RSV1 | OP_FL_RSV2 | OP_FL_RSV3)){
						LOG_POSEIDON_WARNING("Aborting because some reserved bits are set, opcode = ", ch);
						DEBUG_THROW(Exception, ST_PROTOCOL_ERROR, SharedNts::observe("Reserved bits set"));
					}
					m_fin = ch & OP_FL_FIN;
					m_opcode = static_cast<OpCode>(ch & OP_FL_OPCODE);
					if((m_opcode & OP_FL_CONTROL) && !m_fin){
						DEBUG_THROW(Exception, ST_PROTOCOL_ERROR, SharedNts::observe("Control frame fragemented"));
					}
					m_state = S_PAYLOAD_LEN;
					break;

				case S_PAYLOAD_LEN:
					ch = m_payload.get();
					if(ch == -1){
						goto _exitFor;
					}
					if((ch & 0x80) == 0){
						DEBUG_THROW(Exception, ST_ACCESS_DENIED, SharedNts::observe("Non-masked frames not allowed"));
					}
					m_payloadLen = (unsigned char)(ch & 0x7F);
					if(m_payloadLen >= 0x7E){
						if(m_opcode & OP_FL_CONTROL){
							DEBUG_THROW(Exception, ST_PROTOCOL_ERROR, SharedNts::observe("Control frame too large"));
						}
						m_state = S_EX_PAYLOAD_LEN;
					} else {
						m_state = S_MASK;
					}
					break;

				case S_EX_PAYLOAD_LEN:
					if(m_payloadLen == 0x7E){
						if(m_payload.size() < 2){
							goto _exitFor;
						}
						m_payload.get(&temp16, 2);
						m_payloadLen = loadBe(temp16);
					} else {
						if(m_payload.size() < 8){
							goto _exitFor;
						}
						m_payload.get(&temp64, 8);
						m_payloadLen = loadBe(temp64);
					}
					m_state = S_MASK;
					break;

				case S_MASK:
					LOG_POSEIDON_DEBUG("Payload length = ", m_payloadLen);

					if(m_payload.size() < 4){
						goto _exitFor;
					}
					m_payload.get(&temp32, 4);
					m_payloadMask = loadLe(temp32);
					m_state = S_PAYLOAD;
					break;

				case S_PAYLOAD:
					remaining = m_payloadLen - m_whole.size();
					if(m_whole.size() + remaining > maxRequestLength){
						DEBUG_THROW(Exception, ST_MESSAGE_TOO_LARGE, SharedNts::observe("Packet too large"));
					}
					if(m_payload.size() < remaining){
						goto _exitFor;
					}
					for(std::size_t i = 0; i < remaining; ++i){
						ch = m_payload.get();
						ch ^= static_cast<unsigned char>(m_payloadMask);
						m_payloadMask = (m_payloadMask << 24) | (m_payloadMask >> 8);
						m_whole.put(static_cast<unsigned char>(ch));
					}
					if((m_opcode & OP_FL_CONTROL) != 0){
						onControlFrame();
					} else if(m_fin){
						if(m_opcode == OP_DATA_TEXT){
							LOG_POSEIDON_DEBUG("Validating UTF-8 string...");

							std::string temp;
							m_whole.dump(temp);
							if(!isValidUtf8String(temp)){
								DEBUG_THROW(Exception, ST_INCONSISTENT, SharedNts::observe("Invalid UTF-8 string"));
							}
						}

						onRequest(m_opcode, STD_MOVE(m_whole));
						m_whole.clear();
					}
					m_state = S_OPCODE;
					break;

				default:
					LOG_POSEIDON_FATAL("Invalid state: ", static_cast<unsigned>(m_state));
					std::abort();
				}
			}
		_exitFor:
			;
		} catch(Exception &e){
			LOG_POSEIDON(Logger::SP_MAJOR | Logger::LV_INFO, "Websocket::Exception thrown while parsing data: URI = ", getUri(),
				", statusCode = ", e.statusCode(), ", what = ", e.what());
			try {
				shutdown(e.statusCode(), StreamBuffer(e.what()));
			} catch(...){
				forceShutdown();
			}
			throw;
		} catch(...){
			LOG_POSEIDON(Logger::SP_MAJOR | Logger::LV_INFO, "Forwarding exception... uri = ", getUri());
			try {
				shutdown(ST_INTERNAL_ERROR);
			} catch(...){
				forceShutdown();
			}
			throw;
		}
	}

	void Session::onControlFrame(){
		LOG_POSEIDON_DEBUG("Control frame, opcode = ", m_opcode);

		const AUTO(parent, getSafeParent());

		switch(m_opcode){
		case OP_CLOSE:
			LOG_POSEIDON_INFO("Received close frame from ", parent->getRemoteInfo());
			sendFrame(STD_MOVE(m_whole), OP_CLOSE, true, false);
			break;

		case OP_PING:
			LOG_POSEIDON_INFO("Received ping frame from ", parent->getRemoteInfo());
			sendFrame(STD_MOVE(m_whole), OP_PONG, false, false);
			break;

		case OP_PONG:
			LOG_POSEIDON_INFO("Received pong frame from ", parent->getRemoteInfo());
			break;

		default:
			DEBUG_THROW(Exception, ST_PROTOCOL_ERROR, SharedNts::observe("Invalid opcode"));
			break;
		}
	}

	bool Session::sendFrame(StreamBuffer contents, OpCode opcode, bool fin, bool masked){
		StreamBuffer packet;
		unsigned char ch = opcode | OP_FL_FIN;
		packet.put(ch);
		const std::size_t size = contents.size();
		ch = masked ? 0x80 : 0;
		if(size < 0x7E){
			ch |= size;
			packet.put(ch);
		} else if(size < 0x10000){
			ch |= 0x7E;
			packet.put(ch);
			boost::uint16_t temp;
			storeBe(temp, size);
			packet.put(&temp, 2);
		} else {
			ch |= 0x7F;
			packet.put(ch);
			boost::uint64_t temp;
			storeBe(temp, size);
			packet.put(&temp, 8);
		}
		if(masked){
			boost::uint32_t mask;
			storeLe(mask, rand32() | 0x80808080u);
			packet.put(&mask, 4);
			int ch;
			for(;;){
				ch = contents.get();
				if(ch == -1){
					break;
				}
				ch ^= static_cast<unsigned char>(mask);
				packet.put(ch);
				mask = (mask << 24) | (mask >> 8);
			}
		} else {
			packet.splice(contents);
		}
		return Http::UpgradedSessionBase::send(STD_MOVE(packet), fin);
	}

	void Session::onRequest(OpCode opcode, StreamBuffer contents){
		enqueueJob(boost::make_shared<RequestJob>(
			m_adaptor, virtualWeakFromThis<Session>(), opcode, STD_MOVE(contents)));
	}

	bool Session::send(StreamBuffer contents, bool binary, bool fin, bool masked){
		if(!sendFrame(STD_MOVE(contents), binary ? OP_DATA_BIN : OP_DATA_TEXT, false, masked)){
			return false;
		}
		if(fin){
			return shutdown(ST_NORMAL_CLOSURE);
		}
		return true;
	}
	bool Session::shutdown(StatusCode statusCode, StreamBuffer additional){
		StreamBuffer temp;
		boost::uint16_t codeBe;
		storeBe(codeBe, static_cast<unsigned>(statusCode));
		temp.put(&codeBe, 2);
		temp.splice(additional);
		return sendFrame(STD_MOVE(temp), OP_CLOSE, true, false);
	}
}

}
