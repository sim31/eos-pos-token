#include <postoken.hpp>
#include <cmath>

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

void postoken::claim(const name& account, const symbol_code& sym_code) {
   require_auth(account);
   stats statstable( _self, sym_code.raw() );
   const auto& st = statstable.get( sym_code.raw() );
   auto curr_time = now();
   symbol sym     = st.max_supply.symbol;

   check(st.stake_start_time < curr_time, "Can't claim before stake start time");

   // Determine interest rate
   asset interest_rate = get_interest_rate(st, curr_time);
   check(interest_rate.amount > 0, "Nothing to claim: 0 interest rate");

   // Determine coin age
   transfer_ins tr_table(_self, account.value);
   auto index = tr_table.get_index<"symbol"_n>();

   auto itr = index.require_find(sym_code.raw(), "Nothing to claim");
   asset coin_age(0, sym);
   asset balance(0, sym);
   for ( ; itr != index.end() && itr->quantity.symbol == sym; itr++ ) {
      uint32_t start_time = std::max(st.stake_start_time, itr->time);
      uint32_t age = epoch_to_days(curr_time - start_time);
      balance += itr->quantity;
      if( age >= st.min_coin_age ) {
         age = std::min(static_cast<uint32_t>(st.max_coin_age), age);
         coin_age += itr->quantity * age;
      }
   }

   // Calculate resulting reward
   asset m = asset(static_cast<uint64_t>(std::pow(10, coin_age.symbol.precision())), sym);
   asset reward = (coin_age.amount * interest_rate) / (365 * m).amount;

   check(reward.amount > 0, "Nothing to claim");

   // Issue new tokens
   asset rem = st.max_supply - st.supply;
   check(rem.amount > 0, "Max supply reached");
   if( rem < reward )
      reward = rem;

   statstable.modify(st, same_payer, [&](currency_stats& st) {
      st.supply += reward;
   });

   add_balance(account, reward, account);

   // Update transferins
   erase_transferins(index, sym);
   tr_table.emplace(account, [&](transfer_in& tr) {
      tr.id       = tr_table.available_primary_key();
      tr.quantity = balance + reward;
      tr.time     = curr_time;
   });
}

asset postoken::get_interest_rate(const currency_stats& stats, uint32_t epoch_time) {
   asset interest_rate(0, stats.max_supply.symbol);
   uint32_t years_passed = epoch_to_days(epoch_time - stats.stake_start_time) / 365;
   uint16_t y = 0;
   for( const interest_t& rate : stats.anual_interests ) {
      if( rate.years + y > years_passed || rate.years == 0 ) {
         interest_rate = rate.interest_rate;
         break;
      }
      y += rate.years;
   }
   return interest_rate;
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
   erase_transferins(index, value.symbol);

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

void postoken::setstakespec(const timestamp_t stake_start_time, 
                            const uint16_t min_coin_age, const uint16_t max_coin_age, 
                            const std::vector<interest_t>& anual_interests) {

   check(anual_interests.size() > 0, "You have to specify interest rates");

   symbol sym = anual_interests[0].interest_rate.symbol;
   symbol_code sym_code = sym.code();
   stats statstable(_self, sym_code.raw());
   auto st_it = statstable.require_find(sym_code.raw(), "Token with this symbol does not exist");

   require_auth(st_it->issuer);  

   check(st_it->max_supply.symbol == sym, "Invalid token precision");
   for( const interest_t& i : anual_interests )
      check(i.interest_rate.symbol == sym, "All anual interest rates have to have the same symbol");

   uint32_t curr_time = now();  
   check(st_it->stake_start_time < curr_time, "Staking has already started");
   check(stake_start_time >= curr_time, "stake_start_time cannot be in the past");

   check(max_coin_age > 0, "Coin age cannot be 0");
   check(min_coin_age <= max_coin_age, "min_coin_age cannot be greater than max_coin_age");

   statstable.modify(st_it, _self, [&](currency_stats& st) {
      st.stake_start_time = stake_start_time;
      st.min_coin_age     = min_coin_age;
      st.max_coin_age     = max_coin_age;
      st.anual_interests  = anual_interests;
   });
}