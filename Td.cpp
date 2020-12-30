#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include "StockType.h"
#include "Maturity.h"
#include "OptData.h"
#include "Td.h"


extern CThostFtdcTraderApi* TdApi;            // 行情指针
extern char MdFrontAddr[];                      // 模拟行情前置地址
extern TThostFtdcBrokerIDType BrokerID;         // 模拟经纪商代码
extern TThostFtdcInvestorIDType UserID;     // 投资者账户名
extern TThostFtdcPasswordType Password; // 投资者密码
extern TThostFtdcProductInfoType UserProductInfo;
extern TThostFtdcAuthCodeType AuthCode;
extern TThostFtdcAppIDType AppID;

extern TThostFtdcInstrumentIDType TradeInstrumentID;       // 所交易的合约代码
extern TThostFtdcDirectionType TradeDirection;               // 买卖方向
extern TThostFtdcPriceType LimitPrice;                       // 交易价格

TThostFtdcFrontIDType trade_front_id;	//前置编号
TThostFtdcSessionIDType session_id;	//会话编号
TThostFtdcOrderRefType order_ref;	//报单引用


void TdSpi::OnFrontConnected(){
    std::cout << "=====建立网络连接成功=====" << std::endl;
    // authen
    CThostFtdcReqAuthenticateField authenReq;
    memset(&authenReq, 0, sizeof(authenReq));
    strcpy(authenReq.BrokerID, BrokerID);
    strcpy(authenReq.UserID, UserID);
    strcpy(authenReq.UserProductInfo, UserProductInfo);
    strcpy(authenReq.AuthCode, AuthCode);
    strcpy(authenReq.AppID, AppID);
    static int requestID = 0; // 请求编号
    int rt = TdApi->ReqAuthenticate(&authenReq, requestID);
    if (!rt)
        std::cout << ">>>>>>发送认证请求成功" << std::endl;
    else
        std::cerr << "--->>>发送认证请求失败" << std::endl;
}

void TdSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
	if (!isErrorRspInfo(pRspInfo)) {
		// 开始登录
		CThostFtdcReqUserLoginField loginReq;
		memset(&loginReq, 0, sizeof(loginReq));
		strcpy(loginReq.BrokerID, BrokerID);
		strcpy(loginReq.UserID, UserID);
		strcpy(loginReq.Password, Password);
		static int requestID = 0; // 请求编号
		int rt = TdApi->ReqUserLogin(&loginReq, requestID);
		if (!rt)
			std::cout << ">>>>>>发送登录请求成功" << std::endl;
		else
			std::cerr << "--->>>发送登录请求失败" << std::endl;
	}
}

void TdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
	if (!isErrorRspInfo(pRspInfo)) {
		std::cout << "=====账户登录成功=====" << std::endl;
		loginFlag = true;
		std::cout << "交易日： " << pRspUserLogin->TradingDay << std::endl;
		std::cout << "登录时间： " << pRspUserLogin->LoginTime << std::endl;
		std::cout << "经纪商： " << pRspUserLogin->BrokerID << std::endl;
		std::cout << "帐户名： " << pRspUserLogin->UserID << std::endl;
		// 保存会话参数
		trade_front_id = pRspUserLogin->FrontID;
		session_id = pRspUserLogin->SessionID;
		strcpy(order_ref, pRspUserLogin->MaxOrderRef);
		// 投资者结算结果确认
		reqSettlementInfoConfirm();
		
		// 查询合约
		CThostFtdcQryInstrumentField pQryInstrument;
		static int requestID = 0; // 请求编号
		int rt = TdApi->ReqQryInstrument(&pQryInstrument, requestID);
		if (!rt)
			std::cout << ">>>>>>发送合约查询请求成功" << std::endl;
		else
			std::cerr << "--->>>发送合约查询请求失败" << std::endl;
	}
}

extern std::map<StockType, std::vector<std::string>> TradingID;
extern std::vector<StockType> stylist;
extern std::map<StockType, std::vector<Maturity>> matlist;
extern std::map<StockType, std::map<Maturity, std::string>> mat_to_str;
extern std::map<StockType, std::map<std::string, Maturity>> str_to_mat;

