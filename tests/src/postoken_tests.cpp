#include <postoken_tester.hpp>
#include <iostream>

BOOST_AUTO_TEST_SUITE(postoken_tests)

// TODO: Tests for setstakespec

BOOST_FIXTURE_TEST_CASE(transfer_ins_tests, postoken_tester) try {

   // Check transfer_in rows were created for issued tokens
   REQUIRE_SUCCESS(postoken_c.push_action(postoken_c.get_contract_name(), N(issue),
                   mvo()("to", "acca")("quantity", asset_str("10.0000 TOK"))
                        ("memo", "issue")
   ));
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(acca), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 0) );

   REQUIRE_SUCCESS(postoken_c.push_action(postoken_c.get_contract_name(), N(issue),
                   mvo()("to", "accb")("quantity", asset_str("10.0000 TOK"))
                        ("memo", "issue")
   ));
   auto accb_issue_time = LAST_BLOCK_EPOCH_TIME();
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accb), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", accb_issue_time)("id", 0) );

   REQUIRE_SUCCESS(postoken_c.push_action(postoken_c.get_contract_name(), N(issue),
                   mvo()("to", "accc")("quantity", asset_str("10.0000 TOK"))
                        ("memo", "issue")
   ));
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accc), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 0) );

   REQUIRE_SUCCESS(postoken_c.push_action(postoken_c.get_contract_name(), N(issue),
                   mvo()("to", "accd")("quantity", asset_str("10.0000 TOK"))
                        ("memo", "issue")
   ));
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accd), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 0) );

   account_name issuer = postoken_c.get_contract_name();
   // Check if issuer does not have any transfer ins (since he didn't issue to himself and his balance is 0)
   REQUIRE_MATCHING_OBJECT(postoken_c.get_account(issuer, "4,TOK"),
                           mvo()("balance", asset_str("0.0000 TOK")) );
   BOOST_CHECK_EQUAL(postoken_c.get_entry_count(issuer, N(transferins)), 0);
   produce_blocks(2);
   std::cout << LAST_BLOCK_EPOCH_TIME() << std::endl;

   // Check if transfer ins are created on normal transfers
   REQUIRE_SUCCESS(postoken_c.push_action(N(acca), N(transfer), 
                   mvo()("from", "acca")("to", "accb")("quantity", asset_str("10.0000 TOK"))
                        ("memo", "")) );
   REQUIRE_MATCHING_OBJECT(postoken_c.get_account(N(accb), "4,TOK"),
                         mvo()("balance", asset_str("20.0000 TOK")) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accb), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", accb_issue_time)("id", 0) );   // previous block_time
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accb), 1),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 1) );
   BOOST_CHECK_EQUAL(postoken_c.get_entry_count(N(acca), N(transferins)), 0);
   produce_blocks(2);

   // Multiple transfers
   REQUIRE_SUCCESS(postoken_c.push_action(N(accc), N(transfer), 
                   mvo()("from", "accc")("to", "accb")("quantity", asset_str("10.0000 TOK"))
                        ("memo", "")) );
   std::cout << LAST_BLOCK_EPOCH_TIME() << std::endl;
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accb), 2),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 2) );
   BOOST_CHECK_EQUAL(postoken_c.get_entry_count(N(accc), N(transferins)), 0);
   produce_blocks(2);

   // Check if the transfer in is renewed with the current time when the balance is not fully transfered out
   REQUIRE_SUCCESS(postoken_c.push_action(N(accd), N(transfer), 
                   mvo()("from", "accd")("to", "accc")("quantity", asset_str("6.0000 TOK"))
                        ("memo", "")) );
   std::cout << LAST_BLOCK_EPOCH_TIME() << std::endl;
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accc), 0),
                         mvo()("quantity", asset_str("6.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 0) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accd), 0),
                         mvo()("quantity", asset_str("4.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 0) );
   produce_block();

   // Check renewal when transferring from account with multiple transferins
   REQUIRE_SUCCESS(postoken_c.push_action(N(accb), N(transfer), 
                   mvo()("from", "accb")("to", "acca")("quantity", asset_str("6.0000 TOK"))
                        ("memo", "")) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accb), 0),
                         mvo()("quantity", asset_str("24.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 0) );
   // Counts index as an entry
   BOOST_CHECK_EQUAL(postoken_c.get_entry_count(N(accb), N(transferins)), 2);

} FC_LOG_AND_RETHROW()

typedef asset interest_t;

BOOST_FIXTURE_TEST_CASE(claim_tests, postoken_issued_tester) try {
   auto stake_start_time = LAST_BLOCK_EPOCH_TIME() + to_epoch_time(10);
   uint32_t min_coin_age = 1;
   uint32_t max_coin_age = 30;
   std::vector<mutable_variant_object> interests{ 
      // years = 0 means forever
      mvo()("years", 0)("interest_rate", asset_str("0.1000 TOK")) 
   };   
   account_name issuer = postoken_c.get_contract_name();
   symbol s(4, "TOK");
   symbol_code sym_code = s.to_symbol_code();

   REQUIRE_SUCCESS(postoken_c.push_action(issuer, N(setstakespec), 
                   mvo()("stake_start_time", stake_start_time)
                        ("min_coin_age", min_coin_age)
                        ("max_coin_age", max_coin_age)
                        ("anual_interests", interests)) );

   // Check if you can't claim before stake_start_time
   action_result res = postoken_c.push_action(N(acca), N(claim),
                                              mvo()("account", "acca")
                                                   ("sym_code", sym_code) );
   CHECK_ASSERT_MSG(res, "Can't claim before stake start time");

   // Check if tokens issued before stake_start_time start earning from stake_start_time
   produce_block(fc::microseconds(to_epoch_time(20) * (uint64_t)1000000)); // 20 days passed
   CHECK_SUCCESS(postoken_c.push_action(N(acca), N(claim),
                 mvo()("account", "acca")("sym_code", sym_code)) );
   CHECK_MATCHING_OBJECT(postoken_c.get_account(N(acca), "4,TOK"),
                         mvo()("balance", asset_str("10.0274 TOK")) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(acca), 0),
                         mvo()("id", 0)("time", LAST_BLOCK_EPOCH_TIME())
                              ("quantity", asset_str("10.0274 TOK")) );
   BOOST_CHECK_EQUAL(postoken_c.get_entry_count(N(acca), N(transferins)), 2);

   // Check earnings after simple transfer and from multiple transferins
   produce_block(fc::microseconds(to_epoch_time(5) * (uint64_t)1000000)); // 5 days
   REQUIRE_SUCCESS(postoken_c.push_action(N(accb), N(transfer),
                   mvo()("from", "accb")("to", "acca")("quantity", "5.0000 TOK")
                        ("memo", "")) );
   produce_block(fc::microseconds(to_epoch_time(30) * (uint64_t)1000000));
   CHECK_SUCCESS(postoken_c.push_action(N(acca), N(claim),
                 mvo()("account", "acca")("sym_code", sym_code)) );
   CHECK_MATCHING_OBJECT(postoken_c.get_account(N(acca), "4,TOK"),
                         mvo()("balance", asset_str("10.1646 TOK")) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(acca), 0),
                         mvo()("id", 0)("time", LAST_BLOCK_EPOCH_TIME())
                              ("quantity", asset_str("10.1646 TOK")) );
   BOOST_CHECK_EQUAL(postoken_c.get_entry_count(N(acca), N(transferins)), 2);

   CHECK_SUCCESS(postoken_c.push_action(N(accb), N(claim),
                 mvo()("account", "accb")("sym_code", sym_code)) );
   CHECK_MATCHING_OBJECT(postoken_c.get_account(N(accb), "4,TOK"),
                         mvo()("balance", asset_str("5.0410 TOK")) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accb), 0),
                         mvo()("id", 0)("time", LAST_BLOCK_EPOCH_TIME())
                              ("quantity", asset_str("5.0410 TOK")) );
   BOOST_CHECK_EQUAL(postoken_c.get_entry_count(N(accb), N(transferins)), 2);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(variable_interest_rates, postoken_issued_tester) try {
   auto stake_start_time = LAST_BLOCK_EPOCH_TIME() + to_epoch_time(1);
   uint32_t min_coin_age = 1;
   uint32_t max_coin_age = 30;
   std::vector<mutable_variant_object> interests{ 
      // years = 0 means forever (or until max_supply is reached)
      mvo()("years", 1)("interest_rate", asset_str("1.0000 TOK")),
      mvo()("years", 2)("interest_rate", asset_str("0.1000 TOK")),
      mvo()("years", 0)("interest_rate", asset_str("0.0100 TOK"))
   };   
   account_name issuer = postoken_c.get_contract_name();
   symbol s(4, "TOK");
   symbol_code sym_code = s.to_symbol_code();

   REQUIRE_SUCCESS(postoken_c.push_action(issuer, N(setstakespec), 
                   mvo()("stake_start_time", stake_start_time)
                        ("min_coin_age", min_coin_age)
                        ("max_coin_age", max_coin_age)
                        ("anual_interests", interests)) );

   produce_block(fc::microseconds(to_epoch_time(21) * (uint64_t)1000000)); // 20 days passed
   CHECK_SUCCESS(postoken_c.push_action(N(acca), N(claim),
                 mvo()("account", "acca")("sym_code", sym_code)) );
   CHECK_MATCHING_OBJECT(postoken_c.get_account(N(acca), "4,TOK"),
                         mvo()("balance", asset_str("10.5479 TOK")) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(acca), 0),
                         mvo()("id", 0)("time", LAST_BLOCK_EPOCH_TIME())
                              ("quantity", asset_str("10.5479 TOK")) );

   // Second year
   produce_block(fc::microseconds(to_epoch_time(365) * (uint64_t)1000000)); // a year
   // max_coin_age is reached here
   CHECK_SUCCESS(postoken_c.push_action(N(acca), N(claim),
                 mvo()("account", "acca")("sym_code", sym_code)) );
   CHECK_MATCHING_OBJECT(postoken_c.get_account(N(acca), "4,TOK"),
                         mvo()("balance", asset_str("10.6345 TOK")) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(acca), 0),
                         mvo()("id", 0)("time", LAST_BLOCK_EPOCH_TIME())
                              ("quantity", asset_str("10.6345 TOK")) );

   // Reach the end of the second year
   produce_block(fc::microseconds(to_epoch_time(344) * (uint64_t)1000000));
   CHECK_SUCCESS(postoken_c.push_action(N(acca), N(claim),
                 mvo()("account", "acca")("sym_code", sym_code)) );
   CHECK_MATCHING_OBJECT(postoken_c.get_account(N(acca), "4,TOK"),
                         mvo()("balance", asset_str("10.7219 TOK")) );

   // The final interest rate
   produce_block(fc::microseconds(to_epoch_time(29) * (uint64_t)1000000));
   CHECK_SUCCESS(postoken_c.push_action(N(acca), N(claim),
                 mvo()("account", "acca")("sym_code", sym_code)) );
   CHECK_MATCHING_OBJECT(postoken_c.get_account(N(acca), "4,TOK"),
                         mvo()("balance", asset_str("10.7304 TOK")) );

   produce_block(fc::microseconds(to_epoch_time(730) * (uint64_t)1000000));
   CHECK_SUCCESS(postoken_c.push_action(N(acca), N(claim),
                 mvo()("account", "acca")("sym_code", sym_code)) );
   CHECK_MATCHING_OBJECT(postoken_c.get_account(N(acca), "4,TOK"),
                         mvo()("balance", asset_str("10.7329 TOK")) );
            
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(interest_rate_0, postoken_issued_tester) try {
   auto stake_start_time = LAST_BLOCK_EPOCH_TIME() + to_epoch_time(1);
   uint32_t min_coin_age = 1;
   uint32_t max_coin_age = 30;
   std::vector<mutable_variant_object> interests{ 
      // years = 0 means forever (or until max_supply is reached)
      mvo()("years", 1)("interest_rate", asset_str("1.0000 TOK")),
      mvo()("years", 2)("interest_rate", asset_str("0.1000 TOK")),
      mvo()("years", 0)("interest_rate", asset_str("0.0000 TOK"))
   };   
   account_name issuer = postoken_c.get_contract_name();
   symbol s(4, "TOK");
   symbol_code sym_code = s.to_symbol_code();

   REQUIRE_SUCCESS(postoken_c.push_action(issuer, N(setstakespec), 
                   mvo()("stake_start_time", stake_start_time)
                        ("min_coin_age", min_coin_age)
                        ("max_coin_age", max_coin_age)
                        ("anual_interests", interests)) );

   // On a third year interest earnings should be 0
   produce_block(fc::microseconds(to_epoch_time(760) * (uint64_t)1000000)); // 30 days in a third year

   action_result res = postoken_c.push_action(N(acca), N(claim),
                                  mvo()("account", "acca")("sym_code", sym_code) );
   CHECK_ASSERT_MSG(res, "Nothing to claim: 0 interest rates");
            
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(last_interest_rate_unspecified, postoken_issued_tester) try {
   auto stake_start_time = LAST_BLOCK_EPOCH_TIME() + to_epoch_time(1);
   uint32_t min_coin_age = 1;
   uint32_t max_coin_age = 30;
   std::vector<mutable_variant_object> interests{ 
      // years = 0 means forever (or until max_supply is reached)
      mvo()("years", 1)("interest_rate", asset_str("1.0000 TOK")),
      mvo()("years", 2)("interest_rate", asset_str("0.1000 TOK")),
   };   
   account_name issuer = postoken_c.get_contract_name();
   symbol s(4, "TOK");
   symbol_code sym_code = s.to_symbol_code();

   REQUIRE_SUCCESS(postoken_c.push_action(issuer, N(setstakespec), 
                   mvo()("stake_start_time", stake_start_time)
                        ("min_coin_age", min_coin_age)
                        ("max_coin_age", max_coin_age)
                        ("anual_interests", interests)) );

   // On a third year interest earnings should be 0
   produce_block(fc::microseconds(to_epoch_time(760) * (uint64_t)1000000)); // 30 days in a third year

   action_result res = postoken_c.push_action(N(acca), N(claim),
                                  mvo()("account", "acca")("sym_code", sym_code) );
   CHECK_ASSERT_MSG(res, "Nothing to claim: 0 interest rates");
            
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(coin_age_parameters, postoken_issued_tester) try {
   auto stake_start_time = LAST_BLOCK_EPOCH_TIME() + 1;
   uint32_t min_coin_age = 3;
   uint32_t max_coin_age = 60;
   std::vector<mutable_variant_object> interests{ 
      // years = 0 means forever (or until max_supply is reached)
      mvo()("years", 1)("interest_rate", asset_str("0.5000 TOK")),
   };   
   account_name issuer = postoken_c.get_contract_name();
   symbol s(4, "TOK");
   symbol_code sym_code = s.to_symbol_code();

   REQUIRE_SUCCESS(postoken_c.push_action(issuer, N(setstakespec), 
                   mvo()("stake_start_time", stake_start_time)
                        ("min_coin_age", min_coin_age)
                        ("max_coin_age", max_coin_age)
                        ("anual_interests", interests)) );

   // Try claiming before min_coin_age was reached
   produce_block(fc::microseconds(to_epoch_time(2) * (uint64_t)1000000)); 
   action_result res = postoken_c.push_action(N(acca), N(claim),
                                  mvo()("account", "acca")("sym_code", sym_code) );
   CHECK_ASSERT_MSG(res, "Nothing to claim");

   // min_coin_age reached
   produce_block(fc::microseconds(to_epoch_time(3) * (uint64_t)1000000));
   CHECK_SUCCESS(postoken_c.push_action(N(acca), N(claim),
                 mvo()("account", "acca")("sym_code", sym_code)) );
   CHECK_MATCHING_OBJECT(postoken_c.get_account(N(acca), "4,TOK"),
                         mvo()("balance", asset_str("10.0410 TOK")) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(acca), 0),
                         mvo()("id", 0)("time", LAST_BLOCK_EPOCH_TIME())
                              ("quantity", asset_str("10.0410 TOK")) );

   // min_coin_age reached for only 1 of the transferins
   produce_block(fc::microseconds(to_epoch_time(25) * (uint64_t)1000000));
   REQUIRE_SUCCESS(postoken_c.push_action(N(accb), N(transfer), 
                   mvo()("from", "accb")("to", "acca")("quantity", asset_str("9.0000 TOK"))
                        ("memo", "")) );
   produce_block(fc::microseconds(to_epoch_time(1) * (uint64_t)1000000));
   CHECK_SUCCESS(postoken_c.push_action(N(acca), N(claim),
                 mvo()("account", "acca")("sym_code", sym_code)) );
   CHECK_MATCHING_OBJECT(postoken_c.get_account(N(acca), "4,TOK"),
                         mvo()("balance", asset_str("10.3576 TOK")) );

            
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END() // postoken_tests


