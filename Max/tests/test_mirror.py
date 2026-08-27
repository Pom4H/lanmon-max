#!/usr/bin/env python3
"""Current source-contract guard for the MAX adapter.

Behavior is tested by the compiled C++98 unit tests. This guard only protects
important source-level integration invariants that are easy to regress while
keeping the C++Builder 2007 mirror maintainable.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []


def read(path):
    p = ROOT / path
    if not p.is_file():
        errors.append(f"missing file: {path}")
        return ""
    return p.read_text(encoding="utf-8")


def require(path, *tokens):
    value = read(path)
    for token in tokens:
        if token not in value:
            errors.append(f"{path}: missing {token!r}")
    return value


def forbid(path, *tokens):
    value = read(path)
    for token in tokens:
        if token in value:
            errors.append(f"{path}: forbidden stale token {token!r}")
    return value


# Production layout: keep the Telegram-shaped LanMon integration, not a second app.
for required in (
    "maxbot.h", "maxbot.cpp", "maxmsg.h", "maxmsg.cpp", "maxtask.h", "maxtask.cpp",
    "UFMaxBot.cpp", "UFMaxBot.h", "UFMaxBot.dfm",
    "UFMaxBotApi.cpp", "UFMaxBotApi.h", "UFMaxBotApi.dfm",
    "UFMaxUserEdit.cpp", "UFMaxUserEdit.h", "UFMaxUserEdit.dfm",
    "api/maxcore.h", "api/maxcore.cpp", "api/maxclient.h", "api/maxclient.cpp",
    "api/maxindy.h", "api/maxindy.cpp",
):
    if not (ROOT / required).is_file():
        errors.append(f"missing production file: {required}")

for forbidden in (
    "lanmon_bot.cpp", "lanmon_bot.h", "lanmon_commands.cpp", "lanmon_commands.h",
    "maxsettings.cpp", "maxsettings.h", "maxusers.cpp", "maxusers.h", "maxuser.h",
):
    if (ROOT / forbidden).exists():
        errors.append(f"forbidden alternative architecture file remains: {forbidden}")

# Actual customer toolchain: BCB2007 installation uses Id* Indy classes.
require(
    "api/maxindy.h",
    "#include <IdHTTP.hpp>",
    "#include <IdSSLOpenSSL.hpp>",
    "#include <IdMultipartFormData.hpp>",
    "TIdHTTP * Http;",
    "TIdSSLIOHandlerSocketOpenSSL * Ssl;",
)
require(
    "api/maxindy.cpp",
    "TIdMultiPartFormDataStream",
    "TIdSSLVerifyModeSet",
    "sslvTLSv1_2",
    "PostMultipartFile",
)
forbid(
    "api/maxindy.h",
    "<InHTTP.hpp>", "<InSSLOpenSSL.hpp>", "<InMultipartFormData.hpp>",
    "TInHTTP", "TInSSLIOHandlerSocketOpenSSL",
)
forbid(
    "api/maxindy.cpp",
    "TInMultipartFormDataStream", "TInSSLVerifyModeSet", "sslvSSLv23",
)

# Image upload contract. This is what the production client must keep doing:
# 1. obtain upload URL from MAX API;
# 2. upload multipart data to that URL without leaking the bot token;
# 3. accept both documented top-level token and live photos-map responses;
# 4. send an image attachment using the parsed payload.
require(
    "api/maxclient.cpp",
    "SendUploadedAttachment",
    '"/uploads?type="',
    "PostMultipartFile",
    "BuildImageUploadPayload",
    '"photos"',
    'attachmentType=="image"',
    "BuildAttachmentMessageBody",
)

# The compiled regression must exercise the full image flow rather than merely
# checking that implementation strings exist.
require(
    "tests/test_image_upload.cpp",
    '"https://platform-api2.max.ru/uploads?type=image"',
    '"Authorization"',
    '"secret-token"',
    '"https://iu.oneme.ru/uploadImage?apiToken=upload-ticket&photoIds=1"',
    'headers.find("Authorization")==headers.end()',
    '"data"',
    '"map.png"',
    '\\"photos\\"',
    '\\"token\\"',
    '"https://platform-api2.max.ru/messages?chat_id=-777"',
    '\\"type\\":\\"image\\"',
    '\\"payload\\":{\\"photos\\"',
)

# The old documented image response is still covered by the general client test,
# so support for the live photos-map response must not remove token compatibility.
require(
    "tests/test_maxclient.cpp",
    '"image-token"',
    "SendImage",
    "PostMultipartFile",
)

# MaxUser is edited by value (MaxUser copy;) and therefore must remain stack-allocatable.
msg_h = require("maxmsg.h", "class MaxUser\n{")
if "class MaxUser : public TObject" in msg_h or "__published:" in msg_h:
    errors.append("maxmsg.h: MaxUser must remain a plain stack-allocatable value object")

if errors:
    print("CURRENT MAX CONTRACT CHECK FAILED")
    for error in errors:
        print(" -", error)
    sys.exit(1)

print("Current MAX source/image-upload contract check passed")
