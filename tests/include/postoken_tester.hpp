#pragma once

#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio_testing.hpp>
#include <postoken_contract.hpp>

#include "Runtime/Runtime.h"

using namespace eosio_testing;

class postoken_tester : public tester {
public:
   static const std::vector<account_name> accounts;
   static const symbol system_symbol;

   postoken_tester();

   postoken_contract postoken_c;
};

class postoken_issued_tester : public postoken_tester {
public:
   postoken_issued_tester();
};

