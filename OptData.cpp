#include <vector>
#include <math.h>
#include <Windows.h>
#include <string>
#include <algorithm>
#include "OptData.h"
#include "vector_cal.h"
#include "Td.h"


void OptData::S_k0_posi(Maturity mat) {
    vector<pair<OptionInfo, OptionInfo>>& optlist = OptionList[mat];
    int n = optlist.size();
    vector<double> future;
    for (int i = 0; i < n; i++) {
        future.push_back(optlist[i].first.midbidaskspread() - optlist[i].second.midbidaskspread() + optlist[i].first.K);
    }
    double _avg = (vsum(future) - vmax(future) - vmin(future)) / (n - 2);
    S[mat] = _avg;
    vector<double> k_s;
    for (int i = 0; i < n; i++) {
        k_s.push_back(abs(optlist[i].first.K - _avg));
    }
    posi[mat] = vargmin(k_s);
    k0[mat] = optlist[posi[mat]].first.K;
}

double CubicSpline(const vector<pair<double, double>>& marks, double S) // 三次样条插值
{
    // y2
    int n = marks.size();
    // end y' are zero, flat extrapolation
    double yp1 = 0;
    double ypn = 0;
    vector<double> y2;
    y2.resize(n);
    vector<double> u(n - 1);

    y2[0] = -0.5;
    u[0] = (3.0 / (marks[1].first - marks[0].first)) *
        ((marks[1].second - marks[0].second) / (marks[1].first - marks[0].first) - yp1);

    for (int i = 1; i < n - 1; i++) {
        double sig = (marks[i].first - marks[i - 1].first) / (marks[i + 1].first - marks[i - 1].first);
        double p = sig * y2[i - 1] + 2.0;
        y2[i] = (sig - 1.0) / p;
        u[i] = (marks[i + 1].second - marks[i].second) / (marks[i + 1].first - marks[i].first)
            - (marks[i].second - marks[i - 1].second) / (marks[i].first - marks[i - 1].first);
        u[i] = (6.0 * u[i] / (marks[i + 1].first - marks[i - 1].first) - sig * u[i - 1]) / p;
    }

    double qn = 0.5;
    double un = (3.0 / (marks[n - 1].first - marks[n - 2].first)) *
        (ypn - (marks[n - 1].second - marks[n - 2].second) / (marks[n - 1].first - marks[n - 2].first));

    y2[n - 1] = (un - qn * u[n - 2]) / (qn * y2[n - 2] + 1.0);

    for (int i = n - 2; i >= 0; i--) {
        y2[i] = y2[i] * y2[i + 1] + u[i];
    }

    // S
    int i;
    // we use trivial search, but can consider binary search for better performance
    for (i = 0; i < marks.size(); i++)
        if (S < marks[i].first)
            break; // i stores the index of the right end of the bracket

        // extrapolation
    if (i == 0)
        return marks[i].second;
    if (i == marks.size())
        return marks[i - 1].second;

    // interpolate
    double h = marks[i].first - marks[i - 1].first;
    double a = (marks[i].first - S) / h;
    double b = 1 - a;
    double c = (a * a * a - a) * h * h / 6.0;
    double d = (b * b * b - b) * h * h / 6.0;
    return a * marks[i - 1].second + b * marks[i].second + c * y2[i - 1] + d * y2[i]; // cubic spline
}

double OptData::vix(Maturity mat) {
    int n = OptionList[mat].size();
    int cen = posi[mat];
    if (cen == 0 || cen == (n - 1)) {
        return (OptionList[mat][cen].first.iv() + OptionList[mat][cen].second.iv()) / 2;
    }
    else {
        OptionInfo& opt1c = OptionList[mat][cen - 1].first;
        OptionInfo& opt2c = OptionList[mat][cen].first;
        OptionInfo& opt3c = OptionList[mat][cen + 1].first;
        OptionInfo& opt1p = OptionList[mat][cen - 1].second;
        OptionInfo& opt2p = OptionList[mat][cen].second;
        OptionInfo& opt3p = OptionList[mat][cen + 1].second;
        vector<pair<double, double>> marks;
        marks.push_back(pair<double, double>(opt1c.K, (opt1c.iv() + opt1p.iv()) / 2));
        marks.push_back(pair<double, double>(opt2c.K, (opt2c.iv() + opt2p.iv()) / 2));
        marks.push_back(pair<double, double>(opt3c.K, (opt3c.iv() + opt3p.iv()) / 2));
        return CubicSpline(marks, S[mat]);
    }
}

