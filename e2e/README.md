# MAX E2E mock harness

This harness tests the real `MAX_API_CLIENT` over a real local TCP/HTTP connection.
It does not mock `IMaxHttpTransport` in-process.

## What it verifies

1. `GET /me` with `Authorization` header.
2. First `GET /updates` without `marker`.
3. Parsing `message_created`, including UTF-8, quotes and newline.
4. Persisting returned `marker=101`.
5. `POST /messages?chat_id=777` with valid JSON body.
6. Second `GET /updates` contains `marker=101`.
7. New marker `102` is stored.
8. HTTP 401 is surfaced as an API error.

## Run

```bash
./run_e2e.sh
```

Optional port:

```bash
MAX_E2E_PORT=18081 ./run_e2e.sh
```

The script compiles with `g++ -std=gnu++98 -Wall -Wextra -Werror`, starts the Python mock server, runs the C++ harness and shuts the server down.

## Production compatibility change

`MAX_API_CLIENT` accepts an optional third constructor argument `baseUrl`:

```cpp
MAX_API_CLIENT api(transport, token); // production URL remains default
MAX_API_CLIENT testApi(transport, token, "http://127.0.0.1:18080");
```

No production call sites need to change.
