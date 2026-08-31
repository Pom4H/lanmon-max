#!/usr/bin/env python3
"""Exhaustive Telegram -> MAX functional parity contract for LanMon 4.

Source of truth audited in Pom4H/lanmon-cloud:
  lanmon4-windows/lanmon4/Telegram/{tgbot,tgmsg,tgtask,UFTgBot}.*
  lanmon4-windows/lanmon4/src/alarms/{Avaria,HistoryAlarm}.cpp
  lanmon4-windows/lanmon4/src/script/EventFS.*

Portable protocol/API behavior is exercised by C++ tests. VCL-only workflows
are asserted structurally because GitHub-hosted Linux runners cannot compile
C++Builder 2007. This file deliberately inventories the functional use cases,
not window geometry/column-width persistence or other presentation trivia.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []
covered = []


def read(path):
    p = ROOT / path
    if not p.exists():
        errors.append(f"missing source: {path}")
        return ""
    return p.read_text(encoding="utf-8")


def body(text, signature):
    p = text.find(signature)
    if p < 0:
        errors.append(f"missing function {signature!r}")
        return ""
    p = text.find("{", p)
    if p < 0:
        errors.append(f"missing body for {signature!r}")
        return ""
    depth = 0
    for i in range(p, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[p + 1:i]
    errors.append(f"unclosed body for {signature!r}")
    return ""


def need(case_id, label, text, *tokens):
    covered.append((case_id, label))
    for token in tokens:
        if token not in text:
            errors.append(f"{case_id} {label}: missing {token!r}")


def order(case_id, label, text, *tokens):
    covered.append((case_id, label))
    pos = -1
    for token in tokens:
        pos = text.find(token, pos + 1)
        if pos < 0:
            errors.append(f"{case_id} {label}: missing/ordered {token!r}")
            return


def forbid(case_id, label, text, *tokens):
    covered.append((case_id, label))
    for token in tokens:
        if token in text:
            errors.append(f"{case_id} {label}: forbidden {token!r}")


bot = read("maxbot.cpp")
bot_h = read("maxbot.h")
msg = read("maxmsg.cpp")
msg_h = read("maxmsg.h")
task = read("maxtask.cpp")
ui = read("UFMaxBot.cpp")
ui_h = read("UFMaxBot.h")
dfm = read("UFMaxBot.dfm")
core = read("api/maxcore.cpp")
client = read("api/maxclient.cpp")
indy = read("api/maxindy.cpp")
integration = read("INTEGRATION.md")
core_tests = read("tests/test_maxcore.cpp")
client_tests = read("tests/test_maxclient.cpp")
image_tests = read("tests/test_image_upload.cpp")
protocol_error_tests = read("tests/test_indy_protocol_error.py")

# ---------------------------------------------------------------------------
# 1. Worker/thread/task lifecycle.
execute = body(bot, "void __fastcall TMaxBotThread::Execute")
order("T01", "runtime BotApi replacement", execute,
      "if(FlagNewBotApi)", "FBotApi=NewBotApi", "Api->SetToken(FBotApi)")
order("T02", "Active=false disables worker", execute,
      "if(!MaxBot.Active)", "State=tsDISABLED", "continue;")
order("T03", "empty token produces error state", execute,
      "if(!FBotApi.Length())", "State=tsERROR", "continue;")
order("T04", "task work precedes polling", execute,
      "CheckTask();", "if(PeriodicReadMessagesPaused)", "if(!PeriodReadMessages)", "DoReadMessagesPeriodic();")
need("T05", "periodic polling can be paused", bot_h,
     "PeriodicReadMessagesPaused", "SetPeriodicReadMessagesPaused")
need("T06", "all Telegram task kinds exist", body(bot, "void TMaxBotThread::CheckTask"),
     "taskREADMSG", "taskSENDMSG", "taskGETME", "taskSENDPHOTO", "taskSENDDOC")
order("T07", "task queue is thread-safe FIFO", body(task, "bool MB_TASK_LIST::Get"),
      "List->LockList()", "list->Items[0]", "list->Delete(0)", "List->UnlockList()")
need("T08", "task producers exist", task,
     "AddReadMsg", "AddGetMe", "AddSendMsg", "AddSendPhoto", "AddSendDoc")

# ---------------------------------------------------------------------------
# 2. Read/send/GetMe behavior and counters.
read_messages = body(bot, "bool TMaxBotThread::DoReadMessages")
order("R01", "read counters", read_messages,
      "MaxBot.ReadMessagesCount++", "Api->Poll", "MaxBot.ReadMessagesCountOk++")
need("R02", "manual read callback", body(bot, "void TMaxBotThread::DoReadMessagesByTask"), "_ON_TASK_READMSG()")
order("R03", "periodic callback only for nonempty result", body(bot, "void TMaxBotThread::DoReadMessagesPeriodic"),
      "if(DoReadMessages(false))", "if(MsgList.Count)", "_ON_PERIODIC_READMSG()")
need("R04", "GetMe success/failure callback", body(bot, "void TMaxBotThread::DoGetMe"),
     "_ON_GETME();", "BotInfo.CopyFrom(info)")
need("R05", "text send", body(bot, "bool TMaxBotThread::DoSendMessage"),
     "Api->SendMessage", "ERROR_DBG_MSG", "DBG_MSG")
need("R06", "image send with caption", body(bot, "void TMaxBotThread::DoSendPhoto"),
     "Api->SendImage", "MaxUtf8(Task.Caption)")
need("R07", "document send with caption", body(bot, "void TMaxBotThread::DoSendDoc"),
     "Api->SendFile", "MaxUtf8(Task.Caption)")
need("R08", "production retry waits", body(indy, "void TMaxIndyTransport::SleepMilliseconds"), "::Sleep(milliseconds)")
need("R09", "Indy HTTP errors preserve MAX body", indy,
     "EIdHTTPProtocolException", "ErrorCode", "ErrorMessage", "ApplyHttpProtocolError")
need("R10", "protocol-error regression test", protocol_error_tests,
     "EIdHTTPProtocolException", "attachment.not.ready")

for cid, signature, add_call in (
    ("R11", "void MAX_BOT::SendMessage", "AddSendMsg"),
    ("R12", "void MAX_BOT::SendPhoto", "AddSendPhoto"),
    ("R13", "void MAX_BOT::SendDoc", "AddSendDoc"),
):
    order(cid, "legacy OutCount increments when queued", body(bot, signature),
          add_call, "UserList->Find(id)", "user->OutCount++")

# API-level tests cover success and error paths, uploads and retry backoff.
need("R14", "client text send tests", client_tests, "Text send to a user", "Non-2xx response", "Transport error")
need("R15", "client image tests", client_tests, "Image upload", "type=image")
need("R16", "client file tests", client_tests, "File upload", "type=file")
need("R17", "attachment eventual-consistency tests", client_tests,
     "attachment.not.ready retries only", "Persistent attachment.not.ready", "500", "401")
need("R18", "live image payload regression", image_tests, "photos", "token")

# ---------------------------------------------------------------------------
# 3. Long polling / Telegram membership-event equivalents.
need("P01", "poll all functional event types", core,
     '"&types=message_created,bot_added,user_added"')
need("P02", "message_created parser", core,
     'updateType=="message_created"', 'Field(*msg,"sender")', 'Field(*msg,"recipient")', 'Field(*msg,"body")')
need("P03", "bot_added maps Telegram invitation", core,
     'updateType=="bot_added"', 'm.ChatId=Int64(u,"chat_id")', 'ReadUser(Field(u,"user"),m)')
need("P04", "user_added maps Telegram new participant", core,
     'updateType=="user_added"', 'm.ChatType=Bool(u,"is_channel",false)?"channel":"chat"')
need("P05", "VCL invitation type", msg, 'if(msg.UpdateType=="bot_added")Type=mmtINVITATION;')
need("P06", "VCL participant type/data", msg,
     'else if(msg.UpdateType=="user_added")Type=mmtNEWPATICIPANT;', "Participant.Id", "Participant.first_name")
need("P07", "membership event timestamp fallback", msg,
     "msg.MessageTimestamp?msg.MessageTimestamp:msg.UpdateTimestamp")
need("P08", "message model exposes participant diagnostics", msg_h,
     "mmtINVITATION", "mmtNEWPATICIPANT", "MaxFrom Participant")
need("P09", "portable bot_added test", core_tests, "bot_added", "up.Messages[1].ChatId==-777")
need("P10", "portable user_added test", core_tests, "user_added", "up.Messages[2].UserId==202")
need("P11", "portable channel membership test", core_tests, 'ChatType=="channel"')
need("P12", "unknown updates ignored", core_tests, "message_callback", "up.Messages.empty()")
need("P13", "invalid membership without chat ignored", core_tests, "Membership event without chat_id")
need("P14", "marker progression and empty pages", core_tests, "HasMarker", "Empty update page", "nullable marker")
need("P15", "Unicode/emoji parsing", core_tests, "surrogate pair", "Raw UTF-8", "Привет")

# ---------------------------------------------------------------------------
# 4. User/chat address book and aliases.
need("U01", "persist legacy user fields", msg,
     'WriteString(section,"Name"', 'WriteString(section,"id"', 'WriteString(section,"Alias"',
     'WriteString(section,"InCount"', 'WriteString(section,"OutCount"',
     'WriteString(section,"Comment"', 'WriteInteger(section,"Tag"')
need("U02", "persist MAX address namespace", msg,
     'WriteString(section,"PeerType","chat")', 'ReadString(section,"PeerType","user")')
need("U03", "copy personal address", body(msg, "void MaxUser::CopyUserFrom"),
     "FId=msg->From.Id", "FPeerType=maxPeerUser")
need("U04", "copy chat address", body(msg, "void MaxUser::CopyChatFrom"),
     "FId=msg->Chat.Id", "FPeerType=maxPeerChat")
alias = body(msg, "bool MaxUser::HasValidAlias")
order("U05", "alias wildcard semantics", alias,
      "if(!Valid)return false;", 'if(alias=="*")return true;', "if(alias.IsEmpty())return true;", "if(FAlias.Length()<len)return false;")
need("U06", "alias ! positional wildcard", alias, "alias[i]!='!'", "FAlias[i]!=alias[i]")
need("U07", "find by id and alias", msg, "MaxUser_LIST::Find(AnsiString id)", "MaxUser_LIST::FindAlias")
need("U08", "automatic $N alias", body(msg, "AnsiString MaxUser_LIST::GetFreeAlias"),
     'AnsiString alias="$1"', "FindAlias(alias)")
need("U09", "filter users by alias", msg,
     "MaxUser_LIST::GetUsersByAlias", "ui->HasValidAlias(alias)", "user->CopyFrom(ui)")
need("U10", "filter indexes by alias", msg,
     "MaxUser_LIST::GetIndexesByAlias", "list->Add((void *)i)")
need("U11", "manual read increments InCount", body(ui, "void __fastcall TFormMaxBot::OnTaskReadMessages"),
     "UserList->Find(MsgList[i]->Chat.Id)", "user->InCount++")

# Incoming chat/user discovery was a real operator workflow in UFTgBot.
add_user = body(ui, "void __fastcall TFormMaxBot::ButtonAddUserClick")
order("U12", "add selected discovered address", add_user,
      "ListViewMsg->Selected", "MsgList[index]", "MaxBot.UserList->Find(id)", "MaxBot.UserList->AddUser()")
need("U13", "direct MAX dialog becomes user_id", add_user,
     'msg->Chat.type.LowerCase()=="dialog"', "CopyUserFrom(msg)")
need("U14", "group/bot_added becomes chat_id", add_user,
     "CopyChatFrom(msg)", "mmtINVITATION")
need("U15", "new participant alone is not an address", add_user,
     "if(msg->Type==mmtNEWPATICIPANT)return;")
need("U16", "new address gets automatic alias", add_user, "GetFreeAlias()")
need("U17", "UI exposes Add address action", ui_h + dfm, "ButtonAddUser", "ButtonAddUserClick")
need("U18", "edit applies only after OK", body(ui, "void __fastcall TFormMaxBot::ButtonEditUserClick"),
     "MaxUser copy;", "copy.CopyFrom(user)", "ShowModal()==mrOk", "user->CopyFrom(&copy)")
need("U19", "delete address", body(ui, "void __fastcall TFormMaxBot::ButtonDeleteUserClick"), "DeleteUser(index)")

# ---------------------------------------------------------------------------
# 5. Incoming FastScript and built-in command use cases.
on_messages = body(bot, "void MAX_BOT::OnMessages")
order("C01", "FastScript callback occurs before command gate", on_messages,
      "if(scriptid.IsEmpty())continue;", "MaxBot.UserMessageCount++;",
      "OnTgMessage((int)msg->update_id,scriptid,msg->Text);", "if(!FlagSendMaps)continue;")
order("C02", "RequestAlias authorization", on_messages,
      "UserList->Find(scriptid)", "user->HasValidAlias(RequestAlias)", "msg->Text.UpperCase().Trim()")
need("C03", "Russian aliases", on_messages,
     'U8("ЭКРАН")', 'U8("КАРТА")', 'U8("СТОП")', 'U8("ЖУРНАЛ")', 'U8("ТРЕВОГ")')
need("C04", "SCREEN monitor + desktop fallback", on_messages,
     "GetMonitorScreenshot(screenindex-1,fn)", "DesktopScreenshot(fn)", 'SendPhoto(id,fn,U8("Экран"')
need("C05", "MAP screenshot -> PNG -> send", on_messages,
     "CreateMapScreenshot(mapindex-1,bmpfn)", "Bmp2Png(bmpfn,pngfn)", 'SendPhoto(id,pngfn,U8("Карта ")')
need("C06", "MAP reuses LanMon BMP->PNG implementation", bot,
     "bool Bmp2Png(AnsiString bmpfn,AnsiString pngfn);")
need("C07", "STOP", on_messages, "CloseAvariaForm();", 'SendMessage(id,U8("Команда СТОП выполнена"))')
need("C08", "LOGXLS", on_messages, '"_log.xls"', "ExportToXls", "SendDoc(id,fn")
need("C09", "LOG HTML", on_messages, '"_log.html"', "ExportToHtml", "SendDoc(id,fn")
need("C10", "ALARM PDF", on_messages, "CreateAlarmsPdf", "if(fn.Length())", "SendDoc(id,fn")
need("C11", "HELP and ?", on_messages, '"HELP"', "text[1]=='?'", '"?? - дополнительные запросы"')

# ---------------------------------------------------------------------------
# 6. Outbound alias fan-out: functional equivalent of Telegram manual/masked send.
for cid, signature, call in (
    ("A01", "void MAX_BOT::SendMessageByAlias", "SendMessage"),
    ("A02", "void MAX_BOT::SendPhotoByAlias", "SendPhoto"),
    ("A03", "void MAX_BOT::SendDocByAlias", "SendDoc"),
):
    fn = body(bot, signature)
    order(cid, f"{call} fan-out", fn,
          "GetUsersByAlias(userlist,alias)", "if(userlist->Count)", call + "(user->Id")
    need(cid + "N", f"{call} raw numeric-id fallback", fn,
         "if(isdigit(alias[1]))", call + "(alias")

# ---------------------------------------------------------------------------
# 7. Alarm and FastScript integration points from LanMon source.
need("L01", "new-alarm hook documented", integration, "FlagSendAlarms", "OnNewAlarmState")
need("L02", "alarm-end hook documented", integration, "FlagSendAlarmsEnd")
need("L03", "operator-confirmation hook documented", integration, "FlagOperatorAlarm")
need("L04", "existing FastScript event reused", integration, "OnTgMessage")
need("L05", "AlarmAlias fan-out", body(bot, "void MAX_BOT::OnNewAlarmState"),
     "GetUsersByAlias(userlist,AlarmAlias)", "SendMessage(user->Id,mess)")

# ---------------------------------------------------------------------------
# 8. Settings and diagnostics that affect bot operation.
need("S01", "opening settings pauses polling", body(ui, "__fastcall TFormMaxBot::TFormMaxBot"),
     "PeriodicReadMessagesPaused=true")
order("S02", "closing settings saves and resumes", body(ui, "void __fastcall TFormMaxBot::FormClose"),
      'MaxBot.Save(WorkDir+"MaxBot.ini")', "PeriodicReadMessagesPaused=false")
need("S03", "BotApi edit rollback", body(ui, "void __fastcall TFormMaxBot::ButtonEditBotApiClick"),
     "NewBotApi", "NewMyBotInfo", "OldBotApi")
need("S04", "live operational settings", ui,
     "CheckBoxActiveClick", "CheckBoxFlagSendAlarmsClick", "CheckBoxFlagSendAlarmsEndClick",
     "CheckBoxFlagOperatorAlarmClick", "CheckBoxFlagSendMapsClick", "CheckBoxUseLanmonLogClick",
     "EditAlarmAliasChange", "EditRequestAliasChange")
need("S05", "period enable/disable", ui, "CheckBoxPeriodReadMessagesClick", "EditPeriodReadMessagesChange")
need("S06", "JSON diagnostics", ui, "TabSheetJsonShow", "MemoJson->Text=MaxBot.Json")
need("S07", "runtime counters/status", ui, "GetThreadState", "ReadMessagesCount", "ReadMessagesCountOk", "UserMessageCount")
need("S08", "message type/participant diagnostics", body(ui, "void TFormMaxBot::ShowMsgList"),
     "msg->TypeText", "msg->ParticipantText")

load = body(bot, "void MAX_BOT::Load")
save = body(bot, "void MAX_BOT::Save")
for cid, key in (
    ("S09", "Active"), ("S10", "BotApi"), ("S11", "PeriodReadMessages"),
    ("S12", "SendAlarms"), ("S13", "SendAlarmEnd"), ("S14", "OperatorAlarm"),
    ("S15", "AlarmAlias"), ("S16", "RequestAlias"), ("S17", "SendMaps"),
    ("S18", "UseLanmonLog"),
):
    need(cid, f"persist {key}", load + save, f'"{key}"')
need("S19", "persist bot identity", load + save,
     '"MyBotId"', '"MyBotName"', '"MyBotUserName"')
need("S20", "persist address book", load + save,
     "UserList->Load(&ini)", "UserList->Save(&ini)")

# ---------------------------------------------------------------------------
# Safety invariants introduced by MAX must stay covered too.
need("X01", "Authorization only on Bot API", client,
     "MAX_HTTP_HEADERS uploadHeaders", "Headers(false)", "Headers(true)")
need("X02", "upload host receives no bot headers in tests", client_tests,
     "do not leak the bot token", "!HasHeader")
need("X03", "attachment retry reuses same upload", client_tests,
     "expensive multipart upload must happen exactly once", "same-token")
need("X04", "TLS 1.2 + CA verification", indy,
     "sslvTLSv1_2", "RootCertFile", "sslvrfPeer", "VerifyDepth")

# Finite inventory hygiene.
ids = [x[0] for x in covered]
if len(ids) != len(set(ids)):
    dup = sorted({x for x in ids if ids.count(x) > 1})
    errors.append("duplicate case ids: " + ", ".join(dup))

if errors:
    print("TELEGRAM USE-CASE PARITY FAILED")
    for error in errors:
        print(" -", error)
    print(f"checked {len(covered)} functional assertions")
    sys.exit(1)

print(f"Telegram -> MAX functional parity passed: {len(covered)} assertions")
