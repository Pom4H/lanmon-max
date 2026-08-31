#!/usr/bin/env python3
"""Guard MAX retry semantics across Indy HTTP protocol exceptions.

Indy raises EIdHTTPProtocolException for HTTP 4xx/5xx. MAX intentionally uses
HTTP 400 + JSON code attachment.not.ready while a freshly uploaded file is
still processing, so the transport must preserve ErrorCode/ErrorMessage as an
HTTP response instead of converting it into a transport failure.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
indy = (ROOT / "api/maxindy.cpp").read_text(encoding="utf-8")
client = (ROOT / "api/maxclient.cpp").read_text(encoding="utf-8")
errors = []


def require(text, label, token, count=1):
    actual = text.count(token)
    if actual < count:
        errors.append(f"{label}: expected {token!r} at least {count} time(s), got {actual}")


def absent(text, label, token):
    if token in text:
        errors.append(f"{label}: forbidden {token!r}")


# All three Indy operations must distinguish HTTP protocol failures from
# network/runtime exceptions. This is especially important for POST /messages.
require(indy, "Indy HTTP exception", "catch(EIdHTTPProtocolException & E)", 3)
require(indy, "Indy HTTP status", "protocolStatus=E.ErrorCode;", 3)
require(indy, "Indy HTTP body", "protocolBody=E.ErrorMessage;", 3)
require(indy, "Indy normalized response", "ApplyHttpProtocolError(r,protocolStatus,protocolBody);", 3)
require(indy, "normalized HTTP response", 'response.Error="";')

# Production backoff must actually sleep; the interface default is intentionally
# a no-op for deterministic Linux mocks.
require(indy, "production backoff", "void TMaxIndyTransport::SleepMilliseconds(unsigned int milliseconds)")
require(indy, "production backoff", "::Sleep(milliseconds);")

# Client must continue retrying only the documented transient MAX condition.
require(client, "attachment retry predicate", "static bool IsAttachmentNotReady")
require(client, "attachment retry predicate", 'ClientTextContains(response.Body,"attachment.not.ready")')
require(client, "attachment retry loop", "if(!IsAttachmentNotReady(sent) || attempt==maxAttempts-1)return false;")
absent(client, "attachment retry loop", "if(attempt==maxAttempts-1)\n        {\n            return false;")

if errors:
    print("INDY/MAX PROTOCOL ERROR CHECK FAILED")
    for error in errors:
        print(" -", error)
    sys.exit(1)

print("Indy HTTP protocol errors preserve MAX retry semantics")
