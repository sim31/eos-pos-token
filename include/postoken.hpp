#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;
using std::string;

inline uint32_t epoch_to_days(uint32_t epoch_time) {
   return epoch_time / 60 / 60 / 24;
}

class [[eosio::contract("postoken")]] postoken : public contract {
public:
   using contract::contract;

   typedef uint32_t timestamp_t;
   struct interest_t {
      asset    interest_rate;
      uint16_t years;
   };

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
   void setstakespec(const timestamp_t stake_start_time, 
                     const uint16_t min_coin_age, const uint16_t max_coin_age, 
                     const std::vector<interest_t>& anual_interests);

   [[eosio::action]]
   void claim(const name& account, const symbol_code& sym_code);

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
   using claim_action = eosio::action_wrapper<"claim"_n, &postoken::claim>;
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
      uint16_t                min_coin_age; // days
      uint16_t                max_coin_age; // days
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

   asset get_interest_rate(const currency_stats& stats, uint32_t epoch_time);

   template<typename Index>
   void erase_transferins(Index& index, const symbol& sym) {
      symbol_code sym_code = sym.code();
      // Returns lower bound - first matching
      auto itr = index.require_find(sym_code.raw(), "No transfer ins found");
      do {
         check(itr->quantity.symbol == sym, "Invalid precision in transferin!");
         itr = index.erase(itr);
      } while( itr != index.end() && itr->quantity.symbol.code() == sym_code );
   }

};