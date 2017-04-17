#ifndef CTP_TRADER_H
#define CTP_TRADER_H
#include <fstream>
#include <boost/lexical_cast.hpp>
#include "ctpapi/ThostFtdcTraderApi.h"
#include "follow_strategy_mode/defines.h"
class CtpApi : public CThostFtdcTraderSpi {
 public:
  class Delegate {
   public:
    virtual void OnLogon(int frton_id, int session_id, bool success) = 0;
    virtual void OnOrderData(CThostFtdcOrderField* order) = 0;
    virtual void OnPositions(std::vector<OrderPosition> positions) = 0;
    virtual void OnSettlementInfoConfirm() = 0;
  };
  //////////////////////////////////////////////////////////////////////////
  // CtpTrader
  CtpApi(Delegate* delegate, const std::string& folw_path);

  void LoginServer(const std::string& front_server,
                   const std::string& broker_id,
                   const std::string& user_id,
                   const std::string& password);

  void OrderInsert(CThostFtdcInputOrderField order);

  void OrderAction(CThostFtdcInputOrderActionField order);

  void QryInvestorPosition();

  void SettlementInfoConfirm();

  virtual void OnFrontConnected() override;

  virtual void OnFrontDisconnected(int nReason) override;

  virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
                              CThostFtdcRspInfoField* pRspInfo,
                              int nRequestID,
                              bool bIsLast) override;

  virtual void OnRtnOrder(CThostFtdcOrderField* pOrder) override;

  virtual void OnRtnTrade(CThostFtdcTradeField* pTrade);

  virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder,
                                   CThostFtdcRspInfoField* pRspInfo);

  virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction,
                                   CThostFtdcRspInfoField* pRspInfo);

  virtual void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder,
                                CThostFtdcRspInfoField* pRspInfo,
                                int nRequestID,
                                bool bIsLast);

  virtual void OnRspQryInvestorPosition(
      CThostFtdcInvestorPositionField* pInvestorPosition,
      CThostFtdcRspInfoField* pRspInfo,
      int nRequestID,
      bool bIsLast) override;

  virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo,
                          int nRequestID,
                          bool bIsLast) override;

  virtual void OnRspQrySettlementInfoConfirm(
      CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
      CThostFtdcRspInfoField* pRspInfo,
      int nRequestID,
      bool bIsLast) override;

  virtual void OnRspSettlementInfoConfirm(
      CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
      CThostFtdcRspInfoField* pRspInfo,
      int nRequestID,
      bool bIsLast) override;

 private:
  CThostFtdcTraderApi* cta_api_;
  std::string broker_id_;
  std::string user_id_;
  std::string password_;
  Delegate* delegate_;
  std::vector<OrderPosition> positions_;
};

#endif /* CTP_TRADER_H */