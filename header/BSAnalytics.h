#ifndef _BS_ANALYTICS
#define _BS_ANALYTICS


#include "OptionType.h"
#include "cnorm.h"


double bsPricer(OptionType optType, double K, double T, double S_0, double sigma) {
    double rate = 0; // ²»¿¼ÂÇr, q
    double sigmaSqrtT = sigma * std::sqrt(T);
    double d1 = (std::log(S_0 / K) + rate)/sigmaSqrtT + 0.5 * sigmaSqrtT;
    double d2 = d1 - sigmaSqrtT;

    double V_0;
    switch (optType)
    {
    case OptionType::C:
        V_0 = S_0 * cnorm(d1) - K * exp(-rate*T) * cnorm(d2);
        break;
    case OptionType::P:
        V_0 = K * exp(-rate*T) * cnorm(-d2) - S_0 * cnorm(-d1);
        break;
    default:
        throw "unsupported optionType";
    }
    return V_0;
}
   

#endif