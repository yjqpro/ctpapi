#ifndef FOLLOW_STRATEGY_CTA_TRADED_SIGNAL_H
#define FOLLOW_STRATEGY_CTA_TRADED_SIGNAL_H
#include <list>
#include <set>
#include "cta_sigle_observer.h"
#include "enter_order_observer.h"
#include "orders_context.h"
#include "common/api_struct.h"

class CTATradedSignal : public CTASignalObserver {
 public:
  CTATradedSignal(int delayed_open_order);
  void SetOrdersContext(std::shared_ptr<OrdersContext> master_context,
                        std::shared_ptr<OrdersContext> slave_context);

  virtual void BeforeCloseMarket() override;

  virtual void HandleTick(const std::shared_ptr<TickData>& tick) override;

  virtual void HandleOpening(
      const std::shared_ptr<const OrderField>& order_data) override;

  virtual void HandleCloseing(
      const std::shared_ptr<const OrderField>& order_data) override;

  virtual void HandleCanceled(
      const std::shared_ptr<const OrderField>& order_data) override;

  virtual void HandleClosed(
      const std::shared_ptr<const OrderField>& order_data) override;

  virtual void HandleOpened(
      const std::shared_ptr<const OrderField>& order_data) override;

  // virtual void Subscribe(CTASignalObserver::Observable* observer) override;

 private:
  class CompareOrderId {
   public:
    using is_transparent = void;
    bool operator()(const InputOrderSignal& l,
                    const InputOrderSignal& r) const {
      return l.timestamp_ < r.timestamp_;
    }

    bool operator()(const std::string& order_id,
                    const InputOrderSignal& r) const {
      return order_id < r.order_id;
    }

    bool operator()(const InputOrderSignal& l,
                    const std::string& order_id) const {
      return l.order_id < order_id;
    }
    bool operator()(const InputOrderSignal& l, TimeStamp timestamp) const {
      return l.timestamp_ < timestamp;
    }
    bool operator()(TimeStamp timestamp, const InputOrderSignal& l) const {
      return timestamp < l.timestamp_;
    }
  };
  std::string GenerateOrderId();
  std::shared_ptr<OrdersContext> master_context_;
  std::shared_ptr<OrdersContext> slave_context_;
  CTASignalObserver::Observable* observer_;
  std::multiset<InputOrderSignal, CompareOrderId> pending_delayed_open_order_;
  int delayed_open_order_ = 0;
  std::string order_id_prefix_;
  int order_id_seq_ = 0;
};
#endif  // FOLLOW_STRATEGY_CTA_TRADED_SIGNAL_H