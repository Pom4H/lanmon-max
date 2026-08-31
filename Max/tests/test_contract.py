#!/usr/bin/env python3
"""Behavioral source contract for the legacy VCL layer Linux cannot compile.

Protect semantic ordering and MAX-specific safety properties without pinning the
adapter to obsolete Indy class names or literal comments.
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


def require(text, label, *tokens):
    for token in tokens:
        if token not in text:
            errors.append(f"{label}: missing {token!r}")


def absent(text, label, *tokens):
    for token in tokens:
        if token in text:
            errors.append(f"{label}: forbidden {token!r}")


def ordered(text, label, *tokens):
    pos = -1
    for token in tokens:
        nxt = text.find(token, pos + 1)
        if nxt < 0:
            errors.append(f"{label}: missing ordered token {token!r}")
            return
        pos = nxt


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


bot = read("maxbot.cpp")
msg = read("maxmsg.cpp")
task = read("maxtask.cpp")
indy = read("api/maxindy.cpp")
client = read("api/maxclient.cpp")

# Incoming messages preserve the legacy callback/auth/command order.
on_messages = function_body(bot, "void MAX_BOT::OnMessages")
ordered(
    on_messages,
    "OnMessages order",
    "if(scriptid.IsEmpty())continue;",
    "MaxBot.UserMessageCount++;",
    "OnTgMessage((int)msg->update_id,scriptid,msg->Text);",
    "if(!FlagSendMaps)continue;",
    "UserList->Find(scriptid)",
    "user->HasValidAlias(RequestAlias)",
    "msg->Text.UpperCase().Trim()",
)
require(on_messages, "OnMessages commands", '"SCREEN"', '"MAP"', '"STOP"', '"LOGXLS"', '"ALARM"', '"HELP"')

# Alias semantics and user/chat addressing remain compatible with LanMon config.
has_alias = function_body(msg, "bool MaxUser::HasValidAlias")
ordered(has_alias, "HasValidAlias", "if(!Valid)return false;", 'if(alias=="*")return true;', "if(alias.IsEmpty())return true;")
require(has_alias, "HasValidAlias wildcard", "alias[i]!='!'", "FAlias[i]!=alias[i]")
require(function_body(msg, "void MaxUser::CopyUserFrom"), "CopyUserFrom", "FPeerType=maxPeerUser;")
require(function_body(msg, "void MaxUser::CopyChatFrom"), "CopyChatFrom", "FPeerType=maxPeerChat;")
require(function_body(msg, "void MaxUser::Save"), "MaxUser::Save", 'WriteString(section,"PeerType","chat")')
require(function_body(msg, "void MaxUser::Load"), "MaxUser::Load", 'ReadString(section,"PeerType","user")')

# Task queue stays thread-safe FIFO.
get_task = function_body(task, "bool MB_TASK_LIST::Get")
ordered(get_task, "task FIFO", "List->LockList()", "list->Items[0]", "list->Delete(0)", "List->UnlockList()")

# Current production TLS contract: Eugene uses upgraded Indy 10.6 on BCB2007.
require(
    indy,
    "Indy TLS",
    "TIdHTTP", "TIdSSLIOHandlerSocketOpenSSL", "TIdSSLVerifyModeSet",
    "sslvTLSv1_2", '"certs\\\\max-ca.pem"', "RootCertFile=rootCert",
    "sslvrfPeer", "VerifyDepth=9",
)
startup_failed = function_body(indy, "bool TMaxIndyTransport::StartupFailed")
require(startup_failed, "TLS fail-closed", "StartupError", "response.StatusCode=0", "response.Error=StartupError")

# Multipart upload host must never receive Bot API Authorization headers.
require(client, "multipart security", "MAX_HTTP_HEADERS uploadHeaders;", 'uploadUrl,uploadHeaders,"data",filename')
step3 = between(client, "//3. Послать файл multipart/form-data", "//4. Получить token", "multipart step")
absent(step3, "multipart security", "Headers(false)", "Headers(true)")

# Freshly uploaded files are eventually consistent. Retry only the documented
# attachment.not.ready condition, reusing the same token/body and backing off.
upload_flow = function_body(client, "bool MAX_API_CLIENT::SendUploadedAttachment")
ordered(
    upload_flow,
    "attachment retry",
    "const unsigned int retryDelayMs[]={500,1000,2000};",
    "const int maxAttempts=4;",
    "MAX_TEXT body=BuildAttachmentMessageBody",
    "for(int attempt=0;attempt<maxAttempts;attempt++)",
    "Transport->Post(",
    "if(CheckResponse(sent,error))return true;",
    "if(!IsAttachmentNotReady(sent) || attempt==maxAttempts-1)return false;",
    "Transport->SleepMilliseconds(retryDelayMs[attempt]);",
)
retry_part = upload_flow[upload_flow.find("const unsigned int retryDelayMs[]"):]
absent(retry_part, "attachment retry", "PostMultipartFile", "MaxParseUploadUrl", "MaxParseUploadToken")

# Production transport must perform a real delay; only mock/default transport may no-op.
sleep_body = function_body(indy, "void TMaxIndyTransport::SleepMilliseconds")
require(sleep_body, "production retry sleep", "::Sleep(milliseconds);")

if errors:
    print("LANMON/MAX CONTRACT TEST FAILED")
    for error in errors:
        print(" -", error)
    sys.exit(1)

print("LanMon/MAX behavioral contract passed")
