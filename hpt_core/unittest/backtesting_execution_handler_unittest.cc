
#include "gtest/gtest.h"
#include <list>
#include "backtesting/execution_handler.h"
#include "backtesting/backtesting_mail_box.h"
#include "unittest_helper.h"

class UnittestMailBox {
 public:
  template <typename CLASS, typename ARG>
  void Subscribe(void (CLASS::*pfn)(ARG), CLASS* c) {
    std::function<void(ARG)> fn = std::bind(pfn, c, std::placeholders::_1);
    subscribers_.insert({typeid(ARG), std::move(fn)});
  }

  template <typename ARG>
  void Send(const ARG& arg) {
    auto range = subscribers_.equal_range(typeid(arg));
    for (auto it = range.first; it != range.second; ++it) {
      boost::any_cast<std::function<void(const ARG&)>>(it->second)(arg);
    }
  }

 private:
  std::unordered_multimap<std::type_index, boost::any> subscribers_;
};

class TestEventFactory {
 public:
  TestEventFactory(UnittestMailBox* mail_box) : mail_box_(mail_box) {
    mail_box_->Subscribe(&TestEventFactory::EnqueueRtnOrderEvent, this);
  }

  void EnqueueRtnOrderEvent(const std::shared_ptr<OrderField>& order) {
    orders_.push_back(order);
  }

  std::shared_ptr<OrderField> PopupRntOrder() {
    if (orders_.empty()) {
      return std::shared_ptr<OrderField>();
    }
    auto order = orders_.front();
    orders_.pop_front();
    return std::move(order);
  }

  mutable std::list<std::shared_ptr<OrderField>> orders_;

 private:
  UnittestMailBox* mail_box_;
};

TEST(BacktestingExecutionHandler, OpenBuyOrder) {
  UnittestMailBox mail_box;
  TestEventFactory event_factory(&mail_box);
  SimulatedExecutionHandler<UnittestMailBox> execution_handler(&mail_box);

  mail_box.Send(InputOrder{"S1", PositionEffect::kOpen, OrderDirection::kBuy,
                           1.1, 10, 0});
  {
    auto order = event_factory.PopupRntOrder();

    EXPECT_EQ(true, order != nullptr);
    EXPECT_EQ(PositionEffect::kOpen, order->position_effect);
    EXPECT_EQ(OrderStatus::kActive, order->status);
    EXPECT_EQ(0, order->traded_qty);
  }

  mail_box.Send(InputOrder{"S1", PositionEffect::kOpen, OrderDirection::kBuy,
                           0.9, 10, 0});

  {
    auto order = event_factory.PopupRntOrder();

    EXPECT_EQ(true, order != nullptr);
    EXPECT_EQ(PositionEffect::kOpen, order->position_effect);
    EXPECT_EQ(OrderStatus::kActive, order->status);
    EXPECT_EQ(0, order->traded_qty);
  }

  EXPECT_EQ(true, event_factory.PopupRntOrder() == nullptr);
  execution_handler.HandleTick(MakeTick("S1", 1.2, 200));
  EXPECT_EQ(true, event_factory.PopupRntOrder() == nullptr);

  execution_handler.HandleTick(MakeTick("S1", 1.0, 200));

  {
    auto order = event_factory.PopupRntOrder();
    EXPECT_EQ(true, order != nullptr);
    EXPECT_EQ(PositionEffect::kOpen, order->position_effect);
    EXPECT_EQ(OrderDirection::kBuy, order->direction);
    EXPECT_EQ(OrderStatus::kAllFilled, order->status);
    EXPECT_EQ(1.1, order->price);
    EXPECT_EQ(10, order->traded_qty);
  }

  execution_handler.HandleTick(MakeTick("S1", 1.0, 200));

  EXPECT_EQ(true, event_factory.PopupRntOrder() == nullptr);

  execution_handler.HandleTick(MakeTick("S1", 0.9, 100));
  {
    auto order = event_factory.PopupRntOrder();
    EXPECT_EQ(true, order != nullptr);
    EXPECT_EQ(0.9, order->price);
    EXPECT_EQ(10, order->traded_qty);
  }

  execution_handler.HandleTick(MakeTick("S1", 0.9, 100));
  EXPECT_EQ(true, event_factory.PopupRntOrder() == nullptr);
}

