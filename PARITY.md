# Telegram → MAX functional parity

This checklist defines **functional parity with the Telegram integration used by LanMon**. It is intentionally about observable messenger/LanMon behavior; VCL form wiring and the concrete C++Builder worker-thread class remain application integration mechanics and are documented in `README.md`.

## Transport and messages

- [x] `GetMe`
- [x] Long Polling `/updates`
- [x] persistent MAX `marker`
- [x] text send
- [x] image send
- [x] generic document/file send
- [x] `user_id` addressing
- [x] `chat_id` addressing
- [x] stored peer type for alias/alarm fan-out
- [x] JSON escaping
- [x] CP1251 → UTF-8 conversion
- [x] HTTP/network error propagation

## Built-in LanMon commands

- [x] `SCREEN`
- [x] `ЭКРАН`
- [x] monitor screenshot → whole-desktop fallback
- [x] `MAP`
- [x] `КАРТА`
- [x] `STOP`
- [x] `СТОП`
- [x] `LOG`
- [x] `ЖУРНАЛ`
- [x] `LOGXLS`
- [x] `ALARM`
- [x] `ТРЕВОГИ`
- [x] `HELP`
- [x] `?`
- [x] lowercase Russian command handling

## Users and authorization

- [x] user list
- [x] `Id`
- [x] `PeerType` (`user`/`chat`, MAX-specific necessity)
- [x] `Name`
- [x] `Alias`
- [x] `Comment`
- [x] `IsBot`
- [x] `InCount`
- [x] `OutCount`
- [x] `Tag`
- [x] empty alias mask
- [x] `*` alias mask
- [x] `!` single-position wildcard
- [x] `RequestAlias`
- [x] `AlarmAlias`
- [x] numeric alias fallback
- [x] `$N` free-alias generation

## Bot settings/state

- [x] `Active`
- [x] `PeriodReadMessages`
- [x] `PeriodicReadMessagesPaused`
- [x] `FlagSendAlarms`
- [x] `FlagSendAlarmsEnd`
- [x] `FlagOperatorAlarm`
- [x] `FlagSendMaps`
- [x] `UseLanmonLog`
- [x] bot token
- [x] bot info
- [x] INI load/save
- [x] user INI load/save
- [x] `ReadMessagesCount`
- [x] `ReadMessagesCountOk`
- [x] `UserMessageCount`

## Callbacks / FastScript surface

- [x] debug callback
- [x] error callback
- [x] task-read callback
- [x] periodic-read callback
- [x] getMe callback
- [x] per-message callback before command authorization/gating
- [x] `TgSendMessage` equivalent
- [x] `TgSendPhoto` equivalent
- [x] `TgSendDoc` equivalent
- [x] `TgUserCount` equivalent
- [x] `TgGetUser` equivalent
- [x] `TgFindUser` equivalent
- [x] `TgFindUserIndex` equivalent
- [x] `TgFindUserAlias` equivalent
- [x] `TgSetUserTag` equivalent
- [x] `TgUserCanAsk` equivalent
- [x] `TgUserRcvAlarms` equivalent
- [x] `TgUserHasValidAlias` equivalent

## Preserved legacy semantics

- [x] `OnMaxMessage` fires before built-in command checks, matching `OnTgMessage`.
- [x] `FlagSendMaps` gates **all** built-in Telegram commands, despite its name.
- [x] built-in commands require a known user matching `RequestAlias`.
- [x] alarm notifications fan out only to users matching `AlarmAlias`.
- [x] alias sends update user `OutCount`; received messages update `InCount`.

## Automated proof

`tests/test_parity.cpp` checks behavioral parity and edge cases.

`e2e_lanmon/` uses a real local TCP/HTTP server and checks:

1. eight sequential Long Poll commands and marker continuity;
2. text replies;
3. image upload + attachment send;
4. file upload + attachment send;
5. screen fallback;
6. alarm alias fan-out.

All portable code is compiled with `-std=gnu++98 -Wall -Wextra -Werror` in GitHub Actions.

## Outside portable CI

The only remaining acceptance boundary is environment-specific, not missing messenger functionality:

- compile `maxindy.cpp` with the real legacy C++Builder/Indy toolchain;
- configure the MAX-required CA certificate;
- perform a real TLS `GET /me` and one real command round-trip with a MAX bot token.