double OptData::skew_same_T(Maturity mat) {
    vector<pair<OptionInfo, OptionInfo>>& optlist = OptionList[mat];
    int n = optlist.size();
    double _f0 = S[mat];
    double _k0 = k0[mat];
    double p1 = -(1 + log(_f0 / _k0) - _f0 / _k0);
    double p2 = 2 * log(_k0 / _f0) * (_f0 / _k0 - 1) + 1 / 2 * pow(log(_k0 / _f0), 2);
    double p3 = 3 * pow(log(_k0 / _f0), 2) * (1 / 3 * log(_k0 / _f0) - 1 + _f0 / _k0);
    for (int i = 0; i < n; i++) {
        if (optlist[i].first.K <= _f0) {
            if (i == 0) {
                p1 += -1 / pow((optlist[i].second.K), 2) * optlist[i].second.midbidaskspread() * (optlist[i + 1].second.K - optlist[i].second.K);
                p2 += 2 / pow((optlist[i].second.K), 2) * (1 - log(optlist[i].second.K / _f0)) * optlist[i].second.midbidaskspread() * (optlist[i + 1].second.K - optlist[i].second.K);
                p3 += 3 / pow((optlist[i].second.K), 2) * (2 * log(optlist[i].second.K / _f0) - pow(log(optlist[i].second.K / _f0), 2)) * optlist[i].second.midbidaskspread() * (optlist[i + 1].second.K - optlist[i].second.K);
            }
            else if (i == n - 1) {
                p1 += -1 / pow((optlist[i].second.K), 2) * optlist[i].second.midbidaskspread() * (optlist[i].second.K - optlist[i - 1].second.K);
                p2 += 2 / pow((optlist[i].second.K), 2) * (1 - log(optlist[i].second.K / _f0)) * optlist[i].second.midbidaskspread() * (optlist[i].second.K - optlist[i - 1].second.K);
                p3 += 3 / pow((optlist[i].second.K), 2) * (2 * log(optlist[i].second.K / _f0) - pow(log(optlist[i].second.K / _f0), 2)) * optlist[i].second.midbidaskspread() * (optlist[i].second.K - optlist[i - 1].second.K);
            }
            else {
                p1 += -1 / pow((optlist[i].second.K), 2) * optlist[i].second.midbidaskspread() * (optlist[i + 1].second.K - optlist[i - 1].second.K) / 2;
                p2 += 2 / pow((optlist[i].second.K), 2) * (1 - log(optlist[i].second.K / _f0)) * optlist[i].second.midbidaskspread() * (optlist[i + 1].second.K - optlist[i - 1].second.K) / 2;
                p3 += 3 / pow((optlist[i].second.K), 2) * (2 * log(optlist[i].second.K / _f0) - pow(log(optlist[i].second.K / _f0), 2)) * optlist[i].second.midbidaskspread() * (optlist[i + 1].second.K - optlist[i - 1].second.K) / 2;
            }

        }
        else if (optlist[i].first.K >= _f0) {
            if (i == (n - 1)) {
                p1 += -1 / pow((optlist[i].first.K), 2) * optlist[i].first.midbidaskspread() * (optlist[i].first.K - optlist[i - 1].first.K);
                p2 += 2 / pow((optlist[i].first.K), 2) * (1 - log(optlist[i].first.K / _f0)) * optlist[i].first.midbidaskspread() * (optlist[i].first.K - optlist[i - 1].first.K);
                p3 += 3 / pow((optlist[i].first.K), 2) * (2 * log(optlist[i].first.K / _f0) - pow(log(optlist[i].first.K / _f0), 2)) * optlist[i].first.midbidaskspread() * (optlist[i].first.K - optlist[i - 1].first.K);
            }
            else if (i == 0) {
                p1 += -1 / pow((optlist[i].first.K), 2) * optlist[i].first.midbidaskspread() * (optlist[i + 1].first.K - optlist[i].first.K);
                p2 += 2 / pow((optlist[i].first.K), 2) * (1 - log(optlist[i].first.K / _f0)) * optlist[i].first.midbidaskspread() * (optlist[i + 1].first.K - optlist[i].first.K);
                p3 += 3 / pow((optlist[i].first.K), 2) * (2 * log(optlist[i].first.K / _f0) - pow(log(optlist[i].first.K / _f0), 2)) * optlist[i].first.midbidaskspread() * (optlist[i + 1].first.K - optlist[i].first.K);
            }
            else {
                p1 += -1 / pow((optlist[i].first.K), 2) * optlist[i].first.midbidaskspread() * (optlist[i + 1].first.K - optlist[i - 1].first.K) / 2;
                p2 += 2 / pow((optlist[i].first.K), 2) * (1 - log(optlist[i].first.K / _f0)) * optlist[i].first.midbidaskspread() * (optlist[i + 1].first.K - optlist[i - 1].first.K);
                p3 += 3 / pow((optlist[i].first.K), 2) * (2 * log(optlist[i].first.K / _f0) - pow(log(optlist[i].first.K / _f0), 2)) * optlist[i].first.midbidaskspread() * (optlist[i + 1].first.K - optlist[i - 1].first.K);
            }
        }
    }
    if (pow((p2 - pow(p1, 2)), 3) > 0) {
        return (p3 - 3 * p1 * p2 + 2 * pow(p1, 3)) / sqrt(pow((p2 - pow(p1, 2)), 3));
    }
    else {
        return 0;
    }
}

