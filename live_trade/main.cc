#include <unordered_map>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <fstream>
#include <boost/format.hpp>
#include <boost/assign.hpp>
#include <boost/any.hpp>
#include "caf/all.hpp"
#include "common/api_struct.h"
// #include "hpt_core/backtesting/execution_handler.h"
// #include "hpt_core/backtesting/price_handler.h"
// #include "hpt_core/backtesting/backtesting_mail_box.h"
#include "hpt_core/cta_transaction_series_data_base.h"
#include "hpt_core/tick_series_data_base.h"
//#include "hpt_core/portfolio_handler.h"
//#include "strategies/strategy.h"
//#include "strategies/wr_strategy.h"
#include "caf_mail_box.h"
#include "live_trade_data_feed_handler.h"
//#include "strategies/key_input_strategy.h"
#include "follow_strategy/delayed_open_strategy_ex.h"
#include "support_sub_account_broker.h"
#include "ctp_broker/ctp_order_delegate.h"
#include "ctp_broker/ctp_instrument_broker.h"
#include "caf_atom_defines.h"
#include "follow_strategy/cta_order_signal_subscriber.h"
#include "live_trade_broker_handler.h"
#include "live_trade_mail_box.h"
#include "ctp_rtn_order_subscriber.h"

//#include "live_trade_broker_handler.h"
class AbstractExecutionHandler;

class CAFDelayOpenStrategyAgent : public caf::event_based_actor {
 public:
  CAFDelayOpenStrategyAgent(caf::actor_config& cfg,
                            DelayedOpenStrategyEx::StrategyParam param,
                            LiveTradeMailBox* inner_mail_box,
                            LiveTradeMailBox* common_mail_box)
      : caf::event_based_actor(cfg),
        agent_(this, std::move(param), ""),
        inner_mail_box_(inner_mail_box),
        common_mail_box_(common_mail_box) {}

  virtual caf::behavior make_behavior() override { return message_handler_; }

  template <typename... Ts>
  void Send(Ts&&... args) {
    inner_mail_box_->Send(std::forward<Ts>(args)...);
  }

  template <typename CLASS, typename... Ts>
  void Subscribe(void (CLASS::*pfn)(Ts...), CLASS* ptr) {
    common_mail_box_->Subscribe(typeid(std::tuple<std::decay_t<Ts>...>), this);
    message_handler_ = message_handler_.or_else(
        [ptr, pfn](Ts... args) { (ptr->*pfn)(args...); });
  }

  template <typename CLASS>
  void Subscribe(void (CLASS::*pfn)(const std::shared_ptr<OrderField>&),
                 CLASS* ptr) {
    inner_mail_box_->Subscribe(typeid(std::tuple<std::shared_ptr<OrderField>>),
                               this);
    message_handler_ = message_handler_.or_else(
        [ptr, pfn](const std::shared_ptr<OrderField>& order) {
          (ptr->*pfn)(order);
        });
  }

 private:
  LiveTradeMailBox* inner_mail_box_;
  LiveTradeMailBox* common_mail_box_;
  caf::message_handler message_handler_;
  DelayOpenStrategyAgent<CAFDelayOpenStrategyAgent> agent_;
};

class CAFSubAccountBroker : public caf::event_based_actor,
                            public CTPOrderDelegate {
 public:
  CAFSubAccountBroker(caf::actor_config& cfg,
                      LiveTradeMailBox* inner_mail_box,
                      LiveTradeMailBox* common_mail_box)
      : caf::event_based_actor(cfg),
        inner_mail_box_(inner_mail_box),
        common_mail_box_(common_mail_box) {
    // inner_mail_box_->Subscrdibe(
    //    typeid(std::tuple<std::shared_ptr<CTPOrderField>>), this);
    inner_mail_box_->Subscribe(typeid(std::tuple<InputOrder>), this);
    inner_mail_box_->Subscribe(typeid(std::tuple<CancelOrderSignal>), this);
  };

  virtual caf::behavior make_behavior() override {
    return {
        [=](const std::shared_ptr<CTPOrderField>& order) {},
        [=](const InputOrder& order) {
          auto it = instrument_brokers_.find(order.instrument);
          if (it != instrument_brokers_.end()) {
            it->second.HandleInputOrder(order);
          } else {
            CTPInstrumentBroker broker(
                this, order.instrument, false,
                std::bind(&CAFSubAccountBroker::GenerateOrderId, this));
            broker.HandleInputOrder(order);
            instrument_brokers_.insert({order.instrument, std::move(broker)});
          }
        },
        [=](const CancelOrderSignal& cancel) {},
    };
  }

  virtual void EnterOrder(CTPEnterOrder enter_order) override {
    common_mail_box_->Send(enter_order);
  }

  virtual void CancelOrder(const std::string& account_id,
                           const std::string& order_id) override {
    common_mail_box_->Send(account_id, order_id);
  }

  virtual void ReturnOrderField(
      const std::shared_ptr<OrderField>& order) override {
    inner_mail_box_->Send(order);
  }

 private:
  std::string GenerateOrderId() {
    return boost::lexical_cast<std::string>(order_seq_++);
  }
  caf::actor ctp_actor_;
  LiveTradeMailBox* inner_mail_box_;
  LiveTradeMailBox* common_mail_box_;
  CTPOrderDelegate* delegate_;
  std::unordered_map<std::string, CTPInstrumentBroker> instrument_brokers_;
  int order_seq_ = 0;
};

