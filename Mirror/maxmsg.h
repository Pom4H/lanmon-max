//---------------------------------------------------------------------------
#ifndef maxmsgH
#define maxmsgH
//---------------------------------------------------------------------------
#include <vcl.h>
#include <windows.h>
#include <time.h>
#include <vector>
#include "../maxcore.h"
//---------------------------------------------------------------------------
static AnsiString MaxAnsiFromUtf8(const std::string &s)
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
class MaxBotInfo
{
    bool GetValid(void){return Id!=0;}
public:
    __int64 Id;             //Уникальный идентификатор
    AnsiString first_name;  //Имя бота
    AnsiString username;    //Пользовательское имя бота
    MaxBotInfo(){Clear();}
    void Clear(void){Id=0;first_name="";username="";}
    __property bool Valid={read=GetValid};
    void CopyFrom(const MAX_BOT_INFO &bi){Id=bi.Id;first_name=MaxAnsiFromUtf8(bi.FirstName);username=MaxAnsiFromUtf8(bi.UserName);}
};
//---------------------------------------------------------------------------
class MaxMessage
{
public:
    __int64 update_id;
    __int64 message_id;
    __int64 ChatId;
    __int64 UserId;
    time_t Date;
    AnsiString Text;
    AnsiString first_name;
    AnsiString last_name;
    AnsiString username;
    MaxMessage(){update_id=0;message_id=0;ChatId=0;UserId=0;Date=0;}
    void CopyFrom(const MAX_MESSAGE &msg)
    {
        update_id=msg.UpdateTimestamp;ChatId=msg.ChatId;UserId=msg.UserId;
        Date=(time_t)(msg.MessageTimestamp/1000);Text=MaxAnsiFromUtf8(msg.Text);
        first_name=MaxAnsiFromUtf8(msg.FirstName);last_name=MaxAnsiFromUtf8(msg.LastName);username=MaxAnsiFromUtf8(msg.UserName);
    }
};
//---------------------------------------------------------------------------
//Список сообщений
class MaxMessage_LIST
{
    TList * List;
    int GetCount(void){return List->Count;}
public:
    MaxMessage_LIST(){List=new TList;}
    ~MaxMessage_LIST(){Clear();delete List;}
    __property int Count={read=GetCount};
    MaxMessage * Get(int index){if(index<0||index>=Count)return NULL;return (MaxMessage*)List->Items[index];}
    MaxMessage * operator[](int index){return Get(index);}
    void Clear(void){while(Count){delete Get(0);List->Delete(0);}}
    MaxMessage * Add(void){MaxMessage *m=new MaxMessage;List->Add(m);return m;}
    void CopyFrom(const MAX_UPDATES &updates){Clear();for(size_t i=0;i<updates.Messages.size();++i)Add()->CopyFrom(updates.Messages[i]);}
};
//---------------------------------------------------------------------------
#endif
