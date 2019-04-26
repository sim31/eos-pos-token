#include <postoken_tester.hpp>
#include <iostream>

BOOST_AUTO_TEST_SUITE(postoken_tests)

// TODO: Tests for setstakespec

BOOST_FIXTURE_TEST_CASE(transfer_ins_tests, postoken_tester) try {
   // TODO: Check if accounts which were issued tokens contain transfer ins
   //       Check if issuer does not contain any transfer ins
   //       Check if transfer ins are created on normal transfers
   //             with multiple transfers
   //       Check if transfer in is renewed when balance is not fully transfered out

   // Check transfer_in rows were created for issued tokens (issued in postoken_tester constructor) 
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_ins(N(acca), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 0) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_ins(N(accb), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 0) );
   CHECK_MATCHING_OBJECT(postoken_c.get_transfer_ins(N(accc), 0),
                         mvo()("quantity", asset_str("10.0000 TOK"))
                              ("time", LAST_BLOCK_EPOCH_TIME())("id", 0) );

   

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END() // postoken_tests


