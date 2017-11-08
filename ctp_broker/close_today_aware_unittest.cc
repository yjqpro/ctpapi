#include "gtest/gtest.h"
#include "ctp_instrument_broker_test.h"

class CloseTodayAwareTest : public CTPInstrumentBrokerTest {
 public:
  CloseTodayAwareTest() {
    broker_.SetPositionEffectStrategy<
        GenericCTPPositionEffectStrategy,
        CloseTodayAwareCTPPositionEffectFlagStrategy>();
  }
};

TEST_F(CloseTodayAwareTest, CloseYesterday) {
  broker_.InitPosition(std::make_pair(10, 5), std::make_pair(0, 0));

  MakeCloseOrderRequest("abc", OrderDirection::kBuy, 1.2, 10);

  auto enter_order = PopupOrder<CTPEnterOrder>();
  ASSERT_TRUE(enter_order);
  EXPECT_EQ(CTPPositionEffect::kClose, enter_order->position_effect);
  EXPECT_EQ(OrderDirection::kBuy, enter_order->direction);
  EXPECT_EQ(10, enter_order->qty);
  EXPECT_EQ(1.2, enter_order->price);
  EXPECT_EQ("I1", enter_order->instrument);
  EXPECT_EQ("0", enter_order->order_id);
}

TEST_F(CloseTodayAwareTest, kCloseToday) {
  broker_.InitPosition(std::make_pair(0, 10), std::make_pair(0, 0));

  MakeCloseOrderRequest("abc", OrderDirection::kBuy, 1.2, 10);

  auto enter_order = PopupOrder<CTPEnterOrder>();
  ASSERT_TRUE(enter_order);
  EXPECT_EQ(CTPPositionEffect::kCloseToday, enter_order->position_effect);
  EXPECT_EQ(OrderDirection::kBuy, enter_order->direction);
  EXPECT_EQ(10, enter_order->qty);
  EXPECT_EQ(1.2, enter_order->price);
  EXPECT_EQ("I1", enter_order->instrument);
  EXPECT_EQ("0", enter_order->order_id);
}

TEST_F(CloseTodayAwareTest, CloseOrderNoEnoughYesterdayQty) {
  broker_.InitPosition(std::make_pair(5, 10), std::make_pair(0, 0));
  MakeCloseOrderRequest("abc", OrderDirection::kBuy, 1.2, 10);
  {
    auto enter_order = PopupOrder<CTPEnterOrder>();
    ASSERT_TRUE(enter_order);
    EXPECT_EQ(CTPPositionEffect::kClose, enter_order->position_effect);
    EXPECT_EQ(OrderDirection::kBuy, enter_order->direction);
    EXPECT_EQ(5, enter_order->qty);
    EXPECT_EQ("0", enter_order->order_id);
  }

  {
    auto enter_order = PopupOrder<CTPEnterOrder>();
    ASSERT_TRUE(enter_order);
    EXPECT_EQ(CTPPositionEffect::kCloseToday, enter_order->position_effect);
    EXPECT_EQ(OrderDirection::kBuy, enter_order->direction);
    EXPECT_EQ(5, enter_order->qty);
    EXPECT_EQ("1", enter_order->order_id);
  }
}