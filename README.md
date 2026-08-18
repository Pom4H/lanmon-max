# LanMon MAX adapter

Portable C++98 MAX Bot API adapter for the legacy LanMon C++Builder application.

The portable layer is executable in Linux CI; `maxindy.*` is the thin production transport for C++Builder/Indy. The code intentionally preserves the behavior of the existing `TELEGRAM_BOT`, including alias masks, counters, alarm fan-out, command authorization and the historical `FlagSendMaps` semantics.

## Telegram feature parity

| Telegram capability | MAX implementation |
|---|---|
| `GetMe` | `MAX_API_CLIENT::GetMe` / `LANMON_MAX_BOT::GetMe` |
| polling / update cursor | `Poll`, persistent `marker` |
| text | `SendMessage` |
| photo | `SendPhoto` → MAX `image` upload |
| document | `SendDoc` → MAX `file` upload |
| `SCREEN`, `ЭКРАН` | monitor screenshot + desktop fallback |
| `MAP`, `КАРТА` | map image |
| `STOP`, `СТОП` | close alarm window + acknowledgement |
| `LOG`, `ЖУРНАЛ` | HTML export + file send |
| `LOGXLS` | XLS export + file send |
| `ALARM`, `ТРЕВОГИ` | PDF export + file send |
| `HELP`, `?` | full Telegram-equivalent help |
| user list | `MAX_USER_LIST` |
| alias masks (`!`, `*`, empty) | same rules as `TgUser::HasValidAlias` |
| `RequestAlias` authorization | preserved |
| `AlarmAlias` fan-out | `OnNewAlarmState` |
| numeric alias fallback | preserved |
| `InCount`, `OutCount`, `Tag` | preserved |
| INI settings/users | `MaxLoadIni` / `MaxSaveIni` |
| script message callback | `ILanMonMaxEvents::OnMaxMessage` |
| Telegram FastScript user helpers | equivalent methods on `LANMON_MAX_BOT` |
| debug/error/getMe/read callbacks | `ILanMonMaxEvents` |
| active / periodic-pause settings | `MAX_BOT_SETTINGS` |

### Compatibility detail: `FlagSendMaps`

The old Telegram code checks `FlagSendMaps` before **all** built-in commands, not only `MAP`. MAX intentionally preserves that behavior. `OnMaxMessage` is fired before the gate, exactly like `OnTgMessage` in Telegram.

## Layout

- `maxcore.*` — C++98 JSON parser, MAX DTOs, URL/body builders, CP1251 → UTF-8.
- `maxclient.*` — transport-independent MAX API client.
- `maxindy.*` — C++Builder/Indy HTTPS transport.
- `maxusers.*` — users, aliases, counters and tags.
- `maxsettings.*` — INI persistence.
- `lanmon_commands.*` — all built-in Telegram-equivalent LanMon commands.
- `lanmon_bot.*` — Telegram-compatible behavior facade.
- `tests/` — protocol/client/parity tests.
- `e2e/` — real local TCP/HTTP MAX mock.
- `e2e_lanmon/` — full MAX → LanMon → MAX parity E2E.

## Tests

```bash
./tests/run.sh
./e2e/run_e2e.sh
./e2e_lanmon/run.sh
```

CI compiles portable production code with:

```text
-std=gnu++98 -Wall -Wextra -Werror
```

The full parity E2E executes:

```text
MAX /updates
  → STOP
  → MAP 2
  → SCREEN 1
  → SCREEN (desktop fallback)
  → LOG
  → LOGXLS
  → ALARM
  → HELP
  → alarm alias broadcast
  → MAX /messages + image/file uploads
```

See `PARITY.md` for the source-of-truth checklist.

---

# Integration into LanMon

## 1. Add files to the C++Builder project

Copy these into a `Max/` directory and add the `.cpp` files to `lanmon4.cbproj`:

```text
maxcore.cpp/.h
maxclient.cpp/.h
maxindy.cpp/.h
maxusers.cpp/.h
maxsettings.cpp/.h
lanmon_commands.cpp/.h
lanmon_bot.cpp/.h
```

Only `maxindy.*` depends on Indy/VCL. Keep screenshots, log export and alarm PDF generation in LanMon itself through `ILanMonCommandActions`.

## 2. Implement the LanMon action bridge

Create a class derived from `ILanMonCommandActions` and connect it to existing functions:

```cpp
class TLanMonMaxActions : public ILanMonCommandActions
{
public:
    bool CloseAlarmWindow(std::string &error)
    {
        CloseAvariaForm();
        return true;
    }

    bool CreateMapImage(int index, std::string &filename, std::string &error)
    {
        AnsiString bmp=(AnsiString)szBitmapDir+"_max_map.bmp";
        if(!CreateMapScreenshot(index,bmp)) return false;
        AnsiString png=ChangeFileExt(bmp,".png");
        if(!Bmp2Png(bmp,png)) return false;
        filename=png.c_str();
        return true;
    }

    bool CreateMonitorImage(int index, std::string &filename, std::string &error)
    {
        AnsiString fn=(AnsiString)szBitmapDir+"_max_screen.jpg";
        if(!GetMonitorScreenshot(index,fn)) return false;
        filename=fn.c_str();
        return true;
    }

    bool CreateDesktopImage(std::string &filename, std::string &error)
    {
        AnsiString fn=(AnsiString)szBitmapDir+"_max_screen.jpg";
        if(!DesktopScreenshot(fn)) return false;
        filename=fn.c_str();
        return true;
    }

    bool ExportLogHtml(std::string &filename, std::string &error)
    {
        if(!LogView) return true;
        AnsiString fn=(AnsiString)szWorkDir+"_max_log.html";
        DeleteFile(fn);
        LogView->ExportToHtml(fn);
        filename=fn.c_str();
        return true;
    }

    bool ExportLogXls(std::string &filename, std::string &error)
    {
        if(!LogView) return true;
        AnsiString fn=(AnsiString)szWorkDir+"_max_log.xls";
        DeleteFile(fn);
        LogView->ExportToXls(fn);
        filename=fn.c_str();
        return true;
    }

    bool CreateAlarmsPdf(std::string &filename, std::string &error)
    {
        AnsiString fn=::CreateAlarmsPdf();
        filename=fn.c_str();
        return true;
    }

    std::string CurrentDateTimeText() const
    {
        return Now().DateTimeString().c_str();
    }
};
```

`SCREEN` behavior matches Telegram: first try `GetMonitorScreenshot(screenindex-1)`, then fall back to `DesktopScreenshot`.

## 3. Create global objects

Near the existing `TgBot` setup:

```cpp
TMaxIndyTransport *MaxTransport=NULL;
MAX_API_CLIENT *MaxApi=NULL;
TLanMonMaxActions *MaxActions=NULL;
LANMON_MAX_BOT *MaxBot=NULL;
```

Initialize:

```cpp
MaxTransport=new TMaxIndyTransport();
MaxApi=new MAX_API_CLIENT(MaxTransport,"");
MaxActions=new TLanMonMaxActions();
MaxBot=new LANMON_MAX_BOT(MaxApi,MaxActions,&MaxEvents);

std::string error;
MaxBot->Load(((AnsiString)szWorkDir+"max.ini").c_str(),error);
```

Delete in reverse order on shutdown. `Load()` applies `BotToken` to `MAX_API_CLIENT` automatically.

## 4. Configure Indy TLS trust

`TMaxIndyTransport::SSL()` exposes the production `TIdSSLIOHandlerSocketOpenSSL*`:

```cpp
TIdSSLIOHandlerSocketOpenSSL *ssl=MaxTransport->SSL();
ssl->SSLOptions->Method=sslvTLSv1_2;
// Configure the CA/certificate required by MAX here.
```

Do not permanently solve certificate problems by disabling verification.

## 5. Poll in a worker thread

Do not run Long Polling on the VCL UI thread. The existing Telegram implementation already uses `TTgBotThread`; MAX should use the same pattern.

Worker call:

```cpp
if(MaxBot->Settings.Active && !MaxBot->Settings.PeriodicReadMessagesPaused)
{
    std::string error;
    MaxBot->ReadMessages(true,error,30,100);
}
```

Manual read:

```cpp
MaxBot->ReadMessages(false,error,30,100);
```

`MAX_API_CLIENT` stores and sends `marker` automatically.

## 6. FastScript message callback

Implement an event bridge:

```cpp
class TLanMonMaxEvents : public ILanMonMaxEvents
{
public:
    void OnMaxMessage(max_int64 updateTimestamp,
                      max_int64 id,
                      const std::string &text)
    {
        ::OnMaxMessage(updateTimestamp,
                       IntToStr((__int64)id),
                       AnsiString(text.c_str()));
    }
};
```

Add a FastScript event analogous to Telegram:

```text
OnMaxMessage(UpdateTimestamp, Id, Text)
```

For backwards-compatible customer projects the bridge may deliberately call the existing `OnTgMessage` instead, making scripts messenger-neutral without changing the MAX core.

## 7. FastScript functions

Map the Telegram functions one-to-one:

```text
TgSendMessage        → MaxSendMessage
TgSendPhoto          → MaxSendPhoto
TgSendDoc            → MaxSendDoc
TgUserCount          → MaxUserCount
TgGetUser            → MaxGetUser
TgFindUser           → MaxFindUser
TgFindUserIndex      → MaxFindUserIndex
TgFindUserAlias      → MaxFindUserAlias
TgSetUserTag         → MaxSetUserTag
TgUserCanAsk         → MaxUserCanAsk
TgUserRcvAlarms      → MaxUserRcvAlarms
TgUserHasValidAlias  → MaxUserHasValidAlias
```

Thin wrappers call:

```cpp
MaxBot->SendMessageByAlias(alias,text,error);
MaxBot->SendPhotoByAlias(alias,filename,caption,error);
MaxBot->SendDocByAlias(alias,filename,caption,error);

MaxBot->UserCount();
MaxBot->GetUser(index);
MaxBot->FindUser(id);
MaxBot->FindUserIndex(id);
MaxBot->FindUserAlias(alias);
MaxBot->SetUserTag(index,tag);
MaxBot->UserCanAsk(index);
MaxBot->UserRcvAlarms(index);
MaxBot->UserHasValidAlias(index,alias);
```

If you want old scripts to work unchanged, register the old `Tg*` names to a messenger-neutral dispatcher instead of duplicating script logic.

## 8. Alarm hooks

Keep the existing decision points from `src/alarms/Avaria.cpp` and `HistoryAlarm.cpp`, but call MAX too:

```cpp
if(MaxBot && MaxBot->Settings.FlagSendAlarms)
    MaxBot->OnNewAlarmState(MaxUtf8FromCp1251(mess.c_str()),error);

if(MaxBot && MaxBot->Settings.FlagOperatorAlarm)
    MaxBot->OnNewAlarmState(MaxUtf8FromCp1251(mess.c_str()),error);

if(MaxBot && MaxBot->Settings.FlagSendAlarmsEnd)
    MaxBot->OnNewAlarmState(MaxUtf8FromCp1251(mess.c_str()),error);
```

`OnNewAlarmState` sends only to users matching `AlarmAlias`.

## 9. User/config UI

Reuse the Telegram UI model: user ID, name, alias, comment, IsBot, InCount, OutCount and Tag. Store MAX separately, e.g. `max.ini`:

```ini
[SETUP]
Active=1
BotToken=...
PeriodReadMessages=0
PeriodicReadMessagesPaused=0
SendAlarms=1
SendAlarmEnd=1
OperatorAlarm=1
AlarmAlias=A!!!
RequestAlias=R!!!
SendMaps=1

[User0]
Name=Operator
id=123456789
Alias=R001
Comment=
InCount=0
OutCount=0
Tag=0
```

Alias rules are intentionally identical to Telegram:

- empty mask → every valid user;
- `*` → every valid user;
- `!` → wildcard for exactly one character;
- all other characters must match at the same position.

## 10. Encoding

Legacy LanMon text is often CP1251 while MAX JSON is UTF-8:

```cpp
std::string utf8=MaxUtf8FromCp1251(ansi.c_str());
```

Text parsed from MAX is UTF-8. Convert it in the LanMon bridge if an old VCL/FastScript API expects ANSI.

---

# MAX platform requirements

This adapter follows the MAX API contract current in August 2026.

## Required

1. Create a MAX chatbot and obtain its access token.
2. Use `https://platform-api2.max.ru` for API requests.
3. Send the token only as:

   ```text
   Authorization: <access_token>
   ```

   Token-in-query authentication is no longer supported.
4. Add the Ministry of Digital Development (Минцифры) certificate/CA required by MAX to the trust store used by LanMon's Indy/OpenSSL client.
5. Permit outgoing HTTPS to `platform-api2.max.ru` and to the upload URLs returned by `POST /uploads`; image and file uploads may use different hosts and the returned URL must be used unchanged.
6. Keep API traffic within MAX's documented 30 requests/second limit.

Official docs:

- `https://dev.max.ru/docs-api`
- `https://dev.max.ru/docs-api/methods/GET/updates`
- `https://dev.max.ru/docs-api/methods/POST/uploads`
- `https://dev.max.ru/docs-api/methods/POST/messages`

## Long Polling

LanMon uses:

```http
GET /updates?timeout=...&limit=...&marker=...
Authorization: <token>
```

This matches the existing architecture because LanMon creates only outgoing HTTPS connections and does not need a public inbound endpoint.

MAX currently documents Long Polling as a development/testing mechanism and recommends Webhook for production. Long Polling does not work while an active Webhook subscription exists. If a future deployment must switch to Webhook, keep `LANMON_MAX_BOT`, users, aliases, commands and send/upload code unchanged and replace only update delivery.

## Uploads used by LanMon

`SendPhoto` uses `type=image`:

- JPG/JPEG/PNG/GIF/TIFF/BMP/HEIC;
- max 50 MB;
- max 7680×7680 px.

`SendDoc` uses `type=file`:

- common document/file formats;
- max 4 GB according to current MAX API docs.

Flow:

```text
POST /uploads?type=image|file
  → multipart POST file to returned URL
  → receive token
  → POST /messages with attachments.payload.token
```

Do not commit the real bot token to Git or log it. Store it in deployment configuration / `max.ini` with appropriate filesystem permissions and rotate it if exposed.

---

# What Linux CI cannot prove

`maxindy.cpp` requires the old C++Builder/VCL/Indy headers, so Linux CI does not compile that one transport file. Everything above the socket boundary is compiled and executed, including real local TCP/HTTP, marker continuity, aliases, authorization, all Telegram-equivalent commands, image/file uploads and alarm fan-out.

Final Windows acceptance test:

```text
C++Builder build
  → configure MAX CA certificate
  → GET /me with real token
  → receive /updates
  → execute a built-in command
  → receive text/image/file response
```
