#include "follow_strategy_servcie_fixture.h"

const char kMasterAccountID[] = "5000";
const char kSlaveAccountID[] = "5001";

FollowStragetyServiceFixture::FollowStragetyServiceFixture() {
  master_context_ = std::make_shared<OrdersContext>(kMasterAccountID);
  slave_context_ = std::make_shared<OrdersContext>(kSlaveAccountID);
  cta_strategy_ = std::make_shared<CTAGenericStrategy>("0");
  signal_ = std::make_shared<CTASignal>(0);
  signal_->SetOrdersContext(master_context_, slave_context_);
  signal_dispatch_ = std::make_shared<CTASignalDispatch>(signal_);
  signal_dispatch_->SetOrdersContext(master_context_, slave_context_);
  cta_strategy_->Subscribe(&strategy_dispatch_);
  signal_dispatch_->SubscribeEnterOrderObserver(cta_strategy_);
  strategy_dispatch_.SubscribeEnterOrderObserver(this);
  strategy_dispatch_.SubscribeRtnOrderObserver("0", signal_dispatch_);
}

void FollowStragetyServiceFixture::CloseOrder(const std::string& strategy_id,
                                              const std::string& instrument,
                                              const std::string& order_id,
                                              OrderDirection direction,
                                              PositionEffect position_effect,
                                              double price,
                                              int quantity) {
  order_inserts.push_back(OrderInsertForTest{instrument, order_id, direction,
                                             position_effect, price, quantity});
}

void FollowStragetyServiceFixture::OpenOrder(const std::string& strategy_id,
                                             const std::string& instrument,
                                             const std::string& order_id,
                                             OrderDirection direction,
                                             double price,
                                             int quantity) {
  order_inserts.push_back(OrderInsertForTest{
      instrument, order_id, direction, PositionEffect::kOpen, price, quantity});
}

void FollowStragetyServiceFixture::CancelOrder(const std::string& strategy_id,
                                               const std::string& order_id) {
  cancel_orders.push_back(order_id);
}

OrderInsertForTest FollowStragetyServiceFixture::PopOrderInsert() {
  OrderInsertForTest order_insert = order_inserts.front();
  order_inserts.pop_front();
  return order_insert;
}

std::shared_ptr<const OrderField>
FollowStragetyServiceFixture::MakeMasterOrderData(
    const std::string& order_id,
    OrderDirection order_direction,
    PositionEffect position_effect,
    OrderStatus status,
    int filled_quantity /*= 0*/,
    int quantity /*= 10*/,
    double order_price /*= 1234.1*/,
    const std::string& instrument /*= "abc"*/,
    const std::string& user_product_info /*= "Q7"*/) {
  std::shared_ptr<OrderField> field = std::make_shared<OrderField>();
  field->strategy_id = kMasterAccountID;
  field->instrument_id = instrument;
  field->exchange_id = default_order_exchange_id_;
  field->direction = order_direction;
  field->order_id = order_id;
  field->qty = quantity;
  // field->trading_qty = filled_quantity;
  field->leaves_qty = quantity - filled_quantity;
  field->input_price = order_price;
  field->status = status;
  field->position_effect = position_effect;
  return field;
}

std::shared_ptr<const OrderField>
FollowStragetyServiceFixture::MakeSlaveOrderData(
    const std::string& order_id,
    OrderDirection order_direction,
    PositionEffect position_effect,
    OrderStatus status,
    int filled_quantity /*= 0*/,
    int quantity /*= 10*/,
    double order_price /*= 1234.1*/,
    const std::string& instrument /*= "abc"*/) {
  std::shared_ptr<OrderField> field = std::make_shared<OrderField>();
  field->strategy_id = kSlaveAccountID;
  field->instrument_id = instrument;
  field->exchange_id = default_order_exchange_id_;
  field->direction = order_direction;
  field->order_id = order_id;
  field->qty = quantity;
  // field->trading_qty = filled_quantity;
  field->leaves_qty = quantity - filled_quantity;
  field->input_price = order_price;
  field->status = status;
  field->position_effect = position_effect;
  return field;
}

FollowStragetyServiceFixture::TestRetType
FollowStragetyServiceFixture::PushOpenOrderForMaster(
    const std::string& order_id,
    int quantity /*= 10*/,
    OrderDirection direction /*= OrderDirection::kBuy*/) {
  signal_dispatch_->RtnOrder(
      MakeMasterOrderData(order_id, direction, PositionEffect::kOpen,
                          OrderStatus::kActive, 0, quantity));

  return PopOrderEffectForTest();
}

