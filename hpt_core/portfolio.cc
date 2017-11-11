#include "portfolio.h"
#include <algorithm>
#include <boost/assert.hpp>
#include "hpt_core/order_util.h"

bool IsOpenPositionEffect(PositionEffect position_effect) {
  return position_effect == PositionEffect::kOpen;
}

Portfolio::Portfolio(double init_cash, bool frozen_close_qty_by_rtn_order) {
  init_cash_ = init_cash;
  cash_ = init_cash;
  frozen_close_qty_by_rtn_order_ = frozen_close_qty_by_rtn_order;
}

void Portfolio::ResetByNewTradingDate() {
  daily_commission_ = 0;
  order_container_.clear();
  for (auto& item : position_container_) {
    item->Reset();
  }
}

void Portfolio::InitInstrumentDetail(std::string instrument,
                                     double margin_rate,
                                     int constract_multiple,
                                     CostBasis cost_basis) {
  instrument_info_container_.insert(
      {std::move(instrument),
       {margin_rate, constract_multiple, std::move(cost_basis)}});
}

void Portfolio::UpdateTick(const std::shared_ptr<TickData>& tick) {
  {
    auto it_find = position_container_.find(
        std::make_pair(*tick->instrument, OrderDirection::kBuy), HashPosition(),
        ComparePosition());
    if (it_find != position_container_.end()) {
      double update_pnl_ = 0.0;
      (*it_find)->UpdateMarketPrice(tick->tick->last_price, &update_pnl_);
      unrealised_pnl_ += update_pnl_;
    }
  }
  {
    auto it_find = position_container_.find(
        std::make_pair(*tick->instrument, OrderDirection::kSell),
        HashPosition(), ComparePosition());
    if (it_find != position_container_.end()) {
      double update_pnl_ = 0.0;
      (*it_find)->UpdateMarketPrice(tick->tick->last_price, &update_pnl_);
      unrealised_pnl_ += update_pnl_;
    }
  }
}

