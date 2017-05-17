﻿#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/optional.hpp>
#include "caf/all.hpp"
#include "follow_trade_server/ctp_trader.h"

using LoginAtom = caf::atom_constant<caf::atom("login")>;
using InstrumentMarginRateAtom = caf::atom_constant<caf::atom("margin")>;
using QryInstrumentListAtom = caf::atom_constant<caf::atom("inlist")>;

using RspLoginAtom = caf::atom_constant<caf::atom("rsplogin")>;
using RspInstrumentMarginRateAtom = caf::atom_constant<caf::atom("rspmargin")>;
using RspQryInstrumentListAtom = caf::atom_constant<caf::atom("rspinslist")>;

CAF_ALLOW_UNSAFE_MESSAGE_TYPE(std::vector<CThostFtdcInstrumentField>)
// CAF_ALLOW_UNSAFE_MESSAGE_TYPE(CThostFtdcInstrumentMarginRateField)
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(
    boost::optional<CThostFtdcInstrumentMarginRateField>)

class CtpAPIDelegate : public caf::event_based_actor, public CtpApi::Delegate {
 public:
  CtpAPIDelegate(caf::actor_config& config)
      : caf::event_based_actor(config), api_(this, "") {}

  virtual void OnLogon(int frton_id, int session_id, bool success) override {
    send(this, RspLoginAtom::value);
  }

  virtual void OnOrderData(CThostFtdcOrderField* order) override {}

  virtual void OnPositions(std::vector<OrderPosition> positions) override {}

  virtual void OnSettlementInfoConfirm() override {}

  virtual void OnRspQryInstrumentMarginRate(
      CThostFtdcInstrumentMarginRateField* margin) override {
    send(this, RspInstrumentMarginRateAtom::value,
         margin != NULL
             ? boost::optional<CThostFtdcInstrumentMarginRateField>(*margin)
             : boost::optional<CThostFtdcInstrumentMarginRateField>());
  }

  virtual void OnRspQryInstrumentList(
      std::vector<CThostFtdcInstrumentField> instruments) override {
    send(this, RspQryInstrumentListAtom::value, std::move(instruments));
  }

 protected:
  virtual caf::behavior make_behavior() override {
    return {
        [=](LoginAtom) {
          api_.LoginServer("tcp://59.42.241.91:41205", "9080", "38030022",
                           "140616");
          logon_response_promises_ = make_response_promise<bool>();
        },
        [=](InstrumentMarginRateAtom, const std::string& instrument) {
          qry_margin_rate_promise_ = make_response_promise<
              boost::optional<CThostFtdcInstrumentMarginRateField>>();
          api_.ReqQryInstrumentMarginRate(instrument);
        },
        [=](QryInstrumentListAtom) {
          qry_instrument_list_promise_ =
              make_response_promise<std::vector<CThostFtdcInstrumentField>>();
          api_.ReqQryInstrumentList();
        },
        [=](RspLoginAtom) { logon_response_promises_.deliver(true); },
        [=](RspInstrumentMarginRateAtom,
            boost::optional<CThostFtdcInstrumentMarginRateField> margin) {
          qry_margin_rate_promise_.deliver(margin);
        },
        [=](RspQryInstrumentListAtom,
            std::vector<CThostFtdcInstrumentField> instrument_list) {
          qry_instrument_list_promise_.deliver(instrument_list);
        },
    };
  }

 private:
  CtpApi api_;
  typedef caf::detail::make_response_promise_helper<bool>::type
      BoolResponsePromise;
  BoolResponsePromise logon_response_promises_;

  typedef caf::detail::make_response_promise_helper<
      boost::optional<CThostFtdcInstrumentMarginRateField>>::type
      QryInstrumentMarginRateResponsePromise;
  QryInstrumentMarginRateResponsePromise qry_margin_rate_promise_;

  typedef caf::detail::make_response_promise_helper<
      std::vector<CThostFtdcInstrumentField>>::type
      QryInstrumentListResponsePromise;
  QryInstrumentListResponsePromise qry_instrument_list_promise_;
};

bool Logon(caf::actor actor) {
  auto f = caf::make_function_view(actor, {caf::time_unit::seconds, 3});

  auto ret = f(LoginAtom::value);
  if (!ret || !ret->get_as<bool>(0)) {
    return false;
  }
  return true;
}

template <typename RetType, typename AtomType>
RetType CallActorFunction(caf::actor actor, AtomType atom_value) {
  auto f = caf::make_function_view(actor, caf::infinite);

  auto ret = f(atom_value);
  return ret->get_as<RetType>(0);
}

template <typename RetType, typename Param1, typename AtomType>
RetType CallActorFunctionWithParam(caf::actor actor,
                                   AtomType atom_value,
                                   Param1 param) {
  auto f = caf::make_function_view(actor, caf::infinite);

  auto ret = f(atom_value, param);
  return ret->get_as<RetType>(0);
}

namespace boost {
namespace serialization {

template <class Archive>
void serialize(Archive& ar,
               CThostFtdcInstrumentMarginRateField& margin,
               const unsigned int version) {
  ar& margin.InstrumentID;
  ar& margin.InvestorRange;
  ar& margin.BrokerID;
  ar& margin.InvestorID;
  ar& margin.HedgeFlag;
  ar& margin.LongMarginRatioByMoney;
  ar& margin.LongMarginRatioByVolume;
  ar& margin.ShortMarginRatioByMoney;
  ar& margin.ShortMarginRatioByVolume;
  ar& margin.IsRelative;
}

}  // namespace serialization
}  // namespace boost

int caf_main(caf::actor_system& system, const caf::actor_system_config& cfg) {
  auto actor = system.spawn<CtpAPIDelegate>();
  if (!Logon(actor)) {
    return 1;
  }

  auto instruments = CallActorFunction<std::vector<CThostFtdcInstrumentField>>(
      actor, QryInstrumentListAtom::value);

  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  std::ofstream file("instrument_margin_list.bin", std::ios_base::binary);
  boost::archive::binary_oarchive oa(file);

  oa << instruments.size();

  for (auto instrument : instruments) {
    std::cout << instrument.InstrumentID << std::endl;
    if (auto instrument_margins = CallActorFunctionWithParam<
            boost::optional<CThostFtdcInstrumentMarginRateField>>(
            actor, InstrumentMarginRateAtom::value, instrument.InstrumentID)) {
      oa << *instrument_margins;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  }

  /*
  std::ifstream file("instrument_margin_list.bin", std::ios_base::binary);
  boost::archive::binary_iarchive ia(file);

  size_t nums = 0;
  ia >> nums;
  std::vector<CThostFtdcInstrumentMarginRateField> instruments;
  for (int i = 0; i < nums; ++i) {
    CThostFtdcInstrumentMarginRateField instrument;
    ia >> instrument;
    instruments.push_back(std::move(instrument));
  }

  */

  return 0;
}

CAF_MAIN()
