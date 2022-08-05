#!/usr/bin/env bash

cxx='clang++ -g -Wall -Wpedantic -Werror -Wno-unused-variable'
src='bob.cc'
name='bob'

$cxx $src -o $name
