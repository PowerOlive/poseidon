#ifndef POSEIDON_TCP_SERVER_HPP_
#define POSEIDON_TCP_SERVER_HPP_

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "raii.hpp"

namespace Poseidon {

// 抽象工厂模式
class TcpServerBase : boost::noncopyable {
private:
	class SslImplServer;
	class SslImplClient;

private:
	std::string m_bindAddr;
	ScopedFile m_listen;
	boost::scoped_ptr<SslImplServer> m_sslImplServer;

public:
	TcpServerBase(const std::string &bindAddr, unsigned bindPort,
		const std::string &cert, const std::string &privateKey);
	virtual ~TcpServerBase();

protected:
	// 工厂函数。
	// 如果该成员函数返回空指针，连接会被立即挂断。
	virtual boost::shared_ptr<class TcpSessionBase>
		onClientConnect(Move<ScopedFile> client) const = 0;

public:
	boost::shared_ptr<TcpSessionBase> tryAccept() const;
};

}

#endif
