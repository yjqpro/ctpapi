#ifndef follow_strategy_CTA_SIGNAL_DISPATCH_H
#define follow_strategy_CTA_SIGNAL_DISPATCH_H
#include "cta_sigle_observer.h"
#include "enter_order_observer.h"
#include "orders_context.h"
#include "rtn_order_observer.h"
#include "portfolio_observer.h"

class CTASignalDispatch : public CTASignalObserver::Observable,
                          public RtnOrderObserver {
 public:
  CTASignalDispatch(std::shared_ptr<CTASignalObserver> signal_observer);

  virtual void SubscribeEnterOrderObserver(
      std::shared_ptr<EnterOrderObserver> observer);

  // RtnOrderObservable::Observer
  virtual void RtnOrder(
      const std::shared_ptr<const OrderField>& order) override;

  // EnterOrderObservable::Observer
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

  void SetOrdersContext(std::shared_ptr<OrdersContext> master_context,
                        std::shared_ptr<OrdersContext> slave_context);

  void SubscribePortfolioObserver(std::shared_ptr<PortfolioObserver> observer);

 private:
  enum class StragetyStatus {
    kWaitReply,
    kReady,
    kSkip,
  };

  void Trade(const std::string& order_id, OrderStatus status);

  void DoHandleRtnOrder(const std::shared_ptr<const OrderField>& rtn_order);

  StragetyStatus BeforeHandleOrder(
      const std::shared_ptr<const OrderField>& order);

  OrderEventType OrdersContextHandleRtnOrder(
      const std::shared_ptr<const OrderField>& order);

  std::vector<std::pair<std::string, OrderStatus>> waiting_reply_order_;

  std::deque<std::shared_ptr<const OrderField>> outstanding_orders_;
  std::shared_ptr<CTASignalObserver> signal_observer_;
  std::shared_ptr<EnterOrderObserver> enter_order_observer_;
  std::shared_ptr<OrdersContext> master_context_;
  std::shared_ptr<OrdersContext> slave_context_;
  std::shared_ptr<PortfolioObserver> portfolio_observer_;
};
#endif  // follow_strategy_CTA_SIGNAL_DISPATCH_H
