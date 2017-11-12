#ifndef _BACKTESTING_CTA_SIGNAL_BROKER_EX_H
#define _BACKTESTING_CTA_SIGNAL_BROKER_EX_H

#include "hpt_core/order_util.h"
#include "follow_strategy/cta_order_signal_subscriber.h"

template <class MailBox>
class BacktestingCTASignalBrokerEx {
 public:
  BacktestingCTASignalBrokerEx(
      MailBox* mail_box,
      std::vector<std::pair<std::shared_ptr<CTATransaction>, int64_t>>
          cta_signal_container,
      std::string instrument)
      : order_signal_subscriber_(mail_box),
        mail_box_(mail_box),
        keep_memory_(std::move(cta_signal_container)),
        instrument_(std::move(instrument)),
        csv_("debug_log.txt") {
    auto null_deleter = [](CTATransaction*) {};
    for (auto& item : keep_memory_) {
      for (int i = 0; i < item.second; ++i) {
        transactions_.push_back(std::shared_ptr<CTATransaction>(
            &item.first.get()[i], null_deleter));
      }
    }
    range_beg_it_ = transactions_.begin();
    mail_box_->Subscribe(&BacktestingCTASignalBrokerEx::HandleTick, this);
  }

  void BeforeTrading(const BeforeTradingAtom&,
                     const TradingTime& trading_time) {}

  void HandleTick(const std::shared_ptr<TickData>& tick) {
    if (range_beg_it_ == transactions_.end()) {
      return;
    }

    auto end_it =
        std::find_if(range_beg_it_, transactions_.end(),
                     [=](const std::shared_ptr<CTATransaction>& tran) {
                       return tick->tick->timestamp < tran->timestamp;
                     });

    for (auto i = range_beg_it_; i != end_it; ++i) {
      std::string order_id =
          boost::lexical_cast<std::string>(current_order_id_++);
      {
        auto order = std::make_shared<OrderField>();
        order->order_id = order_id;
        order->instrument_id = instrument_;
        order->position_effect = ParseThostFtdcPosition((*i)->position_effect);
        order->direction = ParseThostFtdcOrderDirection((*i)->direction);
        order->position_effect_direction = AdjustDirectionByPositionEffect(
            order->position_effect, order->direction);
        order->status = OrderStatus::kActive;
        order->input_price = (*i)->price;
        order->avg_price = (*i)->price;
        order->trading_price = 0.0;
        order->leaves_qty = (*i)->qty;
        order->qty = (*i)->qty;
        order->trading_qty = 0;
        order->input_timestamp = tick->tick->timestamp;
        order->update_timestamp = tick->tick->timestamp;
        // SendOrder(std::move(order));
        order_signal_subscriber_.HandleCTASignalOrder(CTASignalAtom::value,
                                                      order);
      }

      if (static_cast<OrderStatus>((*i)->status) == OrderStatus::kAllFilled) {
        auto order = std::make_shared<OrderField>();
        order->order_id = order_id;
        order->instrument_id = instrument_;
        order->position_effect = ParseThostFtdcPosition((*i)->position_effect);
        order->direction = ParseThostFtdcOrderDirection((*i)->direction);
        order->position_effect_direction = AdjustDirectionByPositionEffect(
            order->position_effect, order->direction);
        order->status = OrderStatus::kAllFilled;
        order->input_price = (*i)->price;
        order->avg_price = (*i)->price;
        order->trading_price = (*i)->price;
        order->leaves_qty = 0;
        order->qty = (*i)->qty;
        order->trading_qty = (*i)->qty;
        order->input_timestamp = tick->tick->timestamp;
        order->update_timestamp = tick->tick->timestamp;
        order_signal_subscriber_.HandleCTASignalOrder(CTASignalAtom::value,
                                                      order);
        // SendOrder(std::move(order));
      } else if (static_cast<OrderStatus>((*i)->status) ==
                 OrderStatus::kCanceled) {
        if ((*i)->traded_qty != 0) {
          auto order = std::make_shared<OrderField>();
          order->order_id = order_id;
          order->instrument_id = instrument_;

          order->position_effect =
              ParseThostFtdcPosition((*i)->position_effect);
          order->direction = ParseThostFtdcOrderDirection((*i)->direction);
          order->position_effect_direction = AdjustDirectionByPositionEffect(
              order->position_effect, order->direction);
          order->status = OrderStatus::kActive;
          order->input_price = (*i)->price;
          order->avg_price = (*i)->price;
          order->trading_price = 0;
          order->leaves_qty = (*i)->qty - (*i)->traded_qty;
          order->qty = (*i)->qty;
          order->trading_price = 0;
          order->input_timestamp = tick->tick->timestamp;
          order->update_timestamp = tick->tick->timestamp;
          order_signal_subscriber_.HandleCTASignalOrder(CTASignalAtom::value,
                                                        order);
          // SendOrder(std::move(order));
        }
        auto order = std::make_shared<OrderField>();
        order->order_id = order_id;
        order->instrument_id = instrument_;
        order->position_effect = ParseThostFtdcPosition((*i)->position_effect);
        order->direction = ParseThostFtdcOrderDirection((*i)->direction);
        order->position_effect_direction = AdjustDirectionByPositionEffect(
            order->position_effect, order->direction);
        order->status = OrderStatus::kCanceled;
        order->input_price = (*i)->price;
        order->avg_price = (*i)->price;
        order->trading_price = 0.0;
        order->leaves_qty = (*i)->qty - (*i)->traded_qty;
        order->qty = (*i)->qty;
        order->trading_qty = 0;
        order->input_timestamp = tick->tick->timestamp;
        order->update_timestamp = tick->tick->timestamp;
        // SendOrder(std::move(order));
        order_signal_subscriber_.HandleCTASignalOrder(CTASignalAtom::value,
                                                      order);
      } else {
      }
    }

    range_beg_it_ = end_it;
  }

 private:
  PositionEffect ParseThostFtdcPosition(int position_effect) const {
    PositionEffect ret = PositionEffect::kUndefine;
    switch (position_effect) {
      case 0:
        ret = PositionEffect::kOpen;
        break;
      case 1:
      case 2:
        ret = PositionEffect::kClose;
        break;
      default:
        break;
    }
    return ret;
  }

  OrderDirection ParseThostFtdcOrderDirection(int order_direction) const {
    OrderDirection ret = OrderDirection::kUndefine;
    switch (order_direction) {
      case 0:
        ret = OrderDirection::kBuy;
        break;
      case 1:
        ret = OrderDirection::kSell;
        break;
      default:
        break;
    }
    return ret;
  }

  MailBox* mail_box_;
  std::list<std::shared_ptr<CTATransaction>> transactions_;
  std::list<std::shared_ptr<CTATransaction>>::iterator range_beg_it_;
  std::vector<std::pair<std::shared_ptr<CTATransaction>, int64_t>> keep_memory_;

  CTAOrderSignalSubscriber<MailBox> order_signal_subscriber_;
  int delayed_input_order_minute_ = 0;
  int cancel_order_after_minute_ = 0;
  int backtesting_position_effect_ = 0;
  int current_order_id_ = 0;
  std::ofstream csv_;
  std::string instrument_;
};

#endif  // _BACKTESTING_CTA_SIGNAL_BROKER_EX_H