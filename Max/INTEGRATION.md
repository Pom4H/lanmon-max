# Интеграция MAX в LanMon 4

Инструкция рассчитана на фактический legacy-проект LanMon: C++Builder/VCL, `lanmon4.cbproj`, существующий каталог `Telegram/`, `TgBot` в `src/forms/main.cpp` и alarm hooks в `src/alarms/Avaria.cpp` / `HistoryAlarm.cpp`.

Цель — добавить `Max/` рядом с `Telegram/` с минимальным архитектурным diff.

> Текущий `TMaxBotThread` зеркалит Telegram и получает входящие сообщения через Long Polling. Это удобно для первого внедрения и тестирования. Для production MAX входящий transport должен быть переведён на Webhook/relay; исходящая отправка через `MAX_API_CLIENT` остаётся пригодной.

## 0. Итоговое размещение

В исходниках:

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
    certs/
      max-ca.pem
```

В установленном LanMon:

```text
LanMon4/
  lanmon4.exe
  MaxBot.ini
  certs/
    max-ca.pem
  ssleay32.dll / libeay32.dll   # либо другой ABI-совместимый runtime,
                                 # который реально использует версия Indy проекта
```

`Max/certs/max-ca.pem` уже находится в этом репозитории. Отдельно скачивать сертификат для обычной установки не требуется.

---

## 1. Скопировать `Max/` рядом с `Telegram/`

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

`tests/` и `e2e/` в C++Builder project добавлять не нужно.

`Max/certs/max-ca.pem` не компилируется, но должен попасть в install/package LanMon.

---

## 2. Include path

В `lanmon4.cbproj` рядом с уже существующим `Telegram` добавить:

```text
Max;Max\api
```

Пример:

```xml
<IncludePath>...;Telegram;Max;Max\api;Collection;src\forms;...</IncludePath>
```

После этого сохраняется привычный стиль проекта:

```cpp
#include "maxbot.h"
#include "UFMaxBot.h"
```

---

## 3. Добавить исходники и формы в `lanmon4.cbproj`

Проще всего добавить файлы через `Project -> Add to Project` в C++Builder и затем проверить `.cbproj`.

Нужные C++ units:

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

DFM resources:

```xml
<FormResources Include="Max\UFMaxBot.dfm" />
<FormResources Include="Max\UFMaxBotApi.dfm" />
<FormResources Include="Max\UFMaxUserEdit.dfm" />
<FormResources Include="Max\UFMaxMsg.dfm" />
```

---

## 4. Подключить MAX в `main.h`

Рядом с Telegram:

```cpp
#include "tgbot.h"
#include "maxbot.h"
```

В `TMainForm` добавить callbacks:

```cpp
void __fastcall OnMaxTaskReadMessages(MaxMessage_LIST & msglist);
void __fastcall OnMaxPeriodicReadMessages(MaxMessage_LIST & msglist);
void __fastcall OnMaxGetMe(MaxBotInfo & botinfo);
void __fastcall OnMaxDebugMessage(AnsiString msg);
void __fastcall OnMaxErrorDebugMessage(AnsiString msg);
```

---

## 5. Подключить форму MAX в `main.cpp`

Рядом с:

```cpp
#include "UFTgBot.h"
```

добавить:

```cpp
#include "UFMaxBot.h"
```

Пункт меню можно сделать зеркально Telegram:

```cpp
void __fastcall TMainForm::mMaxBotClick(TObject *Sender)
{
    if(!FormMaxBot)
        FormMaxBot=new TFormMaxBot(this);

    FormMaxBot->Show();
    FormMaxBot->BringToFront();
}
```

---

## 6. Загрузить `MaxBot.ini` и назначить callbacks

Рядом с существующей инициализацией `TgBot`:

```cpp
//Загрузка MAX бота
MaxBot.Load(WorkDir + "MaxBot.ini");

