#ifndef COMMON_API_STRUCT_H
#define COMMON_API_STRUCT_H
#include <string>
#include <memory>
#include "common/api_data_type.h"

struct InstrumentField {
  InstrumentIDType instrument;
  double margin_rate;
};

struct OrderField {
  OrderDirection direction;
  PositionEffect position_effect;
  OrderStatus status;
  int qty;
  int leaves_qty;
  int traded_qty;
  int error_id;
  int raw_error_id;
  double price;
  double avg_price;
  std::string strategy_id;
  std::string instrument_id;
  std::string exchange_id;
  std::string date;
  std::string input_time;
  std::string update_time;
  std::string order_id;
  std::string raw_error_message;
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
  int32_t position_effect;
  int32_t direction;
  double price;
  int32_t qty;
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
  int open_commission;
  int close_commission;
  int close_today_commission;
};

#endif  // COMMON_API_STRUCT_H
