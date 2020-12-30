#ifndef _OPTION_INFO
#define _OPTION_INFO


#include "StockType.h"
#include "Maturity.h"
#include "OptionType.h"
#include <time.h>
#include <map>
#include <string>
#include "Td.h"


class OptionInfo
{
public:
    TThostFtdcInstrumentIDType InstrumentID = "";
    StockType sty;
    Maturity mat;
    OptionType oty;
    double K;
    std::string K_str;
    double P;
    double ask;
    double bid;
    double T = 1;
    double S = 1;
    bool cb_if = false;
    double cb_start_time = time(NULL);
    double settlement_price = -1;
    bool written = false;

    OptionInfo(StockType sty, Maturity mat, OptionType oty, double K, std::string K_str, double P, double ask, double bid) : sty(sty), mat(mat), oty(oty), K(K), K_str(K_str), P(P), ask(ask), bid(bid) {};
    ~OptionInfo() {};

    double midbidaskspread();
    double iv();
    double iv_s(double s);
    double iv_p(double p);
    double delta();
    double vega();
    double gamma();
    double theta();
};


#endif