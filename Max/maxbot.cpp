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
char MaxSecSETUP[]="SETUP";
//---------------------------------------------------------------------------
static MAX_PEER MakeMaxPeer(AnsiString id,MAX_PEER_TYPE type)
{
    return MAX_PEER(type,(__int64)_atoi64(id.c_str()));
}
//---------------------------------------------------------------------------
static MAX_TEXT MaxUtf8(AnsiString s)
{
    return MaxUtf8FromCp1251(s);
}
//---------------------------------------------------------------------------
static AnsiString U8(const char * s)
{
    return MaxAnsiFromUtf8(MAX_TEXT(s));
}
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
    OnDebugMessage=NULL;
    OnErrorDebugMessage=NULL;
    OnTaskReadMessages=NULL;
    OnPeriodicReadMessages=NULL;
    OnGetMe=NULL;
    ResponseCode=0;
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
#define DBG_MSG(x)    if(OnDebugMessage){S=x;Synchronize(DebugMessage);}
//---------------------------------------------------------------------------
void __fastcall TMaxBotThread::ErrorDebugMessage()
{
    if(OnErrorDebugMessage)OnErrorDebugMessage(S);
}
#define ERROR_DBG_MSG(x)    if(OnErrorDebugMessage){S=x;Synchronize(ErrorDebugMessage);}
//---------------------------------------------------------------------------
//Прочитаны сообщения по заданию
void __fastcall TMaxBotThread::OnTaskReadMessagesFunc()
{
    if(OnTaskReadMessages)OnTaskReadMessages(MsgList);
}
#define _ON_TASK_READMSG()    {if(OnTaskReadMessages)Synchronize(OnTaskReadMessagesFunc);}
//---------------------------------------------------------------------------
//Прочитаны сообщения периодически
void __fastcall TMaxBotThread::OnPeriodicReadMessagesFunc()
{
    if(OnPeriodicReadMessages)OnPeriodicReadMessages(MsgList);
}
#define _ON_PERIODIC_READMSG()    {if(OnPeriodicReadMessages)Synchronize(OnPeriodicReadMessagesFunc);}
//---------------------------------------------------------------------------
//Информация о себе
void __fastcall TMaxBotThread::OnBotInfo()
{
    if(OnGetMe)OnGetMe(BotInfo);
}
#define _ON_GETME()    {if(OnGetMe)Synchronize(OnBotInfo);}
//---------------------------------------------------------------------------
//Идентификатор разработчика MAX бота: FBotApi
//Установка значения FBotApi
void TMaxBotThread::SetBotApi(AnsiString api)
{
    NewBotApi=api;
    FlagNewBotApi=true;
}
//---------------------------------------------------------------------------
void TMaxBotThread::UpdateResponse(void)
{
    ResponseCode=Api?Api->GetLastStatusCode():0;
    ResponseText=Api?AnsiString(Api->GetLastResponseBody().c_str()):AnsiString("");
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
            Api->SetToken(FBotApi);
        }
        //Разрешить работу MAX
        if(!MaxBot.Active)
        {
            //Работа запрещена
            State=tsDISABLED;    //Запрещён
            ::Sleep(10);
            continue;
        }
        if(!FBotApi.Length())
        {
            //Не задан Id разработчика
            State=tsERROR;    //Ошибка
            ::Sleep(10);
            continue;
        }
        State=tsNONE;
        //Выполнение заданий
        CheckTask();
        //Если есть запрет периодического чтения
        if(PeriodicReadMessagesPaused)
        {
            //Временно не выполнять периодическое чтение сообщений с сервера
            continue;
        }
        //Период чтения сообщений с MAX сервера, с
        if(!PeriodReadMessages)
        {
            //Не нужны периодические чтения
            continue;
        }
        //Время последнего периодического чтения сообщений
        if((GetTickCount()-LastReadTick)>=PeriodReadMessages*1000u)
        {
            //Пора выполнять периодическое чтение сообщений
            if(OnPeriodicReadMessages)
            {
                //Есть обработчик периодического чтения
                //Выполнить чтение сообщений с сервера
                DoReadMessagesPeriodic();
            }
            LastReadTick=GetTickCount();
        }
    }
    State=tsDONE;
}
//---------------------------------------------------------------------------
//Выполнение заданий
void TMaxBotThread::CheckTask(void)
{
    BreakSignal=false;
    //Проверка и выполнение заданий
    //Список заданий записи
    while(TaskList.Get(Task))
    {
        //Есть задание
        if(Terminated)break;
        //Получено задание
        State=tsTASK;    //Выполнение задания
        switch(Task.Type)
        {
            case taskREADMSG:
            {
                //Чтение сообщений по заданию
                DoReadMessagesByTask();
                break;
            }
            case taskSENDMSG:
            {
                //Посылка сообщения
                DoSendMessage();
                break;
            }
            case taskGETME:
            {
                //Информация о себе
                DoGetMe();
                break;
            }
            case taskSENDPHOTO:
            {
                //Посылка файла
                DoSendPhoto();
                break;
            }
            case taskSENDDOC:
            {
                //Посылка файла документа
                DoSendDoc();
                break;
            }
        }
    }
    State=tsNONE;
}
//---------------------------------------------------------------------------
//Чтение сообщений с сервера
bool TMaxBotThread::DoReadMessages(bool bytask)
{
    MaxBot.ReadMessagesCount++;
    //MsgList содержит ранее прочитанные сообщения
    //Курсор MAX хранится внутри MAX_API_CLIENT как marker
    //Очистить список принятых сообщений
    MsgList.Clear();
    ExeptionText="";
    MAX_TEXT error;
    MAX_UPDATES updates;
    bool result=Api->Poll(updates,error,30,100);
    UpdateResponse();
    if(!result)
    {
        //Не удалось
        ExeptionText=error.c_str();
        ERROR_DBG_MSG(GetErrorText());
        return false;
    }
    //Успешно
    //Декодируем JSON
    //Получение сообщений из JSON ответа сервера
    MsgList.CopyFrom(updates);
    if(bytask)
    {
        int cnt=MsgList.Count;
        if(cnt==1)
        {
            DBG_MSG("Успешно прочитано одно сообщение ...");
        }
        else if(cnt>0 && cnt<5)
        {
            DBG_MSG("Успешно прочитаны "+IntToStr(MsgList.Count)+" сообщения ...");
        }
        else
        {
            DBG_MSG("Успешно прочитано "+IntToStr(MsgList.Count)+" сообщений ...");
        }
    }
    MaxBot.ReadMessagesCountOk++;
    return true;
}
//---------------------------------------------------------------------------
//Чтение сообщений по заданию
void TMaxBotThread::DoReadMessagesByTask(void)
{
    //Чтение сообщений с сервера
    if(DoReadMessages(true))
    {
        //Успешно прочитаны сообщения
        _ON_TASK_READMSG();
    }
}
//---------------------------------------------------------------------------
void TMaxBotThread::DoReadMessagesPeriodic(void)
{
    //Чтение сообщений с сервера
    if(DoReadMessages(false))
    {
        if(MsgList.Count)
        {
            //Успешно прочитаны сообщения
            _ON_PERIODIC_READMSG();
        }
    }
}
//---------------------------------------------------------------------------
//Посылка сообщения в Task
bool TMaxBotThread::DoSendMessage(void)
{
    AnsiString inf=" (Посылка: id="+Task.Id+" text="+Task.Text+")";
    ExeptionText="";
    MAX_TEXT error;
    bool result=Api->SendMessage(MakeMaxPeer(Task.Id,Task.PeerType),MaxUtf8(Task.Text),error);
    UpdateResponse();
    if(!result)
    {
        //Не удалось
        ExeptionText=error.c_str();
        ERROR_DBG_MSG(GetErrorText()+inf);
        return false;
    }
    DBG_MSG("Успешно послано сообщение"+inf);
    return true;
}
//---------------------------------------------------------------------------
//Информация о себе
void TMaxBotThread::DoGetMe(void)
{
    BotInfo.Clear();
    ExeptionText="";
    MAX_TEXT error;
    MAX_BOT_INFO info;
    bool result=Api->GetMe(info,error);
    UpdateResponse();
    if(!result)
    {
        //Не удалось
        ExeptionText=error.c_str();
        ERROR_DBG_MSG(GetErrorText());
        _ON_GETME();
        return;
    }
    BotInfo.CopyFrom(info);
    _ON_GETME();
}
//---------------------------------------------------------------------------
//Посылка файла картинки
void TMaxBotThread::DoSendPhoto(void)
{
    AnsiString inf=" (Посылка: id="+Task.Id+" fn="+Task.Text+")";
    ExeptionText="";
    MAX_TEXT error;
    bool result=Api->SendImage(MakeMaxPeer(Task.Id,Task.PeerType),Task.Text,MaxUtf8(Task.Caption),error);
    UpdateResponse();
    if(!result)
    {
        //Не удалось
        ExeptionText=error.c_str();
        ERROR_DBG_MSG(GetErrorText()+inf);
        return;
    }
    DBG_MSG("Успешно послана картинка"+inf);
}
//---------------------------------------------------------------------------
//Посылка файла документа
void TMaxBotThread::DoSendDoc(void)
{
    AnsiString inf=" (Посылка: id="+Task.Id+" fn="+Task.Text+")";
    ExeptionText="";
    MAX_TEXT error;
    bool result=Api->SendFile(MakeMaxPeer(Task.Id,Task.PeerType),Task.Text,MaxUtf8(Task.Caption),error);
    UpdateResponse();
    if(!result)
    {
        //Не удалось
        ExeptionText=error.c_str();
        ERROR_DBG_MSG(GetErrorText()+inf);
        return false;
    }
    DBG_MSG("Успешно послан документ"+inf);
}
//---------------------------------------------------------------------------
//Получение ошибки
AnsiString TMaxBotThread::GetErrorText(void)
{
    AnsiString s=ExeptionText;
    if(ResponseCode)s+=" (HTTP "+IntToStr(ResponseCode)+")";
    return s;
}
//---------------------------------------------------------------------------
//Класс для работы с MAX Bot
//---------------------------------------------------------------------------
MAX_BOT::MAX_BOT()
{
    UserList=new MaxUser_LIST;
    Thread=new TMaxBotThread(true);
    Thread->Resume();
    FPeriodReadMessages=0;
    FPeriodicReadMessagesPaused=false;
    Active=false;
    FlagSendAlarms=false;
    FlagSendAlarmsEnd=false;
    FlagOperatorAlarm=false;
    FlagSendMaps=false;
    UseLanmonLog=false;
    ReadMessagesCount=0;
    ReadMessagesCountOk=0;
    UserMessageCount=0;
}
//---------------------------------------------------------------------------
MAX_BOT::~MAX_BOT()
{
    if(Thread)
    {
        Thread->Terminate();
        Thread->WaitFor();
        delete Thread;
        Thread=NULL;
    }
    if(UserList)delete UserList;
}
//---------------------------------------------------------------------------
//Идентификатор разработчика бота
void MAX_BOT::SetBotApi(AnsiString api)
{
    FBotApi=api;
    if(Thread)Thread->BotApi=api;
}
//---------------------------------------------------------------------------
//Период чтения сообщений с MAX сервера, мс
void MAX_BOT::SetPeriodReadMessages(UINT period)
{
    FPeriodReadMessages=period;
    if(Thread)Thread->PeriodReadMessages=period;
}
//---------------------------------------------------------------------------
void MAX_BOT::SetPeriodicReadMessagesPaused(bool v)
{
    FPeriodicReadMessagesPaused=v;
    if(Thread)Thread->PeriodicReadMessagesPaused=v;
}
//---------------------------------------------------------------------------
MAX_PEER_TYPE MAX_BOT::GetPeerType(AnsiString id)
{
    MaxUser * user=UserList?UserList->Find(id):NULL;
    if(user)return user->PeerType;
    return maxPeerUser;
}
//---------------------------------------------------------------------------
//Передача сообщения
void MAX_BOT::SendMessage(AnsiString id,AnsiString msg)
{
    if(Thread)Thread->TaskList.AddSendMsg(id,msg,GetPeerType(id));
    //Поиск пользователя
    MaxUser * user=UserList->Find(id);
    if(user)user->OutCount++;
}
//---------------------------------------------------------------------------
//Чтение сообщений
void MAX_BOT::ReadMessages(void)
{
    if(Thread)Thread->TaskList.AddReadMsg();
}
//---------------------------------------------------------------------------
//Запрос информации о себе
void MAX_BOT::GetMe(void)
{
    if(Thread)Thread->TaskList.AddGetMe();
}
//---------------------------------------------------------------------------
//Передача картинки
void MAX_BOT::SendPhoto(AnsiString id,AnsiString fn,AnsiString caption)
{
    if(Thread)Thread->TaskList.AddSendPhoto(id,fn,caption,GetPeerType(id));
    //Поиск пользователя
    MaxUser * user=UserList->Find(id);
    if(user)user->OutCount++;
}
//---------------------------------------------------------------------------
//Передача документа
void MAX_BOT::SendDoc(AnsiString id,AnsiString fn,AnsiString caption)
{
    if(Thread)Thread->TaskList.AddSendDoc(id,fn,caption,GetPeerType(id));
    //Поиск пользователя
    MaxUser * user=UserList->Find(id);
    if(user)user->OutCount++;
}
//---------------------------------------------------------------------------
//Задание обработчиков лога
void MAX_BOT::SetOnDebugMessage(TMaxDebugMessage dm)
{
    if(Thread)Thread->OnDebugMessage=dm;
}
TMaxDebugMessage MAX_BOT::GetOnDebugMessage(void)
{
    if(Thread)return Thread->OnDebugMessage;
    return NULL;
}
void MAX_BOT::SetOnErrorDebugMessage(TMaxDebugMessage dm)
{
    if(Thread)Thread->OnErrorDebugMessage=dm;
}
TMaxDebugMessage MAX_BOT::GetOnErrorDebugMessage(void)
{
    if(Thread)return Thread->OnErrorDebugMessage;
    return NULL;
}
//---------------------------------------------------------------------------
//Задание обработчика приема сообщений по заданию
void MAX_BOT::SetOnTaskReadMessages(TMaxOnReadMessages rm)
{
    if(Thread)Thread->OnTaskReadMessages=rm;
}
TMaxOnReadMessages MAX_BOT::GetOnTaskReadMessages(void)
{
    if(Thread)return Thread->OnTaskReadMessages;
    return NULL;
}
//---------------------------------------------------------------------------
//Задание обработчика о периодическом чтении сообщений
void MAX_BOT::SetOnPeriodicReadMessages(TMaxOnReadMessages rm)
{
    if(Thread)Thread->OnPeriodicReadMessages=rm;
}
TMaxOnReadMessages MAX_BOT::GetOnPeriodicReadMessages(void)
{
    if(Thread)return Thread->OnPeriodicReadMessages;
    return NULL;
}
//---------------------------------------------------------------------------
//Задание обработчика приема сообщения о чтении информации о боте
void MAX_BOT::SetOnGetMe(TMaxOnGetMe gm)
{
    if(Thread)Thread->OnGetMe=gm;
}
//---------------------------------------------------------------------------
TMaxOnGetMe MAX_BOT::GetOnGetMe(void)
{
    if(Thread)return Thread->OnGetMe;
    return NULL;
}
//---------------------------------------------------------------------------
//Загрузка из файла
void MAX_BOT::Load(AnsiString fn)
{
    if(!Thread)return;
    if(!FileExists(fn))return;
    TFastIniFile ini(fn);
    //Разрешить работу MAX
    Active=ini.ReadBool(MaxSecSETUP,"Active",true);
    //Идентификатор разработчика MAX бота
    BotApi=ini.ReadString(MaxSecSETUP,"BotApi",ini.ReadString(MaxSecSETUP,"BotToken",""));
    //Уникальный идентификатор бота
    MyBotInfo.Id=ini.ReadString(MaxSecSETUP,"MyBotId","");
    //Имя бота
    MyBotInfo.first_name=ini.ReadString(MaxSecSETUP,"MyBotName","");
    //пользовательское имя бота
    MyBotInfo.username=ini.ReadString(MaxSecSETUP,"MyBotUserName","");
    //Период чтения сообщений с MAX сервера, с
    PeriodReadMessages=ini.ReadInteger(MaxSecSETUP,"PeriodReadMessages",0);
    //Отсылка алармов
    FlagSendAlarms=ini.ReadBool(MaxSecSETUP,"SendAlarms",false);
    //Посылать сообщения о завершении аварии
    FlagSendAlarmsEnd=ini.ReadBool(MaxSecSETUP,"SendAlarmEnd",false);
    //Сообщать о подтверждении срабатывания аларма
    FlagOperatorAlarm=ini.ReadBool(MaxSecSETUP,"OperatorAlarm",false);
    //Маска отсылки алармов
    AlarmAlias=ini.ReadString(MaxSecSETUP,"AlarmAlias","");
    //Маска пользователей, которым разрешено делать запросы
    RequestAlias=ini.ReadString(MaxSecSETUP,"RequestAlias","");
    //Отсылка картинок карт
    FlagSendMaps=ini.ReadBool(MaxSecSETUP,"SendMaps",false);
    //Записывать отладочные сообщения бота в lanmon.log
    UseLanmonLog=ini.ReadBool(MaxSecSETUP,"UseLanmonLog",false);
    //Список пользователей, используемых в программе
    UserList->Load(&ini);
}
//---------------------------------------------------------------------------
//Сохранение в файл
void MAX_BOT::Save(AnsiString fn)
{
    if(!Thread)return;
    DeleteFile(fn);
    TFastIniFile ini(fn,true);
    //Разрешить работу MAX
    ini.WriteBool(MaxSecSETUP,"Active",Active);
    //Идентификатор разработчика MAX бота
    ini.WriteString(MaxSecSETUP,"BotApi",BotApi);
    //Период чтения сообщений с MAX сервера, с
    ini.WriteInteger(MaxSecSETUP,"PeriodReadMessages",PeriodReadMessages);
    //Отсылка алармов
    ini.WriteBool(MaxSecSETUP,"SendAlarms",FlagSendAlarms);
    //Посылать сообщения о завершении аварии
    ini.WriteBool(MaxSecSETUP,"SendAlarmEnd",FlagSendAlarmsEnd);
    //Сообщать о подтверждении срабатывания аларма
    ini.WriteBool(MaxSecSETUP,"OperatorAlarm",FlagOperatorAlarm);
    //Маска отсылки алармов
    ini.WriteString(MaxSecSETUP,"AlarmAlias",AlarmAlias);
    //Маска пользователей, которым разрешено делать запросы
    ini.WriteString(MaxSecSETUP,"RequestAlias",RequestAlias);
    //Отсылка картинок карт
    ini.WriteBool(MaxSecSETUP,"SendMaps",FlagSendMaps);
    //Уникальный идентификатор бота
    ini.WriteString(MaxSecSETUP,"MyBotId",MyBotInfo.Id);
    //Имя бота
    ini.WriteString(MaxSecSETUP,"MyBotName",MyBotInfo.first_name);
    //пользовательское имя бота
    ini.WriteString(MaxSecSETUP,"MyBotUserName",MyBotInfo.username);
    //Записывать отладочные сообщения бота в lanmon.log
    ini.WriteBool(MaxSecSETUP,"UseLanmonLog",UseLanmonLog);
    //Список пользователей, используемых в программе
    UserList->Save(&ini);
}
//---------------------------------------------------------------------------
//Возникла/пропала новая авария LanMon
void MAX_BOT::OnNewAlarmState(AnsiString mess)
{
    //Выбираем кому нужно посылать !!!
    MaxUser_LIST * userlist=new MaxUser_LIST;
    //Заполнить список пользователями, соответствующих маске псевдонима
    UserList->GetUsersByAlias(userlist,AlarmAlias);
    for(int i=0;i<userlist->Count;i++)
    {
        MaxUser * user=userlist->Get(i);
        SendMessage(user->Id,mess);
    }
    delete userlist;
}
//---------------------------------------------------------------------------
//Сделать файл скриншота карты mapindex
bool CreateMapScreenshot(int mapindex,AnsiString fn);
extern char szBitmapDir[];
extern char szWorkDir[];      // Рабочий каталог проекта
bool Bmp2Png(AnsiString bmpfn,AnsiString pngfn);
void CloseAvariaForm(void);
//---------------------------------------------------------------------------
//Получены новые события MAX
void MAX_BOT::OnMessages(MaxMessage_LIST & msglist)
{
    for(int i=0;i<msglist.Count;i++)
    {
        MaxMessage * msg=msglist[i];
        AnsiString scriptid=msg->Chat.Id;
        if(scriptid.IsEmpty())continue;
        MaxBot.UserMessageCount++;
        //Вызов скриптовой функции
        //Compatibility: на первом внедрении MAX использует существующий hook.
        OnTgMessage((int)msg->update_id,scriptid,msg->Text);
        //Проверка сообщения
        if(!FlagSendMaps)continue;
        //Поиск пользователя id
        MaxUser * user=UserList->Find(scriptid);
        if(!user)user=UserList->Find(msg->From.Id);
        if(!user)continue;
        AnsiString id=user->Id;
        //Проверка user на маску RequestAlias
        //Проверка, что пользователь соответствует маске псевдонима
        if(user->HasValidAlias(RequestAlias))
        {
            //Может запрашивать
            AnsiString text=msg->Text.UpperCase().Trim();
            if(text.SubString(1,6)=="SCREEN" || text.SubString(1,5)==U8("ЭКРАН"))
            {
                int screenindex;
                if(text.SubString(1,6)=="SCREEN")screenindex=atoi(text.c_str()+6);
                else screenindex=atoi(text.c_str()+5);
                AnsiString fn;
                fn = (AnsiString)szBitmapDir + "_screen.jpg";
                //Сделать скриншот монитора номер mi в JPG файл
                //Экраны начинаются с 1-цы
                if(GetMonitorScreenshot(screenindex-1,fn))
                {
                    //Есть такой монитор
                    //Передача картинки
                    SendPhoto(id,fn,U8("Экран ")+IntToStr(screenindex));
                }
                else if(DesktopScreenshot(fn))
                {
                    //Скриншот всех мониторов сразу в JPG файл
                    //Передача картинки
                    SendPhoto(id,fn,U8("Экран"));
                }
            }
            else if(text.SubString(1,3)=="MAP" || text.SubString(1,5)==U8("КАРТА"))
            {
                int mapindex;
                if(text.SubString(1,3)=="MAP")mapindex=atoi(text.c_str()+3);
                else mapindex=atoi(text.c_str()+5);
                if(mapindex)
                {
                    AnsiString bmpfn;
                    bmpfn=(AnsiString)szBitmapDir + "_map"+IntToStr(mapindex)+".bmp";
                    //Сделать файл скриншота карты mapindex
                    if(CreateMapScreenshot(mapindex-1,bmpfn))
                    {
                        AnsiString pngfn;
                        pngfn=ChangeFileExt(bmpfn,".png");
                        if(Bmp2Png(bmpfn,pngfn))
                        {
                            //Передача картинки
                            SendPhoto(id,pngfn,U8("Карта ")+IntToStr(mapindex));
                        }
                    }
                }
            }
            else if(text.SubString(1,4)=="STOP" || text.SubString(1,4)==U8("СТОП"))
            {
                CloseAvariaForm();
                SendMessage(id,U8("Команда СТОП выполнена"));
            }
            else if(text.SubString(1,6)=="LOGXLS")
            {
                AnsiString fn=(AnsiString)szWorkDir+(AnsiString)"_log.xls";
                DeleteFile(fn);
                if(LogView)
                {
                    //Экспорт в XLS файл
                    LogView->ExportToXls(fn);
                    //Передача документа
                    SendDoc(id,fn,U8("Журнал ")+Now().DateTimeString());
                }
            }
            else if(text.SubString(1,3)=="LOG" || text.SubString(1,6)==U8("ЖУРНАЛ"))
            {
                AnsiString fn=(AnsiString)szWorkDir+(AnsiString)"_log.html";
                DeleteFile(fn);
                if(LogView)
                {
                    //Экспорт в HTML файл
                    LogView->ExportToHtml(fn);
                    //Передача документа
                    SendDoc(id,fn,U8("Журнал ")+Now().DateTimeString());
                }
            }
            else if(text.SubString(1,5)=="ALARM" || text.SubString(1,6)==U8("ТРЕВОГ"))
            {
extern AnsiString CreateAlarmsPdf(void);
                AnsiString fn=CreateAlarmsPdf();
                if(fn.Length())
                {
                    //Передача документа
                    SendDoc(id,fn,U8("Тревоги ")+Now().DateTimeString());
                }
            }
            else if(text.SubString(1,4)=="HELP" || text[1]=='?')
            {
                AnsiString mess=U8("Возможные запросы:\n"
                "SCREEN (Экран) - все экраны\n"
                "SCREEN x (Экран x) - экран номер x\n"
                "MAP x (Карта x) - карта номер x\n"
                "LOG (Журнал) - текущий журнал\n"
                "LOGXLS - текущий журнал в формате XLS\n"
                "ALARM (Тревоги) - история тревог PDF\n"
                "STOP (Стоп) - закрыть окно аварий\n"
                "HELP (?) - помощь\n"
                "?? - дополнительные запросы");
                SendMessage(id,mess);
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
    MaxUser_LIST * userlist=new MaxUser_LIST;
    //Заполнить список пользователями, соответствующих маске псевдонима alias
    UserList->GetUsersByAlias(userlist,alias);
    if(userlist->Count)
    {
        //Есть кому посылать
        for(int i=0;i<userlist->Count;i++)
        {
            MaxUser * user=userlist->Get(i);
            SendMessage(user->Id,msg);
        }
    }
    else if(alias.Length())
    {
        //По маске alias ничего не найдено
        if(isdigit(alias[1]))
        {
            //В первом символе маски стоит цифра
            SendMessage(alias,msg);
        }
    }
    delete userlist;
}
//---------------------------------------------------------------------------
//Передача картинки по alias (из LanMon)
void MAX_BOT::SendPhotoByAlias(AnsiString alias,AnsiString fn,AnsiString caption)
{
    //Проверка посылки по alias
    //Выбираем кому нужно посылать !!!
    MaxUser_LIST * userlist=new MaxUser_LIST;
    //Заполнить список пользователями, соответствующих маске псевдонима alias
    UserList->GetUsersByAlias(userlist,alias);
    if(userlist->Count)
    {
        //Есть кому посылать
        for(int i=0;i<userlist->Count;i++)
        {
            MaxUser * user=userlist->Get(i);
            SendPhoto(user->Id,fn,caption);
        }
        delete userlist;
        return;
    }
    else if(alias.Length())
    {
        //По маске alias ничего не найдено
        if(isdigit(alias[1]))
        {
            //В первом символе маски стоит цифра
            SendPhoto(alias,fn,caption);
        }
    }
    delete userlist;
}
//---------------------------------------------------------------------------
//Передача документа по alias (из LanMon)
void MAX_BOT::SendDocByAlias(AnsiString alias,AnsiString fn,AnsiString caption)
{
    //Проверка посылки по alias
    //Выбираем кому нужно посылать !!!
    MaxUser_LIST * userlist=new MaxUser_LIST;
    //Заполнить список пользователями, соответствующих маске псевдонима alias
    UserList->GetUsersByAlias(userlist,alias);
    if(userlist->Count)
    {
        //Есть кому посылать
        for(int i=0;i<userlist->Count;i++)
        {
            MaxUser * user=userlist->Get(i);
            SendDoc(user->Id,fn,caption);
        }
        delete userlist;
        return;
    }
    else if(alias.Length())
    {
        //По маске alias ничего не найдено
        if(isdigit(alias[1]))
        {
            //В первом символе маски стоит цифра
            SendDoc(alias,fn,caption);
        }
    }
    delete userlist;
}
//---------------------------------------------------------------------------