TEST(BacktestingExecutionHandler, OpenSellOrder) {
  UnittestMailBox mail_box;
  TestEventFactory event_factory(&mail_box);
  SimulatedExecutionHandler<UnittestMailBox> execution_handler(&mail_box);

  mail_box.Send(InputOrder{"S1", PositionEffect::kOpen, OrderDirection::kSell,
                           1.1, 10, 0});
  {
    auto order = event_factory.PopupRntOrder();

    EXPECT_EQ(true, order != nullptr);
    EXPECT_EQ(PositionEffect::kOpen, order->position_effect);
    EXPECT_EQ(OrderStatus::kActive, order->status);
    EXPECT_EQ(0, order->traded_qty);
  }

  mail_box.Send(InputOrder{"S1", PositionEffect::kOpen, OrderDirection::kSell,
                           0.9, 10, 0});

  {
    auto order = event_factory.PopupRntOrder();

    EXPECT_EQ(true, order != nullptr);
    EXPECT_EQ(PositionEffect::kOpen, order->position_effect);
    EXPECT_EQ(OrderStatus::kActive, order->status);
    EXPECT_EQ(0, order->traded_qty);
  }

  EXPECT_EQ(true, event_factory.PopupRntOrder() == nullptr);
  execution_handler.HandleTick(MakeTick("S1", 0.8, 200));
  EXPECT_EQ(true, event_factory.PopupRntOrder() == nullptr);

  execution_handler.HandleTick(MakeTick("S1", 1.0, 200));

  {
    auto order = event_factory.PopupRntOrder();
    EXPECT_EQ(true, order != nullptr);
    EXPECT_EQ(PositionEffect::kOpen, order->position_effect);
    EXPECT_EQ(OrderDirection::kSell, order->direction);
    EXPECT_EQ(OrderStatus::kAllFilled, order->status);
    EXPECT_EQ(0.9, order->price);
    EXPECT_EQ(10, order->traded_qty);
  }

  execution_handler.HandleTick(MakeTick("S1", 1.0, 200));

  EXPECT_EQ(true, event_factory.PopupRntOrder() == nullptr);

  execution_handler.HandleTick(MakeTick("S1", 1.1, 100));
  {
    auto order = event_factory.PopupRntOrder();
    EXPECT_EQ(true, order != nullptr);
    EXPECT_EQ(1.1, order->price);
    EXPECT_EQ(10, order->traded_qty);
  }

  execution_handler.HandleTick(MakeTick("S1", 0.9, 100));
  EXPECT_EQ(true, event_factory.PopupRntOrder() == nullptr);
}

TEST(BacktestingExecutionHandler, CancelOrder) {
  UnittestMailBox mail_box;
  TestEventFactory event_factory(&mail_box);
  SimulatedExecutionHandler<UnittestMailBox> execution_handler(&mail_box);

  mail_box.Send(InputOrder{"S1", PositionEffect::kOpen, OrderDirection::kSell,
                           1.1, 10, 0});

  auto order = event_factory.PopupRntOrder();
  EXPECT_EQ(PositionEffect::kOpen, order->position_effect);
  EXPECT_EQ(OrderDirection::kSell, order->direction);
  EXPECT_EQ(OrderStatus::kActive, order->status);
  EXPECT_EQ(1.1, order->price);
  EXPECT_EQ(10, order->qty);
  execution_handler.HandleTick(MakeTick("S1", 0.9, 100));
  EXPECT_EQ(true, event_factory.PopupRntOrder() == nullptr);

  mail_box.Send(CancelOrder{order->order_id});
  {
    auto order = event_factory.PopupRntOrder();
    EXPECT_EQ(true, order != nullptr);
    EXPECT_EQ(OrderStatus::kCanceled, order->status);
    EXPECT_EQ(10, order->qty);
    EXPECT_EQ(10, order->leaves_qty);
    EXPECT_EQ(0, order->traded_qty);
  }
}