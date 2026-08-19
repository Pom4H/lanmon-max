# Зеркальный MAX-вариант для LanMon

Эта директория — альтернативный интеграционный слой для ветки `mirror/telegram-style`.

Цель этой ветки — не улучшать архитектуру LanMon, а сделать MAX максимально узнаваемым для разработчика, который уже поддерживает `Telegram/tgbot.*`.

## Соответствие имен

| Telegram | MAX mirror |
|---|---|
| `TTgBotThread` | `TMaxBotThread` |
| `TELEGRAM_BOT` | `MAX_BOT` |
| `TgMessage_LIST` | `MaxMessage_LIST` |
| `TgUser_LIST` | `MaxUser_LIST` |
| `TB_TASK` / `TB_TASK_LIST` | `MB_TASK` / `MB_TASK_LIST` |
| `TgBot` | `MaxBot` |
| `OnTgMessage` | `OnTgMessage` (общий compatibility hook) |

Порядок секций, группировка методов и комментарии над ними перенесены из реального `Telegram/tgbot.h/.cpp` LanMon. Там, где комментарий был Telegram-специфичным, заменено только название транспорта (`Telegram` → `MAX`).

## Что намеренно осталось похожим на Telegram

- `TThread` и очередь заданий;
- `GetMe`, `ReadMessages`, `SendMessage`, `SendPhoto`, `SendDoc` как отдельные task-типы;
- обработчики событий потока;
- поля `Active`, `PeriodReadMessages`, `PeriodicReadMessagesPaused`;
- `FlagSendAlarms`, `FlagSendAlarmsEnd`, `FlagOperatorAlarm`, `FlagSendMaps`;
- `AlarmAlias`, `RequestAlias`;
- `OnNewAlarmState`;
- встроенные команды прямо внутри `MAX_BOT::OnMessages`;
- `SendMessageByAlias`, `SendPhotoByAlias`, `SendDocByAlias`;
- счётчики `ReadMessagesCount`, `ReadMessagesCountOk`, `UserMessageCount`.

Это означает, что здесь снова есть часть дублирования Telegram-кода. В этой ветке это сознательный trade-off ради минимального cognitive diff при передаче заказчику.

## Что не копируется из Telegram

HTTP/JSON протокол Telegram не копируется. `TMaxBotThread` использует уже проверенные:

- `MAX_API_CLIENT`;
- `TMaxIndyTransport`;
- MAX long polling `marker`;
- MAX image/file upload flow;
- `user_id` / `chat_id`.

Для минимального diff зеркальный вариант по умолчанию вызывает существующий `OnTgMessage` и для MAX-сообщений. Отдельный `OnMaxMessage` можно добавить позже, но он не нужен для первого внедрения.

То есть внешний слой выглядит как старый Telegram, а сетевой слой остаётся MAX-специфичным и тестируемым.

## Интеграция

Файлы `Mirror/*.h/.cpp` рассчитаны на размещение рядом с исходным LanMon-кодом, где доступны `EventFS.h`, `screenshot.h`, `ULogView.h`, `LogView`, `CreateMapScreenshot`, `CreateAlarmsPdf`, `CloseAvariaForm` и остальные platform-функции.

Сравнивать при ревью лучше side-by-side:

```text
LanMon/Telegram/tgbot.h      ↔ Mirror/maxbot.h
LanMon/Telegram/tgbot.cpp    ↔ Mirror/maxbot.cpp
LanMon/Telegram/tgtask.h     ↔ Mirror/maxtask.h
LanMon/Telegram/tgmsg.h      ↔ Mirror/maxmsg.h
LanMon/Telegram/tguser.h     ↔ Mirror/maxuser.h
```

Основной `main` репозитория оставлен без изменения: эта ветка существует именно как альтернативный, максимально консервативный вариант интеграции.
