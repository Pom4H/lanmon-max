#include "lanmon_bot.h"
#include <vector>
#include <cctype>
#include <cstdlib>

LANMON_MAX_BOT::LANMON_MAX_BOT(MAX_API_CLIENT *api, ILanMonCommandActions *actions, ILanMonMaxEvents *events)
    : Api(api), Actions(actions), Events(events), Router(api,actions), ReadMessagesCount(0), ReadMessagesCountOk(0), UserMessageCount(0)
{
}

MAX_PEER LANMON_MAX_BOT::PeerForMessage(const MAX_MESSAGE & msg) const
{
    return msg.ChatId?MAX_PEER(maxPeerChat,msg.ChatId):MAX_PEER(maxPeerUser,msg.UserId);
}
void LANMON_MAX_BOT::Error(const std::string &s){if(Events)Events->OnErrorDebugMessage(s);}
void LANMON_MAX_BOT::Debug(const std::string &s){if(Events)Events->OnDebugMessage(s);}

bool LANMON_MAX_BOT::Load(const std::string &fn,std::string &error)
{
    if(!MaxLoadIni(fn,Settings,UserList,error)){Error(error);return false;} if(Api)Api->SetToken(Settings.BotToken); return true;
}
bool LANMON_MAX_BOT::Save(const std::string &fn,std::string &error) const { return MaxSaveIni(fn,Settings,UserList,error); }

bool LANMON_MAX_BOT::GetMe(std::string &error)
{
    if(!Api){error="MAX API is null";Error(error);return false;} MAX_BOT_INFO info;
    if(!Api->GetMe(info,error)){Error(error);return false;} Settings.MyBotInfo=info; if(Events)Events->OnGetMe(info); Debug("MAX getMe ok"); return true;
}

bool LANMON_MAX_BOT::ReadMessages(bool periodic,std::string &error,int timeoutSeconds,int limit)
{
    ++ReadMessagesCount;
    if(periodic && Settings.PeriodicReadMessagesPaused) return true;
    if(!Settings.Active) return true;
    if(!Api){error="MAX API is null";Error(error);return false;}
    MAX_UPDATES updates; if(!Api->Poll(updates,error,timeoutSeconds,limit)){Error(error);return false;} ++ReadMessagesCountOk;
    if(periodic) { if(!updates.Messages.empty() && Events)Events->OnPeriodicReadMessages(updates); }
    else if(Events) Events->OnTaskReadMessages(updates);
    if(!OnMessages(updates,error)){Error(error);return false;} return true;
}

bool LANMON_MAX_BOT::OnMessages(const MAX_UPDATES &updates,std::string &error)
{
    for(size_t i=0;i<updates.Messages.size();++i) {
        const MAX_MESSAGE &msg=updates.Messages[i]; max_int64 peerId=msg.ChatId?msg.ChatId:msg.UserId; if(!peerId)continue;
        ++UserMessageCount;
        MAX_USER *u=UserList.Find(peerId); if(!u && msg.UserId)u=UserList.Find(msg.UserId); if(u)++u->InCount;
        if(Events)Events->OnMaxMessage(msg.UpdateTimestamp,peerId,msg.Text);
        if(!Settings.FlagSendMaps)continue;
        if(!u)continue;
        if(!u->HasValidAlias(Settings.RequestAlias))continue;
        if(!Router.Handle(msg,error))return false;
    }
    return true;
}

static bool SendUserMessage(MAX_API_CLIENT *api,MAX_USER *u,const std::string &text,std::string &error)
{ if(!api||!u)return false; bool ok=api->SendMessage(u->Peer(),text,error);if(ok)++u->OutCount;return ok; }
static bool SendUserPhoto(MAX_API_CLIENT *api,MAX_USER *u,const std::string &fn,const std::string &caption,std::string &error)
{ if(!api||!u)return false; bool ok=api->SendImage(u->Peer(),fn,caption,error);if(ok)++u->OutCount;return ok; }
static bool SendUserDoc(MAX_API_CLIENT *api,MAX_USER *u,const std::string &fn,const std::string &caption,std::string &error)
{ if(!api||!u)return false; bool ok=api->SendFile(u->Peer(),fn,caption,error);if(ok)++u->OutCount;return ok; }

