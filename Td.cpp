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


extern CThostFtdcTraderApi* TdApi;            // ����ָ��
extern char MdFrontAddr[];                      // ģ������ǰ�õ�ַ
extern TThostFtdcBrokerIDType BrokerID;         // ģ�⾭���̴���
extern TThostFtdcInvestorIDType UserID;     // Ͷ�����˻���
extern TThostFtdcPasswordType Password; // Ͷ��������
extern TThostFtdcProductInfoType UserProductInfo;
extern TThostFtdcAuthCodeType AuthCode;
extern TThostFtdcAppIDType AppID;

extern TThostFtdcInstrumentIDType TradeInstrumentID;       // �����׵ĺ�Լ����
extern TThostFtdcDirectionType TradeDirection;               // ��������
extern TThostFtdcPriceType LimitPrice;                       // ���׼۸�

TThostFtdcFrontIDType trade_front_id;	//ǰ�ñ��
TThostFtdcSessionIDType session_id;	//�Ự���
TThostFtdcOrderRefType order_ref;	//��������


void TdSpi::OnFrontConnected(){
    std::cout << "=====�����������ӳɹ�=====" << std::endl;
    // authen
    CThostFtdcReqAuthenticateField authenReq;
    memset(&authenReq, 0, sizeof(authenReq));
    strcpy(authenReq.BrokerID, BrokerID);
    strcpy(authenReq.UserID, UserID);
    strcpy(authenReq.UserProductInfo, UserProductInfo);
    strcpy(authenReq.AuthCode, AuthCode);
    strcpy(authenReq.AppID, AppID);
    static int requestID = 0; // ������
    int rt = TdApi->ReqAuthenticate(&authenReq, requestID);
    if (!rt)
        std::cout << ">>>>>>������֤����ɹ�" << std::endl;
    else
        std::cerr << "--->>>������֤����ʧ��" << std::endl;
}

void TdSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
	if (!isErrorRspInfo(pRspInfo)) {
		// ��ʼ��¼
		CThostFtdcReqUserLoginField loginReq;
		memset(&loginReq, 0, sizeof(loginReq));
		strcpy(loginReq.BrokerID, BrokerID);
		strcpy(loginReq.UserID, UserID);
		strcpy(loginReq.Password, Password);
		static int requestID = 0; // ������
		int rt = TdApi->ReqUserLogin(&loginReq, requestID);
		if (!rt)
			std::cout << ">>>>>>���͵�¼����ɹ�" << std::endl;
		else
			std::cerr << "--->>>���͵�¼����ʧ��" << std::endl;
	}
}

void TdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
	if (!isErrorRspInfo(pRspInfo)) {
		std::cout << "=====�˻���¼�ɹ�=====" << std::endl;
		loginFlag = true;
		std::cout << "�����գ� " << pRspUserLogin->TradingDay << std::endl;
		std::cout << "��¼ʱ�䣺 " << pRspUserLogin->LoginTime << std::endl;
		std::cout << "�����̣� " << pRspUserLogin->BrokerID << std::endl;
		std::cout << "�ʻ����� " << pRspUserLogin->UserID << std::endl;
		// ����Ự����
		trade_front_id = pRspUserLogin->FrontID;
		session_id = pRspUserLogin->SessionID;
		strcpy(order_ref, pRspUserLogin->MaxOrderRef);
		// Ͷ���߽�����ȷ��
		reqSettlementInfoConfirm();
		
		// ��ѯ��Լ
		CThostFtdcQryInstrumentField pQryInstrument;
		static int requestID = 0; // ������
		int rt = TdApi->ReqQryInstrument(&pQryInstrument, requestID);
		if (!rt)
			std::cout << ">>>>>>���ͺ�Լ��ѯ����ɹ�" << std::endl;
		else
			std::cerr << "--->>>���ͺ�Լ��ѯ����ʧ��" << std::endl;
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
	std::cerr << "=====�������ӶϿ�=====" << std::endl;
	std::cerr << "�����룺 " << nReason << std::endl;
}

void TdSpi::OnHeartBeatWarning(int nTimeLapse)
{
	std::cerr << "=====����������ʱ=====" << std::endl;
	std::cerr << "���ϴ�����ʱ�䣺 " << nTimeLapse << std::endl;
}

