//---------------------------------------------------------------------------
#ifndef maxbotH
#define maxbotH
//---------------------------------------------------------------------------
#include <vcl.h>
#include "maxclient.h"
#include "maxindy.h"
#include "maxsettings.h"
#include "maxtask.h"
#include "maxmsg.h"
#include "maxuser.h"
//---------------------------------------------------------------------------
typedef void __fastcall (__closure * TMaxDebugMessage)(AnsiString msg);
typedef void __fastcall (__closure * TMaxOnReadMessages)(MaxMessage_LIST & msglist);
typedef void __fastcall (__closure * TMaxOnGetMe)(MaxBotInfo & botinfo);
//---------------------------------------------------------------------------
//Состояние потока работы с MAX
enum MB_THREAD_STATE
{
    tsNONE=0,
    tsERROR,
    tsDONE,
    tsTASK,
    tsDISABLED
};
//---------------------------------------------------------------------------
//Поток для работы с MAX
//---------------------------------------------------------------------------
class TMaxBotThread : public TThread
{
protected:
    AnsiString S;
    void __fastcall DebugMessage();
    void __fastcall ErrorDebugMessage();
    //Прочитанные сообщения с сервера
    MaxMessage_LIST MsgList;
    //Прочитаны сообщения по заданию
    void __fastcall OnTaskReadMessagesFunc();
    //Прочитаны сообщения периодически
    void __fastcall OnPeriodicReadMessagesFunc();
    //Информация о себе
    MaxBotInfo BotInfo;
    //Получена информация о себе
    void __fastcall OnBotInfo();
private:
    //Идентификатор разработчика MAX бота
    AnsiString FBotApi;
    void SetBotApi(AnsiString api);
    //Задание нового идентификатора через флаг
    AnsiString NewBotApi;
    bool FlagNewBotApi;
protected:
    //Функция потока
    void __fastcall Execute();
    //Время последнего периодического чтения сообщений
    UINT LastReadTick;
    //Доступ по https
    TMaxIndyTransport * Transport;
    MAX_API_CLIENT * Api;
    //Выполняемое задание
    MB_TASK Task;
    //Выполнение заданий
    void CheckTask(void);
    //Чтение сообщений
    bool DoReadMessages(bool bytask);
    void DoReadMessagesByTask(void);
    void DoReadMessagesPeriodic(void);
    //Передача сообщения
    bool DoSendMessage(void);
    //Информация о себе
    void DoGetMe(void);
    //Посылка файла картинки
    void DoSendPhoto(void);
    //Посылка файла документа
    void DoSendDoc(void);
    //Получение ошибки
    AnsiString GetErrorText(void);
    //Исключительная ошибка
    AnsiString ExeptionText;
public:
    //Конструктор
    __fastcall TMaxBotThread(bool CreateSuspended);
    //Деструктор
    __fastcall ~TMaxBotThread(void);
    //Состояние потока
    MB_THREAD_STATE State;
    //Список заданий для потока
    MB_TASK_LIST TaskList;
    bool BreakSignal;
    //Результаты последнего действия
    AnsiString ResponseText;
    int ResponseCode;
    //Идентификатор разработчика бота
    __property AnsiString BotApi={read=FBotApi,write=SetBotApi};
    //Период чтения сообщений с MAX сервера, с
    UINT PeriodReadMessages;
    //Временно не выполнять периодическое чтение сообщений с сервера
    bool PeriodicReadMessagesPaused;
    //****************************************************
    //События
    //Отладочные сообщения
    TMaxDebugMessage OnDebugMessage;
    //Сообщения об ошибках
    TMaxDebugMessage OnErrorDebugMessage;
    //Сообщение о чтении сообщений по заданию
    TMaxOnReadMessages OnTaskReadMessages;
    //Сообщение о периодическом чтении сообщений
    TMaxOnReadMessages OnPeriodicReadMessages;
    //Сообщение о чтении информации о боте
    TMaxOnGetMe OnGetMe;
    //****************************************************
};
//---------------------------------------------------------------------------
//Класс для работы с MAX Bot
//---------------------------------------------------------------------------
class MAX_BOT
{
    TMaxBotThread * Thread;
    //Идентификатор разработчика бота
    AnsiString FBotApi;
    void SetBotApi(AnsiString api);
    //Период чтения сообщений с MAX сервера, мс
    UINT FPeriodReadMessages;
    void SetPeriodReadMessages(UINT period);
    //Прочитать JSON
    AnsiString GetJson(void){if(Thread)return Thread->ResponseText;return "";}
    //Временно не выполнять периодическое чтение сообщений с сервера
    bool FPeriodicReadMessagesPaused;
    void SetPeriodicReadMessagesPaused(bool v);
    void __fastcall ThreadReadMessages(MaxMessage_LIST &msglist);
    void __fastcall ThreadGetMe(MaxBotInfo &botinfo);
public:
    MAX_BOT();
    ~MAX_BOT();
    //Идентификатор разработчика бота
    __property AnsiString BotApi={read=FBotApi,write=SetBotApi};
    //Данные MAX бота
    MaxBotInfo MyBotInfo;
    //Период чтения сообщений с MAX сервера, мс
    __property UINT PeriodReadMessages={read=FPeriodReadMessages,write=SetPeriodReadMessages};
    //Временно не выполнять периодическое чтение сообщений с сервера
    __property bool PeriodicReadMessagesPaused={read=FPeriodicReadMessagesPaused,write=SetPeriodicReadMessagesPaused};
    //Прочитать JSON
    __property AnsiString Json={read=GetJson};
    //Отсылка алармов
    bool FlagSendAlarms;
    //Посылать сообщения о завершении аварии
    bool FlagSendAlarmsEnd;
    //Сообщать о подтверждении срабатывания аларма
    bool FlagOperatorAlarm;
    //Маска отсылки алармов (кому посылать алармы)
    AnsiString AlarmAlias;
    //Маска пользователей, которым разрешено делать запросы
    AnsiString RequestAlias;
    //Разрешение отсылка картинок карт по запросу MAP
    bool FlagSendMaps;
    //Загрузка бота из файла
    void Load(AnsiString fn);
    //Сохранение бота в файл
    void Save(AnsiString fn);
    //Список пользователей, используемых в программе
    MaxUser_LIST * UserList;
    //Записывать отладочные сообщения бота в lanmon.log
    bool UseLanmonLog;
    //*****************************************
    //Добавление заданий потоку
    //Передача сообщения
    void SendMessage(__int64 id,AnsiString msg,MAX_PEER_TYPE peerType=maxPeerUser);
    //Чтение сообщений
    void ReadMessages(void);
    //Запрос информации о себе
    void GetMe(void);
    //Передача картинки
    void SendPhoto(__int64 id,AnsiString fn,AnsiString caption,MAX_PEER_TYPE peerType=maxPeerUser);
    //Передача документа
    void SendDoc(__int64 id,AnsiString fn,AnsiString caption,MAX_PEER_TYPE peerType=maxPeerUser);
    //*****************************************
    //Установка обработчиков событий
    //Задание обработчиков лога
    void SetOnDebugMessage(TMaxDebugMessage dm);
    TMaxDebugMessage GetOnDebugMessage(void);
    void SetOnErrorDebugMessage(TMaxDebugMessage dm);
    TMaxDebugMessage GetOnErrorDebugMessage(void);
    //Задание обработчика приема сообщений по заданию
    void SetOnTaskReadMessages(TMaxOnReadMessages rm);
    TMaxOnReadMessages GetOnTaskReadMessages(void);
    //Задание обработчика приема сообщения о чтении информации о боте
    void SetOnGetMe(TMaxOnGetMe gm);
    TMaxOnGetMe GetOnGetMe(void);
    //Задание обработчика о периодическом чтении сообщений
    void SetOnPeriodicReadMessages(TMaxOnReadMessages rm);
    TMaxOnReadMessages GetOnPeriodicReadMessages(void);
    //*****************************************
    //Возникла новая авария LanMon
    void OnNewAlarmState(AnsiString mess);
    //Получены новые события MAX
    void OnMessages(MaxMessage_LIST & msglist);
    //*****************************************
    //Передача сообщения по alias (из LanMon)
    void SendMessageByAlias(AnsiString alias,AnsiString msg);
    //Передача картинки по alias (из LanMon)
    void SendPhotoByAlias(AnsiString alias,AnsiString fn,AnsiString caption);
    //Передача документа по alias (из LanMon)
    void SendDocByAlias(AnsiString alias,AnsiString fn,AnsiString caption);
    //Разрешить работу MAX
    bool Active;
    //Получить состояние потока
    MB_THREAD_STATE GetThreadState(void){return Thread->State;}
    //статистика
    UINT ReadMessagesCount;
    UINT ReadMessagesCountOk;
    UINT UserMessageCount;
};
//---------------------------------------------------------------------------
extern MAX_BOT MaxBot;
//---------------------------------------------------------------------------
#endif
