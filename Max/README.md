# MAX для LanMon — зеркальная интеграция

Этот каталог рассчитан на размещение рядом с существующим `Telegram/` в LanMon.

Цель: разработчик проекта должен открыть `Telegram/tgbot.cpp` и `Max/maxbot.cpp` и увидеть тот же lifecycle, те же группы методов, тот же task queue и те же комментарии. Нового application abstraction layer здесь нет.

## Соответствие файлов

| Telegram | MAX |
|---|---|
| `tgbot.h/.cpp` | `maxbot.h/.cpp` |
| `tgmsg.h/.cpp` | `maxmsg.h/.cpp` |
| `tgtask.h/.cpp` | `maxtask.h/.cpp` |
| `UFTgBot.*` | `UFMaxBot.*` |
| `UFBotApi.*` | `UFMaxBotApi.*` |
| `UFTgUserEdit.*` | `UFMaxUserEdit.*` |
| `UFTgMsg.*` | `UFMaxMsg.*` |

`api/` содержит только MAX-специфичный HTTP/JSON: `maxcore`, `maxclient`, `maxindy`. Он используется из `TMaxBotThread` вместо прямых Telegram HTTP-запросов.

## Файлы для `lanmon4.cbproj`

Добавить:

```text
Max/maxbot.cpp
Max/maxmsg.cpp
Max/maxtask.cpp
Max/UFMaxBot.cpp
Max/UFMaxBotApi.cpp
Max/UFMaxUserEdit.cpp
Max/UFMaxMsg.cpp
Max/api/maxcore.cpp
Max/api/maxclient.cpp
Max/api/maxindy.cpp
```

DFM-файлы лежат рядом с соответствующими формами. Встроенные Telegram-иконки из исходных DFM не переносятся.

## Подключение в MainForm

Подключение повторяет Telegram. После загрузки `MaxBot.ini` назначить callbacks снаружи:

```cpp
MaxBot.Load(WorkDir+"MaxBot.ini");
MaxBot.SetOnTaskReadMessages(OnMaxTaskReadMessages);
MaxBot.SetOnPeriodicReadMessages(OnMaxPeriodicReadMessages);
MaxBot.SetOnGetMe(OnMaxGetMe);
MaxBot.SetOnDebugMessage(OnMaxDebugMessage);
MaxBot.SetOnErrorDebugMessage(OnMaxErrorDebugMessage);
```

Обработчик периодического чтения должен вызывать бизнес-обработку, так же как Telegram:

```cpp
void __fastcall TMainForm::OnMaxPeriodicReadMessages(MaxMessage_LIST &msglist)
{
    MaxBot.OnMessages(msglist);
}
```

`MAX_BOT` намеренно не подписывает сам себя на thread callbacks.

## FastScript

На первом внедрении `MAX_BOT::OnMessages` вызывает существующий `OnTgMessage(...)` как compatibility hook. Это позволяет не ломать уже существующие пользовательские скрипты. Если нужен отдельный `OnMaxMessage`, его следует добавить в `EventFS` отдельным изменением платформы.

## Аварии

В существующих точках `Avaria.cpp`/`HistoryAlarm.cpp` добавить MAX рядом с Telegram:

```cpp
if(MaxBot.FlagSendAlarms)
    MaxBot.OnNewAlarmState(mess);
```

Аналогично для `FlagSendAlarmsEnd` и `FlagOperatorAlarm` в тех же местах, где сейчас вызывается Telegram.

## Пользователи и адресация

`MaxUser` находится в `maxmsg.*`, как `TgUser` в `tgmsg.*`. Поля повторяют Telegram: `Id`, `Name`, `Alias`, `Comment`, `IsBot`, `InCount`, `OutCount`, `Tag`.

Дополнительное поле:

```text
PeerType=user | chat
```

Оно определяет, использовать `user_id` или `chat_id`. Для старого/ручного значения без `PeerType` используется `user`.

## Команды

Сохранены Telegram-команды и историческая семантика:

```text
SCREEN / ЭКРАН
MAP / КАРТА
STOP / СТОП
LOG / ЖУРНАЛ
LOGXLS
ALARM / ТРЕВОГИ
HELP / ?
```

`OnTgMessage` compatibility hook вызывается до проверки разрешения встроенных команд. `FlagSendMaps`, несмотря на название, блокирует все встроенные команды — это поведение исходного Telegram сохранено намеренно.

## MAX API

Транспорт находится в `api/`. Токен передаётся через `Authorization`; изображения и документы отправляются через MAX upload flow; `marker` long polling хранится в `MAX_API_CLIENT`.

Для production C++Builder используется `TMaxIndyTransport`.

## Кодировка

Исходящий LanMon `AnsiString`/CP1251 преобразуется в UTF-8 перед MAX API. Входящий UTF-8 преобразуется обратно в CP1251 в `maxmsg.cpp` через WinAPI.

## Тесты

```bash
bash ./tests/run.sh
bash ./e2e/run_e2e.sh
```

`tests/test_mirror.py` отдельно запрещает возвращение файлов альтернативной архитектуры (`lanmon_bot`, `lanmon_commands`, `maxsettings`, `maxusers`) и проверяет зеркальные классы/комментарии.
