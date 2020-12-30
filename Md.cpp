#include <iostream>
#include <fstream>
#include <time.h>
#include "StockType.h"
#include "Maturity.h"
#include "OptionType.h"
#include "OptData.h"
#include "Md.h"


extern CThostFtdcMdApi* MdApi;            // ����ָ��
extern char MdFrontAddr[];                      // ģ������ǰ�õ�ַ
extern TThostFtdcBrokerIDType BrokerID;         // ģ�⾭���̴���
extern TThostFtdcInvestorIDType UserID;     // Ͷ�����˻���
extern TThostFtdcPasswordType Password; // Ͷ��������

extern map<StockType, vector<string>> TradingID;
extern vector<StockType> stylist;
extern map<StockType, OptData> opt_data;
extern map<StockType, map<Maturity, int>> num_k;
extern map<StockType, map<Maturity, string>> mat_to_str;
extern map<StockType, map<string, Maturity>> str_to_mat;


// ���ӳɹ�Ӧ��
void MdSpi::OnFrontConnected()
{
	std::cout << "=====�����������ӳɹ�=====" << std::endl;
	// ��ʼ��¼
	CThostFtdcReqUserLoginField loginReq;
	memset(&loginReq, 0, sizeof(loginReq));
	strcpy(loginReq.BrokerID, BrokerID);
	strcpy(loginReq.UserID, UserID);
	strcpy(loginReq.Password, Password);
	static int requestID = 0; // ������
	int rt = MdApi->ReqUserLogin(&loginReq, requestID);
	if (!rt)
		std::cout << ">>>>>>���͵�¼����ɹ�" << std::endl;
	else
		std::cerr << "--->>>���͵�¼����ʧ��" << std::endl;
}

// �Ͽ�����֪ͨ
void MdSpi::OnFrontDisconnected(int nReason)
{
	std::cerr << "=====�������ӶϿ�=====" << std::endl;
	std::cerr << "�����룺 " << nReason << std::endl;
}

// ������ʱ����
void MdSpi::OnHeartBeatWarning(int nTimeLapse)
{
	std::cerr << "=====����������ʱ=====" << std::endl;
	std::cerr << "���ϴ�����ʱ�䣺 " << nTimeLapse << std::endl;
}

