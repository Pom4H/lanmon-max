#!/usr/bin/env python3
"""Behavioral source contract for the VCL layer that Linux cannot compile.

`test_mirror.py` checks that the MAX tree still looks like Telegram.  This file
protects the *order and semantics* of the legacy Telegram contract used by
LanMon, plus the MAX-specific addressing/TLS invariants.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def function_body(text, signature):
    """Return a C/C++ function body using simple balanced-brace scanning."""
    pos = text.find(signature)
    if pos < 0:
        errors.append(f"missing function: {signature}")
        return ""
    start = text.find("{", pos)
    if start < 0:
        errors.append(f"missing body for: {signature}")
        return ""
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1:i]
    errors.append(f"unclosed body for: {signature}")
    return ""


def section(text, start_token, end_token, label):
    """Slice one command branch so repeated SendDoc/SendPhoto calls do not alias."""
    start = text.find(start_token)
    if start < 0:
        errors.append(f"{label}: missing start {start_token!r}")
        return ""
    end = text.find(end_token, start + len(start_token))
    if end < 0:
        errors.append(f"{label}: missing end {end_token!r}")
        return ""
    return text[start:end]


def require(body, label, *tokens):
    for token in tokens:
        if token not in body:
            errors.append(f"{label}: missing {token!r}")


def require_absent(body, label, *tokens):
    for token in tokens:
        if token in body:
            errors.append(f"{label}: forbidden token present {token!r}")


def require_order(body, label, *tokens):
    positions = []
    for token in tokens:
        pos = body.find(token)
        if pos < 0:
            errors.append(f"{label}: missing ordered token {token!r}")
            return
        positions.append(pos)
    if positions != sorted(positions):
        errors.append(f"{label}: wrong operation order: {' -> '.join(tokens)}")


bot = read("maxbot.cpp")
msg = read("maxmsg.cpp")
task = read("maxtask.cpp")
indy = read("api/maxindy.cpp")
client = read("api/maxclient.cpp")

# ---------------------------------------------------------------------------
# Incoming-message contract copied from TELEGRAM_BOT::OnMessages.
on_messages = function_body(bot, "void MAX_BOT::OnMessages")
require_order(
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

# MAX first tries the chat id, then falls back to sender id.
require_order(
    on_messages,
    "OnMessages user lookup",
    "UserList->Find(scriptid)",
    "UserList->Find(msg->From.Id)",
    "if(!user)continue;",
)

# Built-in command set must remain compatible with Telegram.
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

screen = section(on_messages, 'if(text.SubString(1,6)=="SCREEN"', 'else if(text.SubString(1,3)=="MAP"', "SCREEN")
map_cmd = section(on_messages, 'else if(text.SubString(1,3)=="MAP"', 'else if(text.SubString(1,4)=="STOP"', "MAP")
stop = section(on_messages, 'else if(text.SubString(1,4)=="STOP"', 'else if(text.SubString(1,6)=="LOGXLS")', "STOP")
logxls = section(on_messages, 'else if(text.SubString(1,6)=="LOGXLS")', 'else if(text.SubString(1,3)=="LOG"', "LOGXLS")
log = section(on_messages, 'else if(text.SubString(1,3)=="LOG"', 'else if(text.SubString(1,5)=="ALARM"', "LOG")
alarm_cmd = section(on_messages, 'else if(text.SubString(1,5)=="ALARM"', 'else if(text.SubString(1,4)=="HELP"', "ALARM")
help_cmd = on_messages[on_messages.find('else if(text.SubString(1,4)=="HELP"'):]

# SCREEN x uses 1-based user numbering and falls back to the whole desktop.
require_order(
    screen,
    "SCREEN fallback",
    "screenindex=atoi",
    "GetMonitorScreenshot(screenindex-1,fn)",
    "DesktopScreenshot(fn)",
)
require(screen, "SCREEN responses", 'SendPhoto(id,fn,U8("Экран ")+IntToStr(screenindex))', 'SendPhoto(id,fn,U8("Экран"))')

# MAP x preserves 1-based user index -> 0-based internal index, BMP -> PNG -> send.
require_order(
    map_cmd,
    "MAP pipeline",
    "if(mapindex)",
    "CreateMapScreenshot(mapindex-1,bmpfn)",
    "Bmp2Png(bmpfn,pngfn)",
    "SendPhoto(id,pngfn",
)

# STOP acts first, then confirms to the requester.
require_order(stop, "STOP behavior", "CloseAvariaForm();", 'SendMessage(id,U8("Команда СТОП выполнена"));')

# Export commands must create/export data before queueing the document.
require_order(logxls, "LOGXLS behavior", "DeleteFile(fn);", "LogView->ExportToXls(fn);", "SendDoc(id,fn")
require_order(log, "LOG behavior", "DeleteFile(fn);", "LogView->ExportToHtml(fn);", "SendDoc(id,fn")
require_order(alarm_cmd, "ALARM behavior", "CreateAlarmsPdf();", "if(fn.Length())", "SendDoc(id,fn")
require(help_cmd, "HELP behavior", 'SendMessage(id,mess);', '"?? - дополнительные запросы"')

# ---------------------------------------------------------------------------
# Alias semantics are part of the public FastScript/LanMon contract.
has_alias = function_body(msg, "bool MaxUser::HasValidAlias")
require_order(
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

# Every Send*ByAlias keeps the Telegram direct-numeric-id fallback.
for signature, send_call in (
    ("void MAX_BOT::SendMessageByAlias", "SendMessage(alias,msg);"),
    ("void MAX_BOT::SendPhotoByAlias", "SendPhoto(alias,fn,caption);"),
    ("void MAX_BOT::SendDocByAlias", "SendDoc(alias,fn,caption);"),
):
    body = function_body(bot, signature)
    require_order(
        body,
        signature,
        "UserList->GetUsersByAlias(userlist,alias);",
        "if(userlist->Count)",
        "else if(alias.Length())",
        "if(isdigit(alias[1]))",
        send_call,
    )

# Alarm fan-out is Alias matching + normal SendMessage routing.
alarm = function_body(bot, "void MAX_BOT::OnNewAlarmState")
require_order(
    alarm,
    "alarm fan-out",
    "UserList->GetUsersByAlias(userlist,AlarmAlias);",
    "for(int i=0;i<userlist->Count;i++)",
    "SendMessage(user->Id,mess);",
)

# ---------------------------------------------------------------------------
# MAX-only addressing must survive persistence/copying and task queueing.
copy_user = function_body(msg, "void MaxUser::CopyUserFrom")
require(copy_user, "CopyUserFrom", "FPeerType=maxPeerUser;")
copy_chat = function_body(msg, "void MaxUser::CopyChatFrom")
require(copy_chat, "CopyChatFrom", "FPeerType=maxPeerChat;")

save_user = function_body(msg, "void MaxUser::Save")
require(save_user, "MaxUser::Save", 'WriteString(section,"PeerType","chat")')
load_user = function_body(msg, "void MaxUser::Load")
require(load_user, "MaxUser::Load", 'ReadString(section,"PeerType","user")', "maxPeerChat:maxPeerUser")

# Unknown direct IDs default to user_id, exactly as documented for numeric alias fallback.
peer_type = function_body(bot, "MAX_PEER_TYPE MAX_BOT::GetPeerType")
require_order(peer_type, "GetPeerType", "UserList->Find(id)", "if(user)return user->PeerType;", "return maxPeerUser;")

# Queueing outgoing operations preserves original Telegram counter semantics.
for signature, add_call in (
    ("void MAX_BOT::SendMessage", "TaskList.AddSendMsg"),
    ("void MAX_BOT::SendPhoto", "TaskList.AddSendPhoto"),
    ("void MAX_BOT::SendDoc", "TaskList.AddSendDoc"),
):
    body = function_body(bot, signature)
    require_order(body, signature, add_call, "UserList->Find(id)", "user->OutCount++")

# Task list is thread-safe FIFO and carries peer type for all outgoing payloads.
get_task = function_body(task, "bool MB_TASK_LIST::Get")
require_order(get_task, "task FIFO", "List->LockList()", "list->Items[0]", "list->Delete(0)", "List->UnlockList()")
for signature in (
    "void MB_TASK_LIST::AddSendMsg",
    "void MB_TASK_LIST::AddSendPhoto",
    "void MB_TASK_LIST::AddSendDoc",
):
    body = function_body(task, signature)
    require(body, signature, "task.PeerType=peerType;", "Put(task);")

# INI keys remain compatible and BotToken is accepted as a migration fallback.
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

# ---------------------------------------------------------------------------
# Lifecycle remains Telegram-like: suspended thread is configured, then resumed.
ctor = function_body(bot, "MAX_BOT::MAX_BOT")
require_order(ctor, "MAX_BOT lifecycle", "new TMaxBotThread(true)", "Thread->Resume()")
# The bot object must not steal MainForm callbacks; integration wires them externally.
require_absent(
    ctor,
    "MAX_BOT lifecycle",
    "Thread->OnTaskReadMessages=",
    "Thread->OnPeriodicReadMessages=",
    "Thread->OnGetMe=",
)

# SSL trust is an executable integration contract, not just README text.
require(
    indy,
    "Indy TLS",
    '"certs\\\\max-ca.pem"',
    "RootCertFile=rootCert",
    "sslvrfPeer",
    "VerifyDepth=9",
    "sslvTLSv1_2",
    'StartupError="MAX CA bundle not found: "+rootCert;',
)

# Missing CA must fail closed before any network operation starts.
startup_failed = function_body(indy, "bool TMaxIndyTransport::StartupFailed")
require(startup_failed, "Indy TLS fail-closed", "StartupError", "response.StatusCode=0", "response.Error=StartupError.c_str()")
for signature, first_network_token in (
    ("MAX_HTTP_RESPONSE TMaxIndyTransport::Get", "ApplyHeaders(headers);"),
    ("MAX_HTTP_RESPONSE TMaxIndyTransport::Post(", "ApplyHeaders(headers);"),
    ("MAX_HTTP_RESPONSE TMaxIndyTransport::PostMultipartFile", "ApplyHeaders(headers);"),
):
    body = function_body(indy, signature)
    require_order(
        body,
        f"{signature} fail-closed",
        "MAX_HTTP_RESPONSE blocked;",
        "if(StartupFailed(blocked))return blocked;",
        first_network_token,
    )

# Multipart upload-host is deliberately kept separate from Bot API authorization.
require(
    client,
    "MAX multipart security",
    "std::map<std::string,std::string> uploadHeaders;",
    "uploadUrl,uploadHeaders,\"data\",filename",
)
require_absent(
    section(client, "//3. Послать файл multipart/form-data", "//4. Получить token", "multipart step"),
    "MAX multipart security",
    "Headers(false)",
    "Headers(true)",
)

# attachment.not.ready retry must reuse the same token/body and never repeat multipart upload.
upload_flow = function_body(client, "bool MAX_API_CLIENT::SendUploadedAttachment")
require_order(
    upload_flow,
    "attachment.not.ready retry",
    "MaxParseUploadToken(uploaded.Body,token,error)",
    "const unsigned int retryDelayMs[]={500,1000,2000};",
    "const int maxAttempts=4;",
    "std::string body=BuildAttachmentMessageBody",
    "for(int attempt=0;attempt<maxAttempts;attempt++)",
    "Transport->Post(",
    "if(CheckResponse(sent,error))return true;",
    "if(!IsAttachmentNotReady(sent) || attempt==maxAttempts-1)return false;",
    "Transport->SleepMilliseconds(retryDelayMs[attempt]);",
)
retry_part = section(upload_flow, "const unsigned int retryDelayMs[]", "return false;", "attachment retry loop")
require_absent(
    retry_part,
    "attachment.not.ready retry",
    "PostMultipartFile",
    "MaxParseUploadUrl",
    "MaxParseUploadToken",
)

if errors:
    print("LANMON/MAX CONTRACT TEST FAILED")
    for error in errors:
        print(" -", error)
    sys.exit(1)

print("LanMon Telegram-parity behavioral contract passed")
