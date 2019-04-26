#include <eosiolib/eosio.hpp>

using namespace eosio;

class [[eosio::contract("postoken")]] postoken : public contract {
   public:
      using contract::contract;

      [[eosio::action]]
      void hi( name nm );

      using hi_action = action_wrapper<"hi"_n, &postoken::hi>;
};