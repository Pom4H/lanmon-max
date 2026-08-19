# Интеграция MAX в LanMon 4

Эта инструкция написана под фактическую структуру legacy LanMon из проекта: C++Builder/VCL, `lanmon4.cbproj`, существующий каталог `Telegram/`, инициализация `TgBot` в `src/forms/main.cpp`, alarm hooks в `src/alarms/Avaria.cpp` и `HistoryAlarm.cpp`.

Цель интеграции — добавить `Max/` рядом с `Telegram/` с минимальным архитектурным diff.

> Важно: текущий `TMaxBotThread` получает входящие события через Long Polling для зеркальности с Telegram. По требованиям MAX это режим разработки/тестирования. Для production входящие события нужно перевести на Webhook/relay. Исходящая отправка сообщений и файлов при этом остаётся той же.

---

## 0. Что должно получиться

В корне исходников LanMon:

```text
lanmon/
  Telegram/
  Max/
    maxbot.h/.cpp
    maxmsg.h/.cpp
    maxtask.h/.cpp
    UFMaxBot.h/.cpp/.dfm
    UFMaxBotApi.h/.cpp/.dfm
    UFMaxUserEdit.h/.cpp/.dfm
    UFMaxMsg.h/.cpp/.dfm
    api/
      maxcore.h/.cpp
      maxclient.h/.cpp
      maxindy.h/.cpp
```

В каталоге установленного `lanmon4.exe`:

```text
LanMon4/
  lanmon4.exe
  MaxBot.ini
  certs/
    max-ca.pem
  ssleay32.dll / libeay32.dll   # либо другой ABI-совместимый комплект,
                                 # который использует конкретная версия Indy проекта
```

---

# 1. Скопировать каталог `Max/`

Скопировать содержимое этого репозитория `Max/` в корень исходного проекта LanMon рядом с `Telegram/`.

Не переносить `tests/` и `e2e/` в production-проект C++Builder — они нужны только для CI этого адаптера.

Production-файлы:

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

---

# 2. Добавить include path

В `lanmon4.cbproj` сейчас в `IncludePath` уже есть `Telegram`.

Добавить рядом:

```text
Max;Max\api
```

То есть фрагмент должен выглядеть примерно так:

```xml
<IncludePath>...;Telegram;Max;Max\api;Collection;src\forms;...</IncludePath>
```

Это позволяет сохранить тот же стиль include, который уже используется Telegram:

```cpp
#include "maxbot.h"
#include "UFMaxBot.h"
```

---

# 3. Добавить файлы в `lanmon4.cbproj`

В секцию `CppCompile` добавить:

```xml
<CppCompile Include="Max\maxbot.cpp">
  <DependentOn>Max\maxbot.h</DependentOn>
</CppCompile>
<CppCompile Include="Max\maxmsg.cpp">
  <DependentOn>Max\maxmsg.h</DependentOn>
</CppCompile>
<CppCompile Include="Max\maxtask.cpp">
  <DependentOn>Max\maxtask.h</DependentOn>
</CppCompile>

<CppCompile Include="Max\UFMaxBot.cpp">
  <Form>FormMaxBot</Form>
  <DependentOn>Max\UFMaxBot.h</DependentOn>
</CppCompile>
<CppCompile Include="Max\UFMaxBotApi.cpp">
  <Form>FormMaxBotApi</Form>
  <DependentOn>Max\UFMaxBotApi.h</DependentOn>
</CppCompile>
<CppCompile Include="Max\UFMaxUserEdit.cpp">
  <Form>FormMaxUserEdit</Form>
  <DependentOn>Max\UFMaxUserEdit.h</DependentOn>
</CppCompile>
<CppCompile Include="Max\UFMaxMsg.cpp">
  <Form>FormMaxMsg</Form>
  <DependentOn>Max\UFMaxMsg.h</DependentOn>
</CppCompile>

<CppCompile Include="Max\api\maxcore.cpp">
  <DependentOn>Max\api\maxcore.h</DependentOn>
</CppCompile>
<CppCompile Include="Max\api\maxclient.cpp">
  <DependentOn>Max\api\maxclient.h</DependentOn>
</CppCompile>
<CppCompile Include="Max\api\maxindy.cpp">
  <DependentOn>Max\api\maxindy.h</DependentOn>
</CppCompile>
```

В секцию ресурсов форм добавить:

```xml
<FormResources Include="Max\UFMaxBot.dfm" />
<FormResources Include="Max\UFMaxBotApi.dfm" />
<FormResources Include="Max\UFMaxUserEdit.dfm" />
<FormResources Include="Max\UFMaxMsg.dfm" />
```