//Обработчики MAX
MaxBot.SetOnTaskReadMessages(OnMaxTaskReadMessages);
MaxBot.SetOnPeriodicReadMessages(OnMaxPeriodicReadMessages);
MaxBot.SetOnGetMe(OnMaxGetMe);
MaxBot.SetOnDebugMessage(OnMaxDebugMessage);
MaxBot.SetOnErrorDebugMessage(OnMaxErrorDebugMessage);
```

`MAX_BOT MaxBot` уже определён в `Max/maxbot.cpp`. Второй глобальный объект создавать не нужно.

---

## 7. Callbacks `TMainForm`

Минимальный вариант:

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

Важно: как и в Telegram, `MAX_BOT` не забирает callbacks себе. `MaxBot.OnMessages()` вызывается из `MainForm`.

---

## 8. Сертификат Минцифры: что именно сделать

### 8.1 Сертификат уже в репозитории

Использовать:

```text
Max/certs/max-ca.pem
```

Bundle содержит:

```text
Russian Trusted Sub CA
SHA-256: BBBDE2103E790B999EC62BD03CF625A5A2E7C316E10AFE6A490EEDEAD8B3FD9B

Russian Trusted Root CA
SHA-256: D26D2D0231B7C39F92CC738512BA54103519E4405D68B5BD703E9788CA8ECF31
```

Подробности и процедура ротации: `Max/certs/README.md`.

### 8.2 Куда положить при установке LanMon

`TMaxIndyTransport` ищет trust bundle **относительно `Application->ExeName`**:

```cpp
AnsiString rootCert=
    ExtractFilePath(Application->ExeName)+"certs\\max-ca.pem";
```

Поэтому при deployment обязательно скопировать:

```text
Max/certs/max-ca.pem
```

в:

```text
<каталог lanmon4.exe>\certs\max-ca.pem
```

Пример:

```text
C:\Program Files (x86)\LanMon 4\lanmon4.exe
C:\Program Files (x86)\LanMon 4\certs\max-ca.pem
```

### 8.3 Что делает код

Если bundle найден:

```cpp
Ssl->SSLOptions->RootCertFile=rootCert;
Ssl->SSLOptions->VerifyMode=TIdSSLVerifyModeSet()<<sslvrfPeer;
Ssl->SSLOptions->VerifyDepth=9;
Ssl->SSLOptions->Method=sslvTLSv1_2;
```

Если файла **нет**, transport работает fail-closed. HTTP-запрос не выполняется, а наружу возвращается:

```text
MAX CA bundle not found: <полный путь>
```

То есть отсутствие сертификата больше не превращается в незаметно ослабленную TLS-проверку.

### 8.4 Проверить bundle до сборки

В checkout адаптера:

```bash
bash Max/tests/test_cert.sh
```

Проверяются fingerprints, срок действия, CN и цепочка Sub CA -> Root CA.

CI дополнительно устанавливает TLS 1.2 соединение с `platform-api2.max.ru` именно через этот `max-ca.pem`.

### 8.5 OpenSSL DLL старого Indy

`TIdSSLIOHandlerSocketOpenSSL` зависит от OpenSSL ABI своей версии Indy.

Нельзя просто заменить старые `ssleay32.dll/libeay32.dll` на OpenSSL 3.x. Для первого внедрения использовать ABI-совместимый runtime, который соответствует фактической версии Indy LanMon.

После сборки проверить на целевой Windows:

```text
1. certs\max-ca.pem существует рядом с exe;
2. OpenSSL DLL загружаются;
3. GetMe проходит;
4. без/с испорченным max-ca.pem соединение не проходит;
5. в lanmon.log нет TLS/OpenSSL errors.
```

Старые сборки Indy могут отличаться по hostname verification, поэтому это отдельно проверяется acceptance-тестом на фактическом toolchain заказчика.

---

## 9. Firewall / proxy

Минимально разрешить исходящий HTTPS:

```text
platform-api2.max.ru:443
```

Для вложений MAX возвращает отдельный upload URL. Его нельзя переписывать на Bot API host.

Используемые типы могут приводить, например, к:

```text
image -> iu.oneme.ru
file  -> fu.oneme.ru
```

Поэтому корпоративный firewall/proxy должен пропускать и реальные upload hosts.

Bot token на upload-host адаптер **не отправляет**.

При TLS inspection корпоративный CA должен быть учтён отдельно; бездумно отключать `sslvrfPeer` нельзя.

---

## 10. `MaxBot.ini`

Минимальный пример:

```ini
[SETUP]
Active=1
BotApi=<MAX_BOT_TOKEN>
PeriodReadMessages=5
SendAlarms=1
SendAlarmEnd=1
OperatorAlarm=0
AlarmAlias=*
RequestAlias=*
SendMaps=1
UseLanmonLog=1

