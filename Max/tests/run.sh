#!/bin/sh
set -eu

# Запускается из любого текущего каталога, но собирает тесты относительно Max/.
cd "$(dirname "$0")/.."

# Проверка protocol core: JSON, URL, marker, CP1251/UTF-8 и DTO MAX.
g++ -std=gnu++98 -Wall -Wextra -Werror api/maxcore.cpp api/maxclient.cpp tests/test_maxcore.cpp -o tests/test_maxcore
./tests/test_maxcore

# Проверка MAX_API_CLIENT через mock transport без реальной сети.
g++ -std=gnu++98 -Wall -Wextra -Werror api/maxcore.cpp api/maxclient.cpp tests/test_maxclient.cpp -o tests/test_maxclient
./tests/test_maxclient

# Структурная проверка зеркала Telegram и обязательных комментариев.
python3 tests/test_mirror.py
