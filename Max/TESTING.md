# Матрица тестирования LanMon MAX

Цель тестов — доказать не только корректность отдельных JSON-функций, но и сохранение существующего Telegram-контракта LanMon при замене транспорта на MAX.

## Уровень A — protocol core, автоматизировано

`tests/test_maxcore.cpp`, C++98 без VCL и сети.

Проверяется:

- `GET /updates` без marker;
- `GET /updates` с 64-битным marker;
- границы `timeout`: 0 и 90;
- границы `limit`: 1 и 1000;
- адресация `user_id`;
- адресация `chat_id`;
- отрицательный `chat_id`;
- преобразование int64 в строку;
- CP1251 → UTF-8, включая `Ё/ё`;
- JSON escaping кавычек, `\\`, `\n`, `\t`, control chars;
- пустое текстовое сообщение;
- JSON attachment с caption/token;
- корректный `GET /me`;
- отсутствие обязательного `user_id` в `/me`;
- неправильный top-level JSON;
- `message_created`;
- игнорирование посторонних update types;
- `marker=null`;
- ответ без marker;
- `message_created` без объекта message;
- UTF-8 кириллица;
- raw UTF-8 emoji;
- `\\uXXXX` escape;
- malformed JSON;
- отсутствие массива `updates`;
- точное сохранение upload URL с query string;
- отсутствие `url` в upload response;
- получение upload token;
- отсутствие upload token.

## Уровень B — MAX_API_CLIENT state machine, автоматизировано

`tests/test_maxclient.cpp`, C++98 + in-memory HTTP transport.

Проверяется:

- `Transport == NULL`;
- удаление завершающего `/` у `baseUrl`;
- токен находится в `Authorization`, а не URL;
- `GetMe` success;
- `GetMe` с валидным HTTP и невалидным JSON;
- очистка stale error после следующего успешного запроса;
- сохранение `LastStatusCode` и `LastResponseBody`;
- первый Long Poll сохраняет marker;
- malformed JSON не сдвигает уже подтверждённый marker;
- HTTP 503 не сдвигает marker;
- следующий Poll передаёт предыдущий marker;
- `ResetMarker()`;
- send text пользователю;
- send text в signed chat id;
- HTTP 401;
- transport-level error без HTTP status;
- полный image upload flow;
- полный file upload flow;
- `type=image`, не устаревший `photo`;
- upload field называется `data`;
- upload URL используется без переписывания;
- bot token не передаётся на multipart upload-host;
- attachment type/token/caption;
- ошибка `POST /uploads` останавливает pipeline;
- ошибка upload-host останавливает pipeline;
- отсутствие upload token останавливает pipeline;
- ошибка финального `POST /messages` возвращается вызывающему коду;
- HTTP 429 сохраняется в диагностике.

## Уровень C — Telegram/LanMon behavioral contract, автоматизировано

`tests/test_contract.py` проверяет исходный VCL-код, который Ubuntu не может скомпилировать.

Это не обычный grep наличия классов. Проверяется порядок критических операций:

- пустой chat id отбрасывается до обработки;
- `UserMessageCount` увеличивается до script callback;
- существующий `OnTgMessage` вызывается до built-in команд;
- `FlagSendMaps` по историческому контракту блокирует все built-in команды;
- поиск пользователя выполняется до `RequestAlias` authorization;
- команды приводятся через `UpperCase().Trim()`;
- сохранены `SCREEN/ЭКРАН`, `MAP/КАРТА`, `STOP/СТОП`, `LOG`, `LOGXLS`, `ALARM/ТРЕВОГИ`, `HELP/?`;
- SCREEN: конкретный monitor → fallback на desktop;
- MAP: 1-based номер → 0-based индекс → BMP → PNG → отправка;
- STOP: закрытие аварийной формы → подтверждение;
- LOG/LOGXLS: export → отправка документа;
- ALARM: PDF должен существовать перед отправкой;
- HELP содержит `??`;
- alias `*`;
- пустая alias mask;
- `!` как wildcard одного символа;
- numeric alias fallback;
- alarm fan-out через `AlarmAlias`;
- `PeerType` сохраняется для user/chat;
- неизвестный direct id по умолчанию трактуется как `user_id`;
- `OutCount` сохраняет legacy-семантику Telegram;
- task queue остаётся thread-safe FIFO;
- все send tasks несут `PeerType`;
- INI keys и fallback `BotToken`;
- `MAX_BOT` не перехватывает callbacks у `MainForm`;
- thread создаётся suspended и затем resume;
- Indy использует TLS 1.2, `RootCertFile`, peer verification и `certs\\max-ca.pem`;
- multipart upload step не получает Bot API headers.

