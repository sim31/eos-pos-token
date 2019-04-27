#include <postoken_tester.hpp>
#include <contracts.hpp>

const std::vector<account_name> postoken_tester::accounts = std::vector<account_name>{
   N(acca), N(accb), N(accc), N(accd), N(acce), N(accf)
};
const symbol postoken_tester::system_symbol = symbol(4, "EOS");

postoken_tester::postoken_tester() : postoken_c(*this) {

   produce_block();

   postoken_c.init();
   create_accounts(accounts);

   // Create token for testing and issue to some accounts
   REQUIRE_SUCCESS(postoken_c.push_action(postoken_c.get_contract_name(), N(create),
                   mvo()("issuer", postoken_c.get_contract_name())
                        ("maximum_supply", asset_str("1000000.0000 TOK"))
   ));

   // REQUIRE_SUCCESS(postoken_c.push_action(postoken_c.get_contract_name(), N(issue),
   //                 mvo()("to", "acca")("quantity", asset_str("10.0000 TOK"))
   //                      ("memo", "issue")
   // ));
   // REQUIRE_SUCCESS(postoken_c.push_action(postoken_c.get_contract_name(), N(issue),
   //                 mvo()("to", "accb")("quantity", asset_str("10.0000 TOK"))
   //                      ("memo", "issue")
   // ));
   // REQUIRE_SUCCESS(postoken_c.push_action(postoken_c.get_contract_name(), N(issue),
   //                 mvo()("to", "accc")("quantity", asset_str("10.0000 TOK"))
   //                      ("memo", "issue")
   // ));
   // REQUIRE_SUCCESS(postoken_c.push_action(postoken_c.get_contract_name(), N(issue),
   //                 mvo()("to", "accd")("quantity", asset_str("10.0000 TOK"))
   //                      ("memo", "issue")
   // ));
}

