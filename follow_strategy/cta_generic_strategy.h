#ifndef follow_strategy_CTA_GENERIC_STRATEGY_H
#define follow_strategy_CTA_GENERIC_STRATEGY_H
#include "enter_order_observer.h"
#include "strategy_enter_order_observable.h"

class CTAGenericStrategy : public StrategyEnterOrderObservable,
                           public EnterOrderObserver {
 public:
  CTAGenericStrategy(std::string strategy_id);

  virtual void Subscribe(
      StrategyEnterOrderObservable::Observer* observer) override;

  virtual void OpenOrder(const std::string& instrument,
                         const std::string& order_id,
                         OrderDirection direction,
                         double price,
                         int quantity) override;

  virtual void CloseOrder(const std::string& instrument,
                          const std::string& order_id,
                          OrderDirection direction,
                          PositionEffect position_effect,
                          double price,
                          int quantity) override;

  virtual void CancelOrder(const std::string& order_id) override;

 private:
  StrategyEnterOrderObservable::Observer* observer_;
  std::string strategy_id_;
};

#endif  // follow_strategy_CTA_GENERIC_STRATEGY_H