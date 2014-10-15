#include "../precompiled.hpp"
#include "utilities.hpp"
#include <signal.h>
#include <libgen.h>
#include <unistd.h>
#include "log.hpp"
#include "exception.hpp"
#include "singletons/config_file.hpp"
#include "singletons/timer_daemon.hpp"
#include "singletons/mysql_daemon.hpp"
#include "singletons/epoll_daemon.hpp"
#include "singletons/job_dispatcher.hpp"
#include "singletons/player_servlet_manager.hpp"
#include "singletons/http_servlet_manager.hpp"
#include "singletons/websocket_servlet_manager.hpp"
#include "singletons/module_manager.hpp"
#include "singletons/event_listener_manager.hpp"
#include "singletons/profile_manager.hpp"
#include "player/server.hpp"
#include "http/server.hpp"
#include "profiler.hpp"
using namespace Poseidon;

namespace {

void sigTermProc(int){
	LOG_WARNING("Received SIGTERM, will now exit...");
	JobDispatcher::quitModal();
}

void sigIntProc(int){
	static const unsigned long long SAFETY_TIMER_EXPIRES = 300 * 1000000;
	static unsigned long long s_safetyTimer = 0;

	// 系统启动的时候这个时间是从 0 开始的，如果这时候按下 Ctrl+C 就会立即终止。
	// 因此将计时器的起点设为该区间以外。
	const unsigned long long now = getMonoClock() + SAFETY_TIMER_EXPIRES + 1;
	if(s_safetyTimer + SAFETY_TIMER_EXPIRES < now){
		s_safetyTimer = now + 5 * 1000000;
	}
	if(now < s_safetyTimer){
		LOG_WARNING("Received SIGINT, trying to exit gracefully...");
		LOG_WARNING("If I don't terminate in 5 seconds, press ^C again.");
		::raise(SIGTERM);
	} else {
		LOG_FATAL("Received SIGINT, will now terminate abnormally...");
		::raise(SIGKILL);
	}
}

template<typename T>
struct RaiiSingletonRunner : boost::noncopyable {
	RaiiSingletonRunner(){
		T::start();
	}
	~RaiiSingletonRunner(){
		T::stop();
	}
};

void run(){
	PROFILE_ME;

	unsigned logLevel = Log::LV_INFO;
	ConfigFile::get(logLevel, "log_level");
	LOG_INFO("Setting log level to ", logLevel, "...");
	Log::setLevel(logLevel);

	const RaiiSingletonRunner<MySqlDaemon> UNIQUE_ID;
	const RaiiSingletonRunner<TimerDaemon> UNIQUE_ID;
	const RaiiSingletonRunner<EpollDaemon> UNIQUE_ID;

	const RaiiSingletonRunner<PlayerServletManager> UNIQUE_ID;
	const RaiiSingletonRunner<HttpServletManager> UNIQUE_ID;
	const RaiiSingletonRunner<WebSocketServletManager> UNIQUE_ID;
	const RaiiSingletonRunner<ModuleManager> UNIQUE_ID;
	const RaiiSingletonRunner<EventListenerManager> UNIQUE_ID;

	LOG_INFO("Waiting for all MySQL operations to complete...");
	MySqlDaemon::waitForAllAsyncOperations();

	LOG_INFO("Creating player server...");
	std::string bind("0.0.0.0");
	boost::uint16_t port = 0;
	std::string certificate;
	std::string privateKey;
	ConfigFile::get(bind, "socket_bind");
	ConfigFile::get(port, "socket_port");
	ConfigFile::get(certificate, "socket_certificate");
	ConfigFile::get(privateKey, "socket_private_key");
	EpollDaemon::addTcpServer(
		boost::make_shared<PlayerServer>(bind, port, certificate, privateKey));

	LOG_INFO("Creating HTTP server...");
	bind = "0.0.0.0";
	port = 0;
	certificate.clear();
	privateKey.clear();
	ConfigFile::get(bind, "http_bind");
	ConfigFile::get(port, "http_port");
	ConfigFile::get(certificate, "http_certificate");
	ConfigFile::get(privateKey, "http_private_key");
	EpollDaemon::addTcpServer(
		boost::make_shared<HttpServer>(bind, port, certificate, privateKey));

	LOG_INFO("Entering modal loop...");
	JobDispatcher::doModal();
}

}

int main(int argc, char **argv){
	Log::setThreadTag(Log::TAG_PRIMARY);
	LOG_INFO("-------------------------- Starting up -------------------------");

	const RaiiSingletonRunner<ProfileManager> UNIQUE_ID;

	try {
		LOG_INFO("Setting up signal handlers...");
		::signal(SIGINT, sigIntProc);
		::signal(SIGTERM, sigTermProc);

		const char *confPath = "/var/poseidon/config/conf.rc";
		if(1 < argc){
			confPath = argv[1];
		}
		ConfigFile::reload(confPath);

		std::string confDir(confPath);
		char *const realDir = ::dirname(&confDir[0]);
		if(realDir != confDir.c_str()){
			confDir = realDir;
		}
		LOG_INFO("Setting working directory to ", confDir, "...");
		if(::chdir(confDir.c_str()) != 0){
			LOG_FATAL("Error setting working directory.");
			std::abort();
		}

		run();

		LOG_INFO("------------- Server has been shut down gracefully -------------");
		return EXIT_SUCCESS;
	} catch(std::exception &e){
		LOG_ERROR("std::exception thrown in main(): what = ", e.what());
	} catch(...){
		LOG_ERROR("Unknown exception thrown in main().");
	}

	LOG_INFO("----------------- Server has exited abnormally -----------------");
	return EXIT_FAILURE;
}
