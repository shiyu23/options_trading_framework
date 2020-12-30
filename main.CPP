#include <windows.h>
#include "StockType.h"
#include "Maturity.h"
#include "OptionType.h"
#include "OptData.h"
#include "Md.h"
#include "Td.h"
#include "ctp/ThostFtdcMdApi.h"
#include "ctp/ThostFtdcTraderApi.h"


using namespace std;

double interval = 0.5;
map<StockType, vector<string>> TradingID = { {StockType::etf50, {}}, {StockType::h300, {}}, {StockType::gz300, {}}, {StockType::s300, {}} };
vector<StockType> stylist = { StockType::etf50, StockType::h300, StockType::gz300, StockType::s300 };
map<StockType, vector<Maturity>> matlist = { {StockType::etf50, {Maturity::M1, Maturity::M2, Maturity::Q1, Maturity::Q2}}, {StockType::h300, {Maturity::M1, Maturity::M2, Maturity::Q1, Maturity::Q2}}, {StockType::s300, {Maturity::M1, Maturity::M2, Maturity::Q1, Maturity::Q2}}, {StockType::gz300, {Maturity::M1, Maturity::M2, Maturity::M3, Maturity::Q1, Maturity::Q2, Maturity::Q3}} };
map<StockType, OptData> opt_data = { {StockType::etf50, OptData(StockType::etf50)}, {StockType::h300, OptData(StockType::h300)}, {StockType::gz300, OptData(StockType::gz300)}, {StockType::s300, OptData(StockType::s300)} };
map<StockType, map<Maturity, int>> num_k = { {StockType::etf50, { {Maturity::M1, 0}, {Maturity::M2, 0}, {Maturity::Q1, 0}, {Maturity::Q2, 0} }}, {StockType::h300, { {Maturity::M1, 0}, {Maturity::M2, 0}, {Maturity::Q1, 0}, {Maturity::Q2, 0} }}, {StockType::s300, { {Maturity::M1, 0}, {Maturity::M2, 0}, {Maturity::Q1, 0}, {Maturity::Q2, 0} }}, {StockType::gz300, { {Maturity::M1, 0}, {Maturity::M2, 0}, {Maturity::M3, 0}, {Maturity::Q1, 0}, {Maturity::Q2, 0}, {Maturity::Q3, 0} }} };
map<StockType, map<Maturity, string>> mat_to_str = { {StockType::etf50, { {Maturity::M1, "0000"}, {Maturity::M2, "0000"}, {Maturity::Q1, "0000"}, {Maturity::Q2, "0000"} }}, {StockType::h300, { {Maturity::M1, "0000"}, {Maturity::M2, "0000"}, {Maturity::Q1, "0000"}, {Maturity::Q2, "0000"} }}, {StockType::s300, { {Maturity::M1, "0000"}, {Maturity::M2, "0000"}, {Maturity::Q1, "0000"}, {Maturity::Q2, "0000"} }}, {StockType::gz300, { {Maturity::M1, "0000"}, {Maturity::M2, "0000"}, {Maturity::M3, "0000"}, {Maturity::Q1, "0000"}, {Maturity::Q2, "0000"}, {Maturity::Q3, "0000"} }} };
map<StockType, map<string, Maturity>> str_to_mat;

CThostFtdcMdApi* MdApi;            // 行情指针
CThostFtdcTraderApi* TdApi;            // 交易指针
char MdFrontAddr[];            // 模拟行情前置地址
char TdFrontAddr[];            // 模拟交易前置地址
TThostFtdcBrokerIDType BrokerID;            // 模拟经纪商代码
TThostFtdcInvestorIDType UserID;            // 投资者账户名
TThostFtdcPasswordType Password;            // 投资者密码

TThostFtdcInstrumentIDType TradeInstrumentID;       // 所交易的合约代码
TThostFtdcDirectionType TradeDirection;               // 买卖方向 THOST_FTDC_D_Buy / THOST_FTDC_D_Sell
TThostFtdcPriceType LimitPrice;                       // 交易价格


void main()
{

	// 初始化交易线程
	TdApi = CThostFtdcTraderApi::CreateFtdcTraderApi();
	CThostFtdcTraderSpi* pTradeSpi = new TdSpi;
	TdApi->RegisterSpi(pTradeSpi);
	TdApi->SubscribePublicTopic(THOST_TERT_RESTART);
	TdApi->SubscribePrivateTopic(THOST_TERT_RESTART);
	TdApi->RegisterFront(TdFrontAddr);
	TdApi->Init();

	Sleep(10000);

	for (auto const& sty : stylist) {
		for (auto const& mat : matlist[sty]) {
			opt_data[sty].subscribe_init(mat);
			num_k[sty][mat] = opt_data[sty].OptionList[mat].size();
		}
	}

	// 初始化行情线程
	MdApi = CThostFtdcMdApi::CreateFtdcMdApi();
	CThostFtdcMdSpi* pMdUserSpi = new MdSpi;
	MdApi->RegisterSpi(pMdUserSpi);
	MdApi->RegisterFront(MdFrontAddr);
	MdApi->Init();

	// 等到线程退出
	MdApi->Join();
	delete pMdUserSpi;
	MdApi->Release();

	TdApi->Join();
	delete pTradeSpi;
	TdApi->Release();

    return;
} 