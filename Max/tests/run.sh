#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
g++ -std=gnu++98 -Wall -Wextra -Werror maxcore.cpp maxclient.cpp tests/test_maxcore.cpp -o tests/test_maxcore
./tests/test_maxcore
g++ -std=gnu++98 -Wall -Wextra -Werror maxcore.cpp maxclient.cpp tests/test_maxclient.cpp -o tests/test_maxclient
./tests/test_maxclient
g++ -std=gnu++98 -Wall -Wextra -Werror maxcore.cpp maxclient.cpp maxusers.cpp maxsettings.cpp lanmon_commands.cpp lanmon_bot.cpp tests/test_parity.cpp -o tests/test_parity
./tests/test_parity
