#include "strategy_order_dispatch.h"
#include <boost/lexical_cast.hpp>
#include <boost/log/common.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include "follow_strategy_mode/logging_defines.h"

void StrategyOrderDispatch::OpenOrder(const std::string& strategy_id,
                                      const std::string& instrument,
                                      const std::string& order_id,
                                      OrderDirection direction,
                                      double price,
                                      int quantity) {
  std::string adjust_order_id =
      boost::lexical_cast<std::string>(stragety_orders_.size() + 1);
  stragety_orders_.insert(
      StragetyOrderBiMap::value_type({strategy_id, order_id}, adjust_order_id));
  enter_order_->OpenOrder(instrument, adjust_order_id, direction,
                          price, quantity);
  BOOST_LOG(log_) << boost::log::add_value("strategy_id", strategy_id)
                  << "OpenOrder:" << instrument << "," << order_id << ","
                  << (direction == OrderDirection::kBuy ? "B" : "S") << ","
                  << price << "," << quantity;
}

void StrategyOrderDispatch::CloseOrder(const std::string& strategy_id,
                                       const std::string& instrument,
                                       const std::string& order_id,
                                       OrderDirection direction,
                                       PositionEffect position_effect,
                                       double price,
                                       int quantity) {
  std::string adjust_order_id =
      boost::lexical_cast<std::string>(stragety_orders_.size() + 1);
  stragety_orders_.insert(
      StragetyOrderBiMap::value_type({strategy_id, order_id}, adjust_order_id));
  enter_order_->CloseOrder(instrument, adjust_order_id, direction,
                           position_effect, price, quantity);
  BOOST_LOG(log_) << "CloseOrder:"
                  << boost::log::add_value("strategy_id", strategy_id)
                  << instrument << "," << order_id << ","
                  << (direction == OrderDirection::kBuy ? "B" : "S") << ","
                  << price << "," << quantity;
}

void StrategyOrderDispatch::CancelOrder(const std::string& strategy_id,
                                        const std::string& order_id) {
  auto it = stragety_orders_.left.find(StragetyOrder{strategy_id, order_id});
  if (it != stragety_orders_.left.end()) {
    enter_order_->CancelOrder(order_id);
  }
  BOOST_LOG(log_) << "CancelOrder:"
                  << boost::log::add_value("strategy_id", strategy_id)
                  << order_id;
}

void StrategyOrderDispatch::RtnOrder(OrderField order) {
  auto it = stragety_orders_.right.find(order.order_id);
  if (it != stragety_orders_.right.end()) {
    order.order_id = it->second.order_id;
    BOOST_LOG(log_) << boost::log::add_value("strategy_id", it->second.strategy_id)
                    << "RtnOrder:" << order.instrument_id << ","
                    << order.order_id << ","
                    << (order.direction == OrderDirection::kBuy ? "B" : "S")
                    << "," << order.price;
    rtn_order_observers_[it->second.strategy_id]->RtnOrder(std::move(order));
  } else {
    // Exception or maybe muanul control
  }
}

void StrategyOrderDispatch::SubscribeEnterOrderObserver(
    EnterOrderObserver* observer) {
  enter_order_ = observer;
}

void StrategyOrderDispatch::SubscribeRtnOrderObserver(
    const std::string& strategy_id,
    std::shared_ptr<RtnOrderObserver> observer) {
  if (rtn_order_observers_.find(strategy_id) == rtn_order_observers_.end()) {
    rtn_order_observers_.insert({strategy_id, observer});
  }
}
