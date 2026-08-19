# LanMon MAX

MAX-интеграция для legacy LanMon на C++Builder.

Главный принцип текущей реализации: каталог `Max/` повторяет существующий `Telegram/` по структуре и lifecycle, чтобы разработчик LanMon мог сравнивать код side-by-side без изучения новой application-архитектуры.

## Важно: ограничение текущего зеркального варианта

Текущий `TMaxBotThread` повторяет Telegram и получает события через Long Polling (`GET /updates`). По актуальной документации MAX Long Polling предназначен только для разработки и тестирования. Для production MAX требует Webhook через `POST /subscriptions`.

То есть текущая ветка даёт зеркальную интеграцию в legacy LanMon и полный набор Telegram-функций, но для production-развёртывания MAX необходимо отдельно решить доставку входящих событий через публичный HTTPS Webhook (или relay, который передаёт события в LanMon).

Long Polling и Webhook одновременно использовать нельзя.

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

## Ключевые требования MAX

На 19 августа 2026:

- API: `https://platform-api2.max.ru`;
- с 19 июля 2026 старый `platform-api.max.ru` использовать нельзя; требуется новый домен и сертификат Минцифры в trust store;
- токен передаётся только заголовком `Authorization: <token>`; query-параметр для токена больше не поддерживается;
- рекомендуемый лимит — не более 30 запросов/с к API;
- production: только Webhook; Long Polling — только development/test;
- активный Webhook и Long Polling одновременно не работают;
- Webhook должен быть доступен по публичному HTTPS; HTTP и self-signed сертификаты не поддерживаются;
- для изображений используется upload `type=image` (`type=photo` больше не поддерживается);
- изображение: до 50 МБ и не более 7680×7680;
- произвольный файл: до 4 ГБ;
- upload URL надо использовать без изменения; он может вести на отдельный upload-host;
- после загрузки большого вложения MAX может вернуть `attachment.not.ready`; production-код должен делать retry с увеличением интервала.

Токен создаётся/просматривается на платформе MAX для бизнеса в настройках чат-бота. Токен нельзя логировать или хранить в репозитории.

Подробности и ссылки на официальную документацию — в `Max/README.md`.

## Проверка

```bash
bash ./Max/tests/run.sh
bash ./Max/e2e/run_e2e.sh
```

Linux CI проверяет C++98 API/core и структурные invariants зеркального VCL-кода. Сам VCL-слой должен быть финально собран реальным C++Builder/Indy toolchain LanMon.

Подробная инструкция интеграции — в `Max/README.md`.
