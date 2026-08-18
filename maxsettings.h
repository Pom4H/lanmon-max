#ifndef maxsettingsH
#define maxsettingsH

#include "maxusers.h"
#include <string>

struct MAX_BOT_SETTINGS
{
    bool Active;
    std::string BotToken;
    MAX_BOT_INFO MyBotInfo;
    unsigned int PeriodReadMessages;
    bool PeriodicReadMessagesPaused;
    bool FlagSendAlarms;
    bool FlagSendAlarmsEnd;
    bool FlagOperatorAlarm;
    std::string AlarmAlias;
    std::string RequestAlias;
    bool FlagSendMaps;
    bool UseLanmonLog;

    MAX_BOT_SETTINGS();
};

bool MaxLoadIni(const std::string & filename, MAX_BOT_SETTINGS & settings, MAX_USER_LIST & users, std::string & error);
bool MaxSaveIni(const std::string & filename, const MAX_BOT_SETTINGS & settings, const MAX_USER_LIST & users, std::string & error);

#endif
