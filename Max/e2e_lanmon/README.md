# LanMon → MAX command E2E

Runs a real local TCP/HTTP mock of MAX and checks the portable LanMon command router.

Covered flow:

1. Long poll receives `STOP` → `CloseAlarmWindow()` action → text reply to MAX.
2. Next poll carries the previous `marker` and receives `MAP 2`.
3. LanMon action creates a local PNG → `POST /uploads?type=image` → multipart file upload → `POST /messages` with image token.
4. Next poll receives `HELP` → text reply.
5. Final poll proves marker continuity and no duplicate command delivery.

Run:

```bash
./run.sh
```

The C++ side is compiled with `-std=gnu++98 -Wall -Wextra -Werror`.
