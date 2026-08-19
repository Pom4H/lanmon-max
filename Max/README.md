# MAX для LanMon: зеркальный вариант

Каталог `Max/` предназначен для копирования в legacy LanMon рядом с существующим `Telegram/`.

Цель — минимальный cognitive diff для разработчика заказчика. `Max/maxbot.h/.cpp` повторяют порядок секций, имена операций, task queue и комментарии из реального `Telegram/tgbot.*`. MAX-специфичный HTTP/JSON при этом не копирует Telegram API, а делегируется проверенным `MAX_API_CLIENT` и `TMaxIndyTransport`.

## Соответствие Telegram → MAX

| Telegram | MAX |
|---|---|
| `TTgBotThread` | `TMaxBotThread` |
| `TELEGRAM_BOT` | `MAX_BOT` |
| `TB_TASK` / `TB_TASK_LIST` | `MB_TASK` / `MB_TASK_LIST` |
| `TgMessage_LIST` | `MaxMessage_LIST` |
| `TgUser_LIST` | `MaxUser_LIST` |
| `TgBot` | `MaxBot` |
| `GetMe` | `GetMe` |
| `ReadMessages` | `ReadMessages` |
| `SendMessage` | `SendMessage` |
| `SendPhoto` | `SendPhoto` |
| `SendDoc` | `SendDoc` |
| `OnNewAlarmState` | `OnNewAlarmState` |
| `Send*ByAlias` | `Send*ByAlias` |

Входящее MAX-сообщение по умолчанию вызывает существующий `OnTgMessage(...)` как compatibility hook. Это позволяет сначала внедрить MAX без переделки FastScript. Если заказчик захочет разделить события, вызов можно заменить на `OnMaxMessage` отдельным изменением.

## Файлы для production C++Builder

Добавить в `lanmon4.cbproj`:

```text
Max/maxbot.cpp
Max/maxcore.cpp
Max/maxclient.cpp
Max/maxindy.cpp
Max/maxsettings.cpp
Max/maxusers.cpp
```

Headers находятся в той же папке и используют локальные include-пути.

`lanmon_bot.*` и `lanmon_commands.*` нужны portable parity-тестам и не обязаны включаться в production VCL build.

## Что уже реализовано в `MAX_BOT`

- отдельный `TMaxBotThread`;
- очередь `GETME / READMSG / SENDMSG / SENDPHOTO / SENDDOC`;
- ручной и периодический polling;
- MAX `marker` внутри `MAX_API_CLIENT`;
- `Active`, `PeriodReadMessages`, `PeriodicReadMessagesPaused`;
- debug/error/read/getMe events;
- `Json` / `ResponseCode` diagnostic contract через последний HTTP-ответ клиента;
- text/image/file send;
- `user_id` и `chat_id` через `PeerType`;
- `MaxUser_LIST`, aliases, `InCount`, `OutCount`, `Tag`;
- `RequestAlias` authorization;
- `AlarmAlias` fan-out;
- `SCREEN / ЭКРАН`;
- `MAP / КАРТА`;
- `STOP / СТОП`;
- `LOG / ЖУРНАЛ`;
- `LOGXLS`;
- `ALARM / ТРЕВОГИ`;
- `HELP / ?`;
- `SendMessageByAlias`, `SendPhotoByAlias`, `SendDocByAlias`;
- INI load/save.

Историческая семантика Telegram сохранена: script callback вызывается до встроенных команд, а `FlagSendMaps` gate используется для всех built-in команд.

## Кодировка

MAX API возвращает UTF-8. Старый LanMon работает через `AnsiString`/CP1251.

`maxmsg.h` преобразует входящий UTF-8 через WinAPI:

```text
MultiByteToWideChar(CP_UTF8)
  -> WideCharToMultiByte(1251)
  -> AnsiString
```

Исходящий `AnsiString` преобразуется через `MaxUtf8FromCp1251` перед отправкой в MAX.

## `max.ini`

Пример:

```ini
[SETUP]
Active=1
BotToken=...
PeriodReadMessages=30
PeriodicReadMessagesPaused=0
SendAlarms=1
SendAlarmEnd=1
OperatorAlarm=1
AlarmAlias=A!!!
RequestAlias=R!!!
SendMaps=1
UseLanmonLog=0

[User0]
Name=Operator
id=123456789
PeerType=user
Alias=R001
Comment=
IsBot=0
InCount=0
OutCount=0
Tag=0

[User1]
Name=Dispatchers
id=987654321
PeerType=chat
Alias=A001
Comment=Групповой чат аварий
IsBot=0
InCount=0
OutCount=0
Tag=0
```

`PeerType=user` отправляет через `user_id`, `PeerType=chat` — через `chat_id`. Если поле отсутствует, используется `user`.

Правила alias идентичны Telegram: пустая маска и `*` разрешают всех валидных пользователей, `!` заменяет ровно один символ.

## Подключение событий LanMon

В `maxbot.cpp` используются существующие функции/объекты платформы:

```text
OnTgMessage
GetMonitorScreenshot
DesktopScreenshot
CreateMapScreenshot
Bmp2Png
CloseAvariaForm
LogView->ExportToHtml
LogView->ExportToXls
CreateAlarmsPdf
szBitmapDir
szWorkDir
```

То есть интеграция намеренно выглядит так же, как Telegram, вместо введения нового application abstraction layer.

Аварийные точки `Avaria.cpp` / `HistoryAlarm.cpp` подключаются так же, как Telegram, дополнительным вызовом:

```cpp
if(MaxBot.FlagSendAlarms)
    MaxBot.OnNewAlarmState(mess);
```

Аналогично для `FlagSendAlarmsEnd` и `FlagOperatorAlarm` в существующих местах Telegram-вызовов.

## TLS / требования MAX

- API host: `https://platform-api2.max.ru`;
- токен передаётся в `Authorization`;
- исходящий HTTPS должен быть разрешён и к API, и к upload URL, возвращаемому `/uploads`;
- `TMaxIndyTransport::SSL()` позволяет настроить trust store старого Indy/OpenSSL;
- сертификат нельзя обходить постоянным отключением verification;
- long polling нельзя выполнять на VCL UI thread — для этого и сохранён `TMaxBotThread`.

## Тесты

Из корня этого репозитория:

```bash
bash ./Max/tests/run.sh
bash ./Max/e2e/run_e2e.sh
bash ./Max/e2e_lanmon/run.sh
```

Portable части компилируются с:

```text
-std=gnu++98 -Wall -Wextra -Werror
```

VCL mirror (`maxbot.*`, `maxmsg.h`, `maxtask.h`, `maxuser.h`) требует реальный C++Builder и является последней environment-specific acceptance boundary.
