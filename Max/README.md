# MAX для LanMon — зеркальная интеграция

Этот каталог рассчитан на размещение рядом с существующим `Telegram/` в LanMon.

Цель: разработчик проекта должен открыть `Telegram/tgbot.cpp` и `Max/maxbot.cpp` и увидеть тот же lifecycle, те же группы методов, тот же task queue и те же комментарии. Нового application abstraction layer здесь нет.

## Критическое ограничение: Long Polling и production

`TMaxBotThread` намеренно повторяет Telegram и получает обновления через `GET /updates` (Long Polling).

По актуальной документации MAX:

- Long Polling разрешён для разработки и тестирования;
- для production используется только Webhook;
- Webhook и Long Polling нельзя использовать одновременно;
- при активной Webhook-подписке `GET /updates` не работает.

Поэтому текущая зеркальная реализация подходит для интеграции, разработки, тестирования и проверки функционального паритета. Для production нужно добавить Webhook-доставку событий. Для desktop/on-prem LanMon практичный вариант — отдельный HTTPS relay: MAX отправляет Webhook на публичный endpoint, relay передаёт нормализованные события в LanMon. Исходящий `SendMessage/SendPhoto/SendDoc` при этом остаётся без изменений.

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

# Требования и особенности MAX API

Ниже требования по официальной документации MAX, актуальные на 19 августа 2026.

## 1. API endpoint и сертификаты

Использовать:

```text
https://platform-api2.max.ru
```

С 19 июля 2026 для чат-ботов и мини-приложений MAX требует новый домен `platform-api2.max.ru` вместо `platform-api.max.ru` и добавление сертификата Минцифры в список доверенных.

Для LanMon это означает: OpenSSL/Indy trust store на целевой Windows-машине должен доверять требуемой цепочке сертификатов. Нельзя решать проблему отключением TLS-проверки.

## 2. Токен и Authorization

Токен передаётся только HTTP-заголовком:

```http
Authorization: <token>
```

Передача токена через query string больше не поддерживается.

Токен можно получить в платформе MAX для бизнеса: `Чат-боты → Перейти → Расширенные настройки → Настроить`. Для некоторых верифицированных профилей токен также доступен через «MAX для бизнеса».

Требования к эксплуатации:

- не коммитить токен в Git;
- не писать токен в `lanmon.log`;
- ограничить доступ к `MaxBot.ini`;
- при компрометации выпустить/получить новый токен.

## 3. Ограничение запросов

MAX рекомендует не превышать:

```text
30 requests/second
```

на `platform-api2.max.ru`.

При массовой аварийной рассылке отправку нужно очередить, а не создавать неограниченный burst.

## 4. Получение событий

### Development / test

Разрешён Long Polling:

```http
GET /updates
```

`marker` указывает на следующее ожидаемое обновление. После передачи marker предыдущие обновления считаются прочитанными.

Если marker не передать, API возвращает только последнее обновление, поэтому состояние marker нельзя бездумно сбрасывать во время работающей сессии.

### Production

Только Webhook:

```http
POST /subscriptions
```

Webhook endpoint должен быть публично доступен по HTTPS. MAX не поддерживает для Webhook HTTP и self-signed сертификаты. Нужен сертификат доверенного центра сертификации; документация отдельно упоминает сертификаты Минцифры.

Для Webhook требуется внешне доступный сервер; в справке MAX также указан статичный IP как требование такого сценария.

Одновременная работа Webhook и Long Polling запрещена.

## 5. Что это означает для LanMon

Существующий Telegram работает полностью исходящим desktop-процессом: LanMon сам делает polling и не требует публичного сервера.

MAX production устроен иначе. Поэтому для production есть два варианта:

1. добавить публичный HTTPS endpoint непосредственно в инфраструктуру заказчика и доставлять Update в LanMon;
2. использовать небольшой relay/service между MAX и LanMon.

Рекомендуемая схема для legacy приложения:

