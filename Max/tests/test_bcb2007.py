#!/usr/bin/env python3
"""Static compatibility guard for the actual C++Builder 2007/VCL toolchain.

Linux CI cannot run bcc32, so this test protects the source-level contract that
matters for the customer build: no STL in the Borland production path and only
the In* Indy classes that exist in the supplied BCB2007 headers.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def require(text, label, *tokens):
    for token in tokens:
        if token not in text:
            errors.append(f"{label}: missing {token!r}")


def absent(text, label, *tokens):
    for token in tokens:
        if token in text:
            errors.append(f"{label}: forbidden {token!r}")


def first_borland_branch(text, label):
    start = text.find("#ifdef __BORLANDC__")
    if start < 0:
        errors.append(f"{label}: missing __BORLANDC__ branch")
        return ""
    end = text.find("#else", start)
    if end < 0:
        errors.append(f"{label}: missing #else after __BORLANDC__")
        return ""
    return text[start:end]


def function_body(text, signature, label):
    pos = text.find(signature)
    if pos < 0:
        errors.append(f"{label}: missing function {signature!r}")
        return ""
    start = text.find("{", pos)
    if start < 0:
        errors.append(f"{label}: missing function body")
        return ""
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1:i]
    errors.append(f"{label}: unclosed function body")
    return ""


core_h = read("api/maxcore.h")
client_h = read("api/maxclient.h")
core_cpp = read("api/maxcore.cpp")
indy_h = read("api/maxindy.h")
indy_cpp = read("api/maxindy.cpp")
bot_cpp = read("maxbot.cpp")
msg_h = read("maxmsg.h")
msg_cpp = read("maxmsg.cpp")

# Public DTO/string/container types must resolve to VCL in Borland builds.
core_borland = first_borland_branch(core_h, "maxcore.h")
require(core_borland, "maxcore.h Borland", "typedef AnsiString MAX_TEXT;")
absent(core_borland, "maxcore.h Borland", "<string>", "<vector>", "<map>", "std::")
require(core_h, "maxcore.h VCL messages", "class MAX_MESSAGE_ARRAY", "TList * List;")

client_borland = first_borland_branch(client_h, "maxclient.h")
require(client_borland, "maxclient.h Borland", "class MAX_HTTP_HEADERS", "TStringList * Items;")
absent(client_borland, "maxclient.h Borland", "<string>", "<vector>", "<map>", "std::")

# Portable containers are allowed only behind the non-Borland branch for CI.
require(core_cpp, "maxcore.cpp portable guard", "#ifndef __BORLANDC__\n#include <vector>\n#endif")

# Upper VCL layer must not reintroduce STL after the API was made VCL-native.
for path, text in (("maxbot.cpp", bot_cpp), ("maxmsg.h", msg_h), ("maxmsg.cpp", msg_cpp)):
    absent(text, path, "#include <string>", "#include <vector>", "#include <map>", "std::string", "std::vector", "std::map")
require(bot_cpp, "maxbot.cpp", "MAX_TEXT error;", "static MAX_TEXT MaxUtf8")
require(msg_cpp, "maxmsg.cpp", "wchar_t * w=new wchar_t[wn];", "char * a=new char[an];")

# Void worker functions must stay valid C++ after compatibility refactors.
for signature in ("void TMaxBotThread::DoSendPhoto", "void TMaxBotThread::DoSendDoc"):
    body = function_body(bot_cpp, signature, signature)
    absent(body, signature, "return false;", "return true;")

# The customer's BCB2007 headers expose In*, not Id*, classes.
require(
    indy_h,
    "maxindy.h BCB2007 Indy",
    "#include <InHTTP.hpp>",
    "#include <InSSLOpenSSL.hpp>",
    "#include <InMultipartFormData.hpp>",
    "TInHTTP * Http;",
    "TInSSLIOHandlerSocketOpenSSL * Ssl;",
)
absent(indy_h, "maxindy.h BCB2007 Indy", "<IdHTTP.hpp>", "<IdSSLOpenSSL.hpp>", "<IdMultipartFormData.hpp>", "TIdHTTP", "TIdSSLIOHandlerSocketOpenSSL")
require(indy_cpp, "maxindy.cpp BCB2007 Indy", "TInMultipartFormDataStream", "TInSSLVerifyModeSet", "sslvSSLv23")
absent(indy_cpp, "maxindy.cpp BCB2007 Indy", "TIdMultiPartFormDataStream", "TIdSSLVerifyModeSet", "sslvTLSv1_2")

# TLS remains fail-closed even though BCB2007 selects TLS 1.2 through negotiation.
require(indy_cpp, "maxindy.cpp TLS", '"certs\\\\max-ca.pem"', "RootCertFile=rootCert", "sslvrfPeer", "VerifyDepth=9", 'StartupError="MAX CA bundle not found: "+rootCert;')

if errors:
    print("BCB2007 COMPATIBILITY CHECK FAILED")
    for error in errors:
        print(" -", error)
    sys.exit(1)

print("C++Builder 2007 VCL/Indy compatibility source check passed")
