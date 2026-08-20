#!/usr/bin/env python3
"""Structural mirror guard for the Telegram-shaped MAX source tree.

This test prevents the repository from drifting back to a second application
architecture. It checks production structure and documentation density, while
`test_contract.py` protects behavioral order/semantics and `test_bcb2007.py`
protects the actual legacy compiler/VCL compatibility contract.
"""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
errors = []


def text(path):
    p = root / path
    if not p.exists():
        errors.append(f"missing file: {path}")
        return ""
    return p.read_text(encoding="utf-8")


def need(path, *tokens):
    value = text(path)
    for token in tokens:
        if token not in value:
            errors.append(f"{path}: missing {token!r}")
    return value


def comment_count(path, prefix="//"):
    value = text(path)
    return sum(1 for line in value.splitlines() if line.strip().startswith(prefix))


def require_comment_count(path, minimum, prefix="//"):
    count = comment_count(path, prefix)
    if prefix == "#":
        value = text(path)
        count = sum(
            1 for line in value.splitlines()
            if line.strip().startswith("#") and not line.strip().startswith("#!")
        )
    if count < minimum:
        errors.append(f"{path}: only {count} comment lines, expected at least {minimum}")


# ---------------------------------------------------------------------------
# Top-level production layout mirrors Telegram/tgbot,tgmsg,tgtask + VCL forms.
for required in (
    "maxbot.h", "maxbot.cpp",
    "maxmsg.h", "maxmsg.cpp",
    "maxtask.h", "maxtask.cpp",
    "UFMaxBot.cpp", "UFMaxBot.h", "UFMaxBot.dfm",
    "UFMaxBotApi.cpp", "UFMaxBotApi.h", "UFMaxBotApi.dfm",
    "UFMaxUserEdit.cpp", "UFMaxUserEdit.h", "UFMaxUserEdit.dfm",
    "UFMaxMsg.cpp", "UFMaxMsg.h", "UFMaxMsg.dfm",
    "api/maxcore.h", "api/maxcore.cpp",
    "api/maxclient.h", "api/maxclient.cpp",
    "api/maxindy.h", "api/maxindy.cpp",
):
    if not (root / required).exists():
        errors.append(f"missing mirrored production file: {required}")

# The previous alternative application layer must not return.
for forbidden in (
    "lanmon_bot.cpp", "lanmon_bot.h",
    "lanmon_commands.cpp", "lanmon_commands.h",
    "maxsettings.cpp", "maxsettings.h",
    "maxusers.cpp", "maxusers.h", "maxuser.h",
):
    if (root / forbidden).exists():
        errors.append(f"forbidden alternative architecture file remains: {forbidden}")

# ---------------------------------------------------------------------------
# Main Telegram-shaped backend.
h = need(
    "maxbot.h",
    "class TMaxBotThread", "class MAX_BOT", "MB_TASK_LIST TaskList",
    "void SendMessage(AnsiString id,AnsiString msg);",
    "void OnMessages(MaxMessage_LIST & msglist);",
    "//Состояние потока работы с MAX", "//Поток для работы с MAX",
    "//Класс для работы с MAX Bot", "//Добавление заданий потоку",
)
need(
    "maxbot.cpp",
    "while(TaskList.Get(Task))", "OnTgMessage(",
    "SCREEN", "ЭКРАН", "MAP", "КАРТА", "STOP", "СТОП",
    "LOGXLS", "ЖУРНАЛ", "ALARM", "ТРЕВОГ", "HELP",
    "SendMessageByAlias", "SendPhotoByAlias", "SendDocByAlias",
    "TFastIniFile ini", "UserList->Load(&ini)", "UserList->Save(&ini)",
)

need(
    "maxtask.h",
    "struct MB_TASK", "class MB_TASK_LIST",
    "//Типы заданий", "//Задание для потока", "//Список заданий для потока",
)
need(
    "maxtask.cpp",
    "MB_TASK_LIST::AddSendMsg", "MB_TASK_LIST::AddSendPhoto", "MB_TASK_LIST::AddSendDoc",
    "List->LockList()", "list->Items[0]", "list->Delete(0)", "List->UnlockList()",
)

need(
    "maxmsg.h",
    "class MaxMessage", "class MaxUser", "class MaxUser_LIST", "class MaxBotInfo",
    "//От какого пользователя пришло сообщение", "//Объект Chat", "//Сообщение MAX",
    "//Список пользователей, используемых в программе",
)
need(
    "maxmsg.cpp",
    "MaxUser::HasValidAlias", "MaxUser_LIST::GetUsersByAlias", "MaxMessage_LIST::CopyFrom",
    "MaxUser::Save", "MaxUser::Load", "PeerType",
)

# ---------------------------------------------------------------------------
# VCL files must depend on MAX mirror, never Telegram headers directly.
for ui_header in ("UFMaxBot.h", "UFMaxBotApi.h", "UFMaxUserEdit.h"):
    value = text(ui_header)
    if '#include "maxbot.h"' not in value:
        errors.append(f"{ui_header}: must include maxbot.h")
    if "tgbot.h" in value:
        errors.append(f"{ui_header}: Telegram include remains")

need("UFMaxBot.cpp", "FormMaxBot", "MaxBot", "OnTaskReadMessages", "OnGetMe")
need("UFMaxBotApi.cpp", "MaxBot.GetMe", "OnGetMe")
need("UFMaxUserEdit.cpp", "PeerType", "maxPeerUser", "maxPeerChat")
need("UFMaxMsg.cpp", "TFormMaxMsg")

