# Матрица тестирования LanMon MAX

Цель — проверять не количество `CHECK`, а четыре разные границы: протокол MAX, state machine клиента, Telegram-совместимое поведение LanMon и настоящий HTTP/TLS transport.

## A. Protocol core — автоматизировано

`Max/tests/test_maxcore.cpp`, C++98 без VCL и сети.

Проверяется:

- `GET /updates` с/без 64-bit marker;
- границы `timeout` и `limit`;
- `user_id` / signed `chat_id`;
- int64 conversion;
- CP1251 -> UTF-8, включая `Ё/ё`;
- JSON escaping кавычек, slash, newline, tab и control chars;
- пустое сообщение;
- attachment JSON и escaping token/caption;
- `/me` happy path и обязательный `user_id`;
- `message_created` и игнорирование других update types;
- `marker=null` и отсутствие marker;
- malformed/top-level-invalid JSON;
- строгая JSON number grammar: запрещены `1.`, `1e`, `1e+`, `01`, `-`;
- `\uXXXX`;
- UTF-16 surrogate pair `\uD83D\uDE00` -> `😀`;
- lone/mismatched surrogate rejection;
- raw UTF-8/emoji;
- upload URL/token parsing и missing-field errors.

## B. `MAX_API_CLIENT` state machine — автоматизировано

`Max/tests/test_maxclient.cpp`, C++98 + in-memory HTTP transport.

Проверяется:

- `Transport == NULL`;
- нормализация `baseUrl`;
- token только в `Authorization`, не в URL;
- stale error очищается после успешного запроса;
- `LastStatusCode` / `LastResponseBody`;
- marker сохраняется после успешного Poll;
- malformed JSON / 503 не двигают подтверждённый marker;
- следующий Poll использует старый marker;
- `ResetMarker()`;
- send text в user/chat;
- signed chat ID;
- HTTP 401 / 429;
- transport-level failure;
- image upload pipeline;
- file upload pipeline;
- `type=image`, не `photo`;
- multipart field `data`;
- returned upload URL используется без переписывания;
- Bot token не уходит на upload-host;
- failure на каждом этапе upload pipeline;
- `attachment.not.ready` -> повторяется только финальный `POST /messages`;
- retry использует тот же attachment token/body;
- multipart выполняется ровно один раз;
- backoff `500 -> 1000 -> 2000 ms`;
- максимум 4 final-send attempts;
- обычный 429 не ошибочно трактуется как `attachment.not.ready`.

## C. Telegram/LanMon behavioral contract — автоматизировано

`Max/tests/test_contract.py` анализирует VCL/C++Builder source, который Ubuntu не может скомпилировать.

Защищается именно поведение:

- empty chat id отбрасывается первым;
- `UserMessageCount++` до script callback;
- `OnTgMessage` вызывается до built-in logic;
- исторический `FlagSendMaps` блокирует все built-in команды;
- user lookup -> `RequestAlias` -> command;
- `SCREEN/ЭКРАН`, `MAP/КАРТА`, `STOP/СТОП`, `LOG`, `LOGXLS`, `ALARM/ТРЕВОГИ`, `HELP/?`;
- SCREEN monitor -> desktop fallback;
- MAP 1-based -> 0-based -> BMP -> PNG -> send;
- STOP action -> confirmation;
- LOG/LOGXLS export -> send;
- ALARM PDF -> send;
- alias `*`, empty mask, `!` wildcard;
- direct numeric alias fallback;
- alarm fan-out по `AlarmAlias`;
- `PeerType=user|chat` persistence;
- legacy `OutCount` semantics;
- thread-safe FIFO task queue;
- send tasks сохраняют `PeerType`;
- INI compatibility и fallback `BotToken`;
- thread lifecycle остаётся Telegram-like;
- MainForm, а не `MAX_BOT`, владеет callbacks;
- TLS 1.2 + `RootCertFile` + `sslvrfPeer`;
- отсутствие CA -> fail-closed до network call;
- multipart upload не получает Bot API headers;
- attachment retry loop не может повторно вызвать multipart upload.

## D. CA integrity — автоматизировано

`Max/tests/test_cert.sh` проверяет vendored `Max/certs/max-ca.pem`.

Проверяется:

- bundle существует;
- ровно 2 PEM certificates;
- Russian Trusted Sub CA fingerprint:
  `BBBDE2103E790B999EC62BD03CF625A5A2E7C316E10AFE6A490EEDEAD8B3FD9B`;
- Russian Trusted Root CA fingerprint:
  `D26D2D0231B7C39F92CC738512BA54103519E4405D68B5BD703E9788CA8ECF31`;
- ожидаемые subjects;
- сертификаты не истекли и живут больше 24 часов;
- Sub CA криптографически проверяется Root CA;
- combined bundle пригоден как OpenSSL `CAfile`.

