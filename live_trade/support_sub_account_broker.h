#ifndef LIVE_TRADE_SUPPORT_SUB_ACCOUNT_BROKER_H
#define LIVE_TRADE_SUPPORT_SUB_ACCOUNT_BROKER_H
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include "caf/all.hpp"
#include "ctp_trader_api.h"
#include "common/api_struct.h"

class SupportSubAccountBroker : public caf::event_based_actor,
                                public CTPTraderApi::Delegate {
 public:
  SupportSubAccountBroker(caf::actor_config& cfg);

  virtual caf::behavior make_behavior() override;

  virtual void HandleCTPRtnOrder(
      const std::shared_ptr<CTPOrderField>& order) override;

  void Connect(const std::string& server,
               const std::string& broker_id,
               const std::string& user_id,
               const std::string& password);


 private:
  struct SubAccountOrderId {
    std::string sub_account_id;
    std::string order_id;
  };

  struct HashSubAccountOrderId {
    size_t operator()(const SubAccountOrderId& id) const {
      return std::hash<std::string>()(id.sub_account_id + ":" + id.order_id);
    }
  };

  struct CompareSubAccountOrderId {
    bool operator()(const SubAccountOrderId& l,
                    const SubAccountOrderId& r) const {
      return l.sub_account_id == r.sub_account_id && l.order_id == r.order_id;
    }
  };
  std::string GenerateOrderRef();
  using OrderIdBimap =
      boost::bimap<boost::bimaps::unordered_set_of<SubAccountOrderId,
                                                   HashSubAccountOrderId,
                                                   CompareSubAccountOrderId>,
                   std::string>;
  OrderIdBimap order_id_bimap_;
  std::unordered_map<std::string, caf::actor> sub_actors_;
  CTPTraderApi trader_api_;
  int order_seq_ = 0;
};
#endif  // LIVE_TRADE_SUPPORT_SUB_ACCOUNT_BROKER_H