#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "caf/all.hpp"
#include "geek_quant/caf_defines.h"
#include "geek_quant/follow_strategy.h"
#include "geek_quant/ctp_trader.h"

using std::endl;
using std::string;

using namespace caf;

behavior StrategyListener(event_based_actor* self,
                          const CtpObserver& observer_actor) {
  self->request(observer_actor, caf::infinite, AddListenerAtom::value,
                actor_cast<strong_actor_ptr>(self));
  return {
      [=](RtnOrderAtom) { caf::aout(self) << "Listener invoker\n"; },
  };
}

int main() {
  // our CAF environment
  actor_system_config cfg;
  actor_system system{cfg};

  auto observer_actor = system.spawn<FollowStrategy>();

  auto listener = system.spawn(StrategyListener, observer_actor);

  CtpTrader ctp_trader(caf::actor_cast<caf::strong_actor_ptr>(observer_actor));

  // auto ctp_broker = boost::make_shared<CTPTradingSpi>(system);
  // ctp_broker->LoginServer("053867", "8661188");
  ctp_trader.LoginServer("tcp://180.168.146.187:10000", "9999", "053867",
                         "8661188");
  // ctp_broker->LoginServer("tcp://59.42.241.91:41205", "9080",
  // "38030022", "140616");

  // system will wait until both actors are destroyed before leaving main
  std::string input;
  while (std::cin >> input) {
    if (input == "exit") {
      break;
    }
  }
}
