#ifndef FOLLOW_TRADE_FOLLOW_strategy_SERVICE_H
#define FOLLOW_TRADE_FOLLOW_strategy_SERVICE_H
#include "geek_quant/caf_defines.h"
#include "geek_quant/follow_strategy.h"
#include "geek_quant/context.h"
#include "geek_quant/order_id_mananger.h"

class FollowStragetyService : public FollowStragety::Delegate {
 public:
  class Delegate {
   public:
    virtual void OpenOrder(const std::string& instrument,
                           const std::string& order_no,
                           OrderDirection direction,
                           OrderPriceType price_type,
                           double price,
                           int quantity) = 0;

    virtual void CloseOrder(const std::string& instrument,
                            const std::string& order_no,
                            OrderDirection direction,
                            PositionEffect position_effect,
                            OrderPriceType price_type,
                            double price,
                            int quantity) = 0;
    virtual void CancelOrder(const std::string& order_no) = 0;
  };

  FollowStragetyService(const std::string& master_account,
                        const std::string& slave_account,
                        Delegate* delegate,
                        int start_order_id_seq);

  void HandleRtnOrder(OrderData rtn_order);

  virtual void CancelOrder(const std::string& order_no) override;

  virtual void OpenOrder(const std::string& instrument,
                         const std::string& order_no,
                         OrderDirection direction,
                         OrderPriceType price_type,
                         double price,
                         int quantity) override;

  virtual void CloseOrder(const std::string& instrument,
                          const std::string& order_no,
                          OrderDirection direction,
                          PositionEffect position_effect,
                          OrderPriceType price_type,
                          double price,
                          int quantity) override;

 private:
  enum class StragetyStatus {
    kWaitReply,
    kReady,
    kSkip,
  };

  void Trade(const std::string& order_no);

  void DoHandleRtnOrder(OrderData rtn_order);

  StragetyStatus BeforeHandleOrder(OrderData order);
  Context context_;
  FollowStragety stragety_;
  Delegate* delegate_;
  std::vector<std::string> waiting_reply_order_;
  std::deque<OrderData> outstanding_orders_;
  std::string master_account_;
  std::string slave_account_;
};

#endif  // FOLLOW_TRADE_FOLLOW_strategy_SERVICE_H