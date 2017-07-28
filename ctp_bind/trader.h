#ifndef CTP_BIND_DEMO_TRADER_H
#define CTP_BIND_DEMO_TRADER_H
#include <atomic>
#include <boost/any.hpp>
#include <boost/asio.hpp>
#include <boost/bimap.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/variant.hpp>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "common/api_struct.h"
#include "ctpapi/ThostFtdcTraderApi.h"
#include "ctpapi/ThostFtdcUserApiStruct.h"

namespace ctp_bind {
class Trader : public CThostFtdcTraderSpi {
 public:
  Trader(std::string server,
         std::string broker_id,
         std::string user_id,
         std::string password);

  void InitAsio(boost::asio::io_service* io_service = nullptr);

  void Connect(std::function<void(CThostFtdcRspUserLoginField*,
                                  CThostFtdcRspInfoField*)> callback);

  void Run();

  void LimitOrder(std::string sub_account_id,
                  std::string sub_order_id,
                  std::string instrument,
                  PositionEffect position_effect,
                  OrderDirection direction,
                  double price,
                  int volume);

  void LimitOrder(std::string instrument,
                  PositionEffect position_effect,
                  OrderDirection direction,
                  double price,
                  int volume);

  void CancelOrder(std::string sub_accont_id, std::string sub_order_id);

  // void CancelOrder(std::string order_id);

  void SubscribeRtnOrder(
      std::string sub_account_id,
      std::function<void(boost::shared_ptr<OrderField>)> callback);

  void SubscribeRtnOrder(
      std::function<void(boost::shared_ptr<OrderField>)> callback);

  void ReqInvestorPosition(
      std::function<void(InvestorPositionField, bool)> callback);

  virtual void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder,
                                CThostFtdcRspInfoField* pRspInfo,
                                int nRequestID,
                                bool bIsLast) override;

  virtual void OnRspOrderAction(
      CThostFtdcInputOrderActionField* pInputOrderAction,
      CThostFtdcRspInfoField* pRspInfo,
      int nRequestID,
      bool bIsLast) override;

  virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo,
                          int nRequestID,
                          bool bIsLast) override;

  virtual void OnRspAuthenticate(
      CThostFtdcRspAuthenticateField* pRspAuthenticateField,
      CThostFtdcRspInfoField* pRspInfo,
      int nRequestID,
      bool bIsLast) override;

  virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
                              CThostFtdcRspInfoField* pRspInfo,
                              int nRequestID,
                              bool bIsLast) override;

  virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout,
                               CThostFtdcRspInfoField* pRspInfo,
                               int nRequestID,
                               bool bIsLast) override;

  virtual void OnRtnOrder(CThostFtdcOrderField* pOrder) override;

  virtual void OnFrontConnected() override;

  virtual void OnFrontDisconnected(int nReason) override;

  virtual void OnRspQryInvestorPosition(
      CThostFtdcInvestorPositionField* pInvestorPosition,
      CThostFtdcRspInfoField* pRspInfo,
      int nRequestID,
      bool bIsLast) override;

 private:
  void CancelOrderOnIOThread(std::string sub_accont_id, std::string order_id);

  void OnRtnOrderOnIOThread(boost::shared_ptr<CThostFtdcOrderField> order);

  std::string MakeOrderId(TThostFtdcFrontIDType front_id,
                          TThostFtdcSessionIDType session_id,
                          const std::string& order_ref) const;

  OrderStatus ParseTThostFtdcOrderStatus(
      boost::shared_ptr<CThostFtdcOrderField> order) const;

  PositionEffect ParseTThostFtdcPositionEffect(TThostFtdcOffsetFlagType flag);

  CThostFtdcTraderApi* api_;
  boost::asio::io_service* io_service_;
  std::shared_ptr<boost::asio::io_service::work> io_worker_;

  std::unordered_map<std::string, boost::shared_ptr<CThostFtdcOrderField>>
      orders_;

  typedef boost::bimap<std::pair<std::string, std::string>, std::string>
      SubOrderIDBiomap;
  SubOrderIDBiomap sub_order_ids_;

  std::unordered_map<std::string,
                     std::function<void(boost::shared_ptr<OrderField>)>>
      sub_account_on_rtn_order_callbacks_;

  std::unordered_map<int, boost::any> response_;

  std::function<void(CThostFtdcRspUserLoginField*, CThostFtdcRspInfoField*)>
      on_connect_;

  std::function<void(boost::shared_ptr<OrderField>)> on_rtn_order_;

  std::atomic<int> request_id_ = 0;
  std::atomic<int> order_ref_index_ = 0;
  std::string server_;
  std::string broker_id_;
  std::string user_id_;
  std::string password_;
  TThostFtdcSessionIDType session_id_ = 0;
  TThostFtdcFrontIDType front_id_ = 0;
  bool IsErrorRspInfo(CThostFtdcRspInfoField* pRspInfo) const;
  TThostFtdcDirectionType OrderDirectionToTThostOrderDireciton(
      OrderDirection direction);
  TThostFtdcOffsetFlagType PositionEffectToTThostOffsetFlag(
      PositionEffect position_effect);
};
}  // namespace ctp_bind

#endif  // CTP_BIND_DEMO_TRADER_H
