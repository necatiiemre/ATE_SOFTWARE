#!/usr/bin/env bash
# ChecksumTool bagimsiz derleme betigi
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -O2 -Wall -o checksum_tool checksum_tool.cpp
echo "OK -> $(pwd)/checksum_tool"
