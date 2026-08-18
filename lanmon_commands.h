#ifndef lanmon_commandsH
#define lanmon_commandsH

#include "maxclient.h"
#include <string>

class ILanMonCommandActions
{
public:
    virtual ~ILanMonCommandActions() {}
    virtual bool CloseAlarmWindow(std::string & error)=0;
    virtual bool CreateMapImage(int zeroBasedMapIndex, std::string & filename, std::string & error)=0;
    virtual bool CreateMonitorImage(int zeroBasedMonitorIndex, std::string & filename, std::string & error)=0;
    virtual bool CreateDesktopImage(std::string & filename, std::string & error)=0;
    virtual bool ExportLogHtml(std::string & filename, std::string & error)=0;
    virtual bool ExportLogXls(std::string & filename, std::string & error)=0;
    virtual bool CreateAlarmsPdf(std::string & filename, std::string & error)=0;
    virtual std::string CurrentDateTimeText() const=0;
};

class LANMON_MAX_COMMAND_ROUTER
{
    MAX_API_CLIENT * Api;
    ILanMonCommandActions * Actions;
public:
    LANMON_MAX_COMMAND_ROUTER(MAX_API_CLIENT * api, ILanMonCommandActions * actions)
        : Api(api), Actions(actions) {}

    bool Handle(const MAX_MESSAGE & msg, std::string & error);

private:
    static std::string TrimUpper(const std::string & text);
    static int ParsePositiveIndex(const std::string & text, size_t prefixLen);
    MAX_PEER PeerFor(const MAX_MESSAGE & msg) const;
};

#endif