## Уровень D — настоящий TCP/HTTP E2E, автоматизировано

`e2e/run_e2e.sh` поднимает Python MAX server и запускает C++98-клиент через обычные POSIX sockets.

Проверяется:

- настоящий HTTP `GET /me`;
- два последовательных Long Poll запроса;
- marker continuity между разными TCP-запросами;
- русский UTF-8 + кавычки + newline;
- настоящий `POST /messages`;
- image `POST /uploads`;
- настоящий multipart upload реального файла с диска;
- image token → attachment message;
- file `POST /uploads`;
- file token → attachment message;
- signed `chat_id` и `user_id`;
- отсутствие `Authorization` на upload-host;
- HTTP 401 по настоящему сокету.

## Что пока НЕ доказано автоматическими тестами

Эти пункты нельзя честно закрыть Linux mock-тестом.

### 1. Сборка реальным C++Builder

Нужен acceptance build тем же C++Builder/Indy/OpenSSL, которым собирается LanMon. Он должен поймать несовместимость заголовков, версий Indy, DFM и Borland ABI.

### 2. TLS на целевой Windows

Нужно проверить на машине заказчика:

- реальные OpenSSL DLL;
- `certs\\max-ca.pem`;
- цепочку сертификатов `platform-api2.max.ru`;
- ошибку при удалённом/неверном CA;
- успешный `GetMe` только при включённой peer verification.

### 3. Живой MAX bot

С тестовым токеном выполнить:

- `GetMe`;
- личное сообщение;
- групповой чат;
- отрицательный/реальный chat id;
- русские команды;
- image upload;
- PDF/file upload;
- ошибка revoked/invalid token.

### 4. Реальные LanMon actions

Linux contract-test защищает порядок вызовов, но не может выполнить VCL-функции:

- `GetMonitorScreenshot` / `DesktopScreenshot`;
- `CreateMapScreenshot` / `Bmp2Png`;
- `LogView->ExportToHtml/Xls`;
- `CreateAlarmsPdf`;
- `CloseAvariaForm`.

Перед передачей заказчику нужен smoke-test этих команд внутри настоящего `lanmon4.exe`.

### 5. `attachment.not.ready`

MAX прямо допускает эту ошибку сразу после загрузки большого файла и рекомендует повтор с увеличением интервала. Сейчас клиент корректно вернёт ошибку вызывающему коду, но автоматический retry/backoff ещё не реализован.

Нужный тест после реализации:

```text
upload success
→ POST /messages = attachment.not.ready
→ wait
→ retry same attachment token
→ success
```

Важно: повтор не должен заново загружать файл и получать новый token.

### 6. Rate limit / backpressure

MAX рекомендует не превышать 30 запросов/с. Task queue последовательная, но формального rate limiter сейчас нет.

Нужные тесты:

- burst из 100 аварий;
- не более 30 API requests в скользящем окне;
- HTTP 429 → controlled retry;
- сохранение порядка сообщений после retry.

### 7. Потеря соединения

Нужны fault-injection E2E сценарии:

- disconnect до HTTP headers;
- disconnect в середине JSON body;
- timeout Long Poll;
- DNS failure;
- upload disconnect в середине multipart;
- 500/503 с последующим восстановлением.

### 8. Restart semantics marker

Сейчас marker живёт в памяти `MAX_API_CLIENT`. После рестарта Long Poll начинается без marker, а MAX без marker возвращает только последнее обновление.

Для development/test это нужно проверить отдельно. Для production проблема должна исчезнуть вместе с переходом входящих событий на Webhook/relay.

### 9. Production Webhook

Текущий зеркальный вариант использует Long Polling для сходства с Telegram. Production delivery через Webhook/relay пока не реализована, поэтому её тестовый набор должен появиться вместе с transport:

- signature/security policy выбранного relay;
- duplicate delivery/idempotency;
- out-of-order events;
- temporary LanMon offline;
- retry from relay;
- replay protection;
- delivery after relay restart.

## Минимальный acceptance перед отдачей заказчику

Обязательный набор:

```text
[ ] Max/tests/run.sh — green
[ ] Max/e2e/run_e2e.sh — green
[ ] C++Builder build — green
[ ] GetMe через настоящий MAX — green
[ ] личный чат — green
[ ] групповой чат — green
[ ] SCREEN — green
[ ] MAP — green
[ ] STOP — green
[ ] LOG / LOGXLS — green
[ ] ALARM — green
[ ] HELP — green
[ ] авария по AlarmAlias — green
[ ] image upload — green
[ ] file/PDF upload — green
[ ] TLS ломается без корректного CA и работает с ним
```
