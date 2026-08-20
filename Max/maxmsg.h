//---------------------------------------------------------------------------
#ifndef maxmsgH
#define maxmsgH
//---------------------------------------------------------------------------
#include "FastIni.h"
#include "api/maxcore.h"
//---------------------------------------------------------------------------
//От какого пользователя пришло сообщение
class MaxFrom
{
    bool GetValid(void){return (bool)Id.Length();}
    AnsiString GetFullName(void);
public:
    AnsiString Id;
    bool is_bot;
    AnsiString first_name;
    AnsiString last_name;
    AnsiString language_code;
    MaxFrom(){Id="";is_bot=false;}
    void Clear(void){Id="";is_bot=false;first_name="";last_name="";language_code="";}
    __property bool Valid={read=GetValid};
    void CopyFrom(MaxFrom * from);
    MaxFrom operator = (MaxFrom & f){CopyFrom(&f);return *this;}
    __property AnsiString FullName={read=GetFullName};
};
//---------------------------------------------------------------------------
//Объект Chat
class MaxChat
{
    bool GetValid(void){return (bool)Id.Length();}
    AnsiString GetFullName(void);
public:
    AnsiString Id;
    AnsiString first_name;
    AnsiString last_name;
    AnsiString type;
    AnsiString username;
    void Clear(void){Id="";first_name="";last_name="";type="";username="";}
    __property bool Valid={read=GetValid};
    void CopyFrom(MaxChat * chat);
    MaxChat operator = (MaxChat & ch){CopyFrom(&ch);return *this;}
    __property AnsiString FullName={read=GetFullName};
};
//---------------------------------------------------------------------------
//Сообщение MAX
//---------------------------------------------------------------------------
//Распознаваемые виды сообщений
enum MaxMessageType
{
    mmtUNKNOWN=-1,
    mmtMESSAGE=0,
    mmtINVITATION,
    mmtNEWPATICIPANT
};
//---------------------------------------------------------------------------
class MaxMessage
{
    AnsiString GetDateText(void);
    AnsiString GetTypeText(void);
    AnsiString GetParticipantText(void);
public:
    MaxMessageType Type;
    //В MAX отдельного последовательного update_id нет: для сохранения интерфейса
    //Telegram сюда кладётся timestamp события update. Сам курсор чтения MAX — marker —
    //хранится внутри MAX_API_CLIENT и не является свойством одного сообщения.
    __int64 update_id;
    AnsiString message_id;
    MaxFrom From;
    MaxChat Chat;
    long Date;
    AnsiString Text;
    MaxFrom Participant;
    TDateTime GetDateTime(void);
    bool GetIsChat(void){return From.Id!=Chat.Id;}
public:
    MaxMessage(){Type=mmtUNKNOWN;update_id=0;message_id="";Date=0;}
    AnsiString AsString(void);
    __property AnsiString DateText={read=GetDateText};
    __property TDateTime DateTime={read=GetDateTime};
    __property AnsiString TypeText={read=GetTypeText};
    __property AnsiString ParticipantText={read=GetParticipantText};
    __property bool IsChat={read=GetIsChat};
    void CopyFrom(MaxMessage * msg);
    void CopyFrom(const MAX_MESSAGE & msg);
};
//---------------------------------------------------------------------------
//Список сообщений
class MaxMessage_LIST
{
    TList * List;
    int GetCount(void){return List->Count;}
public:
    MaxMessage_LIST();
    ~MaxMessage_LIST();
    __property int Count={read=GetCount};
    MaxMessage * Get(int index);
    MaxMessage * operator [](int index){return Get(index);}
    void Delete(int index);
    void Clear(void);
    void Remove(MaxMessage * tm);
    MaxMessage * Find(__int64 update_id);
    MaxMessage * Add(void);
    void CopyFrom(MaxMessage_LIST & msglist);
    void CopyFrom(const MAX_UPDATES & updates);
    __int64 GetUpdateId(void);
};
//---------------------------------------------------------------------------
//Преобразование входящего UTF-8 MAX в AnsiString/CP1251
AnsiString MaxAnsiFromUtf8(const MAX_TEXT & s);
//---------------------------------------------------------------------------
//Это класс пользователей, используемых в программе
class MaxUser : public TObject
{
    bool GetValid(void){return (bool)FId.Length();}
private:
    AnsiString FId;
    AnsiString FName;
    AnsiString FAlias;
    AnsiString FComment;
    bool FIsBot;
    UINT FInCount;
    UINT FOutCount;
    int FTag;
    MAX_PEER_TYPE FPeerType;
public:
    MaxUser(){FId="";FIsBot=false;FInCount=0;FOutCount=0;FTag=0;FPeerType=maxPeerUser;}
    void Clear(void){FId="";FIsBot=false;FName="";FComment="";FAlias="";FInCount=0;FOutCount=0;FTag=0;FPeerType=maxPeerUser;}
    void Save(TFastIniFile * ini,AnsiString section);
    void Load(TFastIniFile * ini,AnsiString section);
    __property bool Valid={read=GetValid};
    void CopyFrom(MaxUser * user);
    void CopyUserFrom(MaxMessage * msg);
    void CopyChatFrom(MaxMessage * msg);
    bool HasValidAlias(AnsiString alias);
__published:
    __property AnsiString Id = { read=FId, write=FId };
    __property AnsiString Name = { read=FName, write=FName };
    __property AnsiString Alias = { read=FAlias, write=FAlias };
    __property AnsiString Comment = { read=FComment, write=FComment };
    __property bool IsBot = { read=FIsBot, write=FIsBot };
    __property UINT InCount = { read=FInCount, write=FInCount };
    __property UINT OutCount = { read=FOutCount, write=FOutCount };
    __property int Tag = { read=FTag, write=FTag };
    __property MAX_PEER_TYPE PeerType = { read=FPeerType, write=FPeerType };
};
//---------------------------------------------------------------------------
//Список пользователей, используемых в программе
class MaxUser_LIST : public TList
{
public:
    __fastcall MaxUser_LIST(void);
    __fastcall virtual ~MaxUser_LIST(void);
    MaxUser * Get(int index);
    MaxUser * operator [](int index){return Get(index);}
    void DeleteUser(int index);
    void ClearList(void);
    void Remove(MaxUser * user);
    MaxUser * Find(AnsiString id);
    int IndexOfId(AnsiString id);
    MaxUser * FindAlias(AnsiString alias);
    MaxUser * AddUser(void);
    void Save(TFastIniFile * ini);
    void Load(TFastIniFile * ini);
    AnsiString GetFreeAlias(void);
    void GetUsersByAlias(MaxUser_LIST * userlist,AnsiString alias);
    void GetIndexesByAlias(TList * list,AnsiString alias);
};
//---------------------------------------------------------------------------
//Информация о MAX боте
class MaxBotInfo
{
    bool GetValid(void){return (bool)Id.Length();}
public:
    AnsiString Id;
    AnsiString first_name;
    AnsiString username;
    void Clear(void){Id="";first_name="";username="";}
    __property bool Valid={read=GetValid};
    void CopyFrom(const MAX_BOT_INFO & bi);
    void CopyFrom(MaxBotInfo & bi);
    MaxBotInfo operator = (MaxBotInfo & bi){CopyFrom(bi);return *this;}
};
//---------------------------------------------------------------------------
#endif
