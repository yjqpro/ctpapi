#include "delayed_open_strategy_ex.h"

std::string DelayedOpenStrategyEx::GenerateOrderId() {
  return boost::lexical_cast<std::string>(order_id_seq_++);
}

int DelayedOpenStrategyEx::PendingOpenQty(const std::string& instrument,
                                          OrderDirection direction) {
  return std::accumulate(
      pending_delayed_open_order_.begin(), pending_delayed_open_order_.end(), 0,
      [&instrument, direction](int val,
                                     const InputOrderSignal& input_order) {
        if (input_order.instrument != instrument ||
            input_order.direction != direction) {
          return val;
        }
        return val + input_order.qty;
      });
}

void DelayedOpenStrategyEx::CancelUnfillOpeningOrders(
    const std::string& instrument,
    OrderDirection direciton,
    int leaves_cancel_qty) {
  auto orders = portfolio_.UnfillOpenOrders(instrument, direciton);
  std::sort(orders.begin(), orders.end(), [direciton](const auto& l,
                                                      const auto& r) {
    return direciton == OrderDirection::kBuy ? l->input_price < r->input_price
                                             : l->input_price > r->input_price;
  });

  auto for_each_lambda = [=, &leaves_cancel_qty](const auto& order) {
    if (leaves_cancel_qty <= 0) {
      return;
    }
    int cancel_qty = std::min<int>(order->leaves_qty, leaves_cancel_qty);
    delegate_->CancelOrder(order->instrument_id,order->order_id, order->direction, cancel_qty);
    // signal_dispatch_->CancelOrder(order->order_id);

    // int open_qty = order->leaves_qty - cancel_qty;
    // if (open_qty > 0) {
    //  signal_dispatch_->OpenOrder(order->instrument_id, GenerateOrderId(),
    //                              position_direction, order->input_price,
    //                              open_qty);
    //}
    leaves_cancel_qty -= cancel_qty;
  };

  if (direciton == OrderDirection::kBuy) {
    std::for_each(orders.begin(), orders.end(), for_each_lambda);
  } else {
    std::for_each(orders.rbegin(), orders.rend(), for_each_lambda);
  }
}

void DelayedOpenStrategyEx::DecreasePendingOpenOrderQty(
    const std::string& instrument,
    OrderDirection direction,
    int qty) {
  if (qty <= 0)
    return;
  int leaves_qty = qty;
  pending_delayed_open_order_.sort([direction](const auto& l, const auto& r) {
    return direction == OrderDirection::kBuy ? (l.price < r.price)
                                             : (l.price > r.price);
  });
  for (auto& input_order : pending_delayed_open_order_) {
    if (input_order.instrument != instrument ||
        input_order.direction != direction) {
      continue;
    }
    int minus_qty = std::min<int>(input_order.qty, leaves_qty);
    input_order.qty -= minus_qty;
    leaves_qty -= minus_qty;
    if (leaves_qty <= 0)
      break;
  }

  auto remove_it = std::remove_if(
      pending_delayed_open_order_.begin(), pending_delayed_open_order_.end(),
      [](const auto& input_order) { return input_order.qty == 0; });

  pending_delayed_open_order_.erase(remove_it,
                                    pending_delayed_open_order_.end());

  pending_delayed_open_order_.sort([](const auto& l, const auto& r) -> bool {
    return l.timestamp < r.timestamp;
  });
}

std::vector<InputOrderSignal>
DelayedOpenStrategyEx::GetSpecificOrdersInPendingOpenQueue(
    const std::string& instrument,
    OrderDirection direction) {
  std::vector<InputOrderSignal> orders;
  std::copy_if(pending_delayed_open_order_.begin(),
               pending_delayed_open_order_.end(), std::back_inserter(orders),
               [&instrument, direction](const auto& order) {
                 return order.instrument == instrument &&
                        order.direction == direction;
               });
  return orders;
}

void DelayedOpenStrategyEx::CancelSpecificOpeningOrders(
    const std::string& instrument,
    OrderDirection direction) {
  if (portfolio_.UnfillOpenQty(instrument, direction) > 0) {
    auto orders = portfolio_.UnfillOpenOrders(instrument, direction);
    for (const auto& order : orders) {
      delegate_->CancelOrder(order->instrument_id, order->order_id, order->direction, 0);
    }
  }
}

