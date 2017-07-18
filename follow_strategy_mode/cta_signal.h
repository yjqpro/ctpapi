#ifndef FOLLOW_STRATEGY_MODE_CTA_SIGNAL_H
#define FOLLOW_STRATEGY_MODE_CTA_SIGNAL_H
#include "cta_sigle_observer.h"
#include "enter_order_observer.h"
#include "orders_context.h"

class CTASignal : public CTASignalObserver {
 public:
  void SetOrdersContext(std::shared_ptr<OrdersContext> master_context,
                        std::shared_ptr<OrdersContext> slave_context);

  virtual void HandleOpening(const OrderData& order_data) override;

  virtual void HandleCloseing(const OrderData& order_data) override;

  virtual void HandleCanceled(const OrderData& order_data) override;

  virtual void HandleClosed(const OrderData& order_data) override;

  virtual void HandleOpened(const OrderData& order_data) override;

  virtual void Subscribe(CTASignalObserver::Observable* observer) override;
 private:
  std::shared_ptr<OrdersContext> master_context_;
  std::shared_ptr<OrdersContext> slave_context_;
  CTASignalObserver::Observable* observer_;
};

#endif  // FOLLOW_STRATEGY_MODE_CTA_SIGNAL_H
