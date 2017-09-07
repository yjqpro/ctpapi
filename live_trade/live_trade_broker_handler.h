#ifndef LIVE_TRADE_LIVE_TRADE_BROKER_HANDLER_H
#define LIVE_TRADE_LIVE_TRADE_BROKER_HANDLER_H
#include <boost/format.hpp>
#include "ctpapi/ThostFtdcTraderApi.h"
#include "common/api_struct.h"

template <typename MailBox>
class LiveTradeBrokerHandler : public CThostFtdcTraderSpi {
 public:
  LiveTradeBrokerHandler(MailBox* mail_box) : mail_box_(mail_box) {
    api_ = CThostFtdcTraderApi::CreateFtdcTraderApi();
    api_->RegisterSpi(this);
    mail_box_->Subscribe(&LiveTradeBrokerHandler::HandleInputOrder, this);
  }

  void Connect(const std::string& server,
               std::string broker_id,
               std::string user_id,
               std::string password) {
    broker_id_ = std::move(broker_id);
    user_id_ = std::move(user_id);
    password_ = std::move(password);
    char fron_server[255] = {0};
    strcpy(fron_server, server.c_str());
    api_->RegisterFront(fron_server);
    api_->SubscribePublicTopic(THOST_TERT_RESUME);
    api_->SubscribePrivateTopic(THOST_TERT_RESUME);
    api_->Init();
  }

  void HandleInputOrder(const InputOrder& input_order) {
    CThostFtdcInputOrderField field = {0};
    strcpy(field.InstrumentID, input_order.instrument_.c_str());
    // strcpy(field.OrderRef, order_ref.c_str());
    field.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    field.Direction =
        OrderDirectionToTThostOrderDireciton(input_order.order_direction_);
    field.CombOffsetFlag[0] =
        PositionEffectToTThostOffsetFlag(input_order.position_effect_);
    strcpy(field.CombHedgeFlag, "1");
    field.LimitPrice = input_order.price_;
    field.VolumeTotalOriginal = input_order.qty_;
    field.TimeCondition = THOST_FTDC_TC_GFD;
    strcpy(field.GTDDate, "");
    field.VolumeCondition = THOST_FTDC_VC_AV;
    field.MinVolume = 1;
    field.ContingentCondition = THOST_FTDC_CC_Immediately;
    field.StopPrice = 0;
    field.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    field.IsAutoSuspend = 0;
    field.UserForceClose = 0;
    strcpy(field.BrokerID, broker_id_.c_str());
    strcpy(field.UserID, user_id_.c_str());
    strcpy(field.InvestorID, user_id_.c_str());

    if (api_->ReqOrderInsert(&field, 0) != 0) {
      auto order_field = std::make_shared<OrderField>();
      // order_field->strategy_id = sub_account_id;
      // order_field->order_id = sub_order_id;
      order_field->status = OrderStatus::kInputRejected;
      mail_box_->Send(std::move(order_field));
    }
  }

  virtual void OnFrontConnected() override {
    CThostFtdcReqUserLoginField field{0};
    strcpy(field.UserID, user_id_.c_str());
    strcpy(field.Password, password_.c_str());
    strcpy(field.BrokerID, broker_id_.c_str());
    api_->ReqUserLogin(&field, 0);
  }

  virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
                              CThostFtdcRspInfoField* pRspInfo,
                              int nRequestID,
                              bool bIsLast) override {
    if (pRspInfo == NULL || pRspInfo->ErrorID != 0) {
      // TODO: Logon Fail
    } else {
      session_id_ = pRspUserLogin->SessionID;
      front_id_ = pRspUserLogin->FrontID;
    }
  }

  void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout,
                       CThostFtdcRspInfoField* pRspInfo,
                       int nRequestID,
                       bool bIsLast) {}

  virtual void OnRtnOrder(CThostFtdcOrderField* pOrder) override {
    auto order_field = std::make_shared<OrderField>();
    // order_field->instrument_name = order->InstrumentName;
    order_field->instrument_id = pOrder->InstrumentID;
    order_field->exchange_id = pOrder->ExchangeID;
    order_field->direction = pOrder->Direction == THOST_FTDC_D_Buy
                                 ? OrderDirection::kBuy
                                 : OrderDirection::kSell;
    order_field->qty = pOrder->VolumeTotalOriginal;
    order_field->price = pOrder->LimitPrice;
    order_field->position_effect =
        ParseTThostFtdcPositionEffect(pOrder->CombOffsetFlag[0]);
    order_field->date = pOrder->InsertDate;
    // order_field->input_timestamp = pOrder->InsertTime;
    // order_field->update_timestamp = pOrder->UpdateTime;

    order_field->status = ParseTThostFtdcOrderStatus(pOrder);
    order_field->leaves_qty = pOrder->VolumeTotal;
    order_field->traded_qty = pOrder->VolumeTraded;
    order_field->error_id = 0;
    order_field->raw_error_id = 0;
    order_field->strategy_id = pOrder->InvestorID;
    order_field->order_id =
        MakeOrderId(pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef);

    mail_box_->Send(std::move(order_field));
  }

  virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo,
                          int nRequestID,
                          bool bIsLast) override {}

  virtual void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder,
                                CThostFtdcRspInfoField* pRspInfo,
                                int nRequestID,
                                bool bIsLast) override {
    throw std::logic_error("The method or operation is not implemented.");
  }

  virtual void OnRspOrderAction(
      CThostFtdcInputOrderActionField* pInputOrderAction,
      CThostFtdcRspInfoField* pRspInfo,
      int nRequestID,
      bool bIsLast) override {
    throw std::logic_error("The method or operation is not implemented.");
  }

 private:
  std::string MakeOrderId(TThostFtdcFrontIDType front_id,
                          TThostFtdcSessionIDType session_id,
                          const std::string& order_ref) const {
    return str(boost::format("%d:%d:%s") % front_id % session_id % order_ref);
  }

  PositionEffect ParseTThostFtdcPositionEffect(TThostFtdcOffsetFlagType flag) {
    PositionEffect ps = PositionEffect::kUndefine;
    switch (flag) {
      case THOST_FTDC_OF_Open:
        ps = PositionEffect::kOpen;
        break;
      case THOST_FTDC_OF_Close:
      case THOST_FTDC_OF_ForceClose:
      case THOST_FTDC_OF_CloseYesterday:
      case THOST_FTDC_OF_ForceOff:
      case THOST_FTDC_OF_LocalForceClose:
        ps = PositionEffect::kClose;
        break;
      case THOST_FTDC_OF_CloseToday:
        ps = PositionEffect::kCloseToday;
        break;
    }
    return ps;
  }

  OrderStatus ParseTThostFtdcOrderStatus(CThostFtdcOrderField* order) const {
    OrderStatus os = OrderStatus::kActive;
    switch (order->OrderStatus) {
      case THOST_FTDC_OST_AllTraded:
        os = OrderStatus::kAllFilled;
        break;
      case THOST_FTDC_OST_Canceled:
        os = OrderStatus::kCanceled;
        break;
      default:
        break;
    }
    return os;
  }

  TThostFtdcDirectionType OrderDirectionToTThostOrderDireciton(
      OrderDirection direction) {
    return direction == OrderDirection::kBuy ? THOST_FTDC_D_Buy
                                             : THOST_FTDC_D_Sell;
  }

  TThostFtdcOffsetFlagType PositionEffectToTThostOffsetFlag(
      PositionEffect position_effect) {
    return position_effect == PositionEffect::kOpen
               ? THOST_FTDC_OF_Open
               : (position_effect == PositionEffect::kCloseToday
                      ? THOST_FTDC_OF_CloseToday
                      : THOST_FTDC_OF_Close);
  }

  CThostFtdcTraderApi* api_;
  MailBox* mail_box_;
  std::string broker_id_;
  std::string user_id_;
  std::string password_;
  TThostFtdcSessionIDType session_id_ = 0;
  TThostFtdcFrontIDType front_id_ = 0;
};

#endif  // LIVE_TRADE_LIVE_TRADE_BROKER_HANDLER_H