# ---------------------------------------------------------------------------
# MAX-specific protocol layer is allowed only below api/ and must explain why it exists.
need(
    "api/maxcore.h",
    "MAX_PEER_TYPE", "MAX_UPDATES", "MaxUtf8FromCp1251", "MAX_TEXT",
    "//Тип адресата MAX: личный пользователь или чат",
)
need(
    "api/maxcore.cpp",
    "class JsonParser", "MaxBuildUpdatesUrl", "MaxParseUpdates",
    "//Минимальный JSON parser для C++98.",
)
need(
    "api/maxclient.h",
    "class IMaxHttpTransport", "class MAX_API_CLIENT", "SleepMilliseconds", "MAX_HTTP_HEADERS",
    "//Клиент MAX Bot API, независимый от конкретной HTTP-библиотеки",
)
need(
    "api/maxclient.cpp",
    "SendUploadedAttachment", "attachment.not.ready", "uploadHeaders",
    "//Общий MAX upload flow для изображения и документа",
)
need(
    "api/maxindy.h",
    "class TMaxIndyTransport", "StartupError", "StartupFailed",
    "TInHTTP", "TInSSLIOHandlerSocketOpenSSL",
    "//Production HTTP/HTTPS transport MAX для старого C++Builder/Indy",
)
need(
    "api/maxindy.cpp",
    '"certs\\\\max-ca.pem"', "RootCertFile", "sslvrfPeer", "sslvSSLv23",
    "TInMultipartFormDataStream", "MAX CA bundle not found", "SleepMilliseconds",
)

# Vendored CA is now part of the deliverable rather than an undocumented external file.
cert = root / "certs" / "max-ca.pem"
if not cert.is_file():
    errors.append("missing certs/max-ca.pem")
else:
    cert_value = cert.read_text(encoding="ascii")
    if cert_value.count("-----BEGIN CERTIFICATE-----") != 2:
        errors.append("certs/max-ca.pem: expected exactly two PEM certificates")
if not (root / "certs" / "README.md").is_file():
    errors.append("missing certs/README.md")

# ---------------------------------------------------------------------------
# Test files only need to explain their purpose/scenarios; exact prose is not API.
need("tests/test_maxcore.cpp", "// Unit tests protocol-level MAX functions", "MaxParseUpdates", "surrogate")
need("tests/test_maxclient.cpp", "// Unit tests MAX_API_CLIENT behavior", "attachment.not.ready", "SleepMilliseconds")
need("tests/test_contract.py", "Behavioral source contract", "attachment.not.ready", "fail-closed")
need("tests/test_bcb2007.py", "C++Builder 2007", "In* Indy", "no STL")
need("tests/test_cert.sh", "MAX Ministry CA bundle verified", "ROOT_SHA256", "SUB_SHA256")
need("e2e/e2e_harness.cpp", "// End-to-end test of MAX_API_CLIENT", "Retry E2E")
need("e2e/mock_max_server.py", "Local HTTP model of the MAX endpoints", "attachment.not.ready", "unexpected_reupload")
need("e2e/posix_http_transport.h", "TPosixHttpTransport")
need("e2e/posix_http_transport.cpp", "TPosixHttpTransport::Request")
need("tests/run.sh", "test_cert.sh", "test_contract.py", "test_bcb2007.py")
need("e2e/run_e2e.sh", "e2e_harness.cpp", "mock_max_server.py")

# Documentation density guards against stripping the original explanatory style.
for path, minimum in {
    "maxbot.h": 35, "maxbot.cpp": 70,
    "maxmsg.h": 35, "maxmsg.cpp": 45,
    "maxtask.h": 15, "maxtask.cpp": 20,
    "UFMaxBot.cpp": 35, "UFMaxBot.h": 8,
    "UFMaxBotApi.cpp": 8, "UFMaxBotApi.h": 5,
    "UFMaxUserEdit.cpp": 8, "UFMaxUserEdit.h": 4,
    "UFMaxMsg.cpp": 3, "UFMaxMsg.h": 3,
    "api/maxcore.h": 15, "api/maxcore.cpp": 25,
    "api/maxclient.h": 15, "api/maxclient.cpp": 15,
    "api/maxindy.h": 10, "api/maxindy.cpp": 15,
    "tests/test_maxcore.cpp": 8, "tests/test_maxclient.cpp": 9,
    "e2e/e2e_harness.cpp": 7,
    "e2e/posix_http_transport.h": 5,
    "e2e/posix_http_transport.cpp": 12,
}.items():
    require_comment_count(path, minimum)

for path, minimum in {
    "tests/run.sh": 4,
    "tests/test_cert.sh": 5,
    "e2e/run_e2e.sh": 5,
    "e2e/mock_max_server.py": 12,
}.items():
    require_comment_count(path, minimum, "#")

# Header still carries the original mirrored comment groups.
for comment in (
    "//Состояние потока работы с MAX", "//Поток для работы с MAX",
    "//Прочитанные сообщения с сервера", "//Выполнение заданий",
    "//Чтение сообщений", "//Передача сообщения", "//Информация о себе",
    "//Посылка файла картинки", "//Посылка файла документа",
    "//Класс для работы с MAX Bot", "//Загрузка бота из файла",
    "//Сохранение бота в файл", "//Добавление заданий потоку",
    "//Установка обработчиков событий", "//Возникла новая авария LanMon",
    "//Получены новые события MAX", "//Передача сообщения по alias (из LanMon)",
):
    if comment not in h:
        errors.append(f"maxbot.h: missing mirrored comment {comment!r}")

if errors:
    print("MIRROR CHECK FAILED")
    for error in errors:
        print(" -", error)
    sys.exit(1)

print("Telegram-style MAX mirror structure/comments check passed")
