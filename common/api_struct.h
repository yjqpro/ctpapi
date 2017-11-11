#ifndef COMMON_API_STRUCT_H
#define COMMON_API_STRUCT_H
#include <string>
#include <memory>
#include "common/api_data_type.h"

struct Instrument {
  std::string symbol;
  double multiplier;
  double mfprice;
};

struct OrderField {
  OrderDirection direction;
  OrderDirection position_effect_direction;
  PositionEffect position_effect;
  OrderStatus status;
  int qty;
  int leaves_qty;
  int trading_qty;
  int error_id;
  int raw_error_id;
  double input_price;
  double trading_price;
  double avg_price;
  TimeStamp input_timestamp;
  TimeStamp update_timestamp;
  std::string strategy_id;
  std::string instrument_id;
  std::string exchange_id;
  std::string date;
  std::string order_id;
  std::string raw_error_message;
};

struct CTAOrderSignalField {
  int long_qty;
  int short_qty;
  int opening_long_qty;
  int opening_short_qty;
  int closeing_long_qty;
  int closeing_short_qty;
  std::string instrument;
  PositionEffect position_effect;
  OrderDirection direction;
  OrderStatus order_status;
  int qty;
  int leaves_qty;
  int trading_qty;
  double input_price;
  double avg_price;
  double trading_price;
  TimeStamp timestamp;
};

struct InvestorPositionField {
  int qty;
  OrderDirection direction;
  double price;
  std::string instrument_id;
};

struct CorrOrderQuantity {
  std::string order_id;
  int quantity;
  int corr_quantity;
};

struct OrderPosition {
  std::string instrument;
  OrderDirection order_direction;
  int quantity;
};

struct OrderQuantity {
  std::string order_id;
  OrderDirection direction;
  bool is_today_quantity;
  int quantity;
  int closeable_quantity;
};

struct AccountPortfolio {
  std::string instrument;
  OrderDirection direction;
  int closeable;
  int open;
  int close;
};

struct AccountPosition {
  std::string instrument;
  OrderDirection direction;
  int closeable;
};

struct Tick {
  int64_t timestamp;
  double last_price;
  int64_t qty;
  double bid_price1;
  int64_t bid_qty1;
  double ask_price1;
  int64_t ask_qty1;
};

struct TickData {
  std::shared_ptr<std::string> instrument;
  std::shared_ptr<Tick> tick;
};

struct CTATransaction {
  int64_t timestamp;
  OrderIDType order_id;
  int32_t position_effect;
  int32_t direction;
  int32_t status;
  double price;
  int32_t qty;
  int32_t traded_qty;
};

struct OpenOrder {
  std::string instrument;
  TimeStamp open_timestamp;
  OrderDirection direction;
  double opne_price;
  int qty;
};

struct CostBasis {
  CommissionType type;
  double open_commission;
  double close_commission;
  double close_today_commission;
};

struct InputOrderSignal {
  std::string instrument;
  std::string order_id;
  PositionEffect position_effect;
  OrderDirection direction;
  double price;
  int qty;
  TimeStamp timestamp;
};

struct InputOrder {
  std::string instrument;
  std::string order_id;
  PositionEffect position_effect;
  OrderDirection direction;
  double price;
  int qty;
  TimeStamp timestamp;
};

struct CancelOrderSignal {
  std::string order_id;
  std::string instrument;
  int qty;
};

struct CTAPositionQty {
  int position;
  int frozen;
};

struct CTPOrderField {
  OrderDirection direction;
  OrderDirection position_effect_direction;
  CTPPositionEffect position_effect;
  OrderStatus status;
  int qty;
  int leaves_qty;
  int trading_qty;
  int error_id;
  int raw_error_id;
  int front_id;
  int session_id;
  double input_price;
  double trading_price;
  double avg_price;
  TimeStamp input_timestamp;
  TimeStamp update_timestamp;
  std::string instrument;
  std::string exchange_id;
  std::string date;
  std::string order_id;
  std::string order_ref;
  std::string order_sys_id;
  std::string raw_error_message;
};

struct CTPEnterOrder {
  std::string instrument;
  std::string order_id;
  CTPPositionEffect position_effect;
  OrderDirection direction;
  double price;
  int qty;
  TimeStamp timestamp;
};

struct CTPCancelOrder {
  int front_id;
  int session_id;
  std::string order_id;
  std::string order_ref;
  std::string order_sys_id;
  std::string exchange_id;
};

#endif  // COMMON_API_STRUCT_H