[User0]
Id=123456789
Name=Иван
Alias=OPERATOR
Comment=
IsBot=0
InCount=0
OutCount=0
Tag=0
PeerType=user
```

Для группового/чатового адресата:

```ini
PeerType=chat
```

`BotApi`/`BotToken` — секрет. Не коммитить реальный токен.

---

## 11. Алармы

Точки вызова должны быть зеркальны Telegram.

В местах, где сейчас вызывается Telegram notification при изменении alarm state, добавить MAX-вызов с тем же текстом и теми же условиями:

```cpp
if(MaxBot.Active && MaxBot.FlagSendAlarms)
    MaxBot.OnNewAlarmState(message);
```

Для завершения аварии учитывать `FlagSendAlarmsEnd`, для операторского подтверждения — `FlagOperatorAlarm`, так же как в существующей Telegram-логике.

Не переносить alarm decision logic внутрь MAX transport.

---

## 12. FastScript

Для первого внедрения MAX сохраняет существующий callback:

```cpp
OnTgMessage((int)msg->update_id,scriptid,msg->Text);
```

Это сделано намеренно, чтобы существующие LanMon scripts продолжали получать сообщения без обязательной миграции FastScript API.

Если позже понадобится отдельный MAX API для scripts, его лучше добавлять отдельным изменением после принятия первой интеграции.

---

## 13. Кодировка

Legacy LanMon использует `AnsiString`/CP1251, MAX API — UTF-8.

Исходящие строки:

```text
AnsiString / CP1251 -> MaxUtf8FromCp1251 -> JSON UTF-8
```

Входящие строки MAX преобразуются обратно для VCL/FastScript слоя.

Русские команды `ЭКРАН`, `КАРТА`, `СТОП`, `ЖУРНАЛ`, `ТРЕВОГИ` должны проверяться на реальном Windows build, потому что это граница UTF-8/CP1251 и старого `AnsiString`.

---

## 14. Long Polling и production Webhook

Текущий зеркальный `TMaxBotThread` использует:

```text
GET /updates
```

Это оставлено для минимального diff с Telegram и простого первого запуска.

Production-архитектура должна быть:

```text
MAX Webhook
    -> публичный HTTPS relay
    -> защищённая доставка в LanMon
    -> MaxMessage_LIST / MAX_BOT::OnMessages
```

При таком переходе `MAX_BOT`, команды, пользователи, aliases, alarms и исходящая отправка не должны переписываться; меняется источник входящих events.

---

## 15. Проверка перед передачей

В репозитории адаптера:

```bash
bash Max/tests/run.sh
bash Max/e2e/run_e2e.sh
```

На целевой Windows/C++Builder:

```text
[ ] lanmon4.cbproj собирается
[ ] DFM формы открываются
[ ] certs\max-ca.pem установлен рядом с exe
[ ] GetMe проходит через настоящий MAX
[ ] без корректного CA GetMe не проходит
[ ] личный чат работает
[ ] групповой chat_id работает
[ ] SCREEN / ЭКРАН
[ ] MAP / КАРТА
[ ] STOP / СТОП
[ ] LOG / ЖУРНАЛ
[ ] LOGXLS
[ ] ALARM / ТРЕВОГИ
[ ] HELP / ?
[ ] alarm fan-out по AlarmAlias
[ ] image upload
[ ] PDF/file upload
[ ] большой attachment переживает attachment.not.ready retry
[ ] lanmon.log не содержит TLS/OpenSSL ошибок
```

Подробная автоматизированная матрица: `Max/TESTING.md`.