void DelayedOpenStrategyEx::RemoveSpecificPendingOpenOrders(
    const std::string& instrument,
    OrderDirection direction) {
  auto remove_it = std::remove_if(
      pending_delayed_open_order_.begin(), pending_delayed_open_order_.end(),
      [&instrument, direction](const auto& input_order) {
        return input_order.instrument == instrument &&
               input_order.direction == direction;
      });

  pending_delayed_open_order_.erase(remove_it,
                                    pending_delayed_open_order_.end());
}

bool DelayedOpenStrategyEx::ImmediateOpenOrderIfPriceArrive(
    const InputOrderSignal & order,
    const std::shared_ptr<Tick>& tick) {
  if (strategy_param_.price_offset_rate == 0.0) {
    return false;
  }

  double maybe_input_price =
      order.direction == OrderDirection::kBuy
          ? order.price - strategy_param_.price_offset_rate * order.price
          : order.price + strategy_param_.price_offset_rate * order.price;
  if (order.direction == OrderDirection::kBuy &&
      tick->last_price > maybe_input_price) {
    return false;
  } else if (order.direction == OrderDirection::kSell &&
             tick->last_price < maybe_input_price) {
    return false;
  } else {
  }

  BOOST_LOG(log_) << boost::log::add_value("quant_timestamp",
                                           TimeStampToPtime(last_timestamp_))
                  << "Price best immediate open order: open price is :"
                  << maybe_input_price << "LAST TICK:" << order.instrument
                  << ":"
                  << " Last:" << tick->last_price << "(" << tick->qty << ")"
                  << " Bid1:" << tick->bid_price1 << "(" << tick->bid_qty1
                  << ")"
                  << " Ask1:" << tick->ask_price1 << "(" << tick->ask_qty1
                  << ")";

   //signal_dispatch_->OpenOrder(
   //   order.instrument, GenerateOrderId(), order.direction,
   //   maybe_input_price, order.qty);
  delegate_->EnterOrder(InputOrder{order.instrument, GenerateOrderId(), "salve", PositionEffect::kOpen, order.direction,
  maybe_input_price, order.qty, 0});
  return true;
}

void DelayedOpenStrategyEx::HandleCanceled(
    const std::shared_ptr<const OrderField>& rtn_order,
    const CTAPositionQty& position_qty) {
  if (rtn_order->position_effect == PositionEffect::kOpen) {
    return;
  }
  auto it = cta_to_strategy_closing_order_id_.find(rtn_order->order_id);
  if (it != cta_to_strategy_closing_order_id_.end()) {
    auto order = portfolio_.GetOrder(it->second);
    delegate_->CancelOrder(
        order->instrument_id, order->order_id, order->direction, order->leaves_qty);
    cta_to_strategy_closing_order_id_.erase(it);
  }
}