class config : public caf::actor_system_config {
 public:
  int delayed_close_minutes = 10;
  int cancel_after_minutes = 10;
  int position_effect = 0;

  config() {
    opt_group{custom_options_, "global"}
        .add(delayed_close_minutes, "delayed,d", "set delayed close minutes")
        .add(cancel_after_minutes, "cancel,c", "set cancel after minutes")
        .add(position_effect, "open_close", "backtesting open(0) close(1)")
        .add(scheduler_max_threads, "max-threads",
             "sets a fixed number of worker threads for the scheduler");
  }
};

int caf_main(caf::actor_system& system, const config& cfg) {
  using hrc = std::chrono::high_resolution_clock;
  auto beg = hrc::now();
  bool running = true;
  double init_cash = 50 * 10000;
  int delayed_input_order_by_minute = 10;
  int cancel_order_after_minute = 10;
  int backtesting_position_effect = 0;

  std::string instrument = "MA801";

  // std::string market = "dc";
  // std::string instrument = "a1709";
  std::string csv_file_prefix =
      str(boost::format("%s_%d_%d_%s") % instrument %
          delayed_input_order_by_minute % cancel_order_after_minute %
          (backtesting_position_effect == 0 ? "O" : "C"));
  // KeyInputStrategy<CAFMailBox> strategy(&mail_box, instrument);

  LiveTradeMailBox common_mail_box;
  LiveTradeMailBox inner_mail_box;
  DelayedOpenStrategyEx::StrategyParam param;
  system.spawn<CAFDelayOpenStrategyAgent>(std::move(param), &inner_mail_box,
                                          &common_mail_box);
  system.spawn<CAFSubAccountBroker>(&inner_mail_box, &common_mail_box);

  auto cta = system.spawn<CtpRtnOrderSubscriber>(&common_mail_box);

  // DelayOpenStrategyAgent<CAFMailBox> strategy(&mail_box,
  // std::move(param), "1"); CTAOrderSignalSubscriber<CAFMailBox>(&mail_box,
  // "");
  auto support_sub_account_broker = system.spawn<SupportSubAccountBroker>();
  // LiveTradeBrokerHandler<LiveTradeMailBox>
  // live_trade_borker_handler(&mail_box);

  LiveTradeDataFeedHandler<LiveTradeMailBox> live_trade_data_feed_handler(
      &common_mail_box);

  // PortfolioHandler<CAFMailBox> portfolio_handler_(
  //    init_cash, &mail_box, instrument, csv_file_prefix, 0.1, 10,
  //    CostBasis{CommissionType::kFixed, 165, 165, 165});

  // mail_box.FinishInitital();

  live_trade_data_feed_handler.SubscribeInstrument(instrument);

  caf::anon_send(cta, CtpConnectAtom::value, "tcp://180.168.146.187:10000",
                 "9999", "053867", "8661188");
  live_trade_data_feed_handler.Connect("tcp://180.166.11.33:41213", "4200",
                                       "15500011", "Yunqizhi2_");

  // live_trade_borker_handler.Connect();

  // int qty;
  // while (std::cin >> qty) {
  //  mail_box.Send(qty);
  //}

  system.await_all_actors_done();

  std::cout << "espces:"
            << std::chrono::duration_cast<std::chrono::milliseconds>(
                   hrc::now() - beg)
                   .count()
            << "\n";
  return 0;
}

CAF_MAIN()