// ��¼Ӧ��
void MdSpi::OnRspUserLogin(
	CThostFtdcRspUserLoginField* pRspUserLogin,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (!bResult)
	{
		std::cout << "=====�˻���¼�ɹ�=====" << std::endl;
		std::cout << "�����գ� " << pRspUserLogin->TradingDay << std::endl;
		std::cout << "��¼ʱ�䣺 " << pRspUserLogin->LoginTime << std::endl;
		std::cout << "�����̣� " << pRspUserLogin->BrokerID << std::endl;
		std::cout << "�ʻ����� " << pRspUserLogin->UserID << std::endl;
		// ��ʼ��������
		for (auto const& sty : stylist) {
			char* char_subID[1000];
			for (int i = 0; i < TradingID[sty].size(); i++) {
				strcpy(char_subID[i], TradingID[sty][i].c_str());
			}
			int rt = MdApi->SubscribeMarketData(char_subID, TradingID[sty].size());
			if (!rt)
				std::cout << ">>>>>>���Ͷ�����������ɹ�" << std::endl;
			else
				std::cerr << "--->>>���Ͷ�����������ʧ��" << std::endl;
		}
	}
	else
		std::cerr << "���ش���--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

// �ǳ�Ӧ��
void MdSpi::OnRspUserLogout(
	CThostFtdcUserLogoutField* pUserLogout,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (!bResult)
	{
		std::cout << "=====�˻��ǳ��ɹ�=====" << std::endl;
		std::cout << "�����̣� " << pUserLogout->BrokerID << std::endl;
		std::cout << "�ʻ����� " << pUserLogout->UserID << std::endl;
	}
	else
		std::cerr << "���ش���--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

// ����֪ͨ
void MdSpi::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (bResult)
		std::cerr << "���ش���--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
}

// ��������Ӧ��
void MdSpi::OnRspSubMarketData(
	CThostFtdcSpecificInstrumentField* pSpecificInstrument,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	return;
}

void func(StockType sty, Maturity mat, int position, int se, CThostFtdcDepthMarketDataField* pDepthMarketData) {
	// update OptionList
	OptionInfo *opt;
	if (se == 0) {
		opt = &opt_data[sty].OptionList[mat][position].first;
	}
	else {
		opt = &opt_data[sty].OptionList[mat][position].second;
	}
	opt->P = pDepthMarketData->LastPrice;
	opt->bid = pDepthMarketData->BidPrice1;
	opt->ask = pDepthMarketData->AskPrice1;
	opt->settlement_price = pDepthMarketData->PreSettlementPrice;
	// written
	if (opt->written == false) {
		opt->written = true;
	}
	// cb
	if (opt->cb_if == false && pDepthMarketData->AskPrice1 - pDepthMarketData->BidPrice1 == 0) {
		opt->cb_if = true;
		opt->cb_start_time = time(NULL);
	}
	if (opt->cb_if == true && pDepthMarketData->AskPrice1 - pDepthMarketData->BidPrice1 != 0) {
		opt->cb_if =false;
	}
}

// ��������֪ͨ
void MdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)
{
	string id = pDepthMarketData->InstrumentID;
	StockType sty;

	if (id.substr(0, 2) == "IO") {
		sty = StockType::gz300;
	}
	else if (id.substr(0, 6) == "510050") {
		sty = StockType::etf50;
	}
	else if (id.substr(0, 6) == "510300") {
		sty = StockType::h300;
	}
	else if (id.substr(0, 6) == "159919") {
		sty = StockType::s300;
	}

	int len = id.size();
	if (sty == StockType::gz300 and len == strlen("IO2006-C-3500")) {
		// mat
		Maturity mat = str_to_mat[sty][id.substr(2, 4)];
		// position
		int position = 0;
		int se = 0;
		for (int i = 0; i < num_k[sty][mat]; i++) {
			if (id.substr(9, 4)._Equal(opt_data[sty].OptionList[mat][i].first.K_str)) {
				position = i;
				break;
			}
		}
		if (id[7] == 'C') {
			se = 0;
		}
		else {
			se = 1;
		}
		func(sty, mat, position, se, pDepthMarketData);
	}

	if (sty == StockType::etf50 and len == strlen("510050C2006M03000")) {
		// mat
		Maturity mat = str_to_mat[sty][id.substr(7, 4)];
		// position
		int position = 0;
		int se = 0;
		for (int i = 0; i < num_k[sty][mat]; i++) {
			if (id.substr(13, 4)._Equal(opt_data[sty].OptionList[mat][i].first.K_str)) {
				position = i;
				break;
			}
		}
		if (id[6] == 'C') {
			se = 0;
		}
		else {
			se = 1;
		}
		func(sty, mat, position, se, pDepthMarketData);
	}

	if (sty == StockType::etf50 and len == strlen("510300C2006M04000")) {
		// mat
		Maturity mat = str_to_mat[sty][id.substr(7, 4)];
		// position
		int position = 0;
		int se = 0;
		for (int i = 0; i < num_k[sty][mat]; i++) {
			if (id.substr(13, 4)._Equal(opt_data[sty].OptionList[mat][i].first.K_str)) {
				int position = i;
				break;
			}
		}
		if (id[6] == 'C') {
			se = 0;
		}
		else {
			se = 1;
		}
		func(sty, mat, position, se, pDepthMarketData);
	}

	if (sty == StockType::etf50 and len == strlen("159919C2006M004000")) {
		// mat
		Maturity mat = str_to_mat[sty][id.substr(7, 4)];
		// position
		int position = 0; 
		int se = 0;
		for (int i = 0; i < num_k[sty][mat]; i++) {
			if (id.substr(14, 4)._Equal(opt_data[sty].OptionList[mat][i].first.K_str)) {
				int position = i;
				break;
			}
		}
		if (id[6] == 'C') {
			se = 0;
		}
		else {
			se = 1;
		}
		func(sty, mat, position, se, pDepthMarketData);
	}
}