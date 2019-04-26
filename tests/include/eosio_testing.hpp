#pragma once

#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <contracts.hpp>
#include <algorithm>

#include <fc/variant_object.hpp>

#define REQUIRE_SUCCESS(act_result) BOOST_REQUIRE_EQUAL(act_result, eosio::testing::base_tester::success())
#define REQUIRE_ASSERT_MSG(act_result, msg) BOOST_REQUIRE_EQUAL(act_result, wasm_assert_msg(msg))
#define CHECK_ASSERT_MSG(act_result, msg) BOOST_CHECK_EQUAL(act_result, wasm_assert_msg(msg))
#define CHECK_ERROR_MSG(act_result, msg) BOOST_CHECK_EQUAL(act_result, error(msg))
#define CHECK_SUCCESS(act_result) BOOST_CHECK_EQUAL(act_result, eosio::testing::base_tester::success())

#define CHECK_MATCHING_OBJECT(left, right) { auto a = fc::variant( left ); auto b = fc::variant( right ); BOOST_CHECK_EQUAL( true, a.is_object() ); \
   BOOST_CHECK_EQUAL( true, b.is_object() ); \
   if( a.is_object() && b.is_object() ) { \
      auto filtered = ::eosio::testing::filter_fields( a.get_object(), b.get_object() ); \
      BOOST_CHECK_EQUAL_COLLECTIONS( a.get_object().begin(), a.get_object().end(), filtered.begin(), filtered.end() ); } \
   }

#define LAST_BLOCK_TIMEPOINT() control->head_block_state()->header.timestamp.to_time_point()
#define LAST_BLOCK_EPOCH_TIME() LAST_BLOCK_TIMEPOINT().sec_since_epoch()

namespace eosio_testing {

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;
using action_result = base_tester::action_result;

static inline action_result auth_error(const account_name& req_acc_auth) {

   return std::string("missing authority of ") + std::string(req_acc_auth);
}

static inline asset asset_str(const std::string& str) {
   return asset::from_string(str);
}

static inline vector<char> to_bytes(const string& str) {
   return vector<char>(str.begin(), str.end());

}

}
