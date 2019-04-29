#!/bin/sh

CLEOS="cleos.sh"
CONTRACT="postoken"

$CLEOS get table $CONTRACT "$@"