void DelayedOpenStrategyEx::HandleClosed(
    const std::shared_ptr<const OrderField>& rtn_order,
    const CTAPositionQty& position_qty) {
  int closeable_qty = portfolio_.GetPositionCloseableQty(
      rtn_order->instrument_id, rtn_order->direction);
  BOOST_ASSERT(position_qty.position >= position_qty.frozen);
  int pending_open_order_qty =
      PendingOpenQty(rtn_order->instrument_id, rtn_order->direction);
  int opening_order_qty =
      portfolio_.UnfillOpenQty(rtn_order->instrument_id, rtn_order->direction);
  if (position_qty.position == 0) {
    RemoveSpecificPendingOpenOrders(rtn_order->instrument_id,
                                    rtn_order->direction);
    CancelSpecificOpeningOrders(rtn_order->instrument_id, rtn_order->direction);
  } else if (pending_open_order_qty + opening_order_qty >
             position_qty.position) {
    int leaves_cancel_or_remove_qty =
        pending_open_order_qty + opening_order_qty - position_qty.position;
    BOOST_ASSERT(leaves_cancel_or_remove_qty > 0);
    if (leaves_cancel_or_remove_qty > 0) {
      enum class OpenOrderFrom {
        kPendingQueue,
        kUnfillQueue,
      };
      std::vector<std::tuple<OpenOrderFrom, double, int> > orders_from_queue;
      auto pending_open_orders = GetSpecificOrdersInPendingOpenQueue(
          rtn_order->instrument_id, rtn_order->direction);
      std::for_each(
          pending_open_orders.begin(), pending_open_orders.end(),
          [&orders_from_queue](const auto& order) {
            orders_from_queue.push_back(std::make_tuple(
                OpenOrderFrom::kPendingQueue, order.price, order.qty));
          });
      auto opening_orders = portfolio_.UnfillOpenOrders(
          rtn_order->instrument_id, rtn_order->direction);
      std::for_each(opening_orders.begin(), opening_orders.end(),
                    [&orders_from_queue](const auto& order) {
                      orders_from_queue.push_back(std::make_tuple(
                          OpenOrderFrom::kUnfillQueue, order->input_price,
                          order->leaves_qty));
                    });

      std::sort(orders_from_queue.begin(), orders_from_queue.end(),
                [&rtn_order](const auto& l, const auto& r) {
                  return rtn_order->direction == OrderDirection::kBuy
                             ? std::get<1>(l) < std::get<1>(r)
                             : std::get<1>(l) > std::get<1>(r);
                });
      int want_decrease_pending_open_qty = 0;
      int want_cancel_opening_qty = 0;
      for (const auto& item : orders_from_queue) {
        int qty = std::min(std::get<2>(item), leaves_cancel_or_remove_qty);

        if (std::get<0>(item) == OpenOrderFrom::kPendingQueue) {
          want_decrease_pending_open_qty += qty;
        } else {
          want_cancel_opening_qty += qty;
        }

        leaves_cancel_or_remove_qty -= qty;
        if (leaves_cancel_or_remove_qty <= 0) {
          break;
        }
      }
      DecreasePendingOpenOrderQty(rtn_order->instrument_id,
                                  rtn_order->direction,
                                  want_decrease_pending_open_qty);
      CancelUnfillOpeningOrders(rtn_order->instrument_id, rtn_order->direction,
                                want_cancel_opening_qty);
    }
  } else {
  }
}

void DelayedOpenStrategyEx::HandleCloseing(
    const std::shared_ptr<const OrderField>& rtn_order,
    const CTAPositionQty& position_qty) {
  int closeable_qty = portfolio_.GetPositionCloseableQty(
      rtn_order->instrument_id, rtn_order->direction);
  BOOST_ASSERT(position_qty.position >= position_qty.frozen);
  if (position_qty.position == position_qty.frozen) {
    // Close All
    std::string order_id = GenerateOrderId();
    cta_to_strategy_closing_order_id_.insert(
        std::make_pair(rtn_order->order_id, order_id));
    delegate_->EnterOrder(
        InputOrder{rtn_order->instrument_id, std::move(order_id), "slave",
                   PositionEffect::kClose, rtn_order->direction,
                   rtn_order->input_price, closeable_qty, 0});
  } else if (rtn_order->qty <=
             portfolio_.GetPositionCloseableQty(rtn_order->instrument_id,
                                                rtn_order->direction)) {
    std::string order_id = GenerateOrderId();
    cta_to_strategy_closing_order_id_.insert(
        std::make_pair(rtn_order->order_id, order_id));
    delegate_->EnterOrder(
        InputOrder{rtn_order->instrument_id, std::move(order_id), "slave",
                   PositionEffect::kClose, rtn_order->direction,
                   rtn_order->input_price, rtn_order->qty, 0});
  } else if (closeable_qty > 0) {
    std::string order_id = GenerateOrderId();
    cta_to_strategy_closing_order_id_.insert(
        std::make_pair(rtn_order->order_id, order_id));
    delegate_->EnterOrder(
        InputOrder{rtn_order->instrument_id, std::move(order_id), "slave",
                   PositionEffect::kClose, rtn_order->direction,
                   rtn_order->input_price, closeable_qty, 0});
  } else {
  }
}

void DelayedOpenStrategyEx::HandleOpened(
    const std::shared_ptr<const OrderField>& rtn_order,
    const CTAPositionQty& position_qty) {
  pending_delayed_open_order_.push_back(InputOrderSignal{
      rtn_order->instrument_id, "", "S1", PositionEffect::kOpen,
      rtn_order->direction, rtn_order->trading_price, rtn_order->trading_qty,
      rtn_order->update_timestamp});
}