void TdSpi::OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
	if (!isErrorRspInfo(pRspInfo)) {
		std::string id = pInstrument->InstrumentID;
		std::string ed = pInstrument->ExpireDate;

		int if_in_etf50 = std::count(TradingID[StockType::etf50].begin(), TradingID[StockType::etf50].end(), id);
		int if_in_h300 = std::count(TradingID[StockType::h300].begin(), TradingID[StockType::h300].end(), id);
		int if_in_gz300 = std::count(TradingID[StockType::gz300].begin(), TradingID[StockType::gz300].end(), id);
		int if_in_s300 = std::count(TradingID[StockType::s300].begin(), TradingID[StockType::s300].end(), id);

		std::map<StockType, std::vector<std::string>> matstr;
		if (if_in_etf50 == 0 && if_in_h300 == 0 && if_in_gz300 == 0 && if_in_s300 == 0) {
			if (id.size() == strlen("IO2006-C-3500") && id.substr(0, 3) == "IO2" && (id.substr(6, 3) == "-C-" || id.substr(6, 3) == "-P-")) {
				TradingID[StockType::gz300].push_back(id);
				matstr[StockType::gz300].push_back(ed.substr(2, 4));
			}
			else if (id.size() == strlen("510050C2006M03000") && id.substr(0, 6) == "510050" && (id.substr(6, 2) == "C2" || id.substr(6, 2) == "P2") && id[11] == 'M') {
				TradingID[StockType::etf50].push_back(id);
				matstr[StockType::etf50].push_back(ed.substr(2, 4));
			}
			else if (id.size() == strlen("510300C2006M04000") && id.substr(0, 6) == "510300" && (id.substr(6, 2) == "C2" || id.substr(6, 2) == "P2") && id[11] == 'M') {
				TradingID[StockType::h300].push_back(id);
				matstr[StockType::h300].push_back(ed.substr(2, 4));
			}
			else if (id.size() == strlen("159919C2006M004000") && id.substr(0, 6) == "159919" && (id.substr(6, 2) == "C2" || id.substr(6, 2) == "P2") && id[11] == 'M') {
				TradingID[StockType::s300].push_back(id);
				matstr[StockType::s300].push_back(ed.substr(2, 4));
			}
		}

		std::map<StockType, std::vector<std::string>> matstr_dist;
		std::map<StockType, std::vector<std::string>> matstr_sort;

		for (auto const& sty : stylist) {
			for (int i = 0; i < TradingID[sty].size(); i++) {
				int if_in = std::count(TradingID[sty].begin(), TradingID[sty].end(), TradingID[sty][i]);
				if (if_in == 1) {
					matstr_dist[sty].push_back(matstr[sty][i]);
				}
			}
			std::vector<int> mat_int;
			for (auto const& mat : matstr_dist[sty]) {
				mat_int.push_back(atoi(mat.c_str()));
			}
			for (std::vector<int>::iterator it = mat_int.begin(); it != mat_int.end(); it++) {
				char* s = (char*)"0000";
				itoa(*it, s, 10);
				matstr_sort[sty].push_back(s);
			}
		}

		for (auto const& sty : stylist) {
			for (int i = 0; i < matstr_sort[sty].size(); i++) {
				mat_to_str[sty][matlist[sty][i]] = matstr_sort[sty][i];
				str_to_mat[sty][matstr_sort[sty][i]] = matlist[sty][i];
			}
		}
	}
}

void TdSpi::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	isErrorRspInfo(pRspInfo);
}

void TdSpi::OnFrontDisconnected(int nReason)
{
	std::cerr << "=====网络连接断开=====" << std::endl;
	std::cerr << "错误码： " << nReason << std::endl;
}

void TdSpi::OnHeartBeatWarning(int nTimeLapse)
{
	std::cerr << "=====网络心跳超时=====" << std::endl;
	std::cerr << "距上次连接时间： " << nTimeLapse << std::endl;
}

void TdSpi::OnRspUserLogout(
	CThostFtdcUserLogoutField* pUserLogout,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		loginFlag = false; // 登出就不能再交易了 
		std::cout << "=====账户登出成功=====" << std::endl;
		std::cout << "经纪商： " << pUserLogout->BrokerID << std::endl;
		std::cout << "帐户名： " << pUserLogout->UserID << std::endl;
	}
}

void TdSpi::OnRspSettlementInfoConfirm(
	CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====投资者结算结果确认成功=====" << std::endl;
		std::cout << "确认日期： " << pSettlementInfoConfirm->ConfirmDate << std::endl;
		std::cout << "确认时间： " << pSettlementInfoConfirm->ConfirmTime << std::endl;
		// 请求查询合约
		reqQueryInstrument();
	}
}

