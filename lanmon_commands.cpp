#include "lanmon_commands.h"
#include <cctype>
#include <cstdlib>

static std::string Utf8UpperRussian(const std::string & text)
{
    std::string out;
    for(size_t i=0;i<text.size();++i) {
        unsigned char a=(unsigned char)text[i];
        if(a>='a' && a<='z') { out+=(char)(a-'a'+'A'); continue; }
        if(a==0xD0 && i+1<text.size()) {
            unsigned char b=(unsigned char)text[i+1];
            if(b>=0xB0 && b<=0xBF) { out+=(char)0xD0; out+=(char)(b-0x20); ++i; continue; }
        }
        if(a==0xD1 && i+1<text.size()) {
            unsigned char b=(unsigned char)text[i+1];
            if(b>=0x80 && b<=0x8F) { out+=(char)0xD0; out+=(char)(b+0x20); ++i; continue; }
            if(b==0x91) { out+=(char)0xD0; out+=(char)0x81; ++i; continue; }
        }
        out+=(char)a;
    }
    return out;
}

static std::string StopDoneText() { return "Команда СТОП выполнена"; }
static std::string MapCaption(int n) { return std::string("Карта ")+MaxInt64ToString(n); }
static std::string ScreenCaption(int n) { return n>0?std::string("Экран ")+MaxInt64ToString(n):std::string("Экран"); }
static std::string LogCaption(const std::string & dt) { return std::string("Журнал ")+dt; }
static std::string AlarmCaption(const std::string & dt) { return std::string("Тревоги ")+dt; }
static std::string HelpText()
{
    return "Возможные запросы:\n"
           "SCREEN (Экран) - все экраны\n"
           "SCREEN x (Экран x) - экран номер x\n"
           "MAP x (Карта x) - карта номер x\n"
           "LOG (Журнал) - текущий журнал\n"
           "LOGXLS - текущий журнал в формате XLS\n"
           "ALARM (Тревоги) - история тревог PDF\n"
           "STOP (Стоп) - закрыть окно аварий\n"
           "HELP (?) - помощь\n"
           "?? - дополнительные запросы";
}

std::string LANMON_MAX_COMMAND_ROUTER::TrimUpper(const std::string & text)
{
    size_t begin=0; while(begin<text.size() && std::isspace((unsigned char)text[begin])) ++begin;
    size_t end=text.size(); while(end>begin && std::isspace((unsigned char)text[end-1])) --end;
    return Utf8UpperRussian(text.substr(begin,end-begin));
}

int LANMON_MAX_COMMAND_ROUTER::ParsePositiveIndex(const std::string & text, size_t prefixLen)
{
    if(text.size()<=prefixLen) return 0;
    const char *p=text.c_str()+prefixLen; while(*p && std::isspace((unsigned char)*p)) ++p;
    int value=std::atoi(p); return value>0?value:0;
}

MAX_PEER LANMON_MAX_COMMAND_ROUTER::PeerFor(const MAX_MESSAGE & msg) const
{
    return msg.ChatId!=0?MAX_PEER(maxPeerChat,msg.ChatId):MAX_PEER(maxPeerUser,msg.UserId);
}

bool LANMON_MAX_COMMAND_ROUTER::Handle(const MAX_MESSAGE & msg, std::string & error)
{
    if(!Api || !Actions) { error="LanMon MAX router is not initialized"; return false; }
    const std::string text=TrimUpper(msg.Text); const MAX_PEER peer=PeerFor(msg);
    const std::string RU_SCREEN="ЭКРАН", RU_MAP="КАРТА", RU_STOP="СТОП", RU_LOG="ЖУРНАЛ", RU_ALARM="ТРЕВОГИ";

    if(text.compare(0,6,"SCREEN")==0 || text.compare(0,RU_SCREEN.size(),RU_SCREEN)==0) {
        size_t prefix=text.compare(0,6,"SCREEN")==0?6:RU_SCREEN.size();
        int screenIndex=ParsePositiveIndex(text,prefix); std::string fn,ignored;
        bool ok=Actions->CreateMonitorImage(screenIndex-1,fn,ignored);
        if(!ok) { fn.clear(); error.clear(); ok=Actions->CreateDesktopImage(fn,error); }
        if(ok && !fn.empty()) return Api->SendImage(peer,fn,ScreenCaption(screenIndex),error);
        return ok;
    }
    if(text.compare(0,3,"MAP")==0 || text.compare(0,RU_MAP.size(),RU_MAP)==0) {
        size_t prefix=text.compare(0,3,"MAP")==0?3:RU_MAP.size();
        int mapIndex=ParsePositiveIndex(text,prefix); if(mapIndex<=0) return true;
        std::string fn; if(!Actions->CreateMapImage(mapIndex-1,fn,error)) return false;
        return fn.empty()?true:Api->SendImage(peer,fn,MapCaption(mapIndex),error);
    }
    if(text.compare(0,4,"STOP")==0 || text.compare(0,RU_STOP.size(),RU_STOP)==0) {
        if(!Actions->CloseAlarmWindow(error)) return false;
        return Api->SendMessage(peer,StopDoneText(),error);
    }
    if(text.compare(0,6,"LOGXLS")==0) {
        std::string fn; if(!Actions->ExportLogXls(fn,error)) return false;
        return fn.empty()?true:Api->SendFile(peer,fn,LogCaption(Actions->CurrentDateTimeText()),error);
    }
    if(text.compare(0,3,"LOG")==0 || text.compare(0,RU_LOG.size(),RU_LOG)==0) {
        std::string fn; if(!Actions->ExportLogHtml(fn,error)) return false;
        return fn.empty()?true:Api->SendFile(peer,fn,LogCaption(Actions->CurrentDateTimeText()),error);
    }
    if(text.compare(0,5,"ALARM")==0 || text.compare(0,RU_ALARM.size(),RU_ALARM)==0) {
        std::string fn; if(!Actions->CreateAlarmsPdf(fn,error)) return false;
        return fn.empty()?true:Api->SendFile(peer,fn,AlarmCaption(Actions->CurrentDateTimeText()),error);
    }
    if(text.compare(0,4,"HELP")==0 || (!text.empty() && text[0]=='?'))
        return Api->SendMessage(peer,HelpText(),error);
    return true;
}
