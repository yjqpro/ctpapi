#include "simulated_always_treade_execution_handler.h"
#include "hpt_core/order_util.h"
#include "bft_core/make_message.h"

SimulatedAlwaysExcutionHandler::SimulatedAlwaysExcutionHandler(
    bft::ChannelDelegate* mail_box)
    : mail_box_(mail_box) {
  bft::MessageHandler handler;
  handler.Subscribe(&SimulatedAlwaysExcutionHandler::HandlerInputOrder, this);
  handler.Subscribe(&SimulatedAlwaysExcutionHandler::HandleTick, this);
  handler.Subscribe(&SimulatedAlwaysExcutionHandler::HandleCancelOrder, this);
  handler.Subscribe(&SimulatedAlwaysExcutionHandler::BeforeTrading, this);
  mail_box_->Subscribe(std::move(handler));
}

void SimulatedAlwaysExcutionHandler::EnqueueRtnOrderEvent(
    const std::string& order_id,
    const std::string& instrument,
    PositionEffect position_effect,
    OrderDirection direction,
    double price,
    int qty,
    OrderStatus order_status) {
  auto order = std::make_shared<OrderField>();
  order->order_id = order_id;
  order->instrument_id = instrument;
  order->position_effect = position_effect;
  order->direction = direction;
  order->position_effect_direction =
      AdjustDirectionByPositionEffect(position_effect, direction);
  order->status = order_status;
  order->input_price = price;
  order->avg_price = price;
  order->leaves_qty = 0;
  order->qty = qty;
  order->trading_qty = qty;
  order->input_timestamp =
      (current_tick_ != nullptr ? current_tick_->timestamp : 0);
  order->update_timestamp =
      (current_tick_ != nullptr ? current_tick_->timestamp : 0);

  mail_box_->Send(bft::MakeMessage(std::move(order)));
}

void SimulatedAlwaysExcutionHandler::HandleCancelOrder(
    const CancelOrder& cancel_order) {
  BOOST_ASSERT_MSG(false,
                   "Don't support cancel order for this excution handler");
}

void SimulatedAlwaysExcutionHandler::HandlerInputOrder(
    const InputOrder& input_order) {
  auto order = std::make_shared<OrderField>();
  order->order_id = input_order.order_id;
  order->instrument_id = input_order.instrument;
  order->position_effect = input_order.position_effect;
  order->direction = input_order.direction;
  order->position_effect_direction = AdjustDirectionByPositionEffect(
      input_order.position_effect, input_order.direction);
  order->status = OrderStatus::kActive;
  order->input_price = input_order.price;
  order->avg_price = input_order.price;
  order->leaves_qty = input_order.qty;
  order->qty = input_order.qty;
  order->trading_qty = 0;
  // TODO:
  // order->input_timestamp = input_order.timestamp_;
  // order->update_timestamp = input_order.timestamp_;
  order->input_timestamp =
      (current_tick_ != nullptr ? current_tick_->timestamp : 0);
  order->update_timestamp =
      (current_tick_ != nullptr ? current_tick_->timestamp : 0);
  mail_box_->Send(bft::MakeMessage(std::move(order)));

  EnqueueRtnOrderEvent(input_order.order_id, input_order.instrument,
                       input_order.position_effect, input_order.direction,
                       input_order.price, input_order.qty,
                       OrderStatus::kAllFilled);
}

void SimulatedAlwaysExcutionHandler::HandleTick(
    const std::shared_ptr<TickData>& tick) {
  current_tick_ = tick->tick;
}

void SimulatedAlwaysExcutionHandler::BeforeTrading(
    BeforeTradingAtom,
    const TradingTime& trading_time) {}