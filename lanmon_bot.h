#ifndef lanmon_botH
#define lanmon_botH

#include "maxclient.h"
#include "maxsettings.h"
#include "lanmon_commands.h"
#include <string>

class ILanMonMaxEvents
{
public:
    virtual ~ILanMonMaxEvents() {}
    virtual void OnDebugMessage(const std::string &) {}
    virtual void OnErrorDebugMessage(const std::string &) {}
    virtual void OnTaskReadMessages(const MAX_UPDATES &) {}
    virtual void OnPeriodicReadMessages(const MAX_UPDATES &) {}
    virtual void OnGetMe(const MAX_BOT_INFO &) {}
    virtual void OnMaxMessage(max_int64, max_int64, const std::string &) {}
};

class LANMON_MAX_BOT
{
    enum SEND_KIND
    {
        sendText,
        sendPhoto,
        sendDoc
    };

    MAX_API_CLIENT *Api;
    ILanMonMaxEvents *Events;
    LANMON_MAX_COMMAND_ROUTER Router;

    MAX_PEER PeerFromId(max_int64 id) const { return MAX_PEER(maxPeerUser,id); }
    bool SendToPeer(const MAX_PEER & peer, SEND_KIND kind,
                    const std::string & payload, const std::string & caption,
                    std::string & error);
    bool SendToUser(MAX_USER * user, SEND_KIND kind,
                    const std::string & payload, const std::string & caption,
                    std::string & error);
    bool SendByAlias(const std::string & alias, SEND_KIND kind,
                     const std::string & payload, const std::string & caption,
                     std::string & error);
    void Error(const std::string & s);
    void Debug(const std::string & s);
public:
    MAX_BOT_SETTINGS Settings;
    MAX_USER_LIST UserList;
    unsigned long ReadMessagesCount;
    unsigned long ReadMessagesCountOk;
    unsigned long UserMessageCount;

    LANMON_MAX_BOT(MAX_API_CLIENT *api, ILanMonCommandActions *actions, ILanMonMaxEvents *events=0);
    void SetEvents(ILanMonMaxEvents *events) { Events=events; }

    bool Load(const std::string & filename, std::string & error);
    bool Save(const std::string & filename, std::string & error) const;
    bool GetMe(std::string & error);
    bool ReadMessages(bool periodic, std::string & error, int timeoutSeconds=30, int limit=100);
    bool OnMessages(const MAX_UPDATES & updates, std::string & error);
    bool OnNewAlarmState(const std::string & utf8Message, std::string & error);

    bool SendMessage(max_int64 id, const std::string & utf8Text, std::string & error);
    bool SendPhoto(max_int64 id, const std::string & filename, const std::string & utf8Caption, std::string & error);
    bool SendDoc(max_int64 id, const std::string & filename, const std::string & utf8Caption, std::string & error);
    bool SendMessageByAlias(const std::string & alias, const std::string & utf8Text, std::string & error);
    bool SendPhotoByAlias(const std::string & alias, const std::string & filename, const std::string & utf8Caption, std::string & error);
    bool SendDocByAlias(const std::string & alias, const std::string & filename, const std::string & utf8Caption, std::string & error);

    size_t UserCount() const { return UserList.Count(); }
    MAX_USER *GetUser(size_t index) { return UserList.Get(index); }
    MAX_USER *FindUser(max_int64 id) { return UserList.Find(id); }
    int FindUserIndex(max_int64 id) const { return UserList.IndexOfId(id); }
    MAX_USER *FindUserAlias(const std::string & alias) { return UserList.FindAlias(alias); }
    bool SetUserTag(size_t index,int tag);
    bool UserCanAsk(size_t index) const;
    bool UserRcvAlarms(size_t index) const;
    bool UserHasValidAlias(size_t index,const std::string & alias) const;
};

#endif
