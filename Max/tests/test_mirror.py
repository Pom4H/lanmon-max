#!/usr/bin/env python3
"""Structural guard for the Telegram-shaped MAX integration.

Keep this test intentionally semantic. It protects the legacy layout and the
actual C++Builder 2007 + upgraded Indy 10.6 environment without pinning comment
counts or obsolete generated-header names.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []


def read(path):
    p = ROOT / path
    if not p.exists():
        errors.append(f"missing file: {path}")
        return ""
    return p.read_text(encoding="utf-8")


def require(path, *tokens):
    text = read(path)
    for token in tokens:
        if token not in text:
            errors.append(f"{path}: missing {token!r}")
    return text


# Production layout stays close to the existing Telegram integration.
for path in (
    "maxbot.h", "maxbot.cpp", "maxmsg.h", "maxmsg.cpp", "maxtask.h", "maxtask.cpp",
    "UFMaxBot.cpp", "UFMaxBot.h", "UFMaxBot.dfm",
    "UFMaxBotApi.cpp", "UFMaxBotApi.h", "UFMaxBotApi.dfm",
    "UFMaxUserEdit.cpp", "UFMaxUserEdit.h", "UFMaxUserEdit.dfm",
    "UFMaxMsg.cpp", "UFMaxMsg.h", "UFMaxMsg.dfm",
    "api/maxcore.h", "api/maxcore.cpp", "api/maxclient.h", "api/maxclient.cpp",
    "api/maxindy.h", "api/maxindy.cpp",
):
    if not (ROOT / path).exists():
        errors.append(f"missing mirrored production file: {path}")

for forbidden in (
    "lanmon_bot.cpp", "lanmon_bot.h", "lanmon_commands.cpp", "lanmon_commands.h",
    "maxsettings.cpp", "maxsettings.h", "maxusers.cpp", "maxusers.h", "maxuser.h",
):
    if (ROOT / forbidden).exists():
        errors.append(f"forbidden alternative architecture file remains: {forbidden}")

require(
    "maxbot.h",
    "class TMaxBotThread", "class MAX_BOT", "MB_TASK_LIST TaskList",
    "void SendMessage(AnsiString id,AnsiString msg);",
    "void OnMessages(MaxMessage_LIST & msglist);",
)
require(
    "maxbot.cpp",
    "while(TaskList.Get(Task))", "OnTgMessage(", "SendMessageByAlias",
    "SendPhotoByAlias", "SendDocByAlias", "SCREEN", "MAP", "STOP", "LOGXLS", "ALARM",
)
require("maxtask.cpp", "List->LockList()", "list->Items[0]", "list->Delete(0)", "List->UnlockList()")
require("maxmsg.cpp", "MaxUser::HasValidAlias", "MaxMessage_LIST::CopyFrom", "PeerType")

# VCL forms depend on MAX implementation rather than Telegram headers directly.
for path in ("UFMaxBot.h", "UFMaxBotApi.h", "UFMaxUserEdit.h"):
    text = require(path, '#include "maxbot.h"')
    if "tgbot.h" in text:
        errors.append(f"{path}: Telegram include remains")

# Protocol/client separation remains explicit.
require("api/maxcore.h", "MAX_PEER_TYPE", "MAX_UPDATES", "MAX_TEXT", "MaxUtf8FromCp1251")
require("api/maxcore.cpp", "class JsonParser", "MaxBuildUpdatesUrl", "MaxParseUpdates")
require("api/maxclient.h", "class IMaxHttpTransport", "class MAX_API_CLIENT", "MAX_HTTP_HEADERS")
require("api/maxclient.cpp", "SendUploadedAttachment", "attachment.not.ready", "uploadHeaders")

# Eugene's real environment is BCB2007 with upgraded Indy 10.6 generated Id* headers.
require(
    "api/maxindy.h",
    "#include <IdHTTP.hpp>", "#include <IdSSLOpenSSL.hpp>",
    "#include <IdMultipartFormData.hpp>", "TIdHTTP * Http;",
    "TIdSSLIOHandlerSocketOpenSSL * Ssl;",
)
require(
    "api/maxindy.cpp",
    "sslvTLSv1_2", "TIdSSLVerifyModeSet", "TIdMultiPartFormDataStream",
    "RootCertFile=rootCert", "SleepMilliseconds",
)

cert = ROOT / "certs" / "max-ca.pem"
if not cert.is_file():
    errors.append("missing certs/max-ca.pem")
elif cert.read_text(encoding="ascii").count("-----BEGIN CERTIFICATE-----") != 2:
    errors.append("certs/max-ca.pem: expected exactly two PEM certificates")

if errors:
    print("MIRROR CHECK FAILED")
    for error in errors:
        print(" -", error)
    sys.exit(1)

print("Telegram-style MAX production structure check passed")
