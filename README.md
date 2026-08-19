# LanMon MAX — зеркальная интеграция

Эта ветка содержит консервативный вариант интеграции MAX в legacy LanMon: внешний VCL/C++Builder слой повторяет устройство существующего `Telegram/tgbot.*`, а MAX-протокол остаётся в отдельном проверенном C++98 core.

## Структура

Весь код и его тесты находятся в одном переносимом каталоге:

```text
Max/
  maxbot.h/.cpp          # MAX_BOT и TMaxBotThread — зеркало Telegram/tgbot.*
  maxtask.h              # очередь заданий, аналог Telegram task queue
  maxmsg.h               # MaxMessage / MaxMessage_LIST / MaxBotInfo
  maxuser.h              # MaxUser / MaxUser_LIST для VCL-слоя

  maxcore.h/.cpp         # JSON, DTO, URL/body, CP1251 -> UTF-8
  maxclient.h/.cpp       # MAX API без зависимости от Indy/VCL
  maxindy.h/.cpp         # production HTTPS transport C++Builder/Indy
  maxsettings.h/.cpp     # max.ini
  maxusers.h/.cpp        # portable user/alias model

  lanmon_bot.h/.cpp      # portable parity facade для автотестов
  lanmon_commands.h/.cpp # portable command router для автотестов

  tests/                 # C++98 unit/parity
  e2e/                   # MAX HTTP E2E
  e2e_lanmon/            # MAX -> LanMon -> MAX E2E
  README.md              # подробная инструкция интеграции
```

В root намеренно нет production `.cpp/.h`: каталог `Max/` можно переносить в LanMon как единый модуль рядом с `Telegram/`.

## Почему два фасада

`maxbot.*` — вариант для фактической интеграции в старый LanMon. Он повторяет форму существующего Telegram-кода: `TThread`, task queue, события, `MAX_BOT`, `OnMessages`, aliases, alarms и встроенные команды.

`lanmon_bot.*` / `lanmon_commands.*` — portable слой, на котором выполняются Linux unit/E2E. Он не обязателен для production VCL-сборки, но позволяет автоматически доказывать протокол и функциональный parity без C++Builder.

## Проверка

```bash
bash ./Max/tests/run.sh
bash ./Max/e2e/run_e2e.sh
bash ./Max/e2e_lanmon/run.sh
```

GitHub Actions использует те же команды.

VCL-файлы `Max/maxbot.*`, `Max/maxmsg.h`, `Max/maxtask.h`, `Max/maxuser.h` требуют реальный C++Builder/VCL и не компилируются на Ubuntu runner-е.

## Интеграция

Добавить каталог `Max/` в LanMon рядом с `Telegram/`, включить в проект:

```text
Max/maxbot.cpp
Max/maxcore.cpp
Max/maxclient.cpp
Max/maxindy.cpp
Max/maxsettings.cpp
Max/maxusers.cpp
```

и подключить `Max/maxbot.h` в тех же местах, где сейчас используется `Telegram/tgbot.h`.

Подробный side-by-side mapping, `max.ini`, FastScript, alarms, TLS и требования MAX: `Max/README.md`.

`PARITY.md` остаётся checklist функционального соответствия Telegram.
