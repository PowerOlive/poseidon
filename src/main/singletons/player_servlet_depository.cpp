// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "player_servlet_depository.hpp"
#include <map>
#include <boost/noncopyable.hpp>
#include <boost/ref.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include "main_config.hpp"
#include "../log.hpp"
#include "../exception.hpp"
using namespace Poseidon;

struct Poseidon::PlayerServlet : boost::noncopyable {
	const boost::uint16_t protocolId;
	const boost::shared_ptr<const PlayerServletCallback> callback;

	PlayerServlet(boost::uint16_t protocolId_, boost::shared_ptr<const PlayerServletCallback> callback_)
		: protocolId(protocolId_), callback(STD_MOVE(callback_))
	{
	}
};

namespace {

std::size_t g_maxRequestLength = 16 * 0x400;

typedef std::map<std::size_t,
	std::map<boost::uint16_t, boost::weak_ptr<const PlayerServlet> >
	> ServletMap;

boost::shared_mutex g_mutex;
ServletMap g_servlets;

}

void PlayerServletDepository::start(){
	LOG_POSEIDON_INFO("Starting player servlet manager...");

	AUTO_REF(conf, MainConfig::getConfigFile());

	conf.get(g_maxRequestLength, "player_max_request_length");
	LOG_POSEIDON_DEBUG("Max request length = ", g_maxRequestLength);
}
void PlayerServletDepository::stop(){
	LOG_POSEIDON_INFO("Unloading all player servlets...");

	ServletMap servlets;
	{
		const boost::unique_lock<boost::shared_mutex> ulock(g_mutex);
		servlets.swap(servlets);
	}
}

std::size_t PlayerServletDepository::getMaxRequestLength(){
	return g_maxRequestLength;
}

boost::shared_ptr<PlayerServlet> PlayerServletDepository::registerServlet(
	std::size_t category, boost::uint16_t protocolId, PlayerServletCallback callback)
{
	AUTO(sharedCallback, boost::make_shared<PlayerServletCallback>());
	sharedCallback->swap(callback);
	AUTO(servlet, boost::make_shared<PlayerServlet>(protocolId, sharedCallback));
	{
		const boost::unique_lock<boost::shared_mutex> ulock(g_mutex);
		AUTO_REF(old, g_servlets[category][protocolId]);
		if(!old.expired()){
			LOG_POSEIDON_ERROR("Duplicate servlet for id ", protocolId, " in category ", category);
			DEBUG_THROW(Exception, "Duplicate player protocol servlet");
		}
		old = servlet;
	}
	LOG_POSEIDON_DEBUG("Craeted servlet for protocol ", protocolId, " in category ", category);
	return servlet;
}

boost::shared_ptr<const PlayerServletCallback> PlayerServletDepository::getServlet(
	std::size_t category, boost::uint16_t protocolId)
{
    const boost::shared_lock<boost::shared_mutex> slock(g_mutex);
    const AUTO(it, g_servlets.find(category));
    if(it == g_servlets.end()){
        LOG_POSEIDON_DEBUG("No servlet in category ", category);
        return VAL_INIT;
    }
    const AUTO(it2, it->second.find(protocolId));
    if(it2 == it->second.end()){
    	LOG_POSEIDON_DEBUG("No servlet for protocol ", protocolId, " in category ", category);
    	return VAL_INIT;
    }
    const AUTO(servlet, it2->second.lock());
    if(!servlet){
    	LOG_POSEIDON_DEBUG("Expired servlet for protocol ", protocolId, " in category ", category);
    	return VAL_INIT;
    }
    return servlet->callback;
}