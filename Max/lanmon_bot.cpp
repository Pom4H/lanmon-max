#include "lanmon_bot.h"
#include <vector>
#include <cctype>
#include <cstdlib>

LANMON_MAX_BOT::LANMON_MAX_BOT(MAX_API_CLIENT *api, ILanMonCommandActions *actions, ILanMonMaxEvents *events)
    : Api(api), Events(events), Router(api,actions),
      ReadMessagesCount(0), ReadMessagesCountOk(0), UserMessageCount(0)
{
}

void LANMON_MAX_BOT::Error(const std::string &s)
{
    if(Events) Events->OnErrorDebugMessage(s);
}

void LANMON_MAX_BOT::Debug(const std::string &s)
{
    if(Events) Events->OnDebugMessage(s);
}

bool LANMON_MAX_BOT::Load(const std::string &fn,std::string &error)
{
    if(!MaxLoadIni(fn,Settings,UserList,error)) {
        Error(error);
        return false;
    }
    if(Api) Api->SetToken(Settings.BotToken);
    return true;
}

bool LANMON_MAX_BOT::Save(const std::string &fn,std::string &error) const
{
    return MaxSaveIni(fn,Settings,UserList,error);
}

bool LANMON_MAX_BOT::GetMe(std::string &error)
{
    if(!Api) {
        error="MAX API is null";
        Error(error);
        return false;
    }

    MAX_BOT_INFO info;
    if(!Api->GetMe(info,error)) {
        Error(error);
        return false;
    }

    Settings.MyBotInfo=info;
    if(Events) Events->OnGetMe(info);
    Debug("MAX getMe ok");
    return true;
}

bool LANMON_MAX_BOT::ReadMessages(bool periodic,std::string &error,int timeoutSeconds,int limit)
{
    ++ReadMessagesCount;
    if(periodic && Settings.PeriodicReadMessagesPaused) return true;
    if(!Settings.Active) return true;
    if(!Api) {
        error="MAX API is null";
        Error(error);
        return false;
    }

    MAX_UPDATES updates;
    if(!Api->Poll(updates,error,timeoutSeconds,limit)) {
        Error(error);
        return false;
    }
    ++ReadMessagesCountOk;

    if(periodic) {
        if(!updates.Messages.empty() && Events)
            Events->OnPeriodicReadMessages(updates);
    } else if(Events) {
        Events->OnTaskReadMessages(updates);
    }

    if(!OnMessages(updates,error)) {
        Error(error);
        return false;
    }
    return true;
}

bool LANMON_MAX_BOT::OnMessages(const MAX_UPDATES &updates,std::string &error)
{
    for(size_t i=0;i<updates.Messages.size();++i) {
        const MAX_MESSAGE &msg=updates.Messages[i];
        const max_int64 peerId=msg.ChatId?msg.ChatId:msg.UserId;
        if(!peerId) continue;

        ++UserMessageCount;
        MAX_USER *user=UserList.Find(peerId);
        if(!user && msg.UserId) user=UserList.Find(msg.UserId);
        if(user) ++user->InCount;

        if(Events) Events->OnMaxMessage(msg.UpdateTimestamp,peerId,msg.Text);

        // Сохраняем семантику Telegram: FlagSendMaps блокирует все встроенные команды,
        // а callback выше вызывается до проверки разрешений.
        if(!Settings.FlagSendMaps) continue;
        if(!user) continue;
        if(!user->HasValidAlias(Settings.RequestAlias)) continue;
        if(!Router.Handle(msg,error)) return false;
    }
    return true;
}

bool LANMON_MAX_BOT::SendToPeer(const MAX_PEER &peer, SEND_KIND kind,
                                const std::string &payload, const std::string &caption,
                                std::string &error)
{
    if(!Api) {
        error="MAX API is null";
        return false;
    }

    switch(kind) {
        case sendText:
            return Api->SendMessage(peer,payload,error);
        case sendPhoto:
            return Api->SendImage(peer,payload,caption,error);
        case sendDoc:
            return Api->SendFile(peer,payload,caption,error);
    }

    error="unknown MAX send kind";
    return false;
}