void DelayedOpenStrategyEx::HandleOpening(
    const std::shared_ptr<const OrderField>& rtn_order,
    const CTAPositionQty& position_qty) {}

void DelayedOpenStrategyEx::HandleCTARtnOrderSignal(
    const std::shared_ptr<OrderField>& rtn_order,
    const CTAPositionQty& position_qty) {
  switch (rtn_order->status) {
    case OrderStatus::kActive:
      if (rtn_order->leaves_qty == rtn_order->qty) {
        // New Open/Close
        if (IsOpenOrder(rtn_order->position_effect)) {
          HandleOpening(rtn_order, position_qty);
        } else {
          HandleCloseing(rtn_order, position_qty);
        }
      } else {
        if (IsOpenOrder(rtn_order->position_effect)) {
          HandleOpened(rtn_order, position_qty);
        } else {
          HandleClosed(rtn_order, position_qty);
        }
      }
      break;
    case OrderStatus::kAllFilled: {
      if (IsOpenOrder(rtn_order->position_effect)) {
        HandleOpened(rtn_order, position_qty);
      } else {
        HandleClosed(rtn_order, position_qty);
      }
    } break;
    case OrderStatus::kCanceled:
      HandleCanceled(rtn_order, position_qty);
      break;
    default:

      break;
  }
}

void DelayedOpenStrategyEx::HandleRtnOrder(
    const std::shared_ptr<OrderField>& rtn_order) {
  portfolio_.HandleOrder(rtn_order);
}

void DelayedOpenStrategyEx::HandleTick(const std::shared_ptr<TickData>& tick) {
  last_timestamp_ = tick->tick->timestamp;
  if (!pending_delayed_open_order_.empty()) {
    auto it_end = std::remove_if(
        pending_delayed_open_order_.begin(), pending_delayed_open_order_.end(),
        [=](const auto& order) -> bool {
          // is expire
          if (tick->tick->timestamp >=
              order.timestamp +
                  strategy_param_.delayed_open_after_seconds * 1000) {
            double maybe_input_price =
                order.direction == OrderDirection::kBuy
                    ? order.price -
                          strategy_param_.price_offset_rate * order.price
                    : order.price +
                          strategy_param_.price_offset_rate * order.price;
            if (order.direction == OrderDirection::kBuy &&
                tick->tick->last_price > maybe_input_price &&
                tick->tick->last_price < order.price) {
              return false;
            } else if (order.direction == OrderDirection::kSell &&
                       tick->tick->last_price < maybe_input_price &&
                       tick->tick->last_price > order.price) {
              return false;
            } else {
            }

            double input_price =
                order.direction == OrderDirection::kBuy
                    ? std::min(order.price, tick->tick->last_price)
                    : std::max(order.price, tick->tick->last_price);
            BOOST_LOG(log_)
                << boost::log::add_value("quant_timestamp",
                                         TimeStampToPtime(last_timestamp_))
                << "�ӳٿ��ֶ������� ���ּ�:" << input_price
                << "��ǰ Tick :" << *(tick->instrument) << ":"
                << " Last:" << tick->tick->last_price << "(" << tick->tick->qty
                << ")"
                << " Bid1:" << tick->tick->bid_price1 << "("
                << tick->tick->bid_qty1 << ")"
                << " Ask1:" << tick->tick->ask_price1 << "("
                << tick->tick->ask_qty1 << ")";
            delegate_->EnterOrder(
                InputOrder{order.instrument, GenerateOrderId(), "slave",
                           PositionEffect::kOpen, order.direction,
                           input_price, order.qty, 0});
            return true;
          }
          return ImmediateOpenOrderIfPriceArrive(order, tick->tick);
        });
    pending_delayed_open_order_.erase(it_end,
                                      pending_delayed_open_order_.end());
  }
  last_tick_ = tick;
}

DelayedOpenStrategyEx::DelayedOpenStrategyEx(
    DelayedOpenStrategyEx::Delegate* delegate,
    StrategyParam strategy_param,
    const std::string& instrument)
    : delegate_(delegate),
      strategy_param_(std::move(strategy_param)),
      portfolio_(1000000, true) {
  portfolio_.InitInstrumentDetail(instrument, 0.02, 10,
                                  CostBasis{CommissionType::kFixed, 0, 0, 0});
  BOOST_LOG(log_) << "test";
}