FollowStragetyServiceFixture::TestRetType
FollowStragetyServiceFixture::PushOpenOrderForSlave(
    const std::string& order_id,
    int quantity /*= 10*/,
    OrderDirection direction /*= OrderDirection::kBuy*/) {
  strategy_dispatch_.RtnOrder(
      MakeSlaveOrderData(order_id, direction, PositionEffect::kOpen,
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

  signal_dispatch_->RtnOrder(MakeMasterOrderData(
      order_id, direction, PositionEffect::kOpen,
      master_filled_quantity == quantity ? OrderStatus::kAllFilled
                                         : OrderStatus::kActive,
      master_filled_quantity, quantity));

  if (slave_filled_quantity > 0) {
    strategy_dispatch_.RtnOrder(MakeSlaveOrderData(
        order_id, direction, PositionEffect::kOpen,
        slave_filled_quantity == quantity ? OrderStatus::kAllFilled
                                          : OrderStatus::kActive,
        slave_filled_quantity, quantity));
  }
}

FollowStragetyServiceFixture::TestRetType
FollowStragetyServiceFixture::PushNewOpenOrderForMaster(
    const std::string& order_id /*= "0001"*/,
    OrderDirection direction /*= OrderDirection::kBuy*/,
    int quantity /*= 10*/) {
  return PushOrderForMaster(order_id, direction, PositionEffect::kOpen,
                            OrderStatus::kActive, 0, quantity, 1234.1);
}

FollowStragetyServiceFixture::TestRetType
FollowStragetyServiceFixture::PushNewCloseOrderForMaster(
    const std::string& order_id /*= "0002"*/,
    OrderDirection direction /*= OrderDirection::kSell*/,
    int quantity /*= 10*/,
    double price /*= 1234.1*/,
    PositionEffect position_effect) {
  return PushOrderForMaster(order_id, direction, position_effect,
                            OrderStatus::kActive, 0, quantity, price);
}

FollowStragetyServiceFixture::TestRetType
FollowStragetyServiceFixture::PushCloseOrderForMaster(
    const std::string& order_id /*= "0002"*/,
    OrderDirection direction /*= OrderDirection::kSell*/,
    int filled_quantity /*=10*/,
    int quantity /*= 10*/,
    double price /*= 1234.1*/,
    PositionEffect position_effect /*= PositionEffect::kClose*/) {
  return PushOrderForMaster(order_id, direction, position_effect,
                            filled_quantity == quantity
                                ? OrderStatus::kAllFilled
                                : OrderStatus::kActive,
                            filled_quantity, quantity, price);
}

FollowStragetyServiceFixture::TestRetType
FollowStragetyServiceFixture::PushCancelOrderForMaster(
    const std::string& order_id /*= "0001"*/,
    OrderDirection direction /*= OrderDirection::kBuy*/,
    PositionEffect position_effect /*= PositionEffect::kOpen*/,
    int fill_quantity /*= 0*/,
    int quantity /*= 10*/) {
  return PushOrderForMaster(order_id, direction, position_effect,
                            OrderStatus::kCanceled, fill_quantity, quantity,
                            1234.1);
}

FollowStragetyServiceFixture::TestRetType
FollowStragetyServiceFixture::PushNewCloseOrderForSlave(
    const std::string& order_id /*= "0002"*/,
    OrderDirection direction /*= OrderDirection::kSell*/,
    int quantity /*= 10*/,
    PositionEffect position_effect) {
  return PushOrderForSlave(order_id, direction, position_effect,
                           OrderStatus::kActive, 0, quantity, 1234.1);
}

FollowStragetyServiceFixture::TestRetType
FollowStragetyServiceFixture::PushCloseOrderForSlave(
    const std::string& order_id /*= "0002"*/,
    OrderDirection direction /*= OrderDirection::kSell*/,
    int filled_quantity /*= 10*/,
    int quantity /*= 10*/,
    double price /*= 1234.1*/,
    PositionEffect position_effect /*= PositionEffect::kClose*/) {
  return PushOrderForSlave(order_id, direction, position_effect,
                           filled_quantity == quantity ? OrderStatus::kAllFilled
                                                       : OrderStatus::kActive,
                           filled_quantity, quantity, price);
}

FollowStragetyServiceFixture::TestRetType
FollowStragetyServiceFixture::PushCancelOrderForSlave(
    const std::string& order_id /*= "0001"*/,
    OrderDirection direction /*= OrderDirection::kBuy*/,
    PositionEffect position_effect /*= PositionEffect::kOpen*/,
    int fill_quantity /*= 10*/,
    int quantity /*= 10*/) {
  strategy_dispatch_.RtnOrder(
      MakeSlaveOrderData(order_id, direction, position_effect,
                         OrderStatus::kCanceled, fill_quantity, quantity));
  return PopOrderEffectForTest();
}

FollowStragetyServiceFixture::TestRetType
FollowStragetyServiceFixture::PushOrderForMaster(const std::string& order_id,
                                                 OrderDirection direction,
                                                 PositionEffect position_effect,
                                                 OrderStatus status,
                                                 int filled_quantity,
                                                 int quantity,
                                                 double price) {
  signal_dispatch_->RtnOrder(
      MakeMasterOrderData(order_id, direction, position_effect, status,
                          filled_quantity, quantity, price));
  return PopOrderEffectForTest();
}

FollowStragetyServiceFixture::TestRetType
FollowStragetyServiceFixture::PushOrderForSlave(const std::string& order_id,
                                                OrderDirection direction,
                                                PositionEffect position_effect,
                                                OrderStatus status,
                                                int filled_quantity,
                                                int quantity,
                                                double price) {
  strategy_dispatch_.RtnOrder(
      MakeSlaveOrderData(order_id, direction, position_effect, status,
                         filled_quantity, quantity, price));
  return PopOrderEffectForTest();
}

FollowStragetyServiceFixture::TestRetType
FollowStragetyServiceFixture::PopOrderEffectForTest() {
  std::vector<std::string> ret_cancel_orders;
  ret_cancel_orders.swap(cancel_orders);
  return {!order_inserts.empty() ? PopOrderInsert() : OrderInsertForTest(),
          ret_cancel_orders};
}

void FollowStragetyServiceFixture::SetUp() {}

void FollowStragetyServiceFixture::InitDefaultOrderExchangeId(
    std::string exchange_id) {
  default_order_exchange_id_ = std::move(exchange_id);
}