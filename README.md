# LanMon MAX adapter

Portable C++98 MAX Bot API adapter and executable harnesses extracted for integration into LanMon.

## Layout

- `maxcore.*` — C++98 JSON parser, MAX DTOs, URL/body builders, CP1251 -> UTF-8 conversion.
- `maxclient.*` — transport-independent MAX API client.
- `maxindy.*` — thin C++Builder/Indy HTTPS transport using `TIdHTTP` + `TIdSSLIOHandlerSocketOpenSSL` and TLS 1.2.
- `lanmon_commands.*` — portable LanMon command router used by the application-level E2E harness.
- `tests/` — unit/client tests.
- `e2e/` — real TCP/HTTP mock MAX API E2E.
- `e2e_lanmon/` — MAX -> LanMon command -> MAX E2E, including image upload.

## Implemented

- `Authorization: <token>`
- `GET /me`
- long polling `GET /updates` with persistent in-memory `marker`
- parsing `message_created`
- `POST /messages` via `user_id` and `chat_id`
- safe JSON escaping
- LanMon CP1251 text -> UTF-8
- HTTP/network error propagation
- `POST /uploads?type=image`
- multipart image upload
- image attachment message using returned upload token
- portable command routing for `STOP`, `MAP n` and `HELP`

## C++Builder integration

`TMaxIndyTransport::SSL()` exposes the Indy SSL handler so CA/certificate configuration can be applied without changing the tested protocol core.

Production construction keeps the official API base URL:

```cpp
TMaxIndyTransport *transport = new TMaxIndyTransport();
MAX_API_CLIENT *max = new MAX_API_CLIENT(transport, "BOT_TOKEN");
```

The harnesses can inject a local base URL instead, so the same `MAX_API_CLIENT` runs against a real TCP mock server.

## Run locally

```bash
./tests/run.sh
./e2e/run_e2e.sh
./e2e_lanmon/run.sh
```

All portable code is compiled with:

```text
-std=gnu++98 -Wall -Wextra -Werror
```

## CI

GitHub Actions runs all three suites on `ubuntu-latest` for pushes to `main` and pull requests.

The Indy/VCL transport itself is intentionally not compiled in Linux CI because it requires the C++Builder/Indy toolchain; everything above that boundary is executable and covered by the harnesses.
