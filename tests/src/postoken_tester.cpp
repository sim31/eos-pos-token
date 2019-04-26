#include <postoken_tester.hpp>
#include <contracts.hpp>

const std::vector<account_name> postoken_tester::accounts = std::vector<account_name>{
   N(acca), N(accb), N(accc), N(accd), N(acce), N(accf)
};
const symbol postoken_tester::system_symbol = symbol(4, "EOS");

postoken_tester::postoken_tester() : 
    postoken_contract(*this, contracts::postoken_wasm, contracts::postoken_abi, N(postoken)) {
   produce_block();

   postoken_contract.init();
   create_accounts(accounts);

   produce_block();
}