Проще и безопаснее сделать это через IDE C++Builder (`Project → Add to Project`) и затем проверить получившийся `.cbproj`.

---

# 4. Подключить MAX в `main.h`

Рядом с существующим:

```cpp
#include "tgbot.h"
```

добавить:

```cpp
#include "maxbot.h"
```

В `TMainForm` рядом с `OnTg*` callbacks объявить:

```cpp
void __fastcall OnMaxTaskReadMessages(MaxMessage_LIST & msglist);
void __fastcall OnMaxPeriodicReadMessages(MaxMessage_LIST & msglist);
void __fastcall OnMaxGetMe(MaxBotInfo & botinfo);
void __fastcall OnMaxDebugMessage(AnsiString msg);
void __fastcall OnMaxErrorDebugMessage(AnsiString msg);
```

---

# 5. Подключить формы в `main.cpp`

Рядом с:

```cpp
#include "UFTgBot.h"
```

добавить:

```cpp
#include "UFMaxBot.h"
```

Если нужен отдельный пункт меню MAX, создать обработчик по образцу `mTgBotClick`:

```cpp
void __fastcall TMainForm::mMaxBotClick(TObject *Sender)
{
    if(!FormMaxBot)
        FormMaxBot=new TFormMaxBot(this);

    FormMaxBot->Show();
    FormMaxBot->BringToFront();
}
```

Сам пункт меню/Action удобнее добавить через VCL Designer рядом с Telegram.

---

# 6. Загрузить настройки и назначить callbacks

В исходном `main.cpp` Telegram инициализируется блоком `TgBot.Load(...)` + `SetOn*`.

Сразу после него добавить зеркальный MAX-блок:

```cpp
// Загрузка MAX бота
MaxBot.Load(WorkDir + "MaxBot.ini");

// Задание обработчиков MAX
MaxBot.SetOnTaskReadMessages(OnMaxTaskReadMessages);
MaxBot.SetOnPeriodicReadMessages(OnMaxPeriodicReadMessages);
MaxBot.SetOnGetMe(OnMaxGetMe);
MaxBot.SetOnDebugMessage(OnMaxDebugMessage);
MaxBot.SetOnErrorDebugMessage(OnMaxErrorDebugMessage);
```

`MAX_BOT` глобально определён в `Max/maxbot.cpp`:

```cpp
MAX_BOT MaxBot;
```

Отдельно создавать объект в `main.cpp` не нужно.

---

# 7. Добавить callbacks `TMainForm`

По аналогии с Telegram:

```cpp
void __fastcall TMainForm::OnMaxTaskReadMessages(MaxMessage_LIST & msglist)
{
    if(FormMaxBot)
        FormMaxBot->OnTaskReadMessages(msglist);
}

void __fastcall TMainForm::OnMaxPeriodicReadMessages(MaxMessage_LIST & msglist)
{
    if(msglist.Count)
        MaxBot.OnMessages(msglist);
}

void __fastcall TMainForm::OnMaxGetMe(MaxBotInfo & botinfo)
{
    if(FormMaxBot)
        FormMaxBot->OnGetMe(botinfo);
}

void __fastcall TMainForm::OnMaxDebugMessage(AnsiString msg)
{
    if(MaxBot.UseLanmonLog)
        WriteToLog("DEBUG\tMaxBot: %s",msg.c_str());

    if(FormMaxBot)
        FormMaxBot->OnDebugMessage(msg);
}

void __fastcall TMainForm::OnMaxErrorDebugMessage(AnsiString msg)
{
    WriteToLog("ERROR\tMaxBot: %s",msg.c_str());

    if(FormMaxBot)
        FormMaxBot->OnErrorDebugMessage(msg);
}
```

Ключевой момент: `MaxBot.OnMessages()` вызывается снаружи из `OnMaxPeriodicReadMessages`, как это уже сделано для Telegram. `MAX_BOT` сам не подписывает себя на thread callbacks.

---

# 8. Подключить сертификат Минцифры к Indy/OpenSSL

## 8.1 Почему одного Windows Certificate Store недостаточно

`TMaxIndyTransport` использует:

```cpp
TIdSSLIOHandlerSocketOpenSSL
```

У старого Indy/OpenSSL trust store задаётся через `SSLOptions->RootCertFile`. Поэтому для этой интеграции сертификат задаётся явным PEM-файлом.

Transport уже ищет файл по фиксированному пути:

