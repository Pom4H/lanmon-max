# Telegram → MAX parity

Источник контракта — реальный каталог `Telegram/` из LanMon.

## Зеркальная структура

- [x] `TELEGRAM_BOT` → `MAX_BOT`
- [x] `TTgBotThread` → `TMaxBotThread`
- [x] `TB_TASK/TB_TASK_LIST` → `MB_TASK/MB_TASK_LIST`
- [x] `TgMessage/TgMessage_LIST` → `MaxMessage/MaxMessage_LIST`
- [x] `TgUser/TgUser_LIST` → `MaxUser/MaxUser_LIST`
- [x] users/messages находятся в `maxmsg.*`, а не в отдельном application-layer
- [x] task implementation находится в `maxtask.cpp`
- [x] `MAX_BOT` не перехватывает callbacks, их назначает `MainForm`
- [x] прямой `TFastIniFile` Load/Save
- [x] порядок блоков и комментарии `maxbot.*` повторяют `tgbot.*`
- [x] альтернативные `lanmon_bot/lanmon_commands/maxsettings/maxusers` удалены

## Поведение

- [x] `GetMe`
- [x] polling + MAX marker
- [x] text/image/file
- [x] `user_id` / `chat_id`
- [x] `RequestAlias` / `AlarmAlias`
- [x] counters / Tag
- [x] `SCREEN`, `MAP`, `STOP`, `LOG`, `LOGXLS`, `ALARM`, `HELP`
- [x] русские варианты команд
- [x] `SendMessageByAlias`, `SendPhotoByAlias`, `SendDocByAlias`
- [x] `OnNewAlarmState`
- [x] callback до command gate
- [x] исторический общий gate `FlagSendMaps`

## MAX-specific

- `PeerType` добавлен к пользователю, потому что MAX отдельно адресует user и chat.
- HTTP/JSON реализация изолирована в `Max/api/` и не меняет архитектуру LanMon.

## Автоматическая проверка

`Max/tests/run.sh`:
- компилирует API/core в C++98 с `-Wall -Wextra -Werror`;
- проверяет зеркальную структуру исходников.

`Max/e2e/run_e2e.sh` проверяет реальный локальный HTTP flow MAX.

Окончательная граница acceptance — сборка VCL-файлов реальным C++Builder/Indy из проекта LanMon.
