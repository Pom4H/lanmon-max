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

static std::string Trim(const std::string &s)
{
    size_t begin=0;
    size_t end=s.size();

    while(begin<end && (s[begin]==' ' || s[begin]=='\t' || s[begin]=='\r' || s[begin]=='\n'))
        ++begin;
    while(end>begin && (s[end-1]==' ' || s[end-1]=='\t' || s[end-1]=='\r' || s[end-1]=='\n'))
        --end;

    return s.substr(begin,end-begin);
}

static bool ToBool(const std::string &s,bool defaultValue)
{
    if(s=="1" || s=="true" || s=="TRUE" || s=="True") return true;
    if(s=="0" || s=="false" || s=="FALSE" || s=="False") return false;
    return defaultValue;
}

static max_int64 ToInt64(const std::string &s)
{
#ifdef __BORLANDC__
    return _atoi64(s.c_str());
#else
    return (max_int64)strtoll(s.c_str(),0,10);
#endif
}

static std::string Int64Text(max_int64 value)
{
    return MaxInt64ToString(value);
}

static std::string BoolText(bool value)
{
    return value?"1":"0";
}

static std::string UnsignedLongText(unsigned long value)
{
    std::ostringstream out;
    out << value;
    return out.str();
}

static std::string IntText(int value)
{
    std::ostringstream out;
    out << value;
    return out.str();
}

static MAX_PEER_TYPE ParsePeerType(const std::string &s)
{
    return (s=="chat" || s=="CHAT" || s=="1")?maxPeerChat:maxPeerUser;
}

static const char *PeerTypeName(MAX_PEER_TYPE type)
{
    return type==maxPeerChat?"chat":"user";
}

typedef std::map<std::string,std::string> MAX_INI_SECTION;
typedef std::map<std::string,MAX_INI_SECTION> MAX_INI;

static void ReadIni(std::istream &stream,MAX_INI &ini)
{
    std::string section;
    std::string line;

    while(std::getline(stream,line)) {
        line=Trim(line);
        if(line.empty() || line[0]==';' || line[0]=='#') continue;

        if(line[0]=='[' && line[line.size()-1]==']') {
            section=line.substr(1,line.size()-2);
            continue;
        }

        const size_t separator=line.find('=');
        if(separator==std::string::npos) continue;

        ini[section][Trim(line.substr(0,separator))]=Trim(line.substr(separator+1));
    }
}

static std::string GetValue(const MAX_INI_SECTION &section,
                            const std::string &key,
                            const std::string &defaultValue)
{
    MAX_INI_SECTION::const_iterator it=section.find(key);
    return it==section.end()?defaultValue:it->second;
}