void Portfolio::HandleOrder(const std::shared_ptr<OrderField>& order) {
  if (order_container_.find(order->order_id) == order_container_.end()) {
    // New Order
    if (IsOpenPositionEffect(order->position_effect)) {
      BOOST_ASSERT(order->trading_qty == 0);
      BOOST_ASSERT_MSG(instrument_info_container_.find(order->instrument_id) !=
                           instrument_info_container_.end(),
                       "The Instrument info must be found!");
      double margin_rate = 0.0;
      int constract_multiple = 0;
      CostBasis cost_basis;
      memset(&cost_basis, 0, sizeof(CostBasis));
      std::tie(margin_rate, constract_multiple, cost_basis) =
          instrument_info_container_.at(order->instrument_id);
      auto it_pos = position_container_.find(
          std::make_pair(order->instrument_id, order->position_effect_direction),
          HashPosition(), ComparePosition());
      if (it_pos == position_container_.end()) {
        // Position position(margin_rate, constract_multiple, cost_basis);
        position_container_.insert(std::make_shared<Position>(
            order->instrument_id, order->position_effect_direction, margin_rate,
            constract_multiple, cost_basis));
        it_pos = position_container_.find(
            std::make_pair(order->instrument_id, order->position_effect_direction),
            HashPosition(), ComparePosition());
      }
      (*it_pos)->OpenOrder(order->qty);
      double frozen_cash =
          order->input_price * order->qty * margin_rate * constract_multiple +
          CalcCommission(PositionEffect::kOpen, order->input_price, order->qty,
                         constract_multiple, cost_basis);
      frozen_cash_ += frozen_cash;
      cash_ -= frozen_cash;
    } else {
      if (frozen_close_qty_by_rtn_order_) {
        HandleNewInputCloseOrder(order->instrument_id, order->position_effect_direction,
                                 order->qty);
      }
    }
    order_container_.insert({order->order_id, order});
  } else {
    const auto& previous_order = order_container_.at(order->order_id);
    int last_traded_qty = previous_order->leaves_qty - order->leaves_qty;
    switch (order->status) {
      case OrderStatus::kActive:
      case OrderStatus::kAllFilled: {
        if (IsOpenPositionEffect(order->position_effect)) {  // Open
          auto it_pos = position_container_.find(
              std::make_pair(order->instrument_id, order->position_effect_direction),
              HashPosition(), ComparePosition());
          BOOST_VERIFY(it_pos != position_container_.end());

          double add_margin = 0.0;
          (*it_pos)->TradedOpen(order->input_price, last_traded_qty,
                                &add_margin);
          frozen_cash_ -= add_margin;
          margin_ += add_margin;
        } else {  // Close
          auto it_pos = position_container_.find(
              std::make_pair(order->instrument_id,
                             order->position_effect_direction),
              HashPosition(), ComparePosition());
          BOOST_VERIFY(it_pos != position_container_.end());
          double pnl = 0.0;
          double add_cash = 0.0;
          double release_margin = 0.0;
          double update_unrealised_pnl = 0.0;
          (*it_pos)->TradedClose(order->input_price, last_traded_qty, &pnl,
                                 &add_cash, &release_margin,
                                 &update_unrealised_pnl);
          margin_ -= release_margin;
          cash_ += add_cash + pnl;
          realised_pnl_ += pnl;
          unrealised_pnl_ += update_unrealised_pnl;
          if ((*it_pos)->qty() == 0 && (*it_pos)->frozen_open_qty() == 0) {
            position_container_.erase(it_pos);
          }
        }

        if (order->status == OrderStatus::kAllFilled) {
          BOOST_ASSERT(instrument_info_container_.find(order->instrument_id) !=
                       instrument_info_container_.end());
          if (instrument_info_container_.find(order->instrument_id) !=
              instrument_info_container_.end()) {
            double margin_rate = 0;
            int constract_mutiple = 0;
            CostBasis cost_basis;
            std::tie(margin_rate, constract_mutiple, cost_basis) =
                instrument_info_container_.at(order->instrument_id);
            double commission =
                UpdateCostBasis(order->position_effect, order->input_price,
                                order->qty, constract_mutiple, cost_basis);
            if (IsOpenPositionEffect(order->position_effect)) {
              frozen_cash_ -= commission;
            } else {
              cash_ -= commission;
            }
          }
        }
      } break;
      case OrderStatus::kCanceled: {
        BOOST_ASSERT(instrument_info_container_.find(order->instrument_id) !=
                     instrument_info_container_.end());
        if (instrument_info_container_.find(order->instrument_id) !=
            instrument_info_container_.end()) {
          double margin_rate = 0;
          int constract_mutiple = 0;
          CostBasis cost_basis;
          std::tie(margin_rate, constract_mutiple, cost_basis) =
              instrument_info_container_.at(order->instrument_id);
          (void)UpdateCostBasis(order->position_effect, order->input_price,
                                order->qty - order->leaves_qty,
                                constract_mutiple, cost_basis);
          if (IsOpenPositionEffect(order->position_effect)) {
            double unfrozen_margin = order->leaves_qty * order->input_price *
                                     margin_rate * constract_mutiple;
            double unfrozen_cash =
                unfrozen_margin + CalcCommission(order->position_effect,
                                                 order->input_price, order->qty,
                                                 constract_mutiple, cost_basis);
            frozen_cash_ -= unfrozen_cash;
            cash_ += unfrozen_margin +
                     CalcCommission(order->position_effect, order->input_price,
                                    order->leaves_qty, constract_mutiple,
                                    cost_basis);
            auto it_pos = position_container_.find(
                std::make_pair(order->instrument_id,
                               IsOpenPositionEffect(order->position_effect)
                                   ? order->position_effect_direction
                                   : OppositeOrderDirection(order->position_effect_direction)),
                HashPosition(), ComparePosition());
            BOOST_VERIFY(it_pos != position_container_.end());
            (*it_pos)->CancelOpenOrder(order->leaves_qty);
            if ((*it_pos)->qty() == 0 && (*it_pos)->frozen_open_qty() == 0) {
              position_container_.erase(it_pos);
            }
          }
        }
      } break;
      case OrderStatus::kInputRejected:
        break;
      case OrderStatus::kCancelRejected:
        break;
      default:
        break;
    }
    order_container_[order->order_id] = order;
  }
}

void Portfolio::HandleNewInputCloseOrder(const std::string& instrument,
                                         OrderDirection direction,
                                         int qty) {
  auto it_find = position_container_.find(
      std::make_pair(instrument, direction),
      HashPosition(), ComparePosition());
  BOOST_ASSERT(it_find != position_container_.end());
  if (it_find != position_container_.end()) {
    (*it_find)->InputClose(qty);
  }
}

int Portfolio::GetPositionQty(const std::string& instrument,
                              OrderDirection direction) const {
  auto it_find = position_container_.find(std::make_pair(instrument, direction),
                                          HashPosition(), ComparePosition());
  if (it_find != position_container_.end()) {
    return (*it_find)->qty();
  }

  return 0;
}

int Portfolio::GetFrozenQty(const std::string& instrument,
                            OrderDirection direction) const {
  auto it_find = position_container_.find(std::make_pair(instrument, direction),
                                          HashPosition(), ComparePosition());
  if (it_find != position_container_.end()) {
    return (*it_find)->frozen_qty();
  }

  return 0;
}

int Portfolio::GetPositionCloseableQty(const std::string& instrument,
                                       OrderDirection direction) const {
  // TODO: Should be tag warrning!
  //   BOOST_ASSERT(position_container_.find(instrument) !=
  //                position_container_.end());

  auto it_find = position_container_.find(std::make_pair(instrument, direction),
                                          HashPosition(), ComparePosition());
  if (it_find != position_container_.end()) {
    return (*it_find)->closeable_qty();
  }
  return 0;
}

