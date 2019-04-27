#include <postoken.hpp>

void postoken::create( name   issuer,
                       asset  maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( maximum_supply.is_valid(), "invalid supply");
    check( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
       s.min_coin_age = s.max_coin_age = s.stake_start_time = 0;
    });
}


void postoken::issue( name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must issue positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
      SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                          { st.issuer, to, quantity, memo }
      );
    }
}

void postoken::retire( asset quantity, string memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must retire positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity, st.issuer );
}

void postoken::transfer( name    from,
                         name    to,
                         asset   quantity,
                         string  memo )
{
    check( from != to, "cannot transfer to self" );
    require_auth( from );
    check( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.code();
    stats statstable( _self, sym.raw() );
    const auto& st = statstable.get( sym.raw() );

    require_recipient( from );
    require_recipient( to );

    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );
    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    auto payer = has_auth( to ) ? to : from;

    sub_balance( from, quantity, from );
    add_balance( to, quantity, payer );
}

void postoken::sub_balance( name owner, asset value, name ram_payer ) {
   accounts from_acnts( _self, owner.value );

   auto sym_code = value.symbol.code();
   const auto& from = from_acnts.get( sym_code.raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
   
   // Replace all transfer ins with a new one
   // Could take up time in case of a lot of transfer ins, but this could be solved easily by claiming first
   // Transaction exceeding tie limit can be a kind of a warning to the user that there might be a lot of stuff to claim
   transfer_ins transfers(_self, owner.value);
   auto index = transfers.get_index<"symbol"_n>();
   // Returns lower bound - first matching
   auto itr = index.require_find(sym_code.raw(), "No transfer ins found, even though balance exists!");
   do {
      check(itr->quantity.symbol == value.symbol, "Invalid precision in transferin!");
      itr = index.erase(itr);
   } while( itr != index.end() && itr->quantity.symbol.code() == sym_code );

   if( from.balance.amount > 0 ) {
      transfers.emplace(ram_payer, [&](transfer_in& tr) {
         tr.id       = transfers.available_primary_key();
         tr.quantity = from.balance;
         tr.time     = now();
      });
   }
}

void postoken::add_balance( name owner, asset value, name ram_payer )
{
   accounts to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }

   transfer_ins transfers(_self, owner.value);
   transfers.emplace(ram_payer, [&](transfer_in& tr) {
      tr.id       = transfers.available_primary_key();
      tr.quantity = value;
      tr.time     = now();
   });
}

void postoken::open( name owner, const symbol& symbol, name ram_payer )
{
   require_auth( ram_payer );

   auto sym_code_raw = symbol.code().raw();

   stats statstable( _self, sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
   check( st.supply.symbol == symbol, "symbol precision mismatch" );

   accounts acnts( _self, owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }
}

void postoken::close( name owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( _self, owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

[[eosio::action]]
void postoken::setstakespec(const symbol_code& sym_code, const timestamp_t stake_start_time, 
                            const uint32_t min_coin_age, const uint32_t max_coin_age, 
                            const std::vector<interest_t>& anual_interests) {
   stats statstable(_self, sym_code.raw());
   auto st_it = statstable.require_find(sym_code.raw(), "Token with this symbol does not exist");

   require_auth(st_it->issuer);  

   uint32_t curr_time = now();  
   check(st_it->stake_start_time < curr_time, "Staking has already started");
   check(stake_start_time >= curr_time, "stake_start_time cannot be in the past");

   statstable.modify(st_it, _self, [&](currency_stats& st) {
      st.stake_start_time = stake_start_time;
      st.min_coin_age     = min_coin_age;
      st.max_coin_age     = max_coin_age;
      st.anual_interests  = anual_interests;
   });
}