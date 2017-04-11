#ifndef FOLLOW_TRADE_ORDER_ID_MANANGER_H
#define FOLLOW_TRADE_ORDER_ID_MANANGER_H
#include "geek_quant/caf_defines.h"

class OrderIdMananger {
public:
  OrderIdMananger(int start_order_id_seq);
  OrderData AdjustOrder(OrderData&& order);

private:
  std::map<std::pair<std::string, int>, int> session_corr_order_ids_;
  int start_order_id_seq_;
};

#endif // FOLLOW_TRADE_ORDER_ID_MANANGER_H



