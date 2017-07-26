#ifndef FOLLOW_TRADE_DEFINES_H
#define FOLLOW_TRADE_DEFINES_H

enum class OrderEventType {
  kIgnore,
  kNewOpen,
  kNewClose,
  kOpenTraded,
  kCloseTraded,
  kCanceled,
};

enum class OrderDirection {
  kUndefine,
  kBuy,
  kSell 
};

enum class PositionEffect { 
  kUndefine,
  kOpen,
  kClose,
  kCloseToday 
};

enum class OrderStatus {
  kActive,
  kAllFilled,
  kCanceled,
  kInputRejected,
  kCancelRejected,
};


#endif  // FOLLOW_TRADE_DEFINES_H
