#ifndef FOLLOW_TRADE_CONTEXT_H
#define FOLLOW_TRADE_CONTEXT_H
#include "geek_quant/caf_defines.h"
#include "geek_quant/order_manager.h"
#include "geek_quant/position_manager.h"
#include "geek_quant/close_corr_orders_manager.h"

class Context {
 public:
  OrderEventType HandlertnOrder(const OrderData& rtn_order);

  std::vector<OrderQuantity> GetQuantitys(const std::string& account_id,
                                          std::vector<std::string> order_ids);

  int GetCloseableQuantityWithOrderDirection(const std::string& account_id,
                                             const std::string& instrument,
                                             OrderDirection direction) const;

  std::vector<std::pair<std::string, int> > GetCorrOrderQuantiys(
      const std::string& account_id,
      const std::string& order_id);

  std::vector<std::string> GetCloseCorrOrderIds(const std::string& account_id,
                                                const std::string& order_id);

  int ActiveOrderCount(const std::string& account_id,
                       const std::string& instrument,
                       OrderDirection direction) const;

  std::vector<std::string> ActiveOrderIds(const std::string& account_id,
                                          const std::string& instrument,
                                          OrderDirection direction) const;

  bool IsActiveOrder(const std::string& account_id,
                     const std::string& order_id) const;

  int GetCloseableQuantity(const std::string& account_id,
                           const std::string& order_id) const;

  bool IsOppositeOpen(const std::string& account_id,
                      const std::string& instrument,
                      OrderDirection direction) const;

 private:
  std::map<std::string, OrderManager> account_order_mgr_;
  std::map<std::string, PositionManager> account_position_mgr_;
  std::map<std::string, CloseCorrOrdersManager> account_close_corr_orders_mgr_;
};

#endif  // FOLLOW_TRADE_CONTEXT_H