bool LANMON_MAX_BOT::OnNewAlarmState(const std::string &message,std::string &error)
{
    std::vector<MAX_USER*> users; UserList.GetUsersByAlias(users,Settings.AlarmAlias);
    for(size_t i=0;i<users.size();++i) if(!SendUserMessage(Api,users[i],message,error))return false;
    return true;
}

bool LANMON_MAX_BOT::SendMessage(max_int64 id,const std::string &text,std::string &error)
{
    if(!Api){error="MAX API is null";return false;} MAX_USER*u=UserList.Find(id);if(u)return SendUserMessage(Api,u,text,error);return Api->SendMessage(PeerFromId(id),text,error);
}
bool LANMON_MAX_BOT::SendPhoto(max_int64 id,const std::string &fn,const std::string &caption,std::string &error)
{
    if(!Api){error="MAX API is null";return false;} MAX_USER*u=UserList.Find(id);if(u)return SendUserPhoto(Api,u,fn,caption,error);return Api->SendImage(PeerFromId(id),fn,caption,error);
}
bool LANMON_MAX_BOT::SendDoc(max_int64 id,const std::string &fn,const std::string &caption,std::string &error)
{
    if(!Api){error="MAX API is null";return false;} MAX_USER*u=UserList.Find(id);if(u)return SendUserDoc(Api,u,fn,caption,error);return Api->SendFile(PeerFromId(id),fn,caption,error);
}

static bool NumericAlias(const std::string &s){return !s.empty() && std::isdigit((unsigned char)s[0]);}
static max_int64 ParseId(const std::string &s){
#ifdef __BORLANDC__
    return _atoi64(s.c_str());
#else
    return (max_int64)strtoll(s.c_str(),0,10);
#endif
}

bool LANMON_MAX_BOT::SendMessageByAlias(const std::string &alias,const std::string &text,std::string &error)
{
    std::vector<MAX_USER*> us; UserList.GetUsersByAlias(us,alias); if(!us.empty()){for(size_t i=0;i<us.size();++i)if(!SendUserMessage(Api,us[i],text,error))return false;return true;} return !NumericAlias(alias)||Api->SendMessage(MAX_PEER(maxPeerUser,ParseId(alias)),text,error);
}
bool LANMON_MAX_BOT::SendPhotoByAlias(const std::string &alias,const std::string &fn,const std::string &caption,std::string &error)
{
    std::vector<MAX_USER*> us; UserList.GetUsersByAlias(us,alias); if(!us.empty()){for(size_t i=0;i<us.size();++i)if(!SendUserPhoto(Api,us[i],fn,caption,error))return false;return true;} return !NumericAlias(alias)||Api->SendImage(MAX_PEER(maxPeerUser,ParseId(alias)),fn,caption,error);
}
bool LANMON_MAX_BOT::SendDocByAlias(const std::string &alias,const std::string &fn,const std::string &caption,std::string &error)
{
    std::vector<MAX_USER*> us; UserList.GetUsersByAlias(us,alias); if(!us.empty()){for(size_t i=0;i<us.size();++i)if(!SendUserDoc(Api,us[i],fn,caption,error))return false;return true;} return !NumericAlias(alias)||Api->SendFile(MAX_PEER(maxPeerUser,ParseId(alias)),fn,caption,error);
}

bool LANMON_MAX_BOT::SetUserTag(size_t i,int tag){MAX_USER*u=UserList.Get(i);if(!u)return false;u->Tag=tag;return true;}
bool LANMON_MAX_BOT::UserCanAsk(size_t i)const{const MAX_USER*u=UserList.Get(i);return u?u->HasValidAlias(Settings.RequestAlias):false;}
bool LANMON_MAX_BOT::UserRcvAlarms(size_t i)const{const MAX_USER*u=UserList.Get(i);return u?u->HasValidAlias(Settings.AlarmAlias):false;}
bool LANMON_MAX_BOT::UserHasValidAlias(size_t i,const std::string&a)const{const MAX_USER*u=UserList.Get(i);return u?u->HasValidAlias(a):false;}