```text
<каталог lanmon4.exe>\certs\max-ca.pem
```

То есть если exe находится здесь:

```text
C:\Program Files (x86)\LanMon 4\lanmon4.exe
```

нужен файл:

```text
C:\Program Files (x86)\LanMon 4\certs\max-ca.pem
```

В `TMaxIndyTransport` при наличии файла автоматически выполняется эквивалент:

```cpp
Ssl->SSLOptions->RootCertFile=rootCert;
Ssl->SSLOptions->VerifyMode=TIdSSLVerifyModeSet()<<sslvrfPeer;
Ssl->SSLOptions->VerifyDepth=9;
```

## 8.2 Где взять сертификат

MAX с 19 июля 2026 требует использовать `platform-api2.max.ru` и добавить сертификат Минцифры в доверенные.

Не следует класть случайный сертификат из браузера или копировать leaf-сертификат `platform-api2.max.ru`.

Нужно получить актуальный доверенный сертификат/цепочку Минцифры из официального источника, указанного MAX/Минцифры, и сформировать CA bundle.

Сертификат в репозиторий **не добавляется**: он является внешней эксплуатационной зависимостью и может обновляться.

## 8.3 Формат `max-ca.pem`

`RootCertFile` для OpenSSL должен быть PEM-файлом. Он выглядит так:

```text
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
```

Если официальный файл скачан в DER `.cer`, преобразовать его можно установленным OpenSSL:

```bat
openssl x509 -inform DER -in mincifry.cer -out max-ca.pem
```

Если `.cer` уже PEM, преобразование не требуется — достаточно сохранить/переименовать его как `max-ca.pem`.

Если для цепочки нужны несколько CA-сертификатов, `max-ca.pem` может содержать несколько последовательных блоков:

```text
-----BEGIN CERTIFICATE-----
... certificate 1 ...
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
... certificate 2 ...
-----END CERTIFICATE-----
```

## 8.4 OpenSSL DLL

`IdSSLOpenSSL` зависит от OpenSSL DLL, совместимых с конкретной версией Indy, с которой собран LanMon.

Не нужно просто заменять старые DLL на OpenSSL 3.x: старый `IdSSLOpenSSL` использует конкретный ABI.

Для первого внедрения использовать тот же совместимый OpenSSL runtime, с которым уже работает существующий LanMon/Telegram, и отдельно проверить соединение с `platform-api2.max.ru`.

## 8.5 Проверка сертификата

После установки `max-ca.pem`:

1. запустить LanMon;
2. открыть настройки MAX;
3. ввести Bot Token;
4. выполнить `GetMe`;
5. убедиться, что вернулись `Id`, имя и username бота;
6. проверить `lanmon.log` на отсутствие TLS/OpenSSL ошибок.

Если `GetMe` не проходит:

```text
1. проверить существование certs\max-ca.pem относительно lanmon4.exe;
2. проверить, что файл PEM, а не бинарный DER;
3. проверить цепочку сертификатов;
4. проверить совместимость OpenSSL DLL с IdSSLOpenSSL;
5. проверить доступ к platform-api2.max.ru:443;
6. проверить системное время Windows;
7. проверить proxy/firewall/SSL inspection.
```

> Примечание безопасности: `sslvrfPeer` включает проверку доверенной цепочки. Проверку hostname необходимо отдельно подтвердить на фактической версии Indy/OpenSSL, используемой LanMon; старые сборки Indy не следует автоматически считать эквивалентными современному браузеру по всем TLS-проверкам.

---

# 9. Firewall / proxy

Для обычных API-запросов разрешить исходящий HTTPS к:

```text
platform-api2.max.ru:443
```

Для вложений одного этого недостаточно: MAX возвращает отдельный upload URL, который должен использоваться без изменений.

В документации MAX сейчас указаны upload hosts:

```text
file        → fu.oneme.ru
image       → iu.oneme.ru
video/audio → vu.okcdn.ru
```

Поэтому firewall/proxy должен разрешать HTTPS как минимум к используемым типам upload-hosts.

Если корпоративный proxy выполняет TLS inspection, его CA также должен быть доверен тем OpenSSL trust bundle, которым пользуется LanMon, либо inspection для этих адресов должен быть отключён согласно политике заказчика.

---

# 10. Создать `MaxBot.ini`

Минимальный пример:

```ini
[SETUP]
Active=1
BotApi=PUT_MAX_BOT_TOKEN_HERE
PeriodReadMessages=10
SendAlarms=0
SendAlarmEnd=0
OperatorAlarm=0
AlarmAlias=
RequestAlias=
SendMaps=1
UseLanmonLog=1
```