```text
MAX
  ↓ HTTPS Webhook
public relay
  ↓ защищённый внутренний канал
LanMon / MAX_BOT::OnMessages
```

Так зеркальный код LanMon почти не меняется: transport входящих событий заменяется, а `MaxMessage_LIST`, команды, aliases, аварии и исходящая отправка сохраняются.

## 6. Отправка сообщений

Для текста используется `POST /messages` с адресацией по:

```text
user_id
chat_id
```

Поэтому `MaxUser` дополнительно хранит `PeerType`.

## 7. Изображения и файлы

Общий upload flow:

```text
POST /uploads?type=...
  ↓
получить url
  ↓
POST multipart field "data" на полученный url
  ↓
получить token
  ↓
POST /messages с attachments.payload.token
```

URL загрузки нужно использовать без изменений. Он может вести не на основной API-домен, а на отдельный upload-host. В документации указаны, например:

```text
file  → fu.oneme.ru
image → iu.oneme.ru
video/audio → vu.okcdn.ru
```

Это важно для firewall/allowlist заказчика: разрешить только `platform-api2.max.ru` недостаточно для отправки вложений.

## 8. Ограничения вложений

### image

Поддерживаются:

```text
JPG, JPEG, PNG, GIF, TIFF, BMP, HEIC
```

Ограничение одного изображения:

```text
до 50 МБ
и не более 7680 × 7680 px
```

Оба условия должны выполняться одновременно.

`type=photo` больше не поддерживается. Использовать:

```text
type=image
```

### file

Распространённые форматы (`TXT`, `DOC`, `PDF` и др.), размер:

```text
до 4 ГБ
```

Для текущих LanMon `LOG`, `LOGXLS`, `ALARM` это с большим запасом.

## 9. Вложение может быть ещё не готово

После upload MAX может продолжать обработку файла. Если сразу отправить сообщение, возможна ошибка:

```text
attachment.not.ready
```

Для production нужно:

- сделать небольшую паузу после загрузки больших файлов;
- при `attachment.not.ready` повторять отправку;
- использовать увеличивающийся интервал между попытками.

Текущий mirror transport реализует upload/send flow, но полноценный retry/backoff для `attachment.not.ready` следует считать production-hardening задачей.

## 10. Сетевые требования для Windows/LanMon

Минимальный outbound allowlist для текущего polling/test варианта:

```text
platform-api2.max.ru:443
fu.oneme.ru:443
*.oneme.ru:443  # если политика позволяет и MAX меняет upload-host
```

Для изображений должен быть доступен фактически возвращённый MAX upload URL (`iu.oneme.ru` на момент документации). Правильнее разрешать возвращаемые MAX upload-hosts по утверждённой политике, а не хардкодить единственный hostname в приложении.

Для production Webhook дополнительно нужен публичный inbound HTTPS endpoint/relay.

## 11. Официальная документация

- API: https://dev.max.ru/docs-api
- История изменений: https://dev.max.ru/docs-api/changelog-api
- Long Polling: https://dev.max.ru/docs-api/methods/GET/updates
- Webhook: https://dev.max.ru/docs-api/methods/POST/subscriptions
- Upload: https://dev.max.ru/docs-api/methods/POST/uploads

Перед production-внедрением требования нужно сверить ещё раз: MAX API активно меняется.

## Кодировка

Исходящий LanMon `AnsiString`/CP1251 преобразуется в UTF-8 перед MAX API. Входящий UTF-8 преобразуется обратно в CP1251 в `maxmsg.cpp` через WinAPI.

## Тесты

```bash
bash ./tests/run.sh
bash ./e2e/run_e2e.sh
```

`tests/test_mirror.py` отдельно запрещает возвращение файлов альтернативной архитектуры (`lanmon_bot`, `lanmon_commands`, `maxsettings`, `maxusers`) и проверяет зеркальные классы/комментарии.
