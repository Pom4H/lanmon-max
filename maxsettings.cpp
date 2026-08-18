#include "maxsettings.h"
#include <fstream>
#include <sstream>
#include <map>
#include <cstdlib>

MAX_BOT_SETTINGS::MAX_BOT_SETTINGS()
    : Active(true), PeriodReadMessages(0), PeriodicReadMessagesPaused(false),
      FlagSendAlarms(false), FlagSendAlarmsEnd(false), FlagOperatorAlarm(false),
      FlagSendMaps(false), UseLanmonLog(false)
{
}

static std::string Trim(const std::string & s)
{
    size_t a=0,b=s.size(); while(a<b && (s[a]==' '||s[a]=='\t'||s[a]=='\r'||s[a]=='\n')) ++a;
    while(b>a && (s[b-1]==' '||s[b-1]=='\t'||s[b-1]=='\r'||s[b-1]=='\n')) --b;
    return s.substr(a,b-a);
}

static bool ToBool(const std::string & s,bool d)
{
    if(s=="1"||s=="true"||s=="TRUE"||s=="True") return true;
    if(s=="0"||s=="false"||s=="FALSE"||s=="False") return false;
    return d;
}

static max_int64 ToI64(const std::string & s)
{
#ifdef __BORLANDC__
    return _atoi64(s.c_str());
#else
    return (max_int64)strtoll(s.c_str(),0,10);
#endif
}

static std::string I64(max_int64 v) { return MaxInt64ToString(v); }
static std::string Bool(bool v) { return v?"1":"0"; }
static std::string ULong(unsigned long v) { std::ostringstream os; os<<v; return os.str(); }
static std::string Int(int v) { std::ostringstream os; os<<v; return os.str(); }

bool MaxLoadIni(const std::string & filename, MAX_BOT_SETTINGS & s, MAX_USER_LIST & users, std::string & error)
{
    std::ifstream f(filename.c_str()); if(!f){error="cannot open MAX ini: "+filename;return false;}
    std::map<std::string,std::map<std::string,std::string> > ini; std::string section,line;
    while(std::getline(f,line)) {
        line=Trim(line); if(line.empty()||line[0]==';'||line[0]=='#') continue;
        if(line[0]=='[' && line[line.size()-1]==']'){section=line.substr(1,line.size()-2);continue;}
        size_t p=line.find('='); if(p==std::string::npos) continue;
        ini[section][Trim(line.substr(0,p))]=Trim(line.substr(p+1));
    }
    std::map<std::string,std::string> & x=ini[ini.find("SETUP")!=ini.end()?"SETUP":"Setup"];
#define GET(k,d) (x.find(k)!=x.end()?x[k]:std::string(d))
    s.Active=ToBool(GET("Active","1"),true); s.BotToken=GET("BotToken","");
    s.MyBotInfo.Id=ToI64(GET("MyBotId","0")); s.MyBotInfo.FirstName=GET("MyBotName",""); s.MyBotInfo.UserName=GET("MyBotUserName","");
    s.PeriodReadMessages=(unsigned int)std::atoi(GET("PeriodReadMessages","0").c_str());
    s.PeriodicReadMessagesPaused=ToBool(GET("PeriodicReadMessagesPaused","0"),false);
    s.FlagSendAlarms=ToBool(GET("SendAlarms","0"),false); s.FlagSendAlarmsEnd=ToBool(GET("SendAlarmEnd","0"),false);
    s.FlagOperatorAlarm=ToBool(GET("OperatorAlarm","0"),false); s.AlarmAlias=GET("AlarmAlias",""); s.RequestAlias=GET("RequestAlias","");
    s.FlagSendMaps=ToBool(GET("SendMaps","0"),false); s.UseLanmonLog=ToBool(GET("UseLanmonLog","0"),false);
#undef GET
    users.Clear();
    for(int i=0;;++i) {
        std::ostringstream sn; sn<<"User"<<i; if(ini.find(sn.str())==ini.end()) break;
        std::map<std::string,std::string> & u=ini[sn.str()]; MAX_USER user;
#define UGET(k,d) (u.find(k)!=u.end()?u[k]:std::string(d))
        user.Name=UGET("Name",""); user.Id=ToI64(UGET("id","0")); user.IsBot=ToBool(UGET("IsBot","0"),false);
        user.Alias=UGET("Alias",""); user.InCount=(unsigned long)ToI64(UGET("InCount","0")); user.OutCount=(unsigned long)ToI64(UGET("OutCount","0"));
        user.Comment=UGET("Comment",""); user.Tag=std::atoi(UGET("Tag","0").c_str()); users.Add(user);
#undef UGET
    }
    return true;
}

bool MaxSaveIni(const std::string & filename, const MAX_BOT_SETTINGS & s, const MAX_USER_LIST & users, std::string & error)
{
    std::ofstream f(filename.c_str(),std::ios::out|std::ios::trunc); if(!f){error="cannot write MAX ini: "+filename;return false;}
    f << "[SETUP]\nActive="<<Bool(s.Active)<<"\nBotToken="<<s.BotToken<<"\nMyBotId="<<I64(s.MyBotInfo.Id)
      <<"\nMyBotName="<<s.MyBotInfo.FirstName<<"\nMyBotUserName="<<s.MyBotInfo.UserName
      <<"\nPeriodReadMessages="<<s.PeriodReadMessages<<"\nPeriodicReadMessagesPaused="<<Bool(s.PeriodicReadMessagesPaused)
      <<"\nSendAlarms="<<Bool(s.FlagSendAlarms)<<"\nSendAlarmEnd="<<Bool(s.FlagSendAlarmsEnd)
      <<"\nOperatorAlarm="<<Bool(s.FlagOperatorAlarm)<<"\nAlarmAlias="<<s.AlarmAlias<<"\nRequestAlias="<<s.RequestAlias
      <<"\nSendMaps="<<Bool(s.FlagSendMaps)<<"\nUseLanmonLog="<<Bool(s.UseLanmonLog)<<"\n";
    for(size_t i=0;i<users.Count();++i) { const MAX_USER *u=users.Get(i); f<<"\n[User"<<i<<"]\nName="<<u->Name<<"\nid="<<I64(u->Id)<<"\nIsBot="<<Bool(u->IsBot)<<"\nAlias="<<u->Alias<<"\nInCount="<<ULong(u->InCount)<<"\nOutCount="<<ULong(u->OutCount)<<"\nComment="<<u->Comment<<"\nTag="<<Int(u->Tag)<<"\n"; }
    return true;
}
