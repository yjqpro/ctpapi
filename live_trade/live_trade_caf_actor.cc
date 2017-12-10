#include "live_trade_caf_actor.h"

caf::behavior LiveTradeCafActor::make_behavior() {
  return {[=](const std::shared_ptr<bft::Message>& msg) {
    handlers_.at(msg->TypeIndex())->ApplyMessage(*msg);
  }};
}

void LiveTradeCafActor::Subscribe(
    std::unique_ptr<bft::BasedMessageHandler> handler) {
  handlers_.insert({handler->TypeIndex(), std::move(handler)});
}

void LiveTradeCafActor::Send(std::shared_ptr<bft::Message> message) {
  live_trade_system_->Send(env_id_, std::move(message));
}