double Portfolio::UpdateCostBasis(PositionEffect position_effect,
                                  double price,
                                  int qty,
                                  int constract_multiple,
                                  const CostBasis& const_basis) {
  double commission = CalcCommission(position_effect, price, qty,
                                     constract_multiple, const_basis);
  daily_commission_ += commission;
  return commission;
}

double Portfolio::CalcCommission(PositionEffect position_effect,
                                 double price,
                                 int qty,
                                 int constract_multiple,
                                 const CostBasis& cost_basis) {
  double commission_param = 0.0;
  switch (position_effect) {
    case PositionEffect::kOpen:
      commission_param = cost_basis.open_commission;
      break;
    case PositionEffect::kClose:
      commission_param = cost_basis.close_commission;
      break;
    //case PositionEffect::kCloseToday:
    //  commission_param = cost_basis.close_today_commission;
    //  break;
    default:
      BOOST_ASSERT(false);
      break;
  }

  return cost_basis.type == CommissionType::kFixed
             ? qty * commission_param
             : price * qty * constract_multiple * commission_param / 100.0;
}

int Portfolio::UnfillOpenQty(const std::string& instrument,
                             OrderDirection direction) const {
  return std::accumulate(
      order_container_.begin(), order_container_.end(), 0,
      [&instrument, direction](int val, const auto& key_value) {
        const std::shared_ptr<OrderField>& order = key_value.second;
        if (IsCloseOrder(order->position_effect) ||
            order->instrument_id != instrument ||
            order->position_effect_direction != direction) {
          return val;
        }
        return val +
               (order->status == OrderStatus::kActive ? order->leaves_qty : 0);
      });
}

std::vector<std::shared_ptr<OrderField>> Portfolio::UnfillOpenOrders(
    const std::string& instrument,
    OrderDirection direction) const {
  std::vector<std::shared_ptr<OrderField>> ret_orders;
  std::for_each(order_container_.begin(), order_container_.end(),
                [&ret_orders, &instrument, direction](const auto& key_value) {
                  const std::shared_ptr<OrderField>& order = key_value.second;
                  if (IsCloseOrder(order->position_effect) ||
                      order->instrument_id != instrument ||
                      order->position_effect_direction != direction ||
                      order->status != OrderStatus::kActive) {
                    return;
                  }
                  ret_orders.push_back(order);
                });
  return std::move(ret_orders);
}

std::vector<std::string> Portfolio::InstrumentList() const {
  std::vector<std::string> instruments;
  std::for_each(order_container_.begin(), order_container_.end(),
                [&instruments](const auto& key_value) {
                  instruments.push_back(key_value.second->instrument_id);
                });

  auto beg = std::unique(instruments.begin(), instruments.end());
  instruments.erase(beg, instruments.end());
  return std::move(instruments);
}

int Portfolio::UnfillCloseQty(const std::string& instrument,
                              OrderDirection direction) const {
  return std::accumulate(
      order_container_.begin(), order_container_.end(), 0,
      [&instrument, direction](int val, const auto& key_value) {
        const std::shared_ptr<OrderField>& order = key_value.second;
        if (IsOpenOrder(order->position_effect) ||
            order->instrument_id != instrument ||
            order->position_effect_direction != direction) {
          return val;
        }
        return val +
               (order->status == OrderStatus::kActive ? order->leaves_qty : 0);
      });
}

std::vector<std::shared_ptr<OrderField>> Portfolio::UnfillCloseOrders(
    const std::string& instrument,
    OrderDirection direction) const {
  std::vector<std::shared_ptr<OrderField>> ret_orders;
  std::for_each(order_container_.begin(), order_container_.end(),
                [&ret_orders, &instrument, direction](const auto& key_value) {
                  const std::shared_ptr<OrderField>& order = key_value.second;
                  if (IsOpenOrder(order->position_effect) ||
                      order->instrument_id != instrument ||
                      order->position_effect_direction != direction ||
                      order->status != OrderStatus::kActive) {
                    return;
                  }
                  ret_orders.push_back(order);
                });
  return std::move(ret_orders);
}

std::vector<std::shared_ptr<const Position>> Portfolio::GetPositionList()
    const {
  std::vector<std::shared_ptr<const Position>> ret;

  std::for_each(position_container_.begin(), position_container_.end(),
                [&ret](const auto& pos) { ret.push_back(pos); });
  return ret;
}

std::shared_ptr<OrderField> Portfolio::GetOrder(const std::string& order_id) const {
  BOOST_ASSERT(order_container_.find(order_id) != order_container_.end());
  auto it = order_container_.find(order_id);
  if (it == order_container_.end()) {
    return std::shared_ptr<OrderField>();
  }

  return it->second;
}
