#!/usr/bin/env bash
# TimeReader bagimsiz derleme betigi
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -O2 -Wall -o time_reader time_reader.cpp
echo "OK -> $(pwd)/time_reader"