double OptData::skew(Maturity mat1, Maturity mat2) {
    double w = (T[mat2] - 1 / 12) / (T[mat2] - T[mat1]);
    double s1 = skew_same_T(mat1);
    double s2 = skew_same_T(mat2);
    return 100 - 10 * (w * s1 + (1 - w) * s2);
}

extern map<StockType, vector<string>> TradingID;
extern map<StockType, map<Maturity, string>> mat_to_str;

void OptData::subscribe_init(Maturity mat) {
    // subIDand Optionlists // IO2006 - C - 3500, 510050C2006M03000, 510300C2006M03800, 159919C2006M004000
    vector<string> subID_addin;
    for (int i = 0; i < TradingID[sty].size(); i++) {
        if (sty == StockType::gz300) {
            if (TradingID[sty][i].substr(2, 4)._Equal(mat_to_str[sty][mat])) {
                subID_addin.push_back(TradingID[sty][i]);
            }
        }
        else if (sty == StockType::etf50 || sty == StockType::h300 || sty == StockType::s300) {
            if (TradingID[sty][i].substr(7,4)._Equal(mat_to_str[sty][mat])) {
                subID_addin.push_back(TradingID[sty][i]);
            }
        }
    }

    vector<int> subID_addin_C_K;
    for (int i = 0; i < subID_addin.size(); i++) {
        if ((sty == StockType::gz300 and subID_addin[i][7] == 'C') || ((sty == StockType::etf50 || sty == StockType::h300 || sty == StockType::s300) && subID_addin[i][6] == 'C'))
            subID_addin_C_K.push_back(stoi(subID_addin[i].substr(subID_addin[i].size() - 4, 4)));
    }

    sort(subID_addin_C_K.begin(), subID_addin_C_K.end());
    for (auto const& k : subID_addin_C_K) {
        TThostFtdcInstrumentIDType id_c, id_p;
        if (sty == StockType::gz300) {
            for (int i = 0; i < TradingID[sty].size(); i++) {
                if (atoi(TradingID[sty][i].substr(TradingID[sty][i].size() - 4, 4).c_str()) == k && TradingID[sty][i].substr(7, 1) == "C") {
                    strcpy(id_c, TradingID[sty][i].c_str());
                }
                else if (atoi(TradingID[sty][i].substr(TradingID[sty][i].size() - 4, 4).c_str()) == k && TradingID[sty][i].substr(7, 1) == "P") {
                    strcpy(id_p, TradingID[sty][i].c_str());
                }
            }
            OptionInfo opt_c = OptionInfo(sty, mat, OptionType::C, k, k + "0", 10000, 10001, 10000);
            OptionInfo opt_p = OptionInfo(sty, mat, OptionType::P, k, k + "0", 1, 2, 1);
            strcpy(opt_c.InstrumentID, id_c);
            strcpy(opt_p.InstrumentID, id_p);
            OptionList[mat].push_back(make_pair(opt_c, opt_p));
        }
        else if (sty == StockType::etf50 || sty == StockType::h300 || sty == StockType::s300) {
            for (int i = 0; i < TradingID[sty].size(); i++) {
                if (atoi(TradingID[sty][i].substr(TradingID[sty][i].size() - 4, 4).c_str()) == k && TradingID[sty][i].substr(6, 1) == "C") {
                    strcpy(id_c, TradingID[sty][i].c_str());
                }
                else if (atoi(TradingID[sty][i].substr(TradingID[sty][i].size() - 4, 4).c_str()) == k && TradingID[sty][i].substr(6, 1) == "P") {
                    strcpy(id_p, TradingID[sty][i].c_str());
                }
            }
            OptionInfo opt_c = OptionInfo(sty, mat, OptionType::C, k / 1000.0, k + "0", 10000, 10001, 10000);
            OptionInfo opt_p = OptionInfo(sty, mat, OptionType::P, k / 1000.0, k + "0", 1, 2, 1);
            strcpy(opt_c.InstrumentID, id_c);
            strcpy(opt_p.InstrumentID, id_p);
            OptionList[mat].push_back(make_pair(opt_c, opt_p));
        }
    }
    S_k0_posi(mat);
}