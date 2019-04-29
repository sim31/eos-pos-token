#!/bin/sh

CLEOS="cleos.sh"
CONTRACT="postoken"

$CLEOS push action $CONTRACT "$@"



