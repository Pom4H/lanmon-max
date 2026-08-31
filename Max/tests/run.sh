#!/bin/sh
set -eu

# Запускается из любого текущего каталога, но собирает тесты относительно Max/.
cd "$(dirname "$0")/.."

# Проверка vendored цепочки Минцифры: fingerprints, validity и Sub -> Root.
bash tests/test_cert.sh

# Проверка protocol core: JSON, URL, marker, CP1251/UTF-8, membership events и DTO MAX.
g++ -std=gnu++98 -Wall -Wextra -Werror api/maxcore.cpp api/maxclient.cpp tests/test_maxcore.cpp -o tests/test_maxcore
./tests/test_maxcore

# Проверка MAX_API_CLIENT через mock transport без реальной сети.
g++ -std=gnu++98 -Wall -Wextra -Werror api/maxcore.cpp api/maxclient.cpp tests/test_maxclient.cpp -o tests/test_maxclient
./tests/test_maxclient

# Регрессия live MAX: image upload может вернуть photos map вместо top-level token.
g++ -std=gnu++98 -Wall -Wextra -Werror api/maxcore.cpp api/maxclient.cpp tests/test_image_upload.cpp -o tests/test_image_upload
./tests/test_image_upload

# Структурная проверка зеркала Telegram и обязательных комментариев.
python3 tests/test_mirror.py

# Поведенческий контракт VCL-кода: порядок callback/auth/gate, команды,
# alias-логика, alarm fan-out, FIFO задач, PeerType и TLS trust.
python3 tests/test_contract.py

# Полная инвентаризация use cases реального Telegram-модуля LanMon 4.
python3 tests/test_telegram_usecases.py

# Guard фактического toolchain заказчика: BCB2007 + обновлённый Indy Id*.
python3 tests/test_bcb2007.py

# Indy бросает exception на HTTP 400; тело MAX должно сохраняться для retry.
python3 tests/test_indy_protocol_error.py
