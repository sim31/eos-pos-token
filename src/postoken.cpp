#include <postoken.hpp>

[[eosio::action]]
void postoken::hi( name nm ) {
   /* fill in action body */
   print_f("Name : %\n",nm);
}