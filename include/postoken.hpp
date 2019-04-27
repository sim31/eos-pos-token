#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;
using std::string;

class [[eosio::contract("postoken")]] postoken : public contract {
public:
   using contract::contract;

   typedef uint64_t interest_t;
   typedef uint32_t timestamp_t;

   [[eosio::action]]
   void create( name   issuer,
                asset  maximum_supply);

   [[eosio::action]]
   void issue( name to, asset quantity, string memo );

   [[eosio::action]]
   void retire( asset quantity, string memo );

   [[eosio::action]]
   void transfer( name    from,
                  name    to,
                  asset   quantity,
                  string  memo );

   [[eosio::action]]
   void open( name owner, const symbol& symbol, name ram_payer );

   [[eosio::action]]
   void close( name owner, const symbol& symbol );

   [[eosio::action]]
   void setstakespec(const symbol_code& sym_code, const timestamp_t stake_start_time, 
                     const uint32_t min_coin_age, const uint32_t max_coin_age, 
                     const std::vector<interest_t>& anual_interests);

   static asset get_supply( name token_contract_account, symbol_code sym_code )
   {
      stats statstable( token_contract_account, sym_code.raw() );
      const auto& st = statstable.get( sym_code.raw() );
      return st.supply;
   }

   static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
   {
      accounts accountstable( token_contract_account, owner.value );
      const auto& ac = accountstable.get( sym_code.raw() );
      return ac.balance;
   }

   using create_action = eosio::action_wrapper<"create"_n, &postoken::create>;
   using issue_action = eosio::action_wrapper<"issue"_n, &postoken::issue>;
   using retire_action = eosio::action_wrapper<"retire"_n, &postoken::retire>;
   using transfer_action = eosio::action_wrapper<"transfer"_n, &postoken::transfer>;
   using open_action = eosio::action_wrapper<"open"_n, &postoken::open>;
   using close_action = eosio::action_wrapper<"close"_n, &postoken::close>;
private:
   struct [[eosio::table]] account {
      asset    balance;

      uint64_t primary_key()const { return balance.symbol.code().raw(); }
   };

   struct [[eosio::table]] transfer_in {
      uint64_t    id;
      asset       quantity;
      timestamp_t time;

      uint64_t primary_key() const {
         return id;
      }

      uint64_t symbol_key() const {
         return quantity.symbol.code().raw();
      }
   };

   struct [[eosio::table]] currency_stats {
      asset                   supply;
      asset                   max_supply;
      name                    issuer;
      uint32_t                min_coin_age; // days
      uint32_t                max_coin_age; // days
      std::vector<interest_t> anual_interests;
      timestamp_t             stake_start_time; // epoch time in seconds

      uint64_t primary_key() const { return supply.symbol.code().raw(); }
   };

   typedef eosio::multi_index< "accounts"_n, account > accounts;
   typedef eosio::multi_index< "stat"_n, currency_stats > stats;
   typedef eosio::multi_index< "transferins"_n, transfer_in, 
                               indexed_by<"symbol"_n, const_mem_fun<transfer_in, uint64_t, &transfer_in::symbol_key>>
                             > transfer_ins; 

   void sub_balance( name owner, asset value, name ram_payer ); // ram_payer - for transferins
   void add_balance( name owner, asset value, name ram_payer );
};