void TdSpi::OnRspQryTradingAccount(
	CThostFtdcTradingAccountField* pTradingAccount,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====查询投资者资金账户成功=====" << std::endl;
		std::cout << "投资者账号： " << pTradingAccount->AccountID << std::endl;
		std::cout << "可用资金： " << pTradingAccount->Available << std::endl;
		std::cout << "可取资金： " << pTradingAccount->WithdrawQuota << std::endl;
		std::cout << "当前保证金: " << pTradingAccount->CurrMargin << std::endl;
		std::cout << "平仓盈亏： " << pTradingAccount->CloseProfit << std::endl;
		// 请求查询投资者持仓
		reqQueryInvestorPosition();
	}
}

void TdSpi::OnRspQryInvestorPosition(
	CThostFtdcInvestorPositionField* pInvestorPosition,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====查询投资者持仓成功=====" << std::endl;
		if (pInvestorPosition)
		{
			std::cout << "合约代码： " << pInvestorPosition->InstrumentID << std::endl;
			std::cout << "开仓价格： " << pInvestorPosition->OpenAmount << std::endl;
			std::cout << "开仓量： " << pInvestorPosition->OpenVolume << std::endl;
			std::cout << "开仓方向： " << pInvestorPosition->PosiDirection << std::endl;
			std::cout << "占用保证金：" << pInvestorPosition->UseMargin << std::endl;
		}
		else
			std::cout << "----->该合约未持仓" << std::endl;
	}
}

void TdSpi::OnRspOrderInsert(
	CThostFtdcInputOrderField* pInputOrder,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====报单录入成功=====" << std::endl;
		std::cout << "合约代码： " << pInputOrder->InstrumentID << std::endl;
		std::cout << "价格： " << pInputOrder->LimitPrice << std::endl;
		std::cout << "数量： " << pInputOrder->VolumeTotalOriginal << std::endl;
		std::cout << "开仓方向： " << pInputOrder->Direction << std::endl;
	}
}

void TdSpi::OnRspOrderAction(
	CThostFtdcInputOrderActionField* pInputOrderAction,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====报单操作成功=====" << std::endl;
		std::cout << "合约代码： " << pInputOrderAction->InstrumentID << std::endl;
		std::cout << "操作标志： " << pInputOrderAction->ActionFlag;
	}
}

void TdSpi::OnRtnOrder(CThostFtdcOrderField* pOrder)
{
	char str[10];
	sprintf(str, "%d", pOrder->OrderSubmitStatus);
	int orderState = atoi(str) - 48;	//报单状态0=已经提交，3=已经接受

	std::cout << "=====收到报单应答=====" << std::endl;

	if (isMyOrder(pOrder))
	{
		if (isTradingOrder(pOrder))
		{
			std::cout << "--->>> 等待成交中！" << std::endl;
			//reqOrderAction(pOrder); // 这里可以撤单
			//reqUserLogout(); // 登出测试
		}
		else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
			std::cout << "--->>> 撤单成功！" << std::endl;
	}
}

void TdSpi::OnRtnTrade(CThostFtdcTradeField* pTrade)
{
	std::cout << "=====报单成功成交=====" << std::endl;
	std::cout << "成交时间： " << pTrade->TradeTime << std::endl;
	std::cout << "合约代码： " << pTrade->InstrumentID << std::endl;
	std::cout << "成交价格： " << pTrade->Price << std::endl;
	std::cout << "成交量： " << pTrade->Volume << std::endl;
	std::cout << "开平仓方向： " << pTrade->Direction << std::endl;
}

bool TdSpi::isErrorRspInfo(CThostFtdcRspInfoField* pRspInfo)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (bResult)
		std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
	return bResult;
}

