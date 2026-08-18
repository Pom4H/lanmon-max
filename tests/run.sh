#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
g++ -std=gnu++98 -Wall -Wextra -Werror maxcore.cpp maxclient.cpp tests/test_maxcore.cpp -o tests/test_maxcore
./tests/test_maxcore
g++ -std=gnu++98 -Wall -Wextra -Werror maxcore.cpp maxclient.cpp tests/test_maxclient.cpp -o tests/test_maxclient
./tests/test_maxclient