bool MaxLoadIni(const std::string &filename,
                MAX_BOT_SETTINGS &settings,
                MAX_USER_LIST &users,
                std::string &error)
{
    std::ifstream file(filename.c_str());
    if(!file) {
        error="cannot open MAX ini: "+filename;
        return false;
    }

    MAX_INI ini;
    ReadIni(file,ini);

    MAX_INI::const_iterator setupIt=ini.find("SETUP");
    if(setupIt==ini.end()) setupIt=ini.find("Setup");
    const MAX_INI_SECTION emptySection;
    const MAX_INI_SECTION &setup=setupIt==ini.end()?emptySection:setupIt->second;

    settings.Active=ToBool(GetValue(setup,"Active","1"),true);
    settings.BotToken=GetValue(setup,"BotToken","");
    settings.MyBotInfo.Id=ToInt64(GetValue(setup,"MyBotId","0"));
    settings.MyBotInfo.FirstName=GetValue(setup,"MyBotName","");
    settings.MyBotInfo.UserName=GetValue(setup,"MyBotUserName","");
    settings.PeriodReadMessages=(unsigned int)std::atoi(GetValue(setup,"PeriodReadMessages","0").c_str());
    settings.PeriodicReadMessagesPaused=ToBool(GetValue(setup,"PeriodicReadMessagesPaused","0"),false);
    settings.FlagSendAlarms=ToBool(GetValue(setup,"SendAlarms","0"),false);
    settings.FlagSendAlarmsEnd=ToBool(GetValue(setup,"SendAlarmEnd","0"),false);
    settings.FlagOperatorAlarm=ToBool(GetValue(setup,"OperatorAlarm","0"),false);
    settings.AlarmAlias=GetValue(setup,"AlarmAlias","");
    settings.RequestAlias=GetValue(setup,"RequestAlias","");
    settings.FlagSendMaps=ToBool(GetValue(setup,"SendMaps","0"),false);
    settings.UseLanmonLog=ToBool(GetValue(setup,"UseLanmonLog","0"),false);

    users.Clear();
    for(int index=0;;++index) {
        std::ostringstream sectionName;
        sectionName << "User" << index;

        MAX_INI::const_iterator userIt=ini.find(sectionName.str());
        if(userIt==ini.end()) break;

        const MAX_INI_SECTION &section=userIt->second;
        MAX_USER user;
        user.Name=GetValue(section,"Name","");
        user.Id=ToInt64(GetValue(section,"id","0"));
        user.PeerType=ParsePeerType(GetValue(section,"PeerType","user"));
        user.IsBot=ToBool(GetValue(section,"IsBot","0"),false);
        user.Alias=GetValue(section,"Alias","");
        user.InCount=(unsigned long)ToInt64(GetValue(section,"InCount","0"));
        user.OutCount=(unsigned long)ToInt64(GetValue(section,"OutCount","0"));
        user.Comment=GetValue(section,"Comment","");
        user.Tag=std::atoi(GetValue(section,"Tag","0").c_str());
        users.Add(user);
    }

    return true;
}

bool MaxSaveIni(const std::string &filename,
                const MAX_BOT_SETTINGS &settings,
                const MAX_USER_LIST &users,
                std::string &error)
{
    std::ofstream file(filename.c_str(),std::ios::out|std::ios::trunc);
    if(!file) {
        error="cannot write MAX ini: "+filename;
        return false;
    }

    file << "[SETUP]\n"
         << "Active=" << BoolText(settings.Active) << "\n"
         << "BotToken=" << settings.BotToken << "\n"
         << "MyBotId=" << Int64Text(settings.MyBotInfo.Id) << "\n"
         << "MyBotName=" << settings.MyBotInfo.FirstName << "\n"
         << "MyBotUserName=" << settings.MyBotInfo.UserName << "\n"
         << "PeriodReadMessages=" << settings.PeriodReadMessages << "\n"
         << "PeriodicReadMessagesPaused=" << BoolText(settings.PeriodicReadMessagesPaused) << "\n"
         << "SendAlarms=" << BoolText(settings.FlagSendAlarms) << "\n"
         << "SendAlarmEnd=" << BoolText(settings.FlagSendAlarmsEnd) << "\n"
         << "OperatorAlarm=" << BoolText(settings.FlagOperatorAlarm) << "\n"
         << "AlarmAlias=" << settings.AlarmAlias << "\n"
         << "RequestAlias=" << settings.RequestAlias << "\n"
         << "SendMaps=" << BoolText(settings.FlagSendMaps) << "\n"
         << "UseLanmonLog=" << BoolText(settings.UseLanmonLog) << "\n";

    for(size_t i=0;i<users.Count();++i) {
        const MAX_USER *user=users.Get(i);
        file << "\n[User" << i << "]\n"
             << "Name=" << user->Name << "\n"
             << "id=" << Int64Text(user->Id) << "\n"
             << "PeerType=" << PeerTypeName(user->PeerType) << "\n"
             << "IsBot=" << BoolText(user->IsBot) << "\n"
             << "Alias=" << user->Alias << "\n"
             << "InCount=" << UnsignedLongText(user->InCount) << "\n"
             << "OutCount=" << UnsignedLongText(user->OutCount) << "\n"
             << "Comment=" << user->Comment << "\n"
             << "Tag=" << IntText(user->Tag) << "\n";
    }

    return true;
}
