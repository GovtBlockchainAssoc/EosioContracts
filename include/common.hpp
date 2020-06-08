#pragma once

#include <eosio/symbol.hpp>

using namespace eosio;
using std::string;

namespace common {

    static const symbol         S_REWARD                        ("REWARD", 2);
    static const symbol         S_VOTE                          ("VOTEPOW", 2);
   
    static const asset          RAM_ALLOWANCE                   = asset (20000, symbol("TLOS", 4));
};