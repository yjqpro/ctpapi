#include "gtest/gtest.h"
#include <boost/any.hpp>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>
#include <tuple>
#include "ctp_instrument_broker_test.h"

class GenericPositionEffectTest : public CTPInstrumentBrokerTest {
 public:
  GenericPositionEffectTest() {
    broker_.SetPositionEffectStrategy<GenericCTPPositionEffectStrategy,
                                      GenericCTPPositionEffectFlagStrategy>();
  }
};

TEST_F(GenericPositionEffectTest, OpenOrder) {
  // broker_.HandleInputOrder(InputOrder{"I1", "0", "S1", PositionEffect::kOpen,
  //                                    OrderDirection::kBuy, 1.1, 10, 0});
  MakeOpenOrderRequest("xxxx", OrderDirection::kBuy, 1.1, 10);

  auto enter_order = PopupOrder<CTPEnterOrder>();
  ASSERT_TRUE(enter_order);
  EXPECT_EQ(CTPPositionEffect::kOpen, enter_order->position_effect);
  EXPECT_EQ(OrderDirection::kBuy, enter_order->direction);
  EXPECT_EQ(10, enter_order->qty);
  EXPECT_EQ(1.1, enter_order->price);
  EXPECT_EQ("I1", enter_order->instrument);
  EXPECT_EQ("0", enter_order->order_id);
}

TEST_F(GenericPositionEffectTest, OpenOrderThenRecvCTPRtnOrder) {
  MakeOpenOrderRequest("xx", OrderDirection::kBuy, 1.1, 10);
  Clear();
  SimulateCTPNewOpenOrderField("0", OrderDirection::kBuy, 1.1, 10);

  auto order = PopupOrder<std::shared_ptr<OrderField>>();
  EXPECT_EQ("I1", (*order)->instrument_id);
  EXPECT_EQ(PositionEffect::kOpen, (*order)->position_effect);
  EXPECT_EQ(OrderDirection::kBuy, (*order)->direction);
  EXPECT_EQ(1.1, (*order)->input_price);
  EXPECT_EQ(10, (*order)->qty);
  EXPECT_EQ("xx", (*order)->order_id);
}

TEST_F(GenericPositionEffectTest, OpenOrderThenRecvCTPOrderFill) {
  MakeOpenOrderRequest("0", OrderDirection::kBuy, 1.1, 10);
  SimulateCTPNewOpenOrderField("0", OrderDirection::kBuy, 1.1, 10);
  Clear();
  SimulateCTPTradedOrderField("0", 10);

  auto order = PopupOrder<std::shared_ptr<OrderField>>();
  ASSERT_TRUE(order);
  EXPECT_EQ("I1", (*order)->instrument_id);
  EXPECT_EQ(PositionEffect::kOpen, (*order)->position_effect);
  EXPECT_EQ(OrderDirection::kBuy, (*order)->direction);
  EXPECT_EQ(OrderStatus::kAllFilled, (*order)->status);
  EXPECT_EQ(1.1, (*order)->input_price);
  EXPECT_EQ(10, (*order)->qty);
  EXPECT_EQ(10, (*order)->trading_qty);
  EXPECT_EQ(0, (*order)->leaves_qty);
}

TEST_F(GenericPositionEffectTest,
       OpenOrderThenRecvCTPOrderFillWithTradingPrice) {
  MakeOpenOrderRequest("0", OrderDirection::kBuy, 1.1, 10);
  SimulateCTPNewOpenOrderField("0", OrderDirection::kBuy, 1.1, 10);
  Clear();
  SimulateCTPTradedOrderFieldWithPrice("0", 1.2, 10);

  auto order = PopupOrder<std::shared_ptr<OrderField>>();
  ASSERT_TRUE(order);
  EXPECT_EQ("I1", (*order)->instrument_id);
  EXPECT_EQ(PositionEffect::kOpen, (*order)->position_effect);
  EXPECT_EQ(OrderDirection::kBuy, (*order)->direction);
  EXPECT_EQ(OrderStatus::kAllFilled, (*order)->status);
  EXPECT_EQ(1.1, (*order)->input_price);
  EXPECT_EQ(1.2, (*order)->trading_price);
  EXPECT_EQ(10, (*order)->qty);
  EXPECT_EQ(10, (*order)->trading_qty);
  EXPECT_EQ(0, (*order)->leaves_qty);
}

