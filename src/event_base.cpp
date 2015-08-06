// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2015, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "event_base.hpp"
#include "singletons/event_dispatcher.hpp"

namespace Poseidon {

EventBaseWithoutId::~EventBaseWithoutId(){
}

void syncRaiseEvent(const boost::shared_ptr<EventBaseWithoutId> &event){
	EventDispatcher::syncRaise(event);
}
void asyncRaiseEvent(const boost::shared_ptr<EventBaseWithoutId> &event, const boost::shared_ptr<const bool> &withdrawn){
	EventDispatcher::asyncRaise(event, withdrawn);
}

}