После первого сохранения через форму будут записаны данные бота и пользователи.

Для пользователя дополнительно хранится MAX-специфичное поле:

```ini
PeerType=user
```

или:

```ini
PeerType=chat
```

Оно выбирает адресацию `user_id` / `chat_id`.

Токен нельзя коммитить в Git или писать в обычные диагностические сообщения.

---

# 11. Подключить alarm hooks

## `src/alarms/Avaria.cpp`

Рядом с существующим Telegram:

```cpp
if(TgBot.FlagSendAlarms)
    TgBot.OnNewAlarmState(mess);
```

добавить:

```cpp
if(MaxBot.FlagSendAlarms)
    MaxBot.OnNewAlarmState(mess);
```

В обработке подтверждения аварии рядом с `TgBot.FlagOperatorAlarm`:

```cpp
if(MaxBot.FlagOperatorAlarm)
    MaxBot.OnNewAlarmState(mess);
```

## `src/alarms/HistoryAlarm.cpp`

Рядом с `TgBot.FlagSendAlarmsEnd`:

```cpp
if(MaxBot.FlagSendAlarmsEnd)
    MaxBot.OnNewAlarmState(mess);
```

Не переносить точки принятия решения внутрь MAX: решение «какое событие считать аварией» остаётся в LanMon, как и для Telegram.

---

# 12. FastScript

Для минимального первого внедрения новая регистрация FastScript **не обязательна**.

`MAX_BOT::OnMessages` сейчас намеренно вызывает существующий:

```cpp
OnTgMessage(update_id,id,text);
```

То есть существующие пользовательские обработчики сообщений продолжают работать.

Если заказчику нужен отдельный MAX API в скриптах, вторым этапом можно добавить зеркальные:

```text
OnMaxMessage
MaxSendMessage
MaxSendPhoto
MaxSendDoc
MaxUserCount
MaxGetUser
...
```

Это лучше делать отдельным diff после того, как базовая MAX-интеграция собрана и проверена.

---

# 13. Первый запуск

Рекомендуемый порядок:

1. `Active=0`.
2. Собрать LanMon с `Max/`.
3. Положить `certs\max-ca.pem` рядом с установленным exe.
4. Проверить OpenSSL DLL.
5. Запустить LanMon.
6. Открыть окно MAX.
7. Ввести token.
8. Выполнить `GetMe`.
9. Включить `Active`.
10. Отправить тестовое сообщение боту.
11. Проверить `HELP`.
12. Проверить `SCREEN`.
13. Проверить `MAP`.
14. Проверить отправку PDF/HTML/XLS.
15. Включить тестового пользователя в `AlarmAlias` и проверить alarm fan-out.
16. Только после этого включать аварийные уведомления для реальных адресатов.

---

# 14. Что проверить перед передачей заказчику

```text
[ ] Max/ и Max/api добавлены в IncludePath
[ ] все .cpp добавлены в lanmon4.cbproj
[ ] все UFMax*.dfm добавлены как FormResources
[ ] main.h содержит maxbot.h и OnMax* callbacks
[ ] main.cpp загружает MaxBot.ini
[ ] main.cpp назначает SetOn* callbacks
[ ] OnMaxPeriodicReadMessages вызывает MaxBot.OnMessages
[ ] alarm hooks добавлены в Avaria.cpp и HistoryAlarm.cpp
[ ] token не находится в репозитории
[ ] certs\max-ca.pem установлен рядом с exe
[ ] peer certificate verification включился через RootCertFile
[ ] GET /me работает на platform-api2.max.ru
[ ] отправка текста работает
[ ] image upload работает
[ ] file upload работает
[ ] firewall разрешает upload-hosts
[ ] русский текст проходит CP1251 ↔ UTF-8
[ ] aliases и RequestAlias проверены
[ ] production-схема Webhook/relay согласована отдельно
```

---

# 15. Production и Webhook

Зеркальный `TMaxBotThread` оставлен на Long Polling специально, чтобы интеграционный diff был похож на Telegram.

Но production MAX требует Webhook. Для desktop/on-prem LanMon рекомендуемая схема:

```text
MAX
  ↓ HTTPS Webhook
public relay
  ↓ authenticated internal delivery
LanMon
  ↓
MaxMessage_LIST
  ↓
MaxBot.OnMessages(...)
```

При такой схеме команды, пользователи, aliases, alarms и исходящая отправка не переписываются. Меняется только транспорт входящих событий.
