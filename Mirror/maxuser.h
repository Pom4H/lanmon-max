//---------------------------------------------------------------------------
#ifndef maxuserH
#define maxuserH
//---------------------------------------------------------------------------
#include <vcl.h>
#include "../maxusers.h"
//---------------------------------------------------------------------------
class MaxUser
{
public:
    __int64 Id;
    MAX_PEER_TYPE PeerType;
    AnsiString Name;
    AnsiString Alias;
    AnsiString Comment;
    bool IsBot;
    UINT InCount;
    UINT OutCount;
    int Tag;
    MaxUser(){Id=0;PeerType=maxPeerUser;IsBot=false;InCount=0;OutCount=0;Tag=0;}
    bool HasValidAlias(AnsiString alias)
    {
        MAX_USER u;u.Id=Id;u.Alias=Alias.c_str();return u.HasValidAlias(alias.c_str());
    }
    MAX_PEER Peer(void){return MAX_PEER(PeerType,Id);}
};
//---------------------------------------------------------------------------
//Список пользователей
class MaxUser_LIST
{
    TList * List;
    int GetCount(void){return List->Count;}
public:
    MaxUser_LIST(){List=new TList;}
    ~MaxUser_LIST(){Clear();delete List;}
    __property int Count={read=GetCount};
    void Clear(void){while(Count){delete Get(0);List->Delete(0);}}
    MaxUser * Get(int index){if(index<0||index>=Count)return NULL;return (MaxUser*)List->Items[index];}
    MaxUser * Find(__int64 id){for(int i=0;i<Count;i++)if(Get(i)->Id==id)return Get(i);return NULL;}
    int IndexOfId(__int64 id){for(int i=0;i<Count;i++)if(Get(i)->Id==id)return i;return -1;}
    MaxUser * FindAlias(AnsiString alias){for(int i=0;i<Count;i++)if(Get(i)->Alias==alias)return Get(i);return NULL;}
    //Добавление пользователя
    MaxUser * AddUser(void){MaxUser *u=new MaxUser;List->Add(u);return u;}
    //Получить свободный псевдоним автоматически
    AnsiString GetFreeAlias(void){for(int n=1;;n++){AnsiString a="$"+IntToStr(n);if(!FindAlias(a))return a;}}
    //Заполнить список пользователями, соответствующих маске псевдонима
    void GetUsersByAlias(MaxUser_LIST *userlist,AnsiString alias)
    {
        userlist->Clear();for(int i=0;i<Count;i++)if(Get(i)->HasValidAlias(alias)){MaxUser *u=userlist->AddUser();*u=*Get(i);}
    }
};
//---------------------------------------------------------------------------
#endif
