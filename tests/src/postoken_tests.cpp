#include <postoken_tester.hpp>
#include <iostream>

BOOST_AUTO_TEST_SUITE(postoken_tests)

BOOST_FIXTURE_TEST_CASE(postoken_test, postoken_tester) try {
   BOOST_CHECK(true);

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()


