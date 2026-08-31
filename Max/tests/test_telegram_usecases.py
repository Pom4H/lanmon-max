#!/usr/bin/env python3
"""Coverage contract for every behavior used by the LanMon 4 Telegram bot.

The source of truth is lanmon4-windows/lanmon4/Telegram plus its alarm and
FastScript integration points. Portable API behavior has normal C++ tests;
VCL-only behavior is protected here as a source contract because Linux CI
cannot compile C++Builder 2007 forms.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []
checked = []


def read(path):
    p = ROOT / path
    if not p.exists():
        errors.append(f"missing file: {path}")
        return ""
    return p.read_text(encoding="utf-8")


def function_body(text, signature, label):
    pos = text.find(signature)
    if pos < 0:
        errors.append(f"{label}: missing function {signature!r}")
        return ""
    start = text.find("{", pos)
    if start < 0:
        errors.append(f"{label}: missing body")
        return ""
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1:i]
    errors.append(f"{label}: unclosed body")
    return ""


def case(case_id, label, text, *tokens):
    checked.append((case_id, label))
    for token in tokens:
        if token not in text:
            errors.append(f"{case_id} {label}: missing {token!r}")


def absent(case_id, label, text, *tokens):
    checked.append((case_id, label))
    for token in tokens:
        if token in text:
            errors.append(f"{case_id} {label}: forbidden {token!r}")


def ordered(case_id, label, text, *tokens):
    checked.append((case_id, label))
    pos = -1
    for token in tokens:
        nxt = text.find(token, pos + 1)
        if nxt < 0:
            errors.append(f"{case_id} {label}: missing ordered token {token!r}")
            return
        pos = nxt


bot = read("maxbot.cpp")
bot_h = read("maxbot.h")
msg = read("maxmsg.cpp")
msg_h = read("maxmsg.h")
task = read("maxtask.cpp")
task_h = read("maxtask.h")
ui = read("UFMaxBot.cpp")
ui_h = read("UFMaxBot.h")
dfm = read("UFMaxBot.dfm")
core = read("api/maxcore.cpp")
client = read("api/maxclient.cpp")
indy = read("api/maxindy.cpp")
integration = read("INTEGRATION.md")

# ---------------------------------------------------------------------------
# Thread/lifecycle/task semantics from tgbot/tgtask.
execute = function_body(bot, "void __fastcall TMaxBotThread::Execute", "thread loop")
ordered("T01", "runtime token replacement", execute, "if(FlagNewBotApi)", "FBotApi=NewBotApi", "Api->SetToken(FBotApi)")
ordered("T02", "disabled state", execute, "if(!MaxBot.Active)", "State=tsDISABLED", "continue;")
ordered("T03", "missing token state", execute, "if(!FBotApi.Length())", "State=tsERROR", "continue;")
ordered("T04", "task before periodic polling", execute, "CheckTask();", "if(PeriodicReadMessagesPaused)", "if(!PeriodReadMessages)", "DoReadMessagesPeriodic();")
case("T05", "periodic pause exposed", bot_h, "PeriodicReadMessagesPaused", "SetPeriodicReadMessagesPaused")

check_task = function_body(bot, "void TMaxBotThread::CheckTask", "task dispatch")
case("T06", "all Telegram task types", check_task, "taskREADMSG", "taskSENDMSG", "taskGETME", "taskSENDPHOTO", "taskSENDDOC")
ordered("T07", "thread-safe FIFO", function_body(task, "bool MB_TASK_LIST::Get", "task fifo"), "List->LockList()", "list->Items[0]", "list->Delete(0)", "List->UnlockList()")
case("T08", "task constructors", task, "AddReadMsg", "AddGetMe", "AddSendMsg", "AddSendPhoto", "AddSendDoc")

# ---------------------------------------------------------------------------
# Read callbacks, counters, GetMe and send operations.
read_messages = function_body(bot, "bool TMaxBotThread::DoReadMessages", "read messages")
ordered("R01", "read counters", read_messages, "MaxBot.ReadMessagesCount++", "Api->Poll", "MaxBot.ReadMessagesCountOk++")
case("R02", "manual read callback", function_body(bot, "void TMaxBotThread::DoReadMessagesByTask", "manual read"), "_ON_TASK_READMSG()")
ordered("R03", "periodic callback only for nonempty page", function_body(bot, "void TMaxBotThread::DoReadMessagesPeriodic", "periodic read"), "if(DoReadMessages(false))", "if(MsgList.Count)", "_ON_PERIODIC_READMSG()")
case("R04", "GetMe callback on both outcomes", function_body(bot, "void TMaxBotThread::DoGetMe", "GetMe"), "_ON_GETME();", "BotInfo.CopyFrom(info)")
case("R05", "text send", function_body(bot, "bool TMaxBotThread::DoSendMessage", "send text"), "Api->SendMessage", "ERROR_DBG_MSG", "DBG_MSG")
case("R06", "image send", function_body(bot, "void TMaxBotThread::DoSendPhoto", "send image"), "Api->SendImage", "MaxUtf8(Task.Caption)")
case("R07", "document send", function_body(bot, "void TMaxBotThread::DoSendDoc", "send doc"), "Api->SendFile", "MaxUtf8(Task.Caption)")
case("R08", "production retry really sleeps", function_body(indy, "void TMaxIndyTransport::SleepMilliseconds", "retry sleep"), "::Sleep(milliseconds)")
case("R09", "HTTP protocol errors retain MAX body", indy, "EIdHTTPProtocolException", "ErrorCode", "ErrorMessage", "ApplyHttpProtocolError")

# Queue-time OutCount is legacy behavior.
for cid, signature, add in (
    ("R10", "void MAX_BOT::SendMessage", "AddSendMsg"),
    ("R11", "void MAX_BOT::SendPhoto", "AddSendPhoto"),
    ("R12", "void MAX_BOT::SendDoc", "AddSendDoc"),
):
    ordered(cid, signature, function_body(bot, signature, signature), add, "UserList->Find(id)", "user->OutCount++")

# ---------------------------------------------------------------------------
# Protocol parsing: message + Telegram membership equivalents.
case("P01", "poll all parity events", core, '"&types=message_created,bot_added,user_added"')
case("P02", "message_created parser", core, 'updateType=="message_created"', 'Field(*msg,"sender")', 'Field(*msg,"recipient")', 'Field(*msg,"body")')
case("P03", "bot_added invitation parser", core, 'updateType=="bot_added"', 'm.ChatId=Int64(u,"chat_id")', 'ReadUser(Field(u,"user"),m)')
case("P04", "user_added participant parser", core, 'updateType=="user_added"', 'm.ChatType=Bool(u,"is_channel",false)?"channel":"chat"')
case("P05", "VCL invitation mapping", msg, 'if(msg.UpdateType=="bot_added")Type=mmtINVITATION;')
case("P06", "VCL participant mapping", msg, 'else if(msg.UpdateType=="user_added")Type=mmtNEWPATICIPANT;', "Participant.Id", "Participant.first_name")
case("P07", "event timestamp fallback", msg, "msg.MessageTimestamp?msg.MessageTimestamp:msg.UpdateTimestamp")
case("P08", "message model keeps participant", msg_h, "mmtINVITATION", "mmtNEWPATICIPANT", "MaxFrom Participant")

# Portable tests must exercise the protocol details, not only source tokens.
maxcore_test = read("tests/test_maxcore.cpp")
case("P09", "portable bot_added test", maxcore_test, '"bot_added"', "up.Messages[1].ChatId==-777")
case("P10", "portable user_added test", maxcore_test, '"user_added"', "up.Messages[2].UserId==202")
case("P11", "channel membership test", maxcore_test, "ChatType==\"channel\"")
case("P12", "unknown update ignored", maxcore_test, '"message_callback"', "up.Messages.empty()")

# ---------------------------------------------------------------------------
# User/address-book semantics from tgmsg and UFTgBot.
case("U01", "persist user fields", msg, 'WriteString(section,"Name"', 'WriteString(section,"id"', 'WriteString(section,"Alias"', 'WriteString(section,"InCount"', 'WriteString(section,"OutCount"', 'WriteString(section,"Comment"', 'WriteInteger(section,"Tag"')
case("U02", "MAX peer type persisted", msg, 'WriteString(section,"PeerType","chat")', 'ReadString(section,"PeerType","user")')
case("U03", "copy direct user", function_body(msg, "void MaxUser::CopyUserFrom", "copy user"), "FId=msg->From.Id", "FPeerType=maxPeerUser")
case("U04", "copy chat", function_body(msg, "void MaxUser::CopyChatFrom", "copy chat"), "FId=msg->Chat.Id", "FPeerType=maxPeerChat")

alias = function_body(msg, "bool MaxUser::HasValidAlias", "alias")
ordered("U05", "alias special cases", alias, "if(!Valid)return false;", 'if(alias=="*")return true;', "if(alias.IsEmpty())return true;", "if(FAlias.Length()<len)return false;")
case("U06", "alias positional wildcard", alias, "alias[i]!='!'", "FAlias[i]!=alias[i]")
case("U07", "find by id/alias", msg, "MaxUser_LIST::Find(AnsiString id)", "MaxUser_LIST::FindAlias")
case("U08", "automatic $N alias", function_body(msg, "AnsiString MaxUser_LIST::GetFreeAlias", "free alias"), 'AnsiString alias="$1"', "FindAlias(alias)")
case("U09", "filter users by alias", msg, "MaxUser_LIST::GetUsersByAlias", "ui->HasValidAlias(alias)", "user->CopyFrom(ui)")
case("U10", "filter indexes by alias", msg, "MaxUser_LIST::GetIndexesByAlias", "list->Add((void *)i)")
case("U11", "manual read increments InCount", function_body(ui, "void __fastcall TFormMaxBot::OnTaskReadMessages", "manual UI read"), "UserList->Find(MsgList[i]->Chat.Id)", "UserList->Find(MsgList[i]->From.Id)", "user->InCount++")

# Original Telegram UI let an operator add a discovered chat/user from an incoming event.
add_user = function_body(ui, "void __fastcall TFormMaxBot::ButtonAddUserClick", "add incoming address")
ordered("U12", "add selected incoming address", add_user, "ListViewMsg->Selected", "MsgList[index]", "MaxBot.UserList->Find(id)", "MaxBot.UserList->AddUser()")
case("U13", "direct dialog becomes user_id", add_user, 'msg->Chat.type.LowerCase()=="dialog"', "CopyUserFrom(msg)")
case("U14", "group/bot_added becomes chat_id", add_user, "CopyChatFrom(msg)", "mmtINVITATION")
case("U15", "new participant alone is not an address", add_user, "if(msg->Type==mmtNEWPATICIPANT)return;")
case("U16", "new address gets free alias", add_user, "GetFreeAlias()")
case("U17", "UI exposes Add address", ui_h + dfm, "ButtonAddUser", "ButtonAddUserClick")
case("U18", "edit uses copy and applies only OK", function_body(ui, "void __fastcall TFormMaxBot::ButtonEditUserClick", "edit user"), "MaxUser copy;", "copy.CopyFrom(user)", "ShowModal()==mrOk", "user->CopyFrom(&copy)")
case("U19", "delete user", function_body(ui, "void __fastcall TFormMaxBot::ButtonDeleteUserClick", "delete user"), "DeleteUser(index)")

# ---------------------------------------------------------------------------
# Incoming FastScript callback/auth/command contract.
on_messages = function_body(bot, "void MAX_BOT::OnMessages", "OnMessages")
ordered("C01", "FastScript callback before gates", on_messages, "if(scriptid.IsEmpty())continue;", "MaxBot.UserMessageCount++;", "OnTgMessage((int)msg->update_id,scriptid,msg->Text);", "if(!FlagSendMaps)continue;")
ordered("C02", "request authorization", on_messages, "UserList->Find(scriptid)", "user->HasValidAlias(RequestAlias)", "msg->Text.UpperCase().Trim()")
case("C03", "Russian command aliases", on_messages, 'U8("ЭКРАН")', 'U8("КАРТА")', 'U8("СТОП")', 'U8("ЖУРНАЛ")', 'U8("ТРЕВОГ")')
case("C04", "SCREEN monitor plus desktop fallback", on_messages, "GetMonitorScreenshot(screenindex-1,fn)", "DesktopScreenshot(fn)", 'SendPhoto(id,fn,U8("Экран"')
case("C05", "MAP screenshot pipeline", on_messages, "CreateMapScreenshot(mapindex-1,bmpfn)", "Bmp2Png(bmpfn,pngfn)", 'SendPhoto(id,pngfn,U8("Карта ")')
case("C06", "STOP closes alarm form", on_messages, "CloseAvariaForm();", 'SendMessage(id,U8("Команда СТОП выполнена"))')
case("C07", "LOGXLS export", on_messages, '"_log.xls"', "ExportToXls", "SendDoc(id,fn")
case("C08", "LOG HTML export", on_messages, '"_log.html"', "ExportToHtml", "SendDoc(id,fn")
case("C09", "ALARM PDF", on_messages, "CreateAlarmsPdf", "if(fn.Length())", "SendDoc(id,fn")
case("C10", "HELP/?", on_messages, '"HELP"', "text[1]=='?'", '"?? - дополнительные запросы"')
case("C11", "map BMP to PNG helper", bot, "bool Bmp2Png", "FileExists", "TPngImage", "SaveToFile")

# ---------------------------------------------------------------------------
# Alias fan-out/manual-send use cases. The original UI selected users by mask;
# MAX exposes the same operation directly through the three ByAlias methods.
for cid, signature, call in (
    ("A01", "void MAX_BOT::SendMessageByAlias", "SendMessage"),
    ("A02", "void MAX_BOT::SendPhotoByAlias", "SendPhoto"),
    ("A03", "void MAX_BOT::SendDocByAlias", "SendDoc"),
):
    body = function_body(bot, signature, signature)
    ordered(cid, signature, body, "GetUsersByAlias(userlist,alias)", "if(userlist->Count)", call + "(user->Id")
    case(cid + "N", "numeric raw-id fallback", body, "if(isdigit(alias[1]))", call + "(alias")

# ---------------------------------------------------------------------------
# Alarm hooks live in LanMon, not the adapter. INTEGRATION.md is the guarded
# contract for the three original source call sites plus AlarmAlias fan-out.
case("L01", "new alarm hook", integration, "FlagSendAlarms", "OnNewAlarmState")
case("L02", "alarm end hook", integration, "FlagSendAlarmsEnd")
case("L03", "operator confirmation hook", integration, "FlagOperatorAlarm")
case("L04", "FastScript compatibility hook", integration, "OnTgMessage")
case("L05", "alarm fan-out implementation", function_body(bot, "void MAX_BOT::OnNewAlarmState", "alarm fanout"), "GetUsersByAlias(userlist,AlarmAlias)", "SendMessage(user->Id,mess)")

# ---------------------------------------------------------------------------
# Settings/diagnostic form behavior used by Telegram support workflows.
case("S01", "form pauses polling", function_body(ui, "__fastcall TFormMaxBot::TFormMaxBot", "form ctor"), "PeriodicReadMessagesPaused=true")
ordered("S02", "form save/resume", function_body(ui, "void __fastcall TFormMaxBot::FormClose", "form close"), 'MaxBot.Save(WorkDir+"MaxBot.ini")', "PeriodicReadMessagesPaused=false")
case("S03", "BotApi rollback on cancel", function_body(ui, "void __fastcall TFormMaxBot::ButtonEditBotApiClick", "BotApi edit"), "NewBotApi", "NewMyBotInfo", "OldBotApi")
case("S04", "live settings controls", ui, "CheckBoxActiveClick", "CheckBoxFlagSendAlarmsClick", "CheckBoxFlagSendAlarmsEndClick", "CheckBoxFlagOperatorAlarmClick", "CheckBoxFlagSendMapsClick", "CheckBoxUseLanmonLogClick", "EditAlarmAliasChange", "EditRequestAliasChange")
case("S05", "period enable/disable", ui, "CheckBoxPeriodReadMessagesClick", "EditPeriodReadMessagesChange")
case("S06", "JSON diagnostics", ui, "TabSheetJsonShow", "MemoJson->Text=MaxBot.Json")
case("S07", "thread/read/request stats", ui, "GetThreadState", "ReadMessagesCount", "ReadMessagesCountOk", "UserMessageCount")
case("S08", "message type/participant diagnostics", function_body(ui, "void TFormMaxBot::ShowMsgList", "message list"), "msg->TypeText", "msg->ParticipantText")

# Settings persistence must include every Telegram option plus MAX additions.
load = function_body(bot, "void MAX_BOT::Load", "load settings")
save = function_body(bot, "void MAX_BOT::Save", "save settings")
for cid, key in (
    ("S09", "Active"), ("S10", "BotApi"), ("S11", "PeriodReadMessages"),
    ("S12", "SendAlarms"), ("S13", "SendAlarmEnd"), ("S14", "OperatorAlarm"),
    ("S15", "AlarmAlias"), ("S16", "RequestAlias"), ("S17", "SendMaps"),
):
    case(cid, f"persist {key}", load + save, f'"{key}"')
case("S18", "persist bot identity", load + save, '"MyBotId"', '"MyBotName"', '"MyBotUserName"')
case("S19", "persist user list", load + save, "UserList->Load(&ini)", "UserList->Save(&ini)")

# ---------------------------------------------------------------------------
# Ensure this really is an explicit, finite coverage inventory.
ids = [item[0] for item in checked]
if len(ids) != len(set(ids)):
    duplicates = sorted({x for x in ids if ids.count(x) > 1})
    errors.append("duplicate use-case ids: " + ", ".join(duplicates))

if errors:
    print("TELEGRAM USE-CASE PARITY FAILED")
    for error in errors:
        print(" -", error)
    print(f"checked {len(checked)} use-case assertions")
    sys.exit(1)

print(f"Telegram -> MAX use-case parity passed: {len(checked)} assertions")
