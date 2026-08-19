# Telegram → MAX: функциональный паритет

Этот checklist фиксирует наблюдаемое поведение Telegram-интеграции LanMon, которое должно сохраняться в MAX.

## Transport / API

- [x] `GetMe`
- [x] Long Polling `/updates`
- [x] MAX `marker`
- [x] text send
- [x] image send
- [x] document/file send
- [x] `user_id`
- [x] `chat_id`
- [x] `PeerType` для сохранённых пользователей
- [x] JSON escaping
- [x] CP1251 → UTF-8
- [x] UTF-8 → CP1251 для VCL mirror
- [x] HTTP/network errors
- [x] последний HTTP status/body для `ResponseCode` / `Json`

## Команды LanMon

- [x] `SCREEN` / `ЭКРАН`
- [x] monitor screenshot → desktop fallback
- [x] `MAP` / `КАРТА`
- [x] `STOP` / `СТОП`
- [x] `LOG` / `ЖУРНАЛ`
- [x] `LOGXLS`
- [x] `ALARM` / `ТРЕВОГИ`
- [x] `HELP` / `?`
- [x] lowercase Russian commands

## Users / aliases

- [x] `Id`
- [x] `PeerType`
- [x] `Name`
- [x] `Alias`
- [x] `Comment`
- [x] `IsBot`
- [x] `InCount`
- [x] `OutCount`
- [x] `Tag`
- [x] пустая alias mask
- [x] `*`
- [x] `!` wildcard одного символа
- [x] `RequestAlias`
- [x] `AlarmAlias`
- [x] numeric alias fallback
- [x] `$N` free alias

## Settings / state

- [x] `Active`
- [x] `PeriodReadMessages`
- [x] `PeriodicReadMessagesPaused`
- [x] `FlagSendAlarms`
- [x] `FlagSendAlarmsEnd`
- [x] `FlagOperatorAlarm`
- [x] `FlagSendMaps`
- [x] `UseLanmonLog`
- [x] bot token/info
- [x] INI load/save
- [x] user INI load/save
- [x] `ReadMessagesCount`
- [x] `ReadMessagesCountOk`
- [x] `UserMessageCount`

## Mirror VCL layer

- [x] `TMaxBotThread` аналог `TTgBotThread`
- [x] `MAX_BOT` аналог `TELEGRAM_BOT`
- [x] task queue `GETME/READMSG/SENDMSG/SENDPHOTO/SENDDOC`
- [x] debug/error/read/getMe callbacks
- [x] `OnNewAlarmState`
- [x] `SendMessageByAlias`
- [x] `SendPhotoByAlias`
- [x] `SendDocByAlias`
- [x] compatibility callback через существующий `OnTgMessage`
- [x] порядок секций/комментарии повторяют Telegram там, где логика эквивалентна

## Сохранённая legacy-семантика

- [x] script callback вызывается до проверки built-in commands
- [x] `FlagSendMaps` блокирует все built-in Telegram-команды, несмотря на название
- [x] built-in command требует известного пользователя с подходящим `RequestAlias`
- [x] alarm fan-out идёт только по `AlarmAlias`
- [x] `OutCount` увеличивается при отправке, `InCount` — при принятом сообщении

## Автоматическое доказательство

`Max/tests/test_parity.cpp` проверяет behavioral parity и edge cases.

`Max/e2e/` проверяет настоящий TCP/HTTP MAX flow.

`Max/e2e_lanmon/` проверяет:

1. последовательность Long Poll + marker;
2. text replies;
3. image upload + attachment;
4. file upload + attachment;
5. screen fallback;
6. alarm alias fan-out.

Все portable части компилируются в GitHub Actions с:

```text
-std=gnu++98 -Wall -Wextra -Werror
```

## Что остаётся environment-specific

Это не отсутствующий функционал, а последняя acceptance boundary:

- собрать `Max/maxbot.cpp` и `Max/maxindy.cpp` реальным legacy C++Builder/Indy;
- настроить trust store/CA MAX;
- сделать реальный `GET /me`;
- принять одно реальное `/updates` сообщение;
- выполнить команду и проверить text/image/file response.
