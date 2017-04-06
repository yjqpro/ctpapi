#ifndef FOLLOW_TRADE_FOLLOW_STRAGETY_H
#define FOLLOW_TRADE_FOLLOW_STRAGETY_H
#include "boost/optional.hpp"
#include "geek_quant/caf_defines.h"
#include "geek_quant/order_follow.h"
#include "geek_quant/order_follow_manager.h"
#include "geek_quant/pending_order_action.h"

class FollowStragety {
 public:
  FollowStragety(bool wait_sync = false);

  void SyncComplete();

  void AddPositionToTrader(const std::string& order_no,
                           OrderDirection order_direction,
                           int volume);

  void AddPositionToFollower(const std::string& order_no,
                             OrderDirection order_direction,
                             int volume);

  void HandleOrderRtnForTrader(const OrderRtnData& order,
                               EnterOrderData* enter_order,
                               std::vector<std::string>* cancel_order_no_list);

  void HandleOrderRtnForFollow(const OrderRtnData& order,
                               EnterOrderData* enter_order,
                               std::vector<std::string>* cancel_order_no_list);

 private:
  bool WaitSyncOrders();

  EnterOrderData MakeOpenReverseAction(
      const OrderRtnData& order,
      std::vector<std::pair<std::string, int> > order_volumes);

  EnterOrderData MakeOpeningAction(const OrderRtnData& order);

  EnterOrderData MakeCloseingAction(
      const OrderRtnData& order,
      const CloseingActionInfo& action,
      std::vector<std::pair<std::string, int> > order_volumes);

  int GetPendingCloseVolume(OrderDirection order_direction) const;

  bool wait_sync_;

  std::map<std::string, PendingOrderAction> pending_order_actions_;
  OrderFollowMananger trader_orders_;
  OrderFollowMananger follower_orders_;
};

#endif  // FOLLOW_TRADE_FOLLOW_STRAGETY_H