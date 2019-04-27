#include <postoken_tester.hpp>
#include <iostream>

BOOST_AUTO_TEST_SUITE(postoken_tests)

// TODO: Tests for setstakespec

BOOST_FIXTURE_TEST_CASE(transfer_ins_tests, postoken_tester) try {

   // Check transfer_in rows were created for issued tokens (issued in postoken_tester constructor) 
   auto block_time = LAST_BLOCK_EPOCH_TIME();
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(acca), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", block_time)("id", 0) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accb), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", block_time)("id", 0) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accc), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", block_time)("id", 0) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accd), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", block_time)("id", 0) );

   account_name issuer = postoken_c.get_contract_name();
   // Check if issuer does not have any transfer ins (since he didn't issue to himself and his balance is 0)
   REQUIRE_MATCHING_OBJECT(postoken_c.get_account(issuer, "4,TOK"),
                           mvo()("balance", asset_str("0.0000 TOK")) );
   BOOST_CHECK_EQUAL(postoken_c.get_entry_count(issuer, N(transferins)), 0);

   // Check if transfer ins are created on normal transfers
   REQUIRE_SUCCESS(postoken_c.push_action(N(acca), N(transfer), 
                   mvo()("from", "acca")("to", "accb")("quantity", asset_str("10.0000 TOK"))
                        ("memo", "")) );
   produce_block();
   REQUIRE_MATCHING_OBJECT(postoken_c.get_account(N(accb), "4,TOK"),
                         mvo()("balance", asset_str("20.0000 TOK")) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accb), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", block_time)("id", 0) );   // previous block_time
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accb), 1),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 1) );
   BOOST_CHECK_EQUAL(postoken_c.get_entry_count(N(acca), N(transferins)), 0);

   // Multiple transfers
   REQUIRE_SUCCESS(postoken_c.push_action(N(accc), N(transfer), 
                   mvo()("from", "accc")("to", "accb")("quantity", asset_str("10.0000 TOK"))
                        ("memo", "")) );
   produce_block();
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accb), 2),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 2) );
   BOOST_CHECK_EQUAL(postoken_c.get_entry_count(N(accc), N(transferins)), 0);

   // Check if the transfer in is renewed with the current time when the balance is not fully transfered out
   REQUIRE_SUCCESS(postoken_c.push_action(N(accd), N(transfer), 
                   mvo()("from", "accd")("to", "accc")("quantity", asset_str("6.0000 TOK"))
                        ("memo", "")) );
   produce_block();
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accc), 0),
                         mvo()("quantity", asset_str("6.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 0) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_in(N(accd), 0),
                         mvo()("quantity", asset_str("4.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 0) );
   

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END() // postoken_tests