## E. Local TCP/HTTP E2E — автоматизировано

`Max/e2e/run_e2e.sh` поднимает Python HTTP server и настоящий C++98 client через POSIX sockets.

Проверяется:

- `GET /me` по настоящему TCP;
- два Long Poll запроса;
- marker continuity между разными HTTP calls;
- UTF-8, кавычки и newline;
- `POST /messages`;
- `POST /uploads?type=image`;
- реальный multipart image file с диска;
- image token -> attachment message;
- `POST /uploads?type=file`;
- реальный multipart PDF/file;
- file token -> attachment message;
- signed `chat_id` и `user_id`;
- отсутствие `Authorization` на upload-host;
- настоящий `attachment.not.ready` response по TCP;
- retry final message с тем же token;
- mock server падает, если клиент пытается повторно upload-ить файл;
- HTTP 401 по настоящему socket transport.

## F. Live TLS к MAX — GitHub Actions

CI выполняет без Bot Token:

```bash
openssl s_client \
  -connect platform-api2.max.ru:443 \
  -servername platform-api2.max.ru \
  -verify_hostname platform-api2.max.ru \
  -verify_return_error \
  -tls1_2 \
  -CAfile Max/certs/max-ca.pem
```

Это доказывает одновременно:

- runner реально устанавливает TLS 1.2 соединение с текущим MAX host;
- hostname соответствует сертификату;
- цепочка сервера доверяется именно vendored `max-ca.pem`;
- bundle не только синтаксически валиден offline.

## Что ещё нельзя честно закрыть Linux CI

### 1. Реальный C++Builder build

Нужна сборка тем же C++Builder/Indy/OpenSSL, которым собирается LanMon. Она должна поймать:

- несовместимость Borland headers/ABI;
- конкретные Indy enum/property differences;
- DFM/form errors;
- несовместимые OpenSSL DLL.

### 2. TLS внутри старого Indy на целевой Windows

Live OpenSSL CI проверяет сертификат и сервер, но не доказывает поведение конкретного `TIdSSLIOHandlerSocketOpenSSL` заказчика.

Acceptance:

```text
correct max-ca.pem -> GetMe проходит
missing max-ca.pem -> fail-closed до сети
corrupted max-ca.pem -> TLS не проходит
wrong hostname/MITM -> TLS не проходит
```

### 3. Живой MAX Bot Token

Нужен controlled smoke test:

- `GetMe`;
- личный user;
- group chat;
- русские команды;
- image upload;
- PDF/file upload;
- revoked/invalid token.

### 4. Реальные LanMon actions

Только настоящий `lanmon4.exe` может выполнить:

- `GetMonitorScreenshot` / `DesktopScreenshot`;
- `CreateMapScreenshot` / `Bmp2Png`;
- `LogView->ExportToHtml/Xls`;
- `CreateAlarmsPdf`;
- `CloseAvariaForm`.

### 5. Rate limiting / burst

Формального 30 rps limiter сейчас нет. До production нужны:

- burst из 100 alarm messages;
- сохранение порядка;
- backpressure;
- controlled retry при 429;
- отсутствие retry storm.

### 6. Fault injection transport

Ещё полезны:

- disconnect до HTTP headers;
- disconnect посреди response body;
- Long Poll timeout;
- DNS failure;
- disconnect посреди multipart;
- 500/503 с восстановлением.

### 7. Restart semantics Long Poll marker

Marker сейчас живёт в памяти клиента. Для dev/test нужно отдельно принять поведение после рестарта. Для production это должно быть снято переходом входящих событий на Webhook/relay.

### 8. Production Webhook/relay

Когда появится production inbound transport, его тесты должны покрыть:

- duplicate delivery/idempotency;
- out-of-order events;
- LanMon offline;
- relay retry;
- replay protection;
- relay restart;
- delivery acknowledgement.

## Команды CI

Локальные автоматические проверки:

```bash
bash Max/tests/run.sh
bash Max/e2e/run_e2e.sh
```

В GitHub Actions к ним добавлен live TLS step.

## Минимальный acceptance перед заказчиком

```text
[ ] GitHub CI — green
[ ] C++Builder build — green
[ ] certs\max-ca.pem попал в installer/package
[ ] GetMe через реальный MAX — green
[ ] missing/wrong CA -> connection fails
[ ] личный чат — green
[ ] групповой чат — green
[ ] SCREEN — green
[ ] MAP — green
[ ] STOP — green
[ ] LOG / LOGXLS — green
[ ] ALARM — green
[ ] HELP — green
[ ] AlarmAlias fan-out — green
[ ] image upload — green
[ ] file/PDF upload — green
[ ] attachment.not.ready retry — green
```