TEST_F(GenericPositionEffectTest, OpenOrderThenReceMultiCTPOrderField) {
  MakeOpenOrderRequest("0", OrderDirection::kBuy, 1.2, 10);
  SimulateCTPNewOpenOrderField("0", OrderDirection::kBuy, 1.2, 10);
  Clear();
  SimulateCTPTradedOrderField("0", 4);

  {
    auto order = PopupOrder<std::shared_ptr<OrderField>>();
    ASSERT_TRUE(order);
    EXPECT_EQ("I1", (*order)->instrument_id);
    EXPECT_EQ(PositionEffect::kOpen, (*order)->position_effect);
    EXPECT_EQ(OrderDirection::kBuy, (*order)->direction);
    EXPECT_EQ(OrderStatus::kActive, (*order)->status);
    EXPECT_EQ(1.2, (*order)->input_price);
    EXPECT_EQ(10, (*order)->qty);
    EXPECT_EQ(4, (*order)->trading_qty);
    EXPECT_EQ(6, (*order)->leaves_qty);
  }

  SimulateCTPTradedOrderField("0", 6);

  {
    auto order = PopupOrder<std::shared_ptr<OrderField>>();
    ASSERT_TRUE(order);
    EXPECT_EQ("I1", (*order)->instrument_id);
    EXPECT_EQ(PositionEffect::kOpen, (*order)->position_effect);
    EXPECT_EQ(OrderDirection::kBuy, (*order)->direction);
    EXPECT_EQ(OrderStatus::kAllFilled, (*order)->status);
    EXPECT_EQ(1.2, (*order)->input_price);
    EXPECT_EQ(10, (*order)->qty);
    EXPECT_EQ(6, (*order)->trading_qty);
    EXPECT_EQ(0, (*order)->leaves_qty);
  }
}

TEST_F(GenericPositionEffectTest, TestDiffrenceOrderId) {
  MakeOpenOrderRequest("xxxx", OrderDirection::kBuy, 1.1, 10);
  Clear();
  SimulateCTPNewOpenOrderField("0", OrderDirection::kBuy, 1.1, 10);

  auto order = PopupOrder<std::shared_ptr<OrderField>>();
  ASSERT_TRUE(order);
  EXPECT_EQ("xxxx", (*order)->order_id);
}

TEST_F(GenericPositionEffectTest, CloseOrderThenRecvCTPOrder) {
  MakeOpenOrderRequest("0", OrderDirection::kBuy, 1.2, 10);
  SimulateCTPNewOpenOrderField("0", OrderDirection::kBuy, 1.2, 10);
  SimulateCTPTradedOrderField("0", 10);
  Clear();
  MakeCloseOrderRequest("1", OrderDirection::kBuy, 1.3, 10);
  auto enter_order = PopupOrder<CTPEnterOrder>();
  ASSERT_TRUE(enter_order);
  EXPECT_EQ(CTPPositionEffect::kClose, enter_order->position_effect);
  EXPECT_EQ(OrderDirection::kBuy, enter_order->direction);
  EXPECT_EQ(10, enter_order->qty);
  EXPECT_EQ(1.3, enter_order->price);
  EXPECT_EQ("I1", enter_order->instrument);
  EXPECT_EQ("1", enter_order->order_id);
}

TEST_F(GenericPositionEffectTest, InitPosition) {
  broker_.InitPosition(std::make_pair(0, 10), std::make_pair(0, 10));
  MakeCloseOrderRequest("xx", OrderDirection::kBuy, 1.2, 10);
  auto enter_order = PopupOrder<CTPEnterOrder>();
  ASSERT_TRUE(enter_order);
  EXPECT_EQ("0", enter_order->order_id);
  EXPECT_EQ(10, enter_order->qty);
  EXPECT_EQ(1.2, enter_order->price);
}

TEST_F(GenericPositionEffectTest, OpenOrderButComplteLockRightNow) {
  broker_.InitPosition(std::make_pair(0, 10), std::make_pair(0, 10));
  MakeOpenOrderRequest("xx", OrderDirection::kBuy, 1.3, 10);

  auto enter_order = PopupOrder<CTPEnterOrder>();
  ASSERT_TRUE(enter_order);
  EXPECT_EQ("0", enter_order->order_id);
  EXPECT_EQ(CTPPositionEffect::kClose, enter_order->position_effect);
  EXPECT_EQ(OrderDirection::kSell, enter_order->direction);
  EXPECT_EQ(10, enter_order->qty);
  EXPECT_EQ(1.3, enter_order->price);
}

TEST_F(GenericPositionEffectTest, OpenOrderButPartiallyLockRightNow) {
  broker_.InitPosition(std::make_pair(0, 10), std::make_pair(0, 4));
  MakeOpenOrderRequest("xx", OrderDirection::kBuy, 1.3, 10);

  {
    auto enter_order = PopupOrder<CTPEnterOrder>();
    ASSERT_TRUE(enter_order);
    EXPECT_EQ("0", enter_order->order_id);
    EXPECT_EQ(CTPPositionEffect::kClose, enter_order->position_effect);
    EXPECT_EQ(OrderDirection::kSell, enter_order->direction);
    EXPECT_EQ(4, enter_order->qty);
    EXPECT_EQ(1.3, enter_order->price);
  }
  {
    auto enter_order = PopupOrder<CTPEnterOrder>();
    ASSERT_TRUE(enter_order);
    EXPECT_EQ("1", enter_order->order_id);
    EXPECT_EQ(CTPPositionEffect::kOpen, enter_order->position_effect);
    EXPECT_EQ(OrderDirection::kBuy, enter_order->direction);
    EXPECT_EQ(6, enter_order->qty);
    EXPECT_EQ(1.3, enter_order->price);
  }
}