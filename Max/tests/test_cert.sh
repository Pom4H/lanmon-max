#!/usr/bin/env bash
set -euo pipefail

# Проверяем именно тот bundle, который production Indy ищет как certs/max-ca.pem.
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUNDLE="$ROOT_DIR/certs/max-ca.pem"

ROOT_SHA256="D26D2D0231B7C39F92CC738512BA54103519E4405D68B5BD703E9788CA8ECF31"
SUB_SHA256="BBBDE2103E790B999EC62BD03CF625A5A2E7C316E10AFE6A490EEDEAD8B3FD9B"

fail() {
  echo "MAX CA TEST FAILED: $*" >&2
  exit 1
}

[[ -f "$BUNDLE" ]] || fail "missing $BUNDLE"

COUNT="$(grep -c '^-----BEGIN CERTIFICATE-----$' "$BUNDLE")"
[[ "$COUNT" == "2" ]] || fail "expected 2 certificates, found $COUNT"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# Bundle хранится Sub CA -> Root CA. Разбиваем его, чтобы проверить каждый сертификат отдельно.
awk -v dir="$TMP" '
  /^-----BEGIN CERTIFICATE-----$/ { n++ }
  n > 0 { print > (dir "/cert-" n ".pem") }
' "$BUNDLE"

SUB="$TMP/cert-1.pem"
ROOT="$TMP/cert-2.pem"

fingerprint() {
  openssl x509 -in "$1" -outform DER \
    | sha256sum \
    | awk '{print toupper($1)}'
}

[[ "$(fingerprint "$SUB")" == "$SUB_SHA256" ]] \
  || fail "Russian Trusted Sub CA fingerprint mismatch"
[[ "$(fingerprint "$ROOT")" == "$ROOT_SHA256" ]] \
  || fail "Russian Trusted Root CA fingerprint mismatch"

openssl x509 -in "$SUB" -noout -subject | grep -q 'Russian Trusted Sub CA' \
  || fail "unexpected Sub CA subject"
openssl x509 -in "$ROOT" -noout -subject | grep -q 'Russian Trusted Root CA' \
  || fail "unexpected Root CA subject"

# Не принимаем истёкший сертификат даже если fingerprint прежний.
openssl x509 -in "$SUB" -noout -checkend 86400 >/dev/null \
  || fail "Russian Trusted Sub CA expires within 24h or is expired"
openssl x509 -in "$ROOT" -noout -checkend 86400 >/dev/null \
  || fail "Russian Trusted Root CA expires within 24h or is expired"

# Проверяем криптографическую цепочку Sub CA -> Root CA.
openssl verify -CAfile "$ROOT" "$SUB" >/dev/null \
  || fail "Sub CA is not signed by vendored Root CA"

# Проверяем, что OpenSSL способен прочитать bundle целиком как CAfile.
openssl verify -CAfile "$BUNDLE" "$SUB" >/dev/null \
  || fail "combined max-ca.pem is not a usable OpenSSL CAfile"

echo "MAX Ministry CA bundle verified"
echo "  Sub : $SUB_SHA256"
echo "  Root: $ROOT_SHA256"
