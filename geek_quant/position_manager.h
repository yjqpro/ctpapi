#ifndef FOLLOW_TRADE_POSITION_MANAGER_H
#define FOLLOW_TRADE_POSITION_MANAGER_H

#include "geek_quant/caf_defines.h"
#include "geek_quant/instrument_position.h"

class CloseCorrOrdersManager;

class PositionManager {
 public:
  std::vector<OrderQuantity> GetQuantitys(const std::string& instrument,
                                          std::vector<std::string> orders);

  int GetCloseableQuantityWithInstrument(const std::string& instrument,
                                         const std::string& order_id) const;

  int GetCloseableQuantityWithOrderDirection(const std::string& instrument,
                                             OrderDirection direction) const;

  void HandleRtnOrder(const OrderData& rtn_order,
                      CloseCorrOrdersManager* close_corr_orders_mgr);

 private:
  std::map<std::string, InstrumentPosition> instrument_positions_;
};

#endif  // FOLLOW_TRADE_POSITION_MANAGER_H
