#ifndef _OPT_DATA
#define _OPT_DATA


#include "OptionInfo.h"
#include "StockType.h"
#include "Maturity.h"
#include "OptionType.h"
#include <map>
#include <vector>
#include <string>


using namespace std;

class OptData
{
public:
    StockType sty;
    map<Maturity, double> T;
    map<Maturity, double> initT;
    map<Maturity, double> S;
    map<Maturity, double> k0;
    map<Maturity, int> posi;
    map<Maturity, vector<pair<OptionInfo, OptionInfo>>> OptionList;
    double cm = (sty == StockType::gz300) ? 100 : 10000;
    double tradefee = (sty == StockType::gz300) ? 15 : 1.6;
    double exercisefee = (sty == StockType::gz300) ? 2 : 0.8;

    OptData(StockType sty) : sty(sty) {};
    ~OptData() {};

    void S_k0_posi(Maturity mat);
    double vix(Maturity mat);
    double skew_same_T(Maturity mat);
    double skew(Maturity mat1, Maturity mat2);
    void subscribe_init(Maturity mat);
};


#endif