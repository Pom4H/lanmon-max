#!/usr/bin/env python3
"""Behavioral source contract for the VCL/C++Builder layer Linux cannot compile.

The test protects the legacy Telegram behavior mirrored by MAX and a small set
of MAX-specific security/state invariants. It intentionally checks critical
operation order, not formatting. BCB2007 compiler/VCL compatibility is guarded
separately by test_bcb2007.py.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def function_body(text, signature):
    pos = text.find(signature)
    if pos < 0:
        errors.append(f"missing function: {signature}")
        return ""
    start = text.find("{", pos)
    if start < 0:
        errors.append(f"missing body: {signature}")
        return ""
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1:i]
    errors.append(f"unclosed body: {signature}")
    return ""


def between(text, start_token, end_token, label):
    start = text.find(start_token)
    if start < 0:
        errors.append(f"{label}: missing start {start_token!r}")
        return ""
    end = text.find(end_token, start + len(start_token))
    if end < 0:
        errors.append(f"{label}: missing end {end_token!r}")
        return ""
    return text[start:end]


def require(text, label, *tokens):
    for token in tokens:
        if token not in text:
            errors.append(f"{label}: missing {token!r}")


def absent(text, label, *tokens):
    for token in tokens:
        if token in text:
            errors.append(f"{label}: forbidden {token!r}")


def ordered(text, label, *tokens):
    cursor = -1
    for token in tokens:
        pos = text.find(token, cursor + 1)
        if pos < 0:
            errors.append(f"{label}: missing ordered token {token!r}")
            return
        cursor = pos


bot = read("maxbot.cpp")
msg = read("maxmsg.cpp")
task = read("maxtask.cpp")
indy = read("api/maxindy.cpp")
client = read("api/maxclient.cpp")

# ---------------------------------------------------------------------------
# Incoming-message contract from TELEGRAM_BOT::OnMessages.
on_messages = function_body(bot, "void MAX_BOT::OnMessages")
ordered(
    on_messages,
    "OnMessages callback/gate/auth order",
    "if(scriptid.IsEmpty())continue;",
    "MaxBot.UserMessageCount++;",
    "OnTgMessage((int)msg->update_id,scriptid,msg->Text);",
    "if(!FlagSendMaps)continue;",
    "UserList->Find(scriptid)",
    "user->HasValidAlias(RequestAlias)",
    "msg->Text.UpperCase().Trim()",
)
ordered(
    on_messages,
    "OnMessages user lookup",
    "UserList->Find(scriptid)",
    "UserList->Find(msg->From.Id)",
    "if(!user)continue;",
)
require(
    on_messages,
    "OnMessages commands",
    '"SCREEN"', 'U8("ЭКРАН")',
    '"MAP"', 'U8("КАРТА")',
    '"STOP"', 'U8("СТОП")',
    '"LOGXLS"', '"LOG"', 'U8("ЖУРНАЛ")',
    '"ALARM"', 'U8("ТРЕВОГ")',
    '"HELP"', "text[1]=='?'",
    '"?? - дополнительные запросы"',
)

screen = between(on_messages, 'if(text.SubString(1,6)=="SCREEN"', 'else if(text.SubString(1,3)=="MAP"', "SCREEN")
map_cmd = between(on_messages, 'else if(text.SubString(1,3)=="MAP"', 'else if(text.SubString(1,4)=="STOP"', "MAP")
stop = between(on_messages, 'else if(text.SubString(1,4)=="STOP"', 'else if(text.SubString(1,6)=="LOGXLS")', "STOP")
logxls = between(on_messages, 'else if(text.SubString(1,6)=="LOGXLS")', 'else if(text.SubString(1,3)=="LOG"', "LOGXLS")
log = between(on_messages, 'else if(text.SubString(1,3)=="LOG"', 'else if(text.SubString(1,5)=="ALARM"', "LOG")
alarm_cmd = between(on_messages, 'else if(text.SubString(1,5)=="ALARM"', 'else if(text.SubString(1,4)=="HELP"', "ALARM")

ordered(screen, "SCREEN fallback", "screenindex=atoi", "GetMonitorScreenshot(screenindex-1,fn)", "DesktopScreenshot(fn)")
require(screen, "SCREEN sends", 'SendPhoto(id,fn,U8("Экран ")+IntToStr(screenindex))', 'SendPhoto(id,fn,U8("Экран"))')
ordered(map_cmd, "MAP pipeline", "if(mapindex)", "CreateMapScreenshot(mapindex-1,bmpfn)", "Bmp2Png(bmpfn,pngfn)", "SendPhoto(id,pngfn")
ordered(stop, "STOP behavior", "CloseAvariaForm();", 'SendMessage(id,U8("Команда СТОП выполнена"));')
ordered(logxls, "LOGXLS behavior", "DeleteFile(fn);", "LogView->ExportToXls(fn);", "SendDoc(id,fn")
ordered(log, "LOG behavior", "DeleteFile(fn);", "LogView->ExportToHtml(fn);", "SendDoc(id,fn")
ordered(alarm_cmd, "ALARM behavior", "CreateAlarmsPdf();", "if(fn.Length())", "SendDoc(id,fn")

# ---------------------------------------------------------------------------
# User/alias compatibility.
has_alias = function_body(msg, "bool MaxUser::HasValidAlias")
ordered(
    has_alias,
    "HasValidAlias special cases",
    "if(!Valid)return false;",
    'if(alias=="*")return true;',
    "if(alias.IsEmpty())return true;",
    "if(FAlias.Length()<len)return false;",
)
require(has_alias, "HasValidAlias wildcard", "alias[i]!='!'", "FAlias[i]!=alias[i]")

get_by_alias = function_body(msg, "void MaxUser_LIST::GetUsersByAlias")
require(get_by_alias, "GetUsersByAlias", "ui->HasValidAlias(alias)", "user->CopyFrom(ui)")

for signature, send_call in (
    ("void MAX_BOT::SendMessageByAlias", "SendMessage(alias,msg);"),
    ("void MAX_BOT::SendPhotoByAlias", "SendPhoto(alias,fn,caption);"),
    ("void MAX_BOT::SendDocByAlias", "SendDoc(alias,fn,caption);"),
):
    body = function_body(bot, signature)
    ordered(
        body,
        signature,
        "UserList->GetUsersByAlias(userlist,alias);",
        "if(userlist->Count)",
        "else if(alias.Length())",
        "if(isdigit(alias[1]))",
        send_call,
    )

alarm = function_body(bot, "void MAX_BOT::OnNewAlarmState")
ordered(alarm, "alarm fan-out", "UserList->GetUsersByAlias(userlist,AlarmAlias);", "for(int i=0;i<userlist->Count;i++)", "SendMessage(user->Id,mess);")

# MAX-specific user/chat addressing must survive copying and persistence.
require(function_body(msg, "void MaxUser::CopyUserFrom"), "CopyUserFrom", "FPeerType=maxPeerUser;")
require(function_body(msg, "void MaxUser::CopyChatFrom"), "CopyChatFrom", "FPeerType=maxPeerChat;")
require(function_body(msg, "void MaxUser::Save"), "MaxUser::Save", 'WriteString(section,"PeerType","chat")')
require(function_body(msg, "void MaxUser::Load"), "MaxUser::Load", 'ReadString(section,"PeerType","user")', "maxPeerChat:maxPeerUser")

peer_type = function_body(bot, "MAX_PEER_TYPE MAX_BOT::GetPeerType")
ordered(peer_type, "GetPeerType", "UserList->Find(id)", "if(user)return user->PeerType;", "return maxPeerUser;")

# Legacy OutCount semantics: increment after queueing, not after delivery confirmation.
for signature, add_call in (
    ("void MAX_BOT::SendMessage", "TaskList.AddSendMsg"),
    ("void MAX_BOT::SendPhoto", "TaskList.AddSendPhoto"),
    ("void MAX_BOT::SendDoc", "TaskList.AddSendDoc"),
):
    ordered(function_body(bot, signature), signature, add_call, "UserList->Find(id)", "user->OutCount++")

# ---------------------------------------------------------------------------
# Task queue remains thread-safe FIFO and preserves PeerType.
get_task = function_body(task, "bool MB_TASK_LIST::Get")
ordered(get_task, "task FIFO", "List->LockList()", "list->Items[0]", "list->Delete(0)", "List->UnlockList()")
for signature in (
    "void MB_TASK_LIST::AddSendMsg",
    "void MB_TASK_LIST::AddSendPhoto",
    "void MB_TASK_LIST::AddSendDoc",
):
    require(function_body(task, signature), signature, "task.PeerType=peerType;", "Put(task);")

# INI compatibility including BotToken migration fallback.
load_bot = function_body(bot, "void MAX_BOT::Load")
require(
    load_bot,
    "MAX_BOT::Load INI",
    'ReadBool(MaxSecSETUP,"Active"',
    'ReadString(MaxSecSETUP,"BotApi",ini.ReadString(MaxSecSETUP,"BotToken","")',
    'ReadInteger(MaxSecSETUP,"PeriodReadMessages"',
    'ReadBool(MaxSecSETUP,"SendAlarms"',
    'ReadBool(MaxSecSETUP,"SendAlarmEnd"',
    'ReadBool(MaxSecSETUP,"OperatorAlarm"',
    'ReadString(MaxSecSETUP,"AlarmAlias"',
    'ReadString(MaxSecSETUP,"RequestAlias"',
    'ReadBool(MaxSecSETUP,"SendMaps"',
    "UserList->Load(&ini);",
)

# Lifecycle remains Telegram-like; MainForm owns callbacks.
ctor = function_body(bot, "MAX_BOT::MAX_BOT")
ordered(ctor, "MAX_BOT lifecycle", "new TMaxBotThread(true)", "Thread->Resume()")
absent(ctor, "MAX_BOT lifecycle", "Thread->OnTaskReadMessages=", "Thread->OnPeriodicReadMessages=", "Thread->OnGetMe=")

# ---------------------------------------------------------------------------
# TLS is fail-closed and uses the actual Id* Indy API from the customer's BCB2007.
require(
    indy,
    "Indy TLS",
    '"certs\\\\max-ca.pem"',
    "RootCertFile=rootCert",
    "sslvrfPeer",
    "VerifyDepth=9",
    "sslvTLSv1_2",
    "TIdSSLVerifyModeSet",
    'StartupError="MAX CA bundle not found: "+rootCert;',
)
absent(indy, "Indy TLS", "sslvSSLv23", "TInSSLVerifyModeSet")
startup_failed = function_body(indy, "bool TMaxIndyTransport::StartupFailed")
require(startup_failed, "Indy TLS fail-closed", "StartupError", "response.StatusCode=0", "response.Error=StartupError")
for signature in (
    "MAX_HTTP_RESPONSE TMaxIndyTransport::Get",
    "MAX_HTTP_RESPONSE TMaxIndyTransport::Post(",
    "MAX_HTTP_RESPONSE TMaxIndyTransport::PostMultipartFile",
):
    body = function_body(indy, signature)
    ordered(body, f"{signature} fail-closed", "MAX_HTTP_RESPONSE blocked;", "if(StartupFailed(blocked))return blocked;", "ApplyHeaders(headers);")

cert_path = ROOT / "certs" / "max-ca.pem"
if not cert_path.is_file():
    errors.append("vendored MAX CA bundle is missing")
else:
    cert_text = cert_path.read_text(encoding="ascii")
    if cert_text.count("-----BEGIN CERTIFICATE-----") != 2:
        errors.append("vendored MAX CA bundle must contain exactly two certificates")

# Multipart upload host must not receive Bot API Authorization headers.
require(client, "MAX multipart security", "MAX_HTTP_HEADERS uploadHeaders;", 'uploadUrl,uploadHeaders,"data",filename')
multipart_step = between(client, "//3. Послать файл multipart/form-data", "//4. Получить token", "multipart step")
absent(multipart_step, "MAX multipart security", "Headers(false)", "Headers(true)")

# attachment.not.ready: only final POST /messages is retried with the same body/payload.
upload_flow = function_body(client, "bool MAX_API_CLIENT::SendUploadedAttachment")
retry_start = upload_flow.find("const unsigned int retryDelayMs[]={500,1000,2000};")
if retry_start < 0:
    errors.append("attachment retry: missing backoff table")
    retry_loop = ""
else:
    retry_loop = upload_flow[retry_start:]

ordered(
    retry_loop,
    "attachment.not.ready retry",
    "const unsigned int retryDelayMs[]={500,1000,2000};",
    "const int maxAttempts=4;",
    "MAX_TEXT body=BuildAttachmentMessageBody",
    "for(int attempt=0;attempt<maxAttempts;attempt++)",
    "Transport->Post(",
    "if(CheckResponse(sent,error))return true;",
    "if(!IsAttachmentNotReady(sent) || attempt==maxAttempts-1)return false;",
    "Transport->SleepMilliseconds(retryDelayMs[attempt]);",
)
absent(retry_loop, "attachment.not.ready retry", "PostMultipartFile", "MaxParseUploadUrl", "MaxParseUploadToken")

if errors:
    print("LANMON/MAX CONTRACT TEST FAILED")
    for error in errors:
        print(" -", error)
    sys.exit(1)

print("LanMon Telegram-parity behavioral contract passed")
