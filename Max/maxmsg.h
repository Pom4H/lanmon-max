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
    AnsiString Id;            //Уникальный идентификатор пользователя или бота
    bool is_bot;              //True, если этот пользователь является ботом
    AnsiString first_name;    //Имя пользователя или бота
    AnsiString last_name;     //Фамилия пользователя или бота
    AnsiString language_code; //Языковой тег пользователя, если доступен
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
    AnsiString Id;          //Уникальный идентификатор чата
    AnsiString first_name;  //Имя собеседника в приватном чате
    AnsiString last_name;   //Фамилия собеседника в приватном чате
    AnsiString type;        //Тип чата MAX
    AnsiString username;    //Имя пользователя/чата, если доступно
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
    mmtUNKNOWN=-1,       //Нет сообщения
    mmtMESSAGE=0,        //Просто сообщение
    mmtINVITATION,       //Зарезервировано под событие приглашения
    mmtNEWPATICIPANT     //Зарезервировано под нового участника
};
//---------------------------------------------------------------------------
class MaxMessage
{
    //Текст даты времени сообщения
    AnsiString GetDateText(void);
    //Текст типа
    AnsiString GetTypeText(void);
    //Новый участник
    AnsiString GetParticipantText(void);
public:
    //Тип сообщения
    MaxMessageType Type;

    __int64 update_id;      //Идентификатор обновления MAX (timestamp)
    AnsiString message_id;  //Уникальный идентификатор сообщения внутри MAX
    MaxFrom From;           //Отправитель сообщения
    MaxChat Chat;           //Беседа, к которой относится сообщение
    long Date;              //Дата отправки сообщения по времени Unix
    AnsiString Text;        //Текст сообщения
    MaxFrom Participant;    //Новый участник, если такой тип обновления появится
    //Дата и время сообщения
    TDateTime GetDateTime(void);
    //Проверка что это сообщение из чата (а не от user)
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
    //Список
    TList * List;
    int GetCount(void){return List->Count;}
public:
    MaxMessage_LIST();
    ~MaxMessage_LIST();
    //Количество сообщений в списке
    __property int Count={read=GetCount};
    //Доступ по индексу к сообщениям
    MaxMessage * Get(int index);
    MaxMessage * operator [](int index){return Get(index);}
    //Удалить сообщение по индексу
    void Delete(int index);
    //Удалить все сообщения
    void Clear(void);
    //Удалить сообщение
    void Remove(MaxMessage * tm);
    //Поиск сообщения
    MaxMessage * Find(__int64 update_id);
    //Добавление сообщения
    MaxMessage * Add(void);
    //Копирование
    void CopyFrom(MaxMessage_LIST & msglist);
    //Получение сообщений из ответа MAX API
    void CopyFrom(const MAX_UPDATES & updates);
    //Получить последнее значение update_id
    __int64 GetUpdateId(void);
};
//---------------------------------------------------------------------------
//Преобразование входящего UTF-8 MAX в AnsiString/CP1251
AnsiString MaxAnsiFromUtf8(const std::string & s);
//---------------------------------------------------------------------------
//Это класс пользователей, используемых в программе
class MaxUser : public TObject
{
    bool GetValid(void){return (bool)FId.Length();}
private:
    AnsiString FId;          //Уникальный идентификатор пользователя или чата
    AnsiString FName;        //Имя пользователя или чата
    AnsiString FAlias;       //Псевдоним
    AnsiString FComment;     //Комментарий
    bool FIsBot;             //True, если этот пользователь является ботом
    UINT FInCount;           //Счётчик приёма
    UINT FOutCount;          //Счётчик передачи
    int FTag;                //Целочисленная переменная пользователя
    MAX_PEER_TYPE FPeerType; //MAX различает user_id и chat_id
public:
    MaxUser(){FId="";FIsBot=false;FInCount=0;FOutCount=0;FTag=0;FPeerType=maxPeerUser;}
    void Clear(void){FId="";FIsBot=false;FName="";FComment="";
                     FAlias="";FInCount=0;FOutCount=0;FTag=0;FPeerType=maxPeerUser;}
    //Сохранить
    void Save(TFastIniFile * ini,AnsiString section);
    //Загрузить
    void Load(TFastIniFile * ini,AnsiString section);
    __property bool Valid={read=GetValid};
    //Копирование
    void CopyFrom(MaxUser * user);
    void CopyUserFrom(MaxMessage * msg);
    void CopyChatFrom(MaxMessage * msg);
    //Проверка, что пользователь соответствует маске псевдонима
    bool HasValidAlias(AnsiString alias);
__published:
    //Уникальный идентификатор пользователя или чата
    __property AnsiString Id = { read=FId, write=FId };
    //Имя пользователя или чата
    __property AnsiString Name = { read=FName, write=FName };
    //Псевдоним
    __property AnsiString Alias = { read=FAlias, write=FAlias };
    //Комментарий
    __property AnsiString Comment = { read=FComment, write=FComment };
    //True, если этот пользователь является ботом
    __property bool IsBot = { read=FIsBot, write=FIsBot };
    //Счётчик приёма
    __property UINT InCount = { read=FInCount, write=FInCount };
    //Счётчик передачи
    __property UINT OutCount = { read=FOutCount, write=FOutCount };
    //Целочисленная переменная пользователя
    __property int Tag = { read=FTag, write=FTag };
    //Тип адресата MAX
    __property MAX_PEER_TYPE PeerType = { read=FPeerType, write=FPeerType };
};
//---------------------------------------------------------------------------
//Список пользователей, используемых в программе
class MaxUser_LIST : public TList
{
public:
    __fastcall MaxUser_LIST(void);
    __fastcall virtual ~MaxUser_LIST(void);

    //Доступ по индексу к пользователям
    MaxUser * Get(int index);
    MaxUser * operator [](int index){return Get(index);}
    //Удалить пользователя по индексу
    void DeleteUser(int index);
    //Удалить всех пользователей
    void ClearList(void);
    //Удалить пользователя
    void Remove(MaxUser * user);
    //Поиск пользователя по идентификатору Id
    MaxUser * Find(AnsiString id);
    //Поиск индекса идентификатора в списке
    int IndexOfId(AnsiString id);
    //Поиск псевдонима
    MaxUser * FindAlias(AnsiString alias);
    //Добавление пользователя
    MaxUser * AddUser(void);
    //Сохранить всех пользователей
    void Save(TFastIniFile * ini);
    //Загрузить всех пользователей
    void Load(TFastIniFile * ini);
    //Получить свободный псевдоним автоматически
    AnsiString GetFreeAlias(void);
    //Заполнить список пользователями, соответствующих маске псевдонима
    void GetUsersByAlias(MaxUser_LIST * userlist,AnsiString alias);
    //Заполнить список индексами пользователей, соответствующих маске псевдонима
    void GetIndexesByAlias(TList * list,AnsiString alias);
};
//---------------------------------------------------------------------------
class MaxBotInfo
{
    bool GetValid(void){return (bool)Id.Length();}
public:
    AnsiString Id;          //Уникальный идентификатор
    AnsiString first_name;  //Имя бота
    AnsiString username;    //Пользовательское имя бота
    void Clear(void){Id="";first_name="";username="";}
    __property bool Valid={read=GetValid};
    //Получение из ответа сервера
    void CopyFrom(const MAX_BOT_INFO & bi);
    void CopyFrom(MaxBotInfo & bi);
    MaxBotInfo operator = (MaxBotInfo & bi){CopyFrom(bi);return *this;}
};
//---------------------------------------------------------------------------
#endif