void TdSpi::reqUserLogin()
{
	CThostFtdcReqUserLoginField loginReq;
	memset(&loginReq, 0, sizeof(loginReq));
	strcpy(loginReq.BrokerID, BrokerID);
	strcpy(loginReq.UserID, UserID);
	strcpy(loginReq.Password, Password);
	static int requestID = 0; // 请求编号
	int rt = TdApi->ReqUserLogin(&loginReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送登录请求成功" << std::endl;
	else
		std::cerr << "--->>>发送登录请求失败" << std::endl;
}

void TdSpi::reqUserLogout()
{
	CThostFtdcUserLogoutField logoutReq;
	memset(&logoutReq, 0, sizeof(logoutReq));
	strcpy(logoutReq.BrokerID, BrokerID);
	strcpy(logoutReq.UserID, UserID);
	static int requestID = 0; // 请求编号
	int rt = TdApi->ReqUserLogout(&logoutReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送登出请求成功" << std::endl;
	else
		std::cerr << "--->>>发送登出请求失败" << std::endl;
}


void TdSpi::reqSettlementInfoConfirm()
{
	CThostFtdcSettlementInfoConfirmField settlementConfirmReq;
	memset(&settlementConfirmReq, 0, sizeof(settlementConfirmReq));
	strcpy(settlementConfirmReq.BrokerID, BrokerID);
	strcpy(settlementConfirmReq.InvestorID, UserID);
	static int requestID = 0; // 请求编号
	int rt = TdApi->ReqSettlementInfoConfirm(&settlementConfirmReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送投资者结算结果确认请求成功" << std::endl;
	else
		std::cerr << "--->>>发送投资者结算结果确认请求失败" << std::endl;
}

void TdSpi::reqQueryInstrument()
{
	CThostFtdcQryInstrumentField instrumentReq;
	memset(&instrumentReq, 0, sizeof(instrumentReq));
	strcpy(instrumentReq.InstrumentID, TradeInstrumentID);
	static int requestID = 0; // 请求编号
	int rt = TdApi->ReqQryInstrument(&instrumentReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送合约查询请求成功" << std::endl;
	else
		std::cerr << "--->>>发送合约查询请求失败" << std::endl;
}

void TdSpi::reqQueryTradingAccount()
{
	CThostFtdcQryTradingAccountField tradingAccountReq;
	memset(&tradingAccountReq, 0, sizeof(tradingAccountReq));
	strcpy(tradingAccountReq.BrokerID, BrokerID);
	strcpy(tradingAccountReq.InvestorID, UserID);
	static int requestID = 0; // 请求编号
	std::this_thread::sleep_for(std::chrono::milliseconds(700)); // 有时候需要停顿一会才能查询成功
	int rt = TdApi->ReqQryTradingAccount(&tradingAccountReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送投资者资金账户查询请求成功" << std::endl;
	else
		std::cerr << "--->>>发送投资者资金账户查询请求失败" << std::endl;
}

void TdSpi::reqQueryInvestorPosition()
{
	CThostFtdcQryInvestorPositionField postionReq;
	memset(&postionReq, 0, sizeof(postionReq));
	strcpy(postionReq.BrokerID, BrokerID);
	strcpy(postionReq.InvestorID, UserID);
	strcpy(postionReq.InstrumentID, TradeInstrumentID);
	static int requestID = 0; // 请求编号
	std::this_thread::sleep_for(std::chrono::milliseconds(700)); // 有时候需要停顿一会才能查询成功
	int rt = TdApi->ReqQryInvestorPosition(&postionReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送投资者持仓查询请求成功" << std::endl;
	else
		std::cerr << "--->>>发送投资者持仓查询请求失败" << std::endl;
}

void TdSpi::reqOrderInsert()
{
	CThostFtdcInputOrderField orderInsertReq;
	memset(&orderInsertReq, 0, sizeof(orderInsertReq));
	///经纪公司代码
	strcpy(orderInsertReq.BrokerID, BrokerID);
	///投资者代码
	strcpy(orderInsertReq.InvestorID, UserID);
	///合约代码
	strcpy(orderInsertReq.InstrumentID, TradeInstrumentID);
	///报单引用
	strcpy(orderInsertReq.OrderRef, order_ref);
	///报单价格条件: 限价
	orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	///买卖方向: 
	orderInsertReq.Direction = TradeDirection;
	///组合开平标志: 开仓
	orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	///组合投机套保标志
	orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///价格
	orderInsertReq.LimitPrice = LimitPrice;
	///数量：1
	orderInsertReq.VolumeTotalOriginal = 1;
	///有效期类型: 当日有效
	orderInsertReq.TimeCondition = THOST_FTDC_TC_GFD;
	///成交量类型: 任何数量
	orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
	///最小成交量: 1
	orderInsertReq.MinVolume = 1;
	///触发条件: 立即
	orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
	///强平原因: 非强平
	orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///自动挂起标志: 否
	orderInsertReq.IsAutoSuspend = 0;
	///用户强评标志: 否
	orderInsertReq.UserForceClose = 0;

	static int requestID = 0; // 请求编号
	int rt = TdApi->ReqOrderInsert(&orderInsertReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>发送报单录入请求成功" << std::endl;
	else
		std::cerr << "--->>>发送报单录入请求失败" << std::endl;
}

void TdSpi::reqOrderInsert(
	TThostFtdcInstrumentIDType instrumentID,
	TThostFtdcPriceType price,
	TThostFtdcVolumeType volume,
	TThostFtdcDirectionType direction)
{
	CThostFtdcInputOrderField orderInsertReq;
	memset(&orderInsertReq, 0, sizeof(orderInsertReq));
	///经纪公司代码
	strcpy(orderInsertReq.BrokerID, BrokerID);
	///投资者代码
	strcpy(orderInsertReq.InvestorID, UserID);
	///合约代码
	strcpy(orderInsertReq.InstrumentID, instrumentID);
	///报单引用
	strcpy(orderInsertReq.OrderRef, order_ref);
	///报单价格条件: 限价
	orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	///买卖方向: 
	orderInsertReq.Direction = direction;
	///组合开平标志: 开仓
	orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	///组合投机套保标志
	orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///价格
	orderInsertReq.LimitPrice = price;
	///数量：1
	orderInsertReq.VolumeTotalOriginal = volume;
	///有效期类型: 当日有效
	orderInsertReq.TimeCondition = THOST_FTDC_TC_GFD;
	///成交量类型: 任何数量
	orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
	///最小成交量: 1
	orderInsertReq.MinVolume = 1;
	///触发条件: 立即
	orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
	///强平原因: 非强平
	orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///自动挂起标志: 否
	orderInsertReq.IsAutoSuspend = 0;
	///用户强评标志: 否
	orderInsertReq.UserForceClose = 0;

	static int requestID = 0; // 请求编号
	int rt = TdApi->ReqOrderInsert(&orderInsertReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>发送报单录入请求成功" << std::endl;
	else
		std::cerr << "--->>>发送报单录入请求失败" << std::endl;
}

void TdSpi::reqOrderAction(CThostFtdcOrderField* pOrder)
{
	static bool orderActionSentFlag = false; // 是否发送了报单
	if (orderActionSentFlag)
		return;

	CThostFtdcInputOrderActionField orderActionReq;
	memset(&orderActionReq, 0, sizeof(orderActionReq));
	///经纪公司代码
	strcpy(orderActionReq.BrokerID, pOrder->BrokerID);
	///投资者代码
	strcpy(orderActionReq.InvestorID, pOrder->InvestorID);
	///报单操作引用
	//	TThostFtdcOrderActionRefType	OrderActionRef;
	///报单引用
	strcpy(orderActionReq.OrderRef, pOrder->OrderRef);
	///请求编号
	//	TThostFtdcRequestIDType	RequestID;
	///前置编号
	orderActionReq.FrontID = trade_front_id;
	///会话编号
	orderActionReq.SessionID = session_id;
	///交易所代码
	//	TThostFtdcExchangeIDType	ExchangeID;
	///报单编号
	//	TThostFtdcOrderSysIDType	OrderSysID;
	///操作标志
	orderActionReq.ActionFlag = THOST_FTDC_AF_Delete;
	///价格
	//	TThostFtdcPriceType	LimitPrice;
	///数量变化
	//	TThostFtdcVolumeType	VolumeChange;
	///用户代码
	//	TThostFtdcUserIDType	UserID;
	///合约代码
	strcpy(orderActionReq.InstrumentID, pOrder->InstrumentID);
	static int requestID = 0; // 请求编号
	int rt = TdApi->ReqOrderAction(&orderActionReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>发送报单操作请求成功" << std::endl;
	else
		std::cerr << "--->>>发送报单操作请求失败" << std::endl;
	orderActionSentFlag = true;
}

bool TdSpi::isMyOrder(CThostFtdcOrderField* pOrder)
{
	return ((pOrder->FrontID == trade_front_id) &&
		(pOrder->SessionID == session_id) &&
		(strcmp(pOrder->OrderRef, order_ref) == 0));
}

bool TdSpi::isTradingOrder(CThostFtdcOrderField* pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}