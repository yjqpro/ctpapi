#ifndef FOLLOW_TRADE_INSTRUMENT_POSITION_H
#define FOLLOW_TRADE_INSTRUMENT_POSITION_H
#include "geek_quant/caf_defines.h"
class CloseCorrOrdersManager;

class InstrumentPosition {
 public:
  std::vector<OrderQuantity> GetQuantitys(
      std::vector<std::string> orders) const;

  int GetCloseableQuantityWithOrderDirection(OrderDirection direction) const;

  int GetCloseableQuantityWithInstrument(const std::string& order_id) const;

  void HandleRtnOrder(const OrderData& rtn_order,
                      CloseCorrOrdersManager* close_corr_orders_mgr);

 private:
  bool TestPositionEffect(const std::string& exchange_id,
                          PositionEffect position_effect,
                          bool is_today_quantity);
  std::map<std::string, OrderQuantity> positions_;
};

#endif  // FOLLOW_TRADE_INSTRUMENT_POSITION_H
