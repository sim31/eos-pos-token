#pragma once

#include <contract.hpp>
#include <contracts.hpp>

using namespace eosio_testing;

class postoken_contract : public eosio_testing::contract {
public:

   postoken_contract(tester& tester, const account_name& acc_name = N(postoken)) 
      : contract(tester, contracts::postoken_wasm, contracts::postoken_abi, acc_name) 
   {}


   fc::variant get_stats( const string& symbolname )
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      return get_entry(symbol_code, N(stat), "currency_stats", symbol_code);
   }

   fc::variant get_account( account_name acc, const string& symbolname)
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      return get_entry(acc, N(accounts), "account", symbol_code);
   }

   fc::variant get_transfer_in(account_name acc, const uint64_t id) {
      return get_entry(acc, N(transferins), "transfer_in", id);
   }


};