bool LANMON_MAX_BOT::SendToUser(MAX_USER *user, SEND_KIND kind,
                                const std::string &payload, const std::string &caption,
                                std::string &error)
{
    if(!user) {
        error="MAX user is null";
        return false;
    }

    const bool ok=SendToPeer(user->Peer(),kind,payload,caption,error);
    if(ok) ++user->OutCount;
    return ok;
}

static bool NumericAlias(const std::string &s)
{
    return !s.empty() && std::isdigit((unsigned char)s[0]);
}

static max_int64 ParseId(const std::string &s)
{
#ifdef __BORLANDC__
    return _atoi64(s.c_str());
#else
    return (max_int64)strtoll(s.c_str(),0,10);
#endif
}

bool LANMON_MAX_BOT::SendByAlias(const std::string &alias, SEND_KIND kind,
                                 const std::string &payload, const std::string &caption,
                                 std::string &error)
{
    std::vector<MAX_USER*> users;
    UserList.GetUsersByAlias(users,alias);

    if(!users.empty()) {
        for(size_t i=0;i<users.size();++i)
            if(!SendToUser(users[i],kind,payload,caption,error)) return false;
        return true;
    }

    if(!NumericAlias(alias)) return true;
    return SendToPeer(MAX_PEER(maxPeerUser,ParseId(alias)),kind,payload,caption,error);
}

bool LANMON_MAX_BOT::OnNewAlarmState(const std::string &message,std::string &error)
{
    std::vector<MAX_USER*> users;
    UserList.GetUsersByAlias(users,Settings.AlarmAlias);
    for(size_t i=0;i<users.size();++i)
        if(!SendToUser(users[i],sendText,message,std::string(),error)) return false;
    return true;
}

bool LANMON_MAX_BOT::SendMessage(max_int64 id,const std::string &text,std::string &error)
{
    MAX_USER *user=UserList.Find(id);
    if(user) return SendToUser(user,sendText,text,std::string(),error);
    return SendToPeer(PeerFromId(id),sendText,text,std::string(),error);
}

bool LANMON_MAX_BOT::SendPhoto(max_int64 id,const std::string &fn,const std::string &caption,std::string &error)
{
    MAX_USER *user=UserList.Find(id);
    if(user) return SendToUser(user,sendPhoto,fn,caption,error);
    return SendToPeer(PeerFromId(id),sendPhoto,fn,caption,error);
}

bool LANMON_MAX_BOT::SendDoc(max_int64 id,const std::string &fn,const std::string &caption,std::string &error)
{
    MAX_USER *user=UserList.Find(id);
    if(user) return SendToUser(user,sendDoc,fn,caption,error);
    return SendToPeer(PeerFromId(id),sendDoc,fn,caption,error);
}

bool LANMON_MAX_BOT::SendMessageByAlias(const std::string &alias,const std::string &text,std::string &error)
{
    return SendByAlias(alias,sendText,text,std::string(),error);
}

bool LANMON_MAX_BOT::SendPhotoByAlias(const std::string &alias,const std::string &fn,const std::string &caption,std::string &error)
{
    return SendByAlias(alias,sendPhoto,fn,caption,error);
}

bool LANMON_MAX_BOT::SendDocByAlias(const std::string &alias,const std::string &fn,const std::string &caption,std::string &error)
{
    return SendByAlias(alias,sendDoc,fn,caption,error);
}

bool LANMON_MAX_BOT::SetUserTag(size_t i,int tag)
{
    MAX_USER *user=UserList.Get(i);
    if(!user) return false;
    user->Tag=tag;
    return true;
}

bool LANMON_MAX_BOT::UserCanAsk(size_t i) const
{
    const MAX_USER *user=UserList.Get(i);
    return user?user->HasValidAlias(Settings.RequestAlias):false;
}

bool LANMON_MAX_BOT::UserRcvAlarms(size_t i) const
{
    const MAX_USER *user=UserList.Get(i);
    return user?user->HasValidAlias(Settings.AlarmAlias):false;
}

bool LANMON_MAX_BOT::UserHasValidAlias(size_t i,const std::string &alias) const
{
    const MAX_USER *user=UserList.Get(i);
    return user?user->HasValidAlias(alias):false;
}