void TdSpi::OnRspUserLogout(
	CThostFtdcUserLogoutField* pUserLogout,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		loginFlag = false; // �ǳ��Ͳ����ٽ����� 
		std::cout << "=====�˻��ǳ��ɹ�=====" << std::endl;
		std::cout << "�����̣� " << pUserLogout->BrokerID << std::endl;
		std::cout << "�ʻ����� " << pUserLogout->UserID << std::endl;
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
		std::cout << "=====Ͷ���߽�����ȷ�ϳɹ�=====" << std::endl;
		std::cout << "ȷ�����ڣ� " << pSettlementInfoConfirm->ConfirmDate << std::endl;
		std::cout << "ȷ��ʱ�䣺 " << pSettlementInfoConfirm->ConfirmTime << std::endl;
		// �����ѯ��Լ
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
		std::cout << "=====��ѯͶ�����ʽ��˻��ɹ�=====" << std::endl;
		std::cout << "Ͷ�����˺ţ� " << pTradingAccount->AccountID << std::endl;
		std::cout << "�����ʽ� " << pTradingAccount->Available << std::endl;
		std::cout << "��ȡ�ʽ� " << pTradingAccount->WithdrawQuota << std::endl;
		std::cout << "��ǰ��֤��: " << pTradingAccount->CurrMargin << std::endl;
		std::cout << "ƽ��ӯ���� " << pTradingAccount->CloseProfit << std::endl;
		// �����ѯͶ���ֲ߳�
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
		std::cout << "=====��ѯͶ���ֲֳ߳ɹ�=====" << std::endl;
		if (pInvestorPosition)
		{
			std::cout << "��Լ���룺 " << pInvestorPosition->InstrumentID << std::endl;
			std::cout << "���ּ۸� " << pInvestorPosition->OpenAmount << std::endl;
			std::cout << "�������� " << pInvestorPosition->OpenVolume << std::endl;
			std::cout << "���ַ��� " << pInvestorPosition->PosiDirection << std::endl;
			std::cout << "ռ�ñ�֤��" << pInvestorPosition->UseMargin << std::endl;
		}
		else
			std::cout << "----->�ú�Լδ�ֲ�" << std::endl;
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
		std::cout << "=====����¼��ɹ�=====" << std::endl;
		std::cout << "��Լ���룺 " << pInputOrder->InstrumentID << std::endl;
		std::cout << "�۸� " << pInputOrder->LimitPrice << std::endl;
		std::cout << "������ " << pInputOrder->VolumeTotalOriginal << std::endl;
		std::cout << "���ַ��� " << pInputOrder->Direction << std::endl;
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
		std::cout << "=====���������ɹ�=====" << std::endl;
		std::cout << "��Լ���룺 " << pInputOrderAction->InstrumentID << std::endl;
		std::cout << "������־�� " << pInputOrderAction->ActionFlag;
	}
}

void TdSpi::OnRtnOrder(CThostFtdcOrderField* pOrder)
{
	char str[10];
	sprintf(str, "%d", pOrder->OrderSubmitStatus);
	int orderState = atoi(str) - 48;	//����״̬0=�Ѿ��ύ��3=�Ѿ�����

	std::cout << "=====�յ�����Ӧ��=====" << std::endl;

	if (isMyOrder(pOrder))
	{
		if (isTradingOrder(pOrder))
		{
			std::cout << "--->>> �ȴ��ɽ��У�" << std::endl;
			//reqOrderAction(pOrder); // ������Գ���
			//reqUserLogout(); // �ǳ�����
		}
		else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
			std::cout << "--->>> �����ɹ���" << std::endl;
	}
}

void TdSpi::OnRtnTrade(CThostFtdcTradeField* pTrade)
{
	std::cout << "=====�����ɹ��ɽ�=====" << std::endl;
	std::cout << "�ɽ�ʱ�䣺 " << pTrade->TradeTime << std::endl;
	std::cout << "��Լ���룺 " << pTrade->InstrumentID << std::endl;
	std::cout << "�ɽ��۸� " << pTrade->Price << std::endl;
	std::cout << "�ɽ����� " << pTrade->Volume << std::endl;
	std::cout << "��ƽ�ַ��� " << pTrade->Direction << std::endl;
}

bool TdSpi::isErrorRspInfo(CThostFtdcRspInfoField* pRspInfo)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (bResult)
		std::cerr << "���ش���--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
	return bResult;
}

void TdSpi::reqUserLogin()
{
	CThostFtdcReqUserLoginField loginReq;
	memset(&loginReq, 0, sizeof(loginReq));
	strcpy(loginReq.BrokerID, BrokerID);
	strcpy(loginReq.UserID, UserID);
	strcpy(loginReq.Password, Password);
	static int requestID = 0; // ������
	int rt = TdApi->ReqUserLogin(&loginReq, requestID);
	if (!rt)
		std::cout << ">>>>>>���͵�¼����ɹ�" << std::endl;
	else
		std::cerr << "--->>>���͵�¼����ʧ��" << std::endl;
}

