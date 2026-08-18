# Адаптер MAX для LanMon

Портируемый адаптер MAX Bot API на C++98 для старого приложения LanMon на C++Builder.

Портируемый слой полностью запускается в Linux CI. Файлы `maxindy.*` — тонкий production-транспорт для C++Builder/Indy. Код намеренно сохраняет поведение существующего `TELEGRAM_BOT`: маски алиасов, счётчики, рассылку аварий, авторизацию команд и историческую семантику `FlagSendMaps`.

## Функциональный паритет с Telegram

| Возможность Telegram | Реализация в MAX |
|---|---|
| `GetMe` | `MAX_API_CLIENT::GetMe` / `LANMON_MAX_BOT::GetMe` |
| polling / курсор обновлений | `Poll`, сохраняемый `marker` |
| текст | `SendMessage` |
| фото | `SendPhoto` → загрузка MAX `image` |
| документ | `SendDoc` → загрузка MAX `file` |
| `SCREEN`, `ЭКРАН` | снимок монитора + fallback на весь desktop |
| `MAP`, `КАРТА` | изображение карты |
| `STOP`, `СТОП` | закрытие окна аварий + подтверждение |
| `LOG`, `ЖУРНАЛ` | экспорт HTML + отправка файла |
| `LOGXLS` | экспорт XLS + отправка файла |
| `ALARM`, `ТРЕВОГИ` | экспорт истории тревог в PDF |
| `HELP`, `?` | полная помощь, эквивалентная Telegram |
| список пользователей | `MAX_USER_LIST` |
| маски алиасов (`!`, `*`, пустая) | те же правила, что в `TgUser::HasValidAlias` |
| авторизация `RequestAlias` | сохранена |
| рассылка по `AlarmAlias` | `OnNewAlarmState` |
| numeric alias fallback | сохранён |
| `InCount`, `OutCount`, `Tag` | сохранены |
| настройки и пользователи в INI | `MaxLoadIni` / `MaxSaveIni` |
| callback входящего сообщения | `ILanMonMaxEvents::OnMaxMessage` |
| FastScript helpers Telegram | эквивалентные методы `LANMON_MAX_BOT` |
| debug/error/getMe/read callbacks | `ILanMonMaxEvents` |
| active / periodic pause | `MAX_BOT_SETTINGS` |

### Важная совместимость: `FlagSendMaps`

Старый Telegram-код проверяет `FlagSendMaps` перед **всеми** встроенными командами, а не только перед `MAP`. MAX намеренно сохраняет это поведение. `OnMaxMessage` вызывается до этой проверки — так же, как `OnTgMessage` в Telegram.

## Структура

- `maxcore.*` — C++98 JSON parser, DTO MAX, построение URL/body, преобразование CP1251 → UTF-8.
- `maxclient.*` — независимый от транспорта клиент MAX API.
- `maxindy.*` — HTTPS-транспорт C++Builder/Indy.
- `maxusers.*` — пользователи, алиасы, счётчики и теги.
- `maxsettings.*` — сохранение/загрузка INI.
- `lanmon_commands.*` — все встроенные команды LanMon, эквивалентные Telegram.
- `lanmon_bot.*` — фасад поведения, совместимого с Telegram.
- `tests/` — protocol/client/parity тесты.
- `e2e/` — настоящий локальный TCP/HTTP mock MAX.
- `e2e_lanmon/` — полный E2E `MAX → LanMon → MAX`.

## Тесты

```bash
./tests/run.sh
./e2e/run_e2e.sh
./e2e_lanmon/run.sh
```

CI компилирует портируемый production-код строго с:

```text
-std=gnu++98 -Wall -Wextra -Werror
```

Полный parity E2E выполняет:

```text
MAX /updates
  → STOP
  → MAP 2
  → SCREEN 1
  → SCREEN (fallback на desktop)
  → LOG
  → LOGXLS
  → ALARM
  → HELP
  → рассылка аварии по alias
  → MAX /messages + image/file uploads
```

Полный checklist находится в `PARITY.md`.

---

# Интеграция в LanMon

## 1. Добавить файлы в проект C++Builder

Скопировать файлы в каталог `Max/` и добавить `.cpp` в `lanmon4.cbproj`:

