#!/usr/bin/env python3
"""Behavioral source contract for the VCL layer that Linux cannot compile.

This is intentionally stricter than test_mirror.py.  test_mirror.py checks that the
MAX tree still looks like Telegram; this file protects the *order and semantics* of
the legacy Telegram contract used by LanMon.
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


def require(body, label, *tokens):
    for token in tokens:
        if token not in body:
            errors.append(f"{label}: missing {token!r}")


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

# SCREEN x tries one monitor first and only then falls back to all desktops.
require_order(
    on_messages,
    "SCREEN fallback",
    "GetMonitorScreenshot(screenindex-1,fn)",
    "DesktopScreenshot(fn)",
)

# MAP x preserves the legacy 1-based user index -> 0-based internal index.
require_order(
    on_messages,
    "MAP pipeline",
    "CreateMapScreenshot(mapindex-1,bmpfn)",
    "Bmp2Png(bmpfn,pngfn)",
    "SendPhoto(id,pngfn",
)

# STOP acts first, then confirms to the requester.
require_order(
    on_messages,
    "STOP behavior",
    "CloseAvariaForm();",
    'SendMessage(id,U8("Команда СТОП выполнена"));',
)

# Export commands must create/export data before queueing the document.
require_order(
    on_messages,
    "LOGXLS behavior",
    "LogView->ExportToXls(fn);",
    "SendDoc(id,fn",
)
require_order(
    on_messages,
    "LOG behavior",
    "LogView->ExportToHtml(fn);",
    "SendDoc(id,fn",
)
require_order(
    on_messages,
    "ALARM behavior",
    "CreateAlarmsPdf();",
    "if(fn.Length())",
    "SendDoc(id,fn",
)

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

# Alarm fan-out is just Alias matching + normal SendMessage routing.
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

# ---------------------------------------------------------------------------
# Lifecycle remains Telegram-like: suspended thread is configured, then resumed.
ctor = function_body(bot, "MAX_BOT::MAX_BOT")
require_order(ctor, "MAX_BOT lifecycle", "new TMaxBotThread(true)", "Thread->Resume()")
# The bot object must not steal MainForm callbacks; integration wires them externally.
for forbidden in (
    "Thread->OnTaskReadMessages=",
    "Thread->OnPeriodicReadMessages=",
    "Thread->OnGetMe=",
):
    if forbidden in ctor:
        errors.append(f"MAX_BOT lifecycle: self-wired callback returned: {forbidden}")

# SSL trust is an integration contract, not just README text.
require(
    indy,
    "Indy TLS",
    '"certs\\\\max-ca.pem"',
    "RootCertFile=rootCert",
    "sslvrfPeer",
    "VerifyDepth=9",
    "sslvTLSv1_2",
)

if errors:
    print("LANMON/MAX CONTRACT TEST FAILED")
    for error in errors:
        print(" -", error)
    sys.exit(1)

print("LanMon Telegram-parity behavioral contract passed")