void TdSpi::reqUserLogout()
{
	CThostFtdcUserLogoutField logoutReq;
	memset(&logoutReq, 0, sizeof(logoutReq));
	strcpy(logoutReq.BrokerID, BrokerID);
	strcpy(logoutReq.UserID, UserID);
	static int requestID = 0; // ������
	int rt = TdApi->ReqUserLogout(&logoutReq, requestID);
	if (!rt)
		std::cout << ">>>>>>���͵ǳ�����ɹ�" << std::endl;
	else
		std::cerr << "--->>>���͵ǳ�����ʧ��" << std::endl;
}


void TdSpi::reqSettlementInfoConfirm()
{
	CThostFtdcSettlementInfoConfirmField settlementConfirmReq;
	memset(&settlementConfirmReq, 0, sizeof(settlementConfirmReq));
	strcpy(settlementConfirmReq.BrokerID, BrokerID);
	strcpy(settlementConfirmReq.InvestorID, UserID);
	static int requestID = 0; // ������
	int rt = TdApi->ReqSettlementInfoConfirm(&settlementConfirmReq, requestID);
	if (!rt)
		std::cout << ">>>>>>����Ͷ���߽�����ȷ������ɹ�" << std::endl;
	else
		std::cerr << "--->>>����Ͷ���߽�����ȷ������ʧ��" << std::endl;
}

void TdSpi::reqQueryInstrument()
{
	CThostFtdcQryInstrumentField instrumentReq;
	memset(&instrumentReq, 0, sizeof(instrumentReq));
	strcpy(instrumentReq.InstrumentID, TradeInstrumentID);
	static int requestID = 0; // ������
	int rt = TdApi->ReqQryInstrument(&instrumentReq, requestID);
	if (!rt)
		std::cout << ">>>>>>���ͺ�Լ��ѯ����ɹ�" << std::endl;
	else
		std::cerr << "--->>>���ͺ�Լ��ѯ����ʧ��" << std::endl;
}

void TdSpi::reqQueryTradingAccount()
{
	CThostFtdcQryTradingAccountField tradingAccountReq;
	memset(&tradingAccountReq, 0, sizeof(tradingAccountReq));
	strcpy(tradingAccountReq.BrokerID, BrokerID);
	strcpy(tradingAccountReq.InvestorID, UserID);
	static int requestID = 0; // ������
	std::this_thread::sleep_for(std::chrono::milliseconds(700)); // ��ʱ����Ҫͣ��һ����ܲ�ѯ�ɹ�
	int rt = TdApi->ReqQryTradingAccount(&tradingAccountReq, requestID);
	if (!rt)
		std::cout << ">>>>>>����Ͷ�����ʽ��˻���ѯ����ɹ�" << std::endl;
	else
		std::cerr << "--->>>����Ͷ�����ʽ��˻���ѯ����ʧ��" << std::endl;
}

void TdSpi::reqQueryInvestorPosition()
{
	CThostFtdcQryInvestorPositionField postionReq;
	memset(&postionReq, 0, sizeof(postionReq));
	strcpy(postionReq.BrokerID, BrokerID);
	strcpy(postionReq.InvestorID, UserID);
	strcpy(postionReq.InstrumentID, TradeInstrumentID);
	static int requestID = 0; // ������
	std::this_thread::sleep_for(std::chrono::milliseconds(700)); // ��ʱ����Ҫͣ��һ����ܲ�ѯ�ɹ�
	int rt = TdApi->ReqQryInvestorPosition(&postionReq, requestID);
	if (!rt)
		std::cout << ">>>>>>����Ͷ���ֲֲ߳�ѯ����ɹ�" << std::endl;
	else
		std::cerr << "--->>>����Ͷ���ֲֲ߳�ѯ����ʧ��" << std::endl;
}

