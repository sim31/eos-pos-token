# eos-pos-token
EOS token, which allows every holder to earn interest just by holding it.

It's based on standard eosio.token, so it has all the standard actions. In addition, it has `setstakespec` action which should be used after creating a token to specify configuration for the staking mechanism:
* `stake_start_time` - the time when staking starts;
* `minimum_coin_age` - minimum amount of days that has to pass before you can start earning;
* `maximum_coin_age` - amount of days after which no more interest is earned;
* `anual_interests`  - interest rates for each year;

Once coin age reaches configured minimum coin age, earned tokens can be claimed using `mint` action.

## How to Build -
* cd to 'build' directory
* run the command 'cmake ..'
* run the command 'make'

* After build -
  * The built smart contract is under the 'postoken' directory in the 'build' directory
  * You can then do a 'set contract' action with 'cleos' and point in to the './build/postoken' directory

  ---

  Tested with eosio.cdt v1.6.1.