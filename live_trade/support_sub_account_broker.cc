#include "support_sub_account_broker.h"
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include "caf_atom_defines.h"
#include "util.h"

SupportSubAccountBroker::SupportSubAccountBroker(
    caf::actor_config& cfg,
    LiveTradeMailBox* mail_box,
    const std::vector<std::pair<std::string, caf::actor>>& sub_accounts)
    : caf::event_based_actor(cfg), mail_box_(mail_box) {
  ClearUpCTPFolwDirectory(".\\follow_account\\");
  trader_api_ = std::make_unique<CTPTraderApi>(this, ".\\follow_account\\");
  mail_box_->Subscribe(typeid(std::tuple<std::string, CTPEnterOrder>), this);
  mail_box_->Subscribe(typeid(std::tuple<std::string, CTPCancelOrder>), this);
  mail_box_->Subscribe(typeid(std::tuple<std::string, std::string>), this);

  for (const auto& sub_account : sub_accounts) {
    sub_actors_.insert({sub_account.first, sub_account.second});
  }
}

caf::behavior SupportSubAccountBroker::make_behavior() {
  caf::behavior work = {
      [=](const std::shared_ptr<CTPOrderField>& order) {
        auto it = order_id_bimap_.right.find(order->order_id);
        if (it != order_id_bimap_.right.end()) {
          order->order_id = it->second.order_id;
          auto actor_it = sub_actors_.find(it->second.sub_account_id);
          if (actor_it != sub_actors_.end()) {
            send(actor_it->second, order);
          }
        }
      },
      [=](const std::string& instrument, const std::string& order_id,
          double trading_price, int trading_qty, TimeStamp timestamp) {
        auto it = order_id_bimap_.right.find(order_id);
        if (it != order_id_bimap_.right.end()) {
          auto actor_it = sub_actors_.find(it->second.sub_account_id);
          if (actor_it != sub_actors_.end()) {
            send(actor_it->second, instrument, it->second.order_id,
                 trading_price, trading_qty, timestamp);
          }
        }
      },
      [=](const std::string& account_id, const CTPEnterOrder& enter_order) {
        std::string order_ref = GenerateOrderRef();
        order_id_bimap_.insert(OrderIdBimap::value_type(
            SubAccountOrderId{account_id, enter_order.order_id},
            trader_api_->MakeCtpUniqueOrderId(order_ref)));
        trader_api_->InputOrder(enter_order, order_ref);
      },
      [=](const std::string& account_id, const CTPCancelOrder& cancel) {
        auto it = order_id_bimap_.left.find(
            SubAccountOrderId{account_id, cancel.order_id});
        BOOST_ASSERT(it != order_id_bimap_.left.end());
        CThostFtdcInputOrderActionField order = {0};
        order.ActionFlag = THOST_FTDC_AF_Delete;
        order.FrontID = cancel.front_id;
        order.SessionID = cancel.session_id;
        strcpy(order.OrderRef, cancel.order_ref.c_str());
        strcpy(order.ExchangeID, cancel.exchange_id.c_str());
        strcpy(order.OrderSysID, cancel.order_sys_id.c_str());
        trader_api_->CancelOrder(std::move(order));
      },
  };

  caf::behavior sync_history_order = {
      [=](const std::shared_ptr<CTPOrderField>& order) {
        position_restorer_.HandleRtnOrder(order);
      },
      [=](const std::string& instrument, const std::string& order_id,
          double trading_price, int trading_qty, TimeStamp timestamp) {
        position_restorer_.HandleTraded(order_id, trading_price, trading_qty,
                                        timestamp);
      },

      [=](CheckHistoryRtnOrderIsDoneAtom, int last_time_check_count) {
        if (sync_history_rtn_order_count_ == last_time_check_count) {
          auto result = position_restorer_.Result();
          size_t sub_acc_count = sub_actors_.size();
          std::vector<std::vector<CTPPositionField>> sub_positions(
              sub_acc_count);
          for (const auto& item : result) {
            int leaves_today = item.today;
            int leaves_yesterday = item.yesterday;
            for (size_t i = 0; i < sub_acc_count; ++i) {
              int today = static_cast<int>(item.today / sub_acc_count);
              int yesterday = static_cast<int>(item.yesterday / sub_acc_count);
              if (i != sub_acc_count - 1) {
                if (today > 0 || yesterday > 0) {
                  sub_positions[i].push_back(CTPPositionField{
                      item.instrument, item.direction, today, yesterday});
                  leaves_today -= today;
                  leaves_yesterday -= yesterday;
                }
              } else {
                sub_positions[i].push_back(
                    CTPPositionField{item.instrument, item.direction,
                                     leaves_today, leaves_yesterday});
              }
            }
          }

          for (const auto& item : sub_actors_) {
            BOOST_ASSERT(sub_positions.size() > 0);
            send(item.second, sub_positions.back());
            sub_positions.pop_back();
          }
          set_default_handler(NULL);
          become(work);
        } else {
          delayed_send(this, std::chrono::milliseconds(500),
                       CheckHistoryRtnOrderIsDoneAtom::value,
                       sync_history_rtn_order_count_);
        }
      }};

  set_default_handler(caf::skip());
  return {[=](CtpConnectAtom, const std::string& server,
              const std::string& broker_id, const std::string& user_id,
              const std::string& password) {
            trader_api_->Connect(server, broker_id, user_id, password);
          },

          [=](std::vector<OrderPosition> yesterday_positions) {
            for (auto pos : yesterday_positions) {
              position_restorer_.AddYesterdayPosition(
                  pos.instrument, pos.order_direction, pos.quantity);
            }
            delayed_send(this, std::chrono::milliseconds(500),
                         CheckHistoryRtnOrderIsDoneAtom::value,
                         sync_history_rtn_order_count_);
            become(sync_history_order);
          }};
}

void SupportSubAccountBroker::HandleCTPRtnOrder(
    const std::shared_ptr<CTPOrderField>& order) {
  send(this, order);
}

std::string SupportSubAccountBroker::GenerateOrderRef() {
  return boost::lexical_cast<std::string>(order_seq_++);
}

void SupportSubAccountBroker::HandleCTPTradeOrder(const std::string& instrument,
                                                  const std::string& order_id,
                                                  double trading_price,
                                                  int trading_qty,
                                                  TimeStamp timestamp) {
  send(this, instrument, order_id, trading_price, trading_qty, timestamp);
}

void SupportSubAccountBroker::HandleLogon() {
  trader_api_->RequestYesterdayPosition();
}

void SupportSubAccountBroker::HandleRspYesterdayPosition(
    std::vector<OrderPosition> yesterday_positions) {
  send(this, std::move(yesterday_positions));
}