void TdSpi::reqOrderInsert()
{
	CThostFtdcInputOrderField orderInsertReq;
	memset(&orderInsertReq, 0, sizeof(orderInsertReq));
	///���͹�˾����
	strcpy(orderInsertReq.BrokerID, BrokerID);
	///Ͷ���ߴ���
	strcpy(orderInsertReq.InvestorID, UserID);
	///��Լ����
	strcpy(orderInsertReq.InstrumentID, TradeInstrumentID);
	///��������
	strcpy(orderInsertReq.OrderRef, order_ref);
	///�����۸�����: �޼�
	orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	///��������: 
	orderInsertReq.Direction = TradeDirection;
	///��Ͽ�ƽ��־: ����
	orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	///���Ͷ���ױ���־
	orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///�۸�
	orderInsertReq.LimitPrice = LimitPrice;
	///������1
	orderInsertReq.VolumeTotalOriginal = 1;
	///��Ч������: ������Ч
	orderInsertReq.TimeCondition = THOST_FTDC_TC_GFD;
	///�ɽ�������: �κ�����
	orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
	///��С�ɽ���: 1
	orderInsertReq.MinVolume = 1;
	///��������: ����
	orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
	///ǿƽԭ��: ��ǿƽ
	orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///�Զ������־: ��
	orderInsertReq.IsAutoSuspend = 0;
	///�û�ǿ����־: ��
	orderInsertReq.UserForceClose = 0;

	static int requestID = 0; // ������
	int rt = TdApi->ReqOrderInsert(&orderInsertReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>���ͱ���¼������ɹ�" << std::endl;
	else
		std::cerr << "--->>>���ͱ���¼������ʧ��" << std::endl;
}

void TdSpi::reqOrderInsert(
	TThostFtdcInstrumentIDType instrumentID,
	TThostFtdcPriceType price,
	TThostFtdcVolumeType volume,
	TThostFtdcDirectionType direction)
{
	CThostFtdcInputOrderField orderInsertReq;
	memset(&orderInsertReq, 0, sizeof(orderInsertReq));
	///���͹�˾����
	strcpy(orderInsertReq.BrokerID, BrokerID);
	///Ͷ���ߴ���
	strcpy(orderInsertReq.InvestorID, UserID);
	///��Լ����
	strcpy(orderInsertReq.InstrumentID, instrumentID);
	///��������
	strcpy(orderInsertReq.OrderRef, order_ref);
	///�����۸�����: �޼�
	orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	///��������: 
	orderInsertReq.Direction = direction;
	///��Ͽ�ƽ��־: ����
	orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	///���Ͷ���ױ���־
	orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///�۸�
	orderInsertReq.LimitPrice = price;
	///������1
	orderInsertReq.VolumeTotalOriginal = volume;
	///��Ч������: ������Ч
	orderInsertReq.TimeCondition = THOST_FTDC_TC_GFD;
	///�ɽ�������: �κ�����
	orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
	///��С�ɽ���: 1
	orderInsertReq.MinVolume = 1;
	///��������: ����
	orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
	///ǿƽԭ��: ��ǿƽ
	orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///�Զ������־: ��
	orderInsertReq.IsAutoSuspend = 0;
	///�û�ǿ����־: ��
	orderInsertReq.UserForceClose = 0;

	static int requestID = 0; // ������
	int rt = TdApi->ReqOrderInsert(&orderInsertReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>���ͱ���¼������ɹ�" << std::endl;
	else
		std::cerr << "--->>>���ͱ���¼������ʧ��" << std::endl;
}

void TdSpi::reqOrderAction(CThostFtdcOrderField* pOrder)
{
	static bool orderActionSentFlag = false; // �Ƿ����˱���
	if (orderActionSentFlag)
		return;

	CThostFtdcInputOrderActionField orderActionReq;
	memset(&orderActionReq, 0, sizeof(orderActionReq));
	///���͹�˾����
	strcpy(orderActionReq.BrokerID, pOrder->BrokerID);
	///Ͷ���ߴ���
	strcpy(orderActionReq.InvestorID, pOrder->InvestorID);
	///������������
	//	TThostFtdcOrderActionRefType	OrderActionRef;
	///��������
	strcpy(orderActionReq.OrderRef, pOrder->OrderRef);
	///������
	//	TThostFtdcRequestIDType	RequestID;
	///ǰ�ñ��
	orderActionReq.FrontID = trade_front_id;
	///�Ự���
	orderActionReq.SessionID = session_id;
	///����������
	//	TThostFtdcExchangeIDType	ExchangeID;
	///�������
	//	TThostFtdcOrderSysIDType	OrderSysID;
	///������־
	orderActionReq.ActionFlag = THOST_FTDC_AF_Delete;
	///�۸�
	//	TThostFtdcPriceType	LimitPrice;
	///�����仯
	//	TThostFtdcVolumeType	VolumeChange;
	///�û�����
	//	TThostFtdcUserIDType	UserID;
	///��Լ����
	strcpy(orderActionReq.InstrumentID, pOrder->InstrumentID);
	static int requestID = 0; // ������
	int rt = TdApi->ReqOrderAction(&orderActionReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>���ͱ�����������ɹ�" << std::endl;
	else
		std::cerr << "--->>>���ͱ�����������ʧ��" << std::endl;
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