```text
maxcore.cpp/.h
maxclient.cpp/.h
maxindy.cpp/.h
maxusers.cpp/.h
maxsettings.cpp/.h
lanmon_commands.cpp/.h
lanmon_bot.cpp/.h
```

Только `maxindy.*` зависит от Indy/VCL. Создание скриншотов, экспорт журнала и PDF тревог должно остаться внутри LanMon через `ILanMonCommandActions`.

## 2. Реализовать мост к действиям LanMon

Создать класс от `ILanMonCommandActions` и подключить существующие функции LanMon:

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

Поведение `SCREEN` повторяет Telegram: сначала вызывается `GetMonitorScreenshot(screenindex-1)`, при неудаче выполняется fallback на `DesktopScreenshot`.

## 3. Создать глобальные объекты

Рядом с текущей инициализацией `TgBot`:

```cpp
TMaxIndyTransport *MaxTransport=NULL;
MAX_API_CLIENT *MaxApi=NULL;
TLanMonMaxActions *MaxActions=NULL;
LANMON_MAX_BOT *MaxBot=NULL;
```

Инициализация:

```cpp
MaxTransport=new TMaxIndyTransport();
MaxApi=new MAX_API_CLIENT(MaxTransport,"");
MaxActions=new TLanMonMaxActions();
MaxBot=new LANMON_MAX_BOT(MaxApi,MaxActions,&MaxEvents);

std::string error;
MaxBot->Load(((AnsiString)szWorkDir+"max.ini").c_str(),error);
```

При завершении удалить объекты в обратном порядке. `Load()` автоматически передаёт `BotToken` в `MAX_API_CLIENT`.

## 4. Настроить доверие TLS в Indy

`TMaxIndyTransport::SSL()` возвращает production `TIdSSLIOHandlerSocketOpenSSL*`:

```cpp
TIdSSLIOHandlerSocketOpenSSL *ssl=MaxTransport->SSL();
ssl->SSLOptions->Method=sslvTLSv1_2;
// Здесь настроить CA/сертификат, требуемый MAX.
```

Не следует решать проблемы сертификата постоянным отключением проверки TLS.

## 5. Выполнять polling в worker thread

Long Polling нельзя выполнять в VCL UI thread. В Telegram уже используется `TTgBotThread`; для MAX следует применить тот же подход.

Вызов из worker:

```cpp
if(MaxBot->Settings.Active && !MaxBot->Settings.PeriodicReadMessagesPaused)
{
    std::string error;
    MaxBot->ReadMessages(true,error,30,100);
}
```

Ручное чтение:

```cpp
MaxBot->ReadMessages(false,error,30,100);
```

`MAX_API_CLIENT` автоматически хранит и передаёт `marker`.

## 6. Подключить callback FastScript

Реализовать мост событий:

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

Добавить FastScript-событие, аналогичное Telegram:

```text
OnMaxMessage(UpdateTimestamp, Id, Text)
```

Если нужно сохранить существующие пользовательские скрипты без изменений, мост может намеренно вызывать старый `OnTgMessage`. Тогда скрипты фактически станут независимыми от конкретного мессенджера.

## 7. Добавить функции FastScript

Отобразить Telegram-функции один к одному:

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

Тонкие wrappers вызывают:

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

Чтобы старые скрипты продолжили работать вообще без изменений, можно зарегистрировать старые имена `Tg*` на messenger-neutral dispatcher вместо дублирования скриптовой логики.

## 8. Подключить аварийные события

Сохранить существующие точки принятия решения из `src/alarms/Avaria.cpp` и `HistoryAlarm.cpp`, добавив отправку в MAX:

```cpp
if(MaxBot && MaxBot->Settings.FlagSendAlarms)
    MaxBot->OnNewAlarmState(MaxUtf8FromCp1251(mess.c_str()),error);

if(MaxBot && MaxBot->Settings.FlagOperatorAlarm)
    MaxBot->OnNewAlarmState(MaxUtf8FromCp1251(mess.c_str()),error);

if(MaxBot && MaxBot->Settings.FlagSendAlarmsEnd)
    MaxBot->OnNewAlarmState(MaxUtf8FromCp1251(mess.c_str()),error);
```

`OnNewAlarmState` отправляет сообщение только пользователям, соответствующим `AlarmAlias`.

## 9. Пользователи и `max.ini`

Можно переиспользовать UI-модель Telegram: ID, имя, alias, comment, IsBot, InCount, OutCount и Tag. Для MAX дополнительно нужен `PeerType`, потому что API различает `user_id` и `chat_id`.

MAX лучше хранить отдельно, например в `max.ini`:

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
PeerType=user
Alias=R001
Comment=
InCount=0
OutCount=0
Tag=0

[User1]
Name=Dispatchers
id=987654321
PeerType=chat
Alias=A001
Comment=Групповой чат аварий
InCount=0
OutCount=0
Tag=0
```

Значения `PeerType`:

- `user` — отправка через `user_id`;
- `chat` — отправка через `chat_id`.

Если `PeerType` отсутствует в старом конфиге, используется `user`.

Правила alias намеренно идентичны Telegram:

- пустая маска → любой валидный пользователь;
- `*` → любой валидный пользователь;
- `!` → wildcard ровно для одного символа;
- остальные символы должны совпадать на той же позиции.

## 10. Кодировка

Старый LanMon часто использует CP1251, а JSON MAX — UTF-8:

```cpp
std::string utf8=MaxUtf8FromCp1251(ansi.c_str());
```

Текст, полученный из MAX, уже UTF-8. Если старый VCL/FastScript API ожидает ANSI, преобразование следует выполнять на границе интеграции LanMon.

---

# Требования платформы MAX

Адаптер следует контракту MAX API, актуальному на август 2026 года.

## Обязательные требования

1. Создать чат-бота MAX и получить access token.
2. Использовать для API адрес `https://platform-api2.max.ru`.
3. Передавать токен только в заголовке:

   ```text
   Authorization: <access_token>
   ```

   Передача токена в query больше не поддерживается.
4. Добавить требуемый MAX сертификат/CA Минцифры в trust store, используемый Indy/OpenSSL внутри LanMon.
5. Разрешить исходящий HTTPS к `platform-api2.max.ru` и к upload URL, которые возвращает `POST /uploads`. Для image/file MAX может вернуть другой host — полученный URL нужно использовать без изменения.
6. Не превышать документированный MAX лимит 30 запросов в секунду.

Официальная документация:

- `https://dev.max.ru/docs-api`
- `https://dev.max.ru/docs-api/methods/GET/updates`
- `https://dev.max.ru/docs-api/methods/POST/uploads`
- `https://dev.max.ru/docs-api/methods/POST/messages`

## Long Polling

LanMon использует:

```http
GET /updates?timeout=...&limit=...&marker=...
Authorization: <token>
```

Это хорошо соответствует текущей архитектуре LanMon: приложение делает только исходящие HTTPS-соединения и не требует публичного входящего endpoint.

MAX сейчас описывает Long Polling как механизм для разработки/тестирования и рекомендует Webhook для production. Long Polling не работает при активной Webhook-подписке. Если в будущем потребуется перейти на Webhook, `LANMON_MAX_BOT`, пользователи, aliases, команды и код отправки/upload останутся прежними — заменить нужно только доставку входящих updates.

## Загрузка файлов

`SendPhoto` использует `type=image`:

- JPG/JPEG/PNG/GIF/TIFF/BMP/HEIC;
- максимум 50 MB;
- максимум 7680×7680 px.

`SendDoc` использует `type=file`:

- обычные document/file форматы;
- максимум 4 GB согласно текущей документации MAX API.

Поток загрузки:

```text
POST /uploads?type=image|file
  → multipart POST файла на возвращённый URL
  → получение token
  → POST /messages с attachments.payload.token
```

Реальный token бота нельзя коммитить в Git или писать в логи. Его следует хранить в deployment-конфигурации / `max.ini` с подходящими правами на файл и менять при утечке.

---

# Что не может доказать Linux CI

`maxindy.cpp` требует старые headers C++Builder/VCL/Indy, поэтому Linux CI не компилирует только этот transport-файл. Всё выше границы сокета компилируется и реально выполняется: TCP/HTTP, `marker`, aliases, авторизация, все Telegram-эквивалентные команды, image/file upload и alarm fan-out.

Финальная приёмка на Windows:

```text
сборка C++Builder
  → настройка CA MAX
  → GET /me с реальным token
  → получение /updates
  → выполнение встроенной команды
  → получение ответа text/image/file
```
