//---------------------------------------------------------------------------
#include <vcl.h>
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "maxbot.h"
#include "EventFS.h"
#include "screenshot.h"
#include "ULogView.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
MAX_BOT MaxBot;
//---------------------------------------------------------------------------
static std::string Utf8(AnsiString s){return MaxUtf8FromCp1251(s.c_str());}
static AnsiString U8(const char *s){return MaxAnsiFromUtf8(std::string(s));}
static MAX_PEER MakePeer(__int64 id,MAX_PEER_TYPE type){return MAX_PEER(type,id);}
//---------------------------------------------------------------------------
//Поток для работы с MAX
//---------------------------------------------------------------------------
//Конструктор
__fastcall TMaxBotThread::TMaxBotThread(bool CreateSuspended)
    : TThread(CreateSuspended)
{
    Transport=new TMaxIndyTransport();
    Api=new MAX_API_CLIENT(Transport,"");
    State=tsNONE;
    //Период чтения сообщений с MAX сервера, с
    PeriodReadMessages=0;
    //Временно не выполнять периодическое чтение сообщений с сервера
    PeriodicReadMessagesPaused=false;
    //Установка значения FBotApi
    FlagNewBotApi=false;
    //Время последнего периодического чтения сообщений
    LastReadTick=0;
    BreakSignal=false;
    ResponseCode=0;
    OnDebugMessage=NULL;OnErrorDebugMessage=NULL;OnTaskReadMessages=NULL;OnPeriodicReadMessages=NULL;OnGetMe=NULL;
}
//---------------------------------------------------------------------------
//Деструктор
__fastcall TMaxBotThread::~TMaxBotThread(void)
{
    delete Api;
    delete Transport;
}
//---------------------------------------------------------------------------
void __fastcall TMaxBotThread::DebugMessage()
{
    if(OnDebugMessage)OnDebugMessage(S);
}
#define DBG_MSG(x) if(OnDebugMessage){S=x;Synchronize(DebugMessage);}
//---------------------------------------------------------------------------
void __fastcall TMaxBotThread::ErrorDebugMessage()
{
    if(OnErrorDebugMessage)OnErrorDebugMessage(S);
}
#define ERROR_DBG_MSG(x) if(OnErrorDebugMessage){S=x;Synchronize(ErrorDebugMessage);}
//---------------------------------------------------------------------------
//Прочитаны сообщения по заданию
void __fastcall TMaxBotThread::OnTaskReadMessagesFunc()
{
    if(OnTaskReadMessages)OnTaskReadMessages(MsgList);
}
#define _ON_TASK_READMSG() {if(OnTaskReadMessages)Synchronize(OnTaskReadMessagesFunc);}
//---------------------------------------------------------------------------
//Прочитаны сообщения периодически
void __fastcall TMaxBotThread::OnPeriodicReadMessagesFunc()
{
    if(OnPeriodicReadMessages)OnPeriodicReadMessages(MsgList);
}
#define _ON_PERIODIC_READMSG() {if(OnPeriodicReadMessages)Synchronize(OnPeriodicReadMessagesFunc);}
//---------------------------------------------------------------------------
//Информация о себе
void __fastcall TMaxBotThread::OnBotInfo()
{
    if(OnGetMe)OnGetMe(BotInfo);
}
#define _ON_GETME() {if(OnGetMe)Synchronize(OnBotInfo);}
//---------------------------------------------------------------------------
//Идентификатор разработчика MAX бота: FBotApi
//Установка значения FBotApi
void TMaxBotThread::SetBotApi(AnsiString api)
{
    NewBotApi=api;
    FlagNewBotApi=true;
}
//---------------------------------------------------------------------------
//Функция потока
void __fastcall TMaxBotThread::Execute()
{
    while(!Terminated)
    {
        ::Sleep(1);
        //Проверить запись идентификатора разработчика
        if(FlagNewBotApi)
        {
            FlagNewBotApi=false;
            //Задан новый FBotApi
            FBotApi=NewBotApi;
            Api->SetToken(FBotApi.c_str());
        }
        //Разрешить работу MAX
        if(!MaxBot.Active)
        {
            //Работа запрещена
            State=tsDISABLED; //Запрещён
            ::Sleep(10);
            continue;
        }
        if(!FBotApi.Length())
        {
            //Не задан Id разработчика
            State=tsERROR; //Ошибка
            ::Sleep(10);
            continue;
        }
        State=tsNONE;
        //Выполнение заданий
        CheckTask();
        //Если есть запрет периодического чтения
        if(PeriodicReadMessagesPaused)continue;
        //Периодическое чтение сообщений
        DoReadMessagesPeriodic();
    }
    State=tsDONE;
}
//---------------------------------------------------------------------------
//Выполнение заданий
//Проверка и выполнение заданий
void TMaxBotThread::CheckTask(void)
{
    //Есть задание
    if(!TaskList.Get(Task))return;
    //Получено задание
    State=tsTASK;
    //Выполнение задания
    switch(Task.Type)
    {
        case taskGETME:      DoGetMe();break;
        case taskREADMSG:    DoReadMessagesByTask();break;
        case taskSENDMSG:    DoSendMessage();break;
        case taskSENDPHOTO:  DoSendPhoto();break;
        case taskSENDDOC:    DoSendDoc();break;
    }
}
//---------------------------------------------------------------------------
//Чтение сообщений
//Чтение сообщений с сервера
bool TMaxBotThread::DoReadMessages(bool bytask)
{
    std::string error;
    MAX_UPDATES updates;
    bool ok=Api->Poll(updates,error,30,100);
    if(!ok){ExeptionText=error.c_str();ERROR_DBG_MSG(ExeptionText);return false;}
    //Очистить список принятых сообщений и заполнить прочитанными
    MsgList.CopyFrom(updates);
    //Успешно прочитаны сообщения
    ResponseCode=200;
    if(bytask)_ON_TASK_READMSG() else _ON_PERIODIC_READMSG();
    return true;
}
//---------------------------------------------------------------------------
void TMaxBotThread::DoReadMessagesByTask(void)
{
    MaxBot.ReadMessagesCount++;
    if(DoReadMessages(true))MaxBot.ReadMessagesCountOk++;
}
//---------------------------------------------------------------------------
void TMaxBotThread::DoReadMessagesPeriodic(void)
{
    if(!PeriodReadMessages)return;
    UINT tick=GetTickCount();
    if((tick-LastReadTick)<PeriodReadMessages*1000)return;
    LastReadTick=tick;
    MaxBot.ReadMessagesCount++;
    if(DoReadMessages(false))MaxBot.ReadMessagesCountOk++;
}
//---------------------------------------------------------------------------
//Передача сообщения
//Посылка сообщения в Task
bool TMaxBotThread::DoSendMessage(void)
{
    std::string error;
    bool ok=Api->SendMessage(MakePeer(Task.Id,Task.PeerType),Utf8(Task.Text),error);
    if(!ok){ExeptionText=error.c_str();ERROR_DBG_MSG(ExeptionText);}
    return ok;
}
//---------------------------------------------------------------------------
//Информация о себе
void TMaxBotThread::DoGetMe(void)
{
    std::string error;MAX_BOT_INFO info;
    if(!Api->GetMe(info,error)){ExeptionText=error.c_str();ERROR_DBG_MSG(ExeptionText);return;}
    BotInfo.CopyFrom(info);_ON_GETME();
}
//---------------------------------------------------------------------------
//Посылка файла картинки
//Посылаем файл с помощью специального MAX upload flow
void TMaxBotThread::DoSendPhoto(void)
{
    std::string error;
    if(!Api->SendImage(MakePeer(Task.Id,Task.PeerType),Task.Text.c_str(),Utf8(Task.Caption),error))
    {ExeptionText=error.c_str();ERROR_DBG_MSG(ExeptionText);}
}
//---------------------------------------------------------------------------
//Посылка файла документа
//Посылаем файл с помощью специального MAX upload flow
void TMaxBotThread::DoSendDoc(void)
{
    std::string error;
    if(!Api->SendFile(MakePeer(Task.Id,Task.PeerType),Task.Text.c_str(),Utf8(Task.Caption),error))
    {ExeptionText=error.c_str();ERROR_DBG_MSG(ExeptionText);}
}
//---------------------------------------------------------------------------
//Получение ошибки
AnsiString TMaxBotThread::GetErrorText(void){return ExeptionText;}
//---------------------------------------------------------------------------
//Класс для работы с MAX Bot
//---------------------------------------------------------------------------
MAX_BOT::MAX_BOT()
{
    UserList=new MaxUser_LIST;
    Active=true;FlagSendAlarms=false;FlagSendAlarmsEnd=false;FlagOperatorAlarm=false;FlagSendMaps=false;
    UseLanmonLog=false;FPeriodReadMessages=0;FPeriodicReadMessagesPaused=false;
    ReadMessagesCount=0;ReadMessagesCountOk=0;UserMessageCount=0;
    Thread=new TMaxBotThread(true);
    Thread->OnPeriodicReadMessages=ThreadReadMessages;
    Thread->OnTaskReadMessages=ThreadReadMessages;
    Thread->OnGetMe=ThreadGetMe;
    Thread->Resume();
}
//---------------------------------------------------------------------------
MAX_BOT::~MAX_BOT()
{
    Thread->Terminate();Thread->WaitFor();delete Thread;delete UserList;
}
//---------------------------------------------------------------------------
void MAX_BOT::SetBotApi(AnsiString api){FBotApi=api;Thread->BotApi=api;}
void MAX_BOT::SetPeriodReadMessages(UINT period){FPeriodReadMessages=period;Thread->PeriodReadMessages=period;}
void MAX_BOT::SetPeriodicReadMessagesPaused(bool v){FPeriodicReadMessagesPaused=v;Thread->PeriodicReadMessagesPaused=v;}
void __fastcall MAX_BOT::ThreadReadMessages(MaxMessage_LIST &msglist){OnMessages(msglist);}
void __fastcall MAX_BOT::ThreadGetMe(MaxBotInfo &botinfo){MyBotInfo=botinfo;}
//---------------------------------------------------------------------------
//Загрузка бота из файла
void MAX_BOT::Load(AnsiString fn)
{
    MAX_BOT_SETTINGS s;MAX_USER_LIST users;std::string error;
    if(!MaxLoadIni(fn.c_str(),s,users,error))return;
    BotApi=s.BotToken.c_str();PeriodReadMessages=s.PeriodReadMessages;PeriodicReadMessagesPaused=s.PeriodicReadMessagesPaused;
    Active=s.Active;FlagSendAlarms=s.FlagSendAlarms;FlagSendAlarmsEnd=s.FlagSendAlarmsEnd;FlagOperatorAlarm=s.FlagOperatorAlarm;
    FlagSendMaps=s.FlagSendMaps;AlarmAlias=s.AlarmAlias.c_str();RequestAlias=s.RequestAlias.c_str();UseLanmonLog=s.UseLanmonLog;
    UserList->Clear();
    for(size_t i=0;i<users.Count();++i){const MAX_USER *src=users.Get(i);MaxUser *u=UserList->AddUser();u->Id=src->Id;u->PeerType=src->PeerType;u->Name=src->Name.c_str();u->Alias=src->Alias.c_str();u->Comment=src->Comment.c_str();u->IsBot=src->IsBot;u->InCount=src->InCount;u->OutCount=src->OutCount;u->Tag=src->Tag;}
}
//---------------------------------------------------------------------------
//Сохранение бота в файл
void MAX_BOT::Save(AnsiString fn)
{
    MAX_BOT_SETTINGS s;MAX_USER_LIST users;std::string error;
    s.BotToken=BotApi.c_str();s.PeriodReadMessages=PeriodReadMessages;s.PeriodicReadMessagesPaused=PeriodicReadMessagesPaused;
    s.Active=Active;s.FlagSendAlarms=FlagSendAlarms;s.FlagSendAlarmsEnd=FlagSendAlarmsEnd;s.FlagOperatorAlarm=FlagOperatorAlarm;
    s.FlagSendMaps=FlagSendMaps;s.AlarmAlias=AlarmAlias.c_str();s.RequestAlias=RequestAlias.c_str();s.UseLanmonLog=UseLanmonLog;
    for(int i=0;i<UserList->Count;i++){MaxUser *src=UserList->Get(i);MAX_USER u;u.Id=src->Id;u.PeerType=src->PeerType;u.Name=src->Name.c_str();u.Alias=src->Alias.c_str();u.Comment=src->Comment.c_str();u.IsBot=src->IsBot;u.InCount=src->InCount;u.OutCount=src->OutCount;u.Tag=src->Tag;users.Add(u);}
    MaxSaveIni(fn.c_str(),s,users,error);
}
//---------------------------------------------------------------------------
//Добавление заданий потоку
//Передача сообщения
void MAX_BOT::SendMessage(__int64 id,AnsiString msg,MAX_PEER_TYPE peerType)
{
    Thread->TaskList.AddSendMsg(MakePeer(id,peerType),msg);
    MaxUser *user=UserList->Find(id);if(user)user->OutCount++;
}
//---------------------------------------------------------------------------
//Чтение сообщений
void MAX_BOT::ReadMessages(void){Thread->TaskList.AddReadMsg();}
//---------------------------------------------------------------------------
//Запрос информации о себе
void MAX_BOT::GetMe(void){Thread->TaskList.AddGetMe();}
//---------------------------------------------------------------------------
//Передача картинки
void MAX_BOT::SendPhoto(__int64 id,AnsiString fn,AnsiString caption,MAX_PEER_TYPE peerType)
{
    Thread->TaskList.AddSendPhoto(MakePeer(id,peerType),fn,caption);
    MaxUser *user=UserList->Find(id);if(user)user->OutCount++;
}
//---------------------------------------------------------------------------
//Передача документа
void MAX_BOT::SendDoc(__int64 id,AnsiString fn,AnsiString caption,MAX_PEER_TYPE peerType)
{
    Thread->TaskList.AddSendDoc(MakePeer(id,peerType),fn,caption);
    MaxUser *user=UserList->Find(id);if(user)user->OutCount++;
}
//---------------------------------------------------------------------------
//Установка обработчиков событий
//Задание обработчиков лога
void MAX_BOT::SetOnDebugMessage(TMaxDebugMessage dm){Thread->OnDebugMessage=dm;}
TMaxDebugMessage MAX_BOT::GetOnDebugMessage(void){return Thread->OnDebugMessage;}
void MAX_BOT::SetOnErrorDebugMessage(TMaxDebugMessage dm){Thread->OnErrorDebugMessage=dm;}
TMaxDebugMessage MAX_BOT::GetOnErrorDebugMessage(void){return Thread->OnErrorDebugMessage;}
//Задание обработчика приема сообщений по заданию
void MAX_BOT::SetOnTaskReadMessages(TMaxOnReadMessages rm){Thread->OnTaskReadMessages=rm;}
TMaxOnReadMessages MAX_BOT::GetOnTaskReadMessages(void){return Thread->OnTaskReadMessages;}
//Задание обработчика приема сообщения о чтении информации о боте
void MAX_BOT::SetOnGetMe(TMaxOnGetMe gm){Thread->OnGetMe=gm;}
TMaxOnGetMe MAX_BOT::GetOnGetMe(void){return Thread->OnGetMe;}
//Задание обработчика о периодическом чтении сообщений
void MAX_BOT::SetOnPeriodicReadMessages(TMaxOnReadMessages rm){Thread->OnPeriodicReadMessages=rm;}
TMaxOnReadMessages MAX_BOT::GetOnPeriodicReadMessages(void){return Thread->OnPeriodicReadMessages;}
//---------------------------------------------------------------------------
//Возникла/пропала новая авария LanMon
void MAX_BOT::OnNewAlarmState(AnsiString mess)
{
    //Выбираем кому нужно посылать !!!
    MaxUser_LIST *userlist=new MaxUser_LIST;
    //Заполнить список пользователями, соответствующих маске псевдонима
    UserList->GetUsersByAlias(userlist,AlarmAlias);
    for(int i=0;i<userlist->Count;i++){MaxUser *user=userlist->Get(i);SendMessage(user->Id,mess,user->PeerType);}
    delete userlist;
}
//---------------------------------------------------------------------------
//Сделать файл скриншота карты mapindex
bool CreateMapScreenshot(int mapindex,AnsiString fn);
extern char szBitmapDir[];
extern char szWorkDir[]; // Рабочий каталог проекта
bool Bmp2Png(AnsiString bmpfn,AnsiString pngfn);
void CloseAvariaForm(void);
//---------------------------------------------------------------------------
//Получены новые события MAX
void MAX_BOT::OnMessages(MaxMessage_LIST & msglist)
{
    for(int i=0;i<msglist.Count;i++)
    {
        MaxMessage *msg=msglist[i];
        __int64 id=msg->ChatId?msg->ChatId:msg->UserId;
        if(!id)continue;
        UserMessageCount++;
        //Вызов скриптовой функции
        //Для минимального diff используем существующий FastScript hook Telegram.
        OnTgMessage(msg->update_id,IntToStr(id),msg->Text);
        //Проверка сообщения
        if(!FlagSendMaps)continue;
        //Поиск пользователя id
        MaxUser *user=UserList->Find(id);if(!user && msg->UserId)user=UserList->Find(msg->UserId);
        if(!user)continue;
        user->InCount++;
        //Проверка user на маску RequestAlias
        //Проверка, что пользователь соответствует маске псевдонима
        if(user->HasValidAlias(RequestAlias))
        {
            //Может запрашивать
            AnsiString text=msg->Text.UpperCase().Trim();
            if(text.SubString(1,6)=="SCREEN" || text.SubString(1,5)==U8("ЭКРАН"))
            {
                int screenindex=text.SubString(1,6)=="SCREEN"?atoi(text.c_str()+6):atoi(text.c_str()+5);
                AnsiString fn=(AnsiString)szBitmapDir+"_screen.jpg";
                //Сделать скриншот монитора номер mi в JPG файл
                //Экраны начинаются с 1-цы
                if(GetMonitorScreenshot(screenindex-1,fn))
                {
                    //Есть такой монитор
                    //Передача картинки
                    SendPhoto(id,fn,U8("Экран ")+IntToStr(screenindex),user->PeerType);
                }
                else if(DesktopScreenshot(fn))
                {
                    //Скриншот всех мониторов сразу в JPG файл
                    //Передача картинки
                    SendPhoto(id,fn,U8("Экран"),user->PeerType);
                }
            }
            else if(text.SubString(1,3)=="MAP" || text.SubString(1,5)==U8("КАРТА"))
            {
                int mapindex=text.SubString(1,3)=="MAP"?atoi(text.c_str()+3):atoi(text.c_str()+5);
                if(mapindex)
                {
                    AnsiString bmpfn=(AnsiString)szBitmapDir+"_map"+IntToStr(mapindex)+".bmp";
                    //Сделать файл скриншота карты mapindex
                    if(CreateMapScreenshot(mapindex-1,bmpfn))
                    {
                        AnsiString pngfn=ChangeFileExt(bmpfn,".png");
                        if(Bmp2Png(bmpfn,pngfn))
                        {
                            //Передача картинки
                            SendPhoto(id,pngfn,U8("Карта ")+IntToStr(mapindex),user->PeerType);
                        }
                    }
                }
            }
            else if(text.SubString(1,4)=="STOP" || text.SubString(1,4)==U8("СТОП"))
            {
                CloseAvariaForm();
                SendMessage(id,U8("Команда СТОП выполнена"),user->PeerType);
            }
            else if(text.SubString(1,6)=="LOGXLS")
            {
                AnsiString fn=(AnsiString)szWorkDir+"_log.xls";DeleteFile(fn);
                if(LogView){/*Экспорт в XLS файл*/LogView->ExportToXls(fn);/*Передача документа*/SendDoc(id,fn,U8("Журнал ")+Now().DateTimeString(),user->PeerType);}
            }
            else if(text.SubString(1,3)=="LOG" || text.SubString(1,6)==U8("ЖУРНАЛ"))
            {
                AnsiString fn=(AnsiString)szWorkDir+"_log.html";DeleteFile(fn);
                if(LogView){/*Экспорт в HTML файл*/LogView->ExportToHtml(fn);/*Передача документа*/SendDoc(id,fn,U8("Журнал ")+Now().DateTimeString(),user->PeerType);}
            }
            else if(text.SubString(1,5)=="ALARM" || text.SubString(1,6)==U8("ТРЕВОГ"))
            {
extern AnsiString CreateAlarmsPdf(void);
                AnsiString fn=CreateAlarmsPdf();
                if(fn.Length()){/*Передача документа*/SendDoc(id,fn,U8("Тревоги ")+Now().DateTimeString(),user->PeerType);}
            }
            else if(text.SubString(1,4)=="HELP" || text[1]=='?')
            {
                AnsiString mess=U8("Возможные запросы:\n"
                    "SCREEN (Экран) - все экраны\nSCREEN x (Экран x) - экран номер x\n"
                    "MAP x (Карта x) - карта номер x\nLOG (Журнал) - текущий журнал\n"
                    "LOGXLS - текущий журнал в формате XLS\nALARM (Тревоги) - история тревог PDF\n"
                    "STOP (Стоп) - закрыть окно аварий\nHELP (?) - помощь\n?? - дополнительные запросы");
                SendMessage(id,mess,user->PeerType);
            }
        }
    }
}
//---------------------------------------------------------------------------
//Передача сообщения по alias (из LanMon)
void MAX_BOT::SendMessageByAlias(AnsiString alias,AnsiString msg)
{
    //Проверка посылки по alias
    //Выбираем кому нужно посылать !!!
    MaxUser_LIST *userlist=new MaxUser_LIST;
    //Заполнить список пользователями, соответствующих маске псевдонима alias
    UserList->GetUsersByAlias(userlist,alias);
    if(userlist->Count){/*Есть кому посылать*/for(int i=0;i<userlist->Count;i++){MaxUser *user=userlist->Get(i);SendMessage(user->Id,msg,user->PeerType);}}
    else if(alias.Length() && isdigit(alias[1])){/*В первом символе маски стоит цифра*/SendMessage(_atoi64(alias.c_str()),msg);}
    delete userlist;
}
//---------------------------------------------------------------------------
//Передача картинки по alias (из LanMon)
void MAX_BOT::SendPhotoByAlias(AnsiString alias,AnsiString fn,AnsiString caption)
{
    //Проверка посылки по alias
    MaxUser_LIST *userlist=new MaxUser_LIST;UserList->GetUsersByAlias(userlist,alias);
    if(userlist->Count){for(int i=0;i<userlist->Count;i++){MaxUser *user=userlist->Get(i);SendPhoto(user->Id,fn,caption,user->PeerType);}}
    else if(alias.Length() && isdigit(alias[1]))SendPhoto(_atoi64(alias.c_str()),fn,caption);
    delete userlist;
}
//---------------------------------------------------------------------------
//Передача документа по alias (из LanMon)
void MAX_BOT::SendDocByAlias(AnsiString alias,AnsiString fn,AnsiString caption)
{
    //Проверка посылки по alias
    MaxUser_LIST *userlist=new MaxUser_LIST;UserList->GetUsersByAlias(userlist,alias);
    if(userlist->Count){for(int i=0;i<userlist->Count;i++){MaxUser *user=userlist->Get(i);SendDoc(user->Id,fn,caption,user->PeerType);}}
    else if(alias.Length() && isdigit(alias[1]))SendDoc(_atoi64(alias.c_str()),fn,caption);
    delete userlist;
}
//---------------------------------------------------------------------------
