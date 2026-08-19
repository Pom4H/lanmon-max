//---------------------------------------------------------------------------
#include <vcl.h>
#include <windows.h>
#include <time.h>
#include <vector>
//---------------------------------------------------------------------------
#pragma hdrstop
#include "maxmsg.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
AnsiString MaxAnsiFromUtf8(const std::string & s)
{
    if(s.empty())return "";
    int wn=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,NULL,0);
    if(wn<=0)return AnsiString(s.c_str());
    std::vector<wchar_t> w((size_t)wn);
    MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,&w[0],wn);
    int an=WideCharToMultiByte(1251,0,&w[0],-1,NULL,0,NULL,NULL);
    if(an<=0)return AnsiString(s.c_str());
    std::vector<char> a((size_t)an);
    WideCharToMultiByte(1251,0,&w[0],-1,&a[0],an,NULL,NULL);
    return AnsiString(&a[0]);
}
//---------------------------------------------------------------------------
void MaxFrom::CopyFrom(MaxFrom * from)
{
    Id=from->Id;
    is_bot=from->is_bot;
    first_name=from->first_name;
    last_name=from->last_name;
    language_code=from->language_code;
}
//---------------------------------------------------------------------------
AnsiString MaxFrom::GetFullName(void)
{
    AnsiString s;
    if(first_name.Length())s=first_name;
    if(last_name.Length())
    {
        if(s.Length())s+=" ";
        s+=last_name;
    }
    return s;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void MaxChat::CopyFrom(MaxChat * chat)
{
    Id=chat->Id;
    first_name=chat->first_name;
    last_name=chat->last_name;
    type=chat->type;
    username=chat->username;
}
//---------------------------------------------------------------------------
AnsiString MaxChat::GetFullName(void)
{
    if(username.Length())return username;
    AnsiString s;
    if(first_name.Length())s=first_name;
    if(last_name.Length())
    {
        if(s.Length())s+=" ";
        s+=last_name;
    }
    return s;
}
//---------------------------------------------------------------------------
AnsiString MaxMessage::AsString(void)
{
    AnsiString s;
    s=(AnsiString)"update_id="+IntToStr(update_id);
    s+=(AnsiString)" message_id="+message_id;
    s+=(AnsiString)" Date="+DateText;
    s+=(AnsiString)" Text="+Text;
    return s;
}
//---------------------------------------------------------------------------
AnsiString MaxMessage::GetDateText(void)
{
    time_t t=(time_t)Date;
    struct tm * tmv = localtime(&t);
    if(!tmv)return "";
    AnsiString s;
    s.sprintf("%02d/%02d/%04d %02d:%02d:%02d",
        tmv->tm_mday,tmv->tm_mon+1,tmv->tm_year+1900,
        tmv->tm_hour,tmv->tm_min,tmv->tm_sec);
    return s;
}
//---------------------------------------------------------------------------
TDateTime MaxMessage::GetDateTime(void)
{
    time_t t=(time_t)Date;
    struct tm * tmv = localtime(&t);
    if(!tmv)return TDateTime();
    TDateTime dt(tmv->tm_year+1900,tmv->tm_mon+1,tmv->tm_mday,
                 tmv->tm_hour,tmv->tm_min,tmv->tm_sec,0);
    return dt;
}
//---------------------------------------------------------------------------
AnsiString MaxMessage::GetTypeText(void)
{
    switch(Type)
    {
        case mmtMESSAGE:   return MaxAnsiFromUtf8("Сообщение");
        case mmtINVITATION:return MaxAnsiFromUtf8("Приглашение");
        case mmtNEWPATICIPANT:return MaxAnsiFromUtf8("Новый участник");
    }
    return "?";
}
//---------------------------------------------------------------------------
//Новый участник
AnsiString MaxMessage::GetParticipantText(void)
{
    if(Type==mmtNEWPATICIPANT || Type==mmtINVITATION)
    {
        AnsiString s=Participant.FullName;
        if(Participant.Id.Length())s+=" (id="+Participant.Id+")";
        if(Chat.Id.Length())s+=" chat: "+Chat.FullName+" ("+Chat.Id+")";
        return s;
    }
    return "";
}
//---------------------------------------------------------------------------
void MaxMessage::CopyFrom(MaxMessage * msg)
{
    Type=msg->Type;
    update_id=msg->update_id;
    message_id=msg->message_id;
    From.CopyFrom(&msg->From);
    Chat.CopyFrom(&msg->Chat);
    Participant.CopyFrom(&msg->Participant);
    Date=msg->Date;
    Text=msg->Text;
}
//---------------------------------------------------------------------------
void MaxMessage::CopyFrom(const MAX_MESSAGE & msg)
{
    Type=mmtMESSAGE;
    update_id=(__int64)msg.UpdateTimestamp;
    message_id=msg.MessageId.c_str();
    From.Clear();
    From.Id=MaxInt64ToString(msg.UserId).c_str();
    From.is_bot=msg.SenderIsBot;
    From.first_name=MaxAnsiFromUtf8(msg.FirstName);
    From.last_name=MaxAnsiFromUtf8(msg.LastName);
    Chat.Clear();
    Chat.Id=MaxInt64ToString(msg.ChatId).c_str();
    Chat.type=msg.ChatType.c_str();
    Chat.first_name=From.first_name;
    Chat.last_name=From.last_name;
    Chat.username=MaxAnsiFromUtf8(msg.UserName);
    Date=(long)(msg.MessageTimestamp/1000);
    Text=MaxAnsiFromUtf8(msg.Text);
}
//---------------------------------------------------------------------------
//Список сообщений
//---------------------------------------------------------------------------
MaxMessage_LIST::MaxMessage_LIST()
{
    List=new TList;
}
//---------------------------------------------------------------------------
MaxMessage_LIST::~MaxMessage_LIST()
{
    Clear();
    delete List;
}
//---------------------------------------------------------------------------
MaxMessage * MaxMessage_LIST::Get(int index)
{
    if(index<0 || index>=Count)return NULL;
    return (MaxMessage *)List->Items[index];
}
//---------------------------------------------------------------------------
void MaxMessage_LIST::Delete(int index)
{
    MaxMessage * tm=Get(index);
    if(tm)
    {
        delete tm;
        List->Delete(index);
    }
}
//---------------------------------------------------------------------------
void MaxMessage_LIST::Clear(void)
{
    while(Count)Delete(0);
}
//---------------------------------------------------------------------------
void MaxMessage_LIST::Remove(MaxMessage * tm)
{
    for(int i=0;i<Count;i++)
    {
        MaxMessage * tmi=Get(i);
        if(tmi && tmi==tm){Delete(i);break;}
    }
}
//---------------------------------------------------------------------------
MaxMessage * MaxMessage_LIST::Find(__int64 update_id)
{
    for(int i=0;i<Count;i++)
    {
        MaxMessage * tmi=Get(i);
        if(tmi && tmi->update_id==update_id)return tmi;
    }
    return NULL;
}
//---------------------------------------------------------------------------
MaxMessage * MaxMessage_LIST::Add(void)
{
    MaxMessage * tm=new MaxMessage();
    List->Add(tm);
    return tm;
}
//---------------------------------------------------------------------------
void MaxMessage_LIST::CopyFrom(MaxMessage_LIST & msglist)
{
    Clear();
    for(int i=0;i<msglist.Count;i++)
    {
        MaxMessage * tmi=msglist.Get(i);
        if(tmi){MaxMessage * tm=Add();tm->CopyFrom(tmi);}
    }
}
//---------------------------------------------------------------------------
void MaxMessage_LIST::CopyFrom(const MAX_UPDATES & updates)
{
    Clear();
    for(size_t i=0;i<updates.Messages.size();i++)Add()->CopyFrom(updates.Messages[i]);
}
//---------------------------------------------------------------------------
__int64 MaxMessage_LIST::GetUpdateId(void)
{
    __int64 result=0;
    for(int i=0;i<Count;i++)if(Get(i) && Get(i)->update_id>result)result=Get(i)->update_id;
    return result;
}
//---------------------------------------------------------------------------
//Сохранить
void MaxUser::Save(TFastIniFile * ini,AnsiString section)
{
    if(FName.Length())ini->WriteString(section,"Name",FName);
    if(FId.Length())ini->WriteString(section,"id",FId);
    if(FIsBot)ini->WriteBool(section,"IsBot",FIsBot);
    if(FAlias.Length())ini->WriteString(section,"Alias",FAlias);
    if(FInCount)ini->WriteString(section,"InCount",IntToStr((__int64)FInCount));
    if(FOutCount)ini->WriteString(section,"OutCount",IntToStr((__int64)FOutCount));
    if(FComment.Length())ini->WriteString(section,"Comment",FComment);
    if(FTag)ini->WriteInteger(section,"Tag",FTag);
    if(FPeerType==maxPeerChat)ini->WriteString(section,"PeerType","chat");
}
//---------------------------------------------------------------------------
//Загрузить
void MaxUser::Load(TFastIniFile * ini,AnsiString section)
{
    Clear();
    FName=ini->ReadString(section,"Name","");
    FId=ini->ReadString(section,"id","");
    FIsBot=ini->ReadBool(section,"IsBot",false);
    FAlias=ini->ReadString(section,"Alias","");
    FInCount=_atoi64(ini->ReadString(section,"InCount","0").c_str());
    FOutCount=_atoi64(ini->ReadString(section,"OutCount","0").c_str());
    FComment=ini->ReadString(section,"Comment","");
    FTag=ini->ReadInteger(section,"Tag",0);
    FPeerType=ini->ReadString(section,"PeerType","user").LowerCase()=="chat"?maxPeerChat:maxPeerUser;
}
//---------------------------------------------------------------------------
void MaxUser::CopyFrom(MaxUser * user)
{
    FId=user->FId;
    FIsBot=user->FIsBot;
    FName=user->FName;
    FAlias=user->FAlias;
    FInCount=user->FInCount;
    FOutCount=user->FOutCount;
    FComment=user->FComment;
    FTag=user->FTag;
    FPeerType=user->FPeerType;
}
//---------------------------------------------------------------------------
void MaxUser::CopyUserFrom(MaxMessage * msg)
{
    Clear();
    FId=msg->From.Id;
    FIsBot=msg->From.is_bot;
    FName=msg->From.FullName;
    FPeerType=maxPeerUser;
}
//---------------------------------------------------------------------------
void MaxUser::CopyChatFrom(MaxMessage * msg)
{
    Clear();
    FId=msg->Chat.Id;
    FIsBot=false;
    FName=msg->Chat.FullName;
    FPeerType=maxPeerChat;
}
//---------------------------------------------------------------------------
bool MaxUser::HasValidAlias(AnsiString alias)
{
    if(!Valid)return false;
    if(alias=="*")return true;
    if(alias.IsEmpty())return true;
    int len=alias.Length();
    if(FAlias.Length()<len)return false;
    for(int i=1;i<=len;i++)
    {
        if(alias[i]!='!' && FAlias[i]!=alias[i])return false;
    }
    return true;
}
//---------------------------------------------------------------------------
__fastcall MaxUser_LIST::MaxUser_LIST()
{
}
//---------------------------------------------------------------------------
__fastcall MaxUser_LIST::~MaxUser_LIST()
{
    ClearList();
}
//---------------------------------------------------------------------------
MaxUser * MaxUser_LIST::Get(int index)
{
    if(index<0 || index>=Count)return NULL;
    return (MaxUser *)Items[index];
}
//---------------------------------------------------------------------------
void MaxUser_LIST::DeleteUser(int index)
{
    MaxUser * user=Get(index);
    if(user)
    {
        delete user;
        TList::Delete(index);
    }
}
//---------------------------------------------------------------------------
void MaxUser_LIST::ClearList(void)
{
    while(Count)DeleteUser(0);
}
//---------------------------------------------------------------------------
void MaxUser_LIST::Remove(MaxUser * user)
{
    for(int i=0;i<Count;i++)
    {
        MaxUser * ui=Get(i);
        if(ui && ui==user){DeleteUser(i);break;}
    }
}
//---------------------------------------------------------------------------
MaxUser * MaxUser_LIST::Find(AnsiString id)
{
    for(int i=0;i<Count;i++)
    {
        MaxUser * user=Get(i);
        if(user && user->Id==id)return user;
    }
    return NULL;
}
//---------------------------------------------------------------------------
int MaxUser_LIST::IndexOfId(AnsiString id)
{
    return IndexOf(Find(id));
}
//---------------------------------------------------------------------------
MaxUser * MaxUser_LIST::FindAlias(AnsiString alias)
{
    for(int i=0;i<Count;i++)
    {
        MaxUser * user=Get(i);
        if(user && user->Alias==alias)return user;
    }
    return NULL;
}
//---------------------------------------------------------------------------
MaxUser * MaxUser_LIST::AddUser(void)
{
    MaxUser * user=new MaxUser();
    TList::Add(user);
    return user;
}
//---------------------------------------------------------------------------
void MaxUser_LIST::Save(TFastIniFile * ini)
{
    for(int i=0;i<Count;i++)Get(i)->Save(ini,"User"+IntToStr(i));
}
//---------------------------------------------------------------------------
void MaxUser_LIST::Load(TFastIniFile * ini)
{
    ClearList();
    int i=0;
    AnsiString section="User0";
    while(ini->SectionExists(section))
    {
        MaxUser * user=AddUser();
        user->Load(ini,section);
        section="User"+IntToStr(++i);
    }
}
//---------------------------------------------------------------------------
AnsiString MaxUser_LIST::GetFreeAlias(void)
{
    int num=1;
    AnsiString alias="$1";
    while(FindAlias(alias)){num++;alias="$"+IntToStr(num);}
    return alias;
}
//---------------------------------------------------------------------------
void MaxUser_LIST::GetUsersByAlias(MaxUser_LIST * userlist,AnsiString alias)
{
    userlist->ClearList();
    for(int i=0;i<Count;i++)
    {
        MaxUser * ui=Get(i);
        if(ui->HasValidAlias(alias))
        {
            MaxUser * user=userlist->AddUser();
            user->CopyFrom(ui);
        }
    }
}
//---------------------------------------------------------------------------
void MaxUser_LIST::GetIndexesByAlias(TList * list,AnsiString alias)
{
    list->Clear();
    for(int i=0;i<Count;i++)
    {
        MaxUser * ui=Get(i);
        if(ui->HasValidAlias(alias))list->Add((void *)i);
    }
}
//---------------------------------------------------------------------------
void MaxBotInfo::CopyFrom(const MAX_BOT_INFO & bi)
{
    Clear();
    Id=MaxInt64ToString(bi.Id).c_str();
    first_name=MaxAnsiFromUtf8(bi.FirstName);
    username=MaxAnsiFromUtf8(bi.UserName);
}
//---------------------------------------------------------------------------
void MaxBotInfo::CopyFrom(MaxBotInfo & bi)
{
    Id=bi.Id;
    first_name=bi.first_name;
    username=bi.username;
}
//---------------------------------------------------------------------------
