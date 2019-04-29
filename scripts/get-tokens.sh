#!/bin/bash

TOKEN_CONTRACTS=("eosio.token" "postoken")
CLEOS='cleos.sh'

print_help() {
  echo "Usage:    get-balances.sh ACCOUNT"
}

if [[ $# != 1 || $1 == "-h" || $1 == "--help" ]]; then
  print_help
  exit 1
fi

for c in "${TOKEN_CONTRACTS[@]}"; do
  $CLEOS get currency balance $c $1
done

