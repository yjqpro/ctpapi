#include "follow_strategy_servcie_fixture.h"

const char kMasterAccountID[] = "5000";
const char kSlaveAccountID[] = "5001";

FollowStragetyServiceFixture::FollowStragetyServiceFixture()
    : service(kMasterAccountID, kSlaveAccountID, this, 1) {}

void FollowStragetyServiceFixture::CloseOrder(const std::string& instrument,
                                              const std::string& order_no,
                                              OrderDirection direction,
                                              PositionEffect position_effect,
                                              OrderPriceType price_type,
                                              double price,
                                              int quantity) {
  order_inserts.push_back(OrderInsertForTest{instrument, order_no, direction,
                                             position_effect, price, quantity});
}

void FollowStragetyServiceFixture::OpenOrder(const std::string& instrument,
                                             const std::string& order_no,
                                             OrderDirection direction,
                                             OrderPriceType price_type,
                                             double price,
                                             int quantity) {
  order_inserts.push_back(OrderInsertForTest{
      instrument, order_no, direction, PositionEffect::kOpen, price, quantity});
}

void FollowStragetyServiceFixture::CancelOrder(const std::string& order_no) {
  cancel_orders.push_back(order_no);
}

OrderInsertForTest FollowStragetyServiceFixture::PopOrderInsert() {
  OrderInsertForTest order_insert = order_inserts.front();
  order_inserts.pop_front();
  return order_insert;
}

OrderData FollowStragetyServiceFixture::MakeMasterOrderData(
    const std::string& order_no,
    OrderDirection order_direction,
    PositionEffect position_effect,
    OrderStatus status,
    int filled_quantity /*= 0*/,
    int quantity /*= 10*/,
    double order_price /*= 1234.1*/,
    const std::string& instrument /*= "abc"*/,
    const std::string& user_product_info /*= "Q7"*/) {
  return OrderData{
      kMasterAccountID,  // account_id,
      order_no,          // order_id,
      instrument,        // instrument,
      "",                // datetime,
      "q7",              // user_product_info,
      "",
      quantity,                // quanitty,
      filled_quantity,         // filled_quantity,
      0,                       // session_id,
      order_price,             // price,
      order_direction,         // direction
      OrderPriceType::kLimit,  // type
      status,                  // status
      position_effect          // position_effect
  };
}

OrderData FollowStragetyServiceFixture::MakeSlaveOrderData(
    const std::string& order_no,
    OrderDirection order_direction,
    PositionEffect position_effect,
    OrderStatus status,
    int filled_quantity /*= 0*/,
    int quantity /*= 10*/,
    double order_price /*= 1234.1*/,
    const std::string& instrument /*= "abc"*/,
    const std::string& user_product_info /*= kStrategyUserProductInfo*/) {
  return OrderData{
      kSlaveAccountID,           // account_id,
      order_no,                  // order_id,
      instrument,                // instrument,
      "",                        // datetime,
      kStrategyUserProductInfo,  // user_product_info,
      "",                        // Exchange Id
      quantity,                  // quanitty,
      filled_quantity,           // filled_quantity,
      1,                         // session_id,
      order_price,               // price,
      order_direction,           // direction
      OrderPriceType::kLimit,    // type
      status,                    // status
      position_effect            // position_effect
  };
}

std::tuple<OrderInsertForTest, std::vector<std::string>>
FollowStragetyServiceFixture::PushOpenOrderForMaster(
    const std::string& order_id,
    int quantity /*= 10*/,
    OrderDirection direction /*= OrderDirection::kBuy*/) {
  service.HandleRtnOrder(
      MakeMasterOrderData(order_id, direction, PositionEffect::kOpen,
                          OrderStatus::kActive, 0, quantity));

  return PopOrderEffectForTest();
}

std::tuple<OrderInsertForTest, std::vector<std::string>>
FollowStragetyServiceFixture::PushOpenOrderForSlave(
    const std::string& order_id,
    int quantity /*= 10*/,
    OrderDirection direction /*= OrderDirection::kBuy*/) {
  service.HandleRtnOrder(MakeSlaveOrderData(order_id, direction,
                                            PositionEffect::kOpen,
                                            OrderStatus::kActive, 0, quantity));

  return PopOrderEffectForTest();
}

void FollowStragetyServiceFixture::PushOpenOrder(
    const std::string& order_id,
    int quantity /*= 10*/,
    OrderDirection direction /*= OrderDirection::kBuy*/) {
  (void)PushOpenOrderForMaster(order_id, quantity, direction);
  (void)PushOpenOrderForSlave(order_id, quantity, direction);
}

void FollowStragetyServiceFixture::OpenAndFilledOrder(
    const std::string& order_id,
    int quantity /*= 10*/,
    int master_filled_quantity /*= 10*/,
    int slave_filled_quantity /*= 10*/,
    OrderDirection direction /*= OrderDirection::kBuy*/) {
  PushOpenOrder(order_id, quantity, direction);

  service.HandleRtnOrder(MakeMasterOrderData(
      order_id, direction, PositionEffect::kOpen,
      master_filled_quantity == quantity ? OrderStatus::kFilled
                                         : OrderStatus::kActive,
      master_filled_quantity, quantity));

  if (slave_filled_quantity > 0) {
    service.HandleRtnOrder(MakeSlaveOrderData(
        order_id, direction, PositionEffect::kOpen,
        slave_filled_quantity == quantity ? OrderStatus::kFilled
                                          : OrderStatus::kActive,
        slave_filled_quantity, quantity));
  }
}

std::tuple<OrderInsertForTest, std::vector<std::string>>
FollowStragetyServiceFixture::PushNewOpenOrderForMaster(
    const std::string& order_no /*= "0001"*/,
    OrderDirection direction /*= OrderDirection::kBuy*/,
    int quantity /*= 10*/) {
  service.HandleRtnOrder(
      MakeMasterOrderData(order_no, direction, PositionEffect::kOpen,
                          OrderStatus::kActive, 0, quantity));
  return PopOrderEffectForTest();
}

std::tuple<OrderInsertForTest, std::vector<std::string>>
FollowStragetyServiceFixture::PushNewCloseOrderForMaster(
    const std::string& order_id /*= "0002"*/,
    OrderDirection direction /*= OrderDirection::kSell*/,
    int quantity /*= 10*/,
    double price /*= 1234.1*/) {
  service.HandleRtnOrder(
      MakeMasterOrderData(order_id, direction, PositionEffect::kClose,
                          OrderStatus::kActive, 0, quantity, price));

  return PopOrderEffectForTest();
}

std::tuple<OrderInsertForTest, std::vector<std::string>>
FollowStragetyServiceFixture::PushNewCloseOrderForSlave(
    const std::string& order_id /*= "0002"*/,
    OrderDirection direction /*= OrderDirection::kSell*/,
    int quantity /*= 10*/) {
  service.HandleRtnOrder(MakeSlaveOrderData(order_id, direction,
                                            PositionEffect::kClose,
                                            OrderStatus::kActive, 0, quantity));

  return PopOrderEffectForTest();
}

std::tuple<OrderInsertForTest, std::vector<std::string>>
FollowStragetyServiceFixture::PopOrderEffectForTest() {
  std::vector<std::string> ret_cancel_orders;
  ret_cancel_orders.swap(cancel_orders);
  return {!order_inserts.empty() ? PopOrderInsert() : OrderInsertForTest(),
          ret_cancel_orders};
}

std::tuple<OrderInsertForTest, std::vector<std::string>>
FollowStragetyServiceFixture::PushCancelOrderForMaster(
    const std::string& order_no /*= "0001"*/,
    OrderDirection direction /*= OrderDirection::kBuy*/,
    PositionEffect position_effect /*= PositionEffect::kOpen*/,
    int fill_quantity /*= 0*/,
    int quantity /*= 10*/) {
  service.HandleRtnOrder(
      MakeMasterOrderData(order_no, direction, position_effect,
                          OrderStatus::kCancel, fill_quantity, quantity));

  return PopOrderEffectForTest();
}

std::tuple<OrderInsertForTest, std::vector<std::string>>
FollowStragetyServiceFixture::PushCancelOrderForSlave(
    const std::string& order_no /*= "0001"*/,
    OrderDirection direction /*= OrderDirection::kBuy*/,
    PositionEffect position_effect /*= PositionEffect::kOpen*/,
    int fill_quantity /*= 10*/,
    int quantity /*= 10*/) {
  service.HandleRtnOrder(
      MakeSlaveOrderData(order_no, direction, position_effect,
                         OrderStatus::kCancel, fill_quantity, quantity));
  return PopOrderEffectForTest();
}