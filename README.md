# LanMon MAX

MAX-интеграция для legacy LanMon на C++Builder.

Главный принцип текущей реализации: каталог `Max/` повторяет существующий `Telegram/` по структуре и lifecycle, чтобы разработчик LanMon мог сравнивать код side-by-side без изучения новой application-архитектуры.

## Структура

```text
Max/
  maxbot.h/.cpp       # аналог Telegram/tgbot.*
  maxmsg.h/.cpp       # аналог Telegram/tgmsg.*; сообщения + пользователи
  maxtask.h/.cpp      # аналог Telegram/tgtask.*
  UFMaxBot.*          # аналог UFTgBot.*
  UFMaxBotApi.*       # аналог UFBotApi.*
  UFMaxUserEdit.*     # аналог UFTgUserEdit.*
  UFMaxMsg.*          # аналог UFTgMsg.*
  api/                # внутренний MAX HTTP/JSON transport layer
  tests/              # C++98 API tests + проверка зеркальной структуры
  e2e/                # локальный MAX HTTP E2E
```

`Max/api/` — техническая реализация протокола MAX. Она не задаёт архитектуру LanMon и скрыта за `MAX_BOT`/`TMaxBotThread`.

## Что сохранено из Telegram

- `TThread` + task queue;
- `MAX_BOT` как аналог `TELEGRAM_BOT`;
- `GetMe`, `ReadMessages`, `SendMessage`, `SendPhoto`, `SendDoc`;
- callbacks потока назначаются снаружи, как в существующем `MainForm`;
- `UserList`, aliases, `RequestAlias`, `AlarmAlias`, counters, `Tag`;
- прямой INI `Load/Save` через `TFastIniFile`;
- `OnNewAlarmState`;
- `OnMessages` и команды `SCREEN/MAP/STOP/LOG/LOGXLS/ALARM/HELP` с русскими вариантами;
- `Send*ByAlias`;
- порядок секций и комментарии в `maxbot.h/.cpp` перенесены из `tgbot.h/.cpp`.

MAX-специфичное отличие: пользователь хранит `PeerType=user|chat`, потому что MAX разделяет `user_id` и `chat_id`.

## Проверка

```bash
bash ./Max/tests/run.sh
bash ./Max/e2e/run_e2e.sh
```

Linux CI проверяет C++98 API/core и структурные invariants зеркального VCL-кода. Сам VCL-слой должен быть финально собран реальным C++Builder/Indy toolchain LanMon.

Подробная инструкция интеграции — в `Max/README.md`.
