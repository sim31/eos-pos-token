#!/bin/sh

CLEOS='cleos.sh'

print_help() {
  echo "Usage:   transfer.sh CONTRACT FROM TO AMOUNT [MEMO]"
}

if [[ $# < 4 || $1 == "-h" || $1 == "--help" ]]; then
  print_help
  exit 1
fi

$CLEOS push action $1 transfer "[$2, $3, \"$4\", \"$5\"]" -p $2
