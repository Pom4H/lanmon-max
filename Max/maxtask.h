//---------------------------------------------------------------------------
#ifndef maxtaskH
#define maxtaskH
//---------------------------------------------------------------------------
#include <vcl.h>
#include "api/maxcore.h"
//---------------------------------------------------------------------------
//Типы заданий
enum MB_TASKTYPE
{
    taskNONE,
    taskGETME,          //Информация о себе
    taskREADMSG,        //Чтение сообщений
    taskSENDMSG,        //Посылка сообщения
    taskSENDPHOTO,      //Посылка файла картинки
    taskSENDDOC         //Посылка файла документа
};
//---------------------------------------------------------------------------
//Задание для потока
struct MB_TASK
{
    MB_TASKTYPE Type;
    //Идентификатор чата
    AnsiString Id;
    //MAX различает user_id и chat_id
    MAX_PEER_TYPE PeerType;
    //Содержит текст сообщения или имя файла
    AnsiString Text;
    //Подпись
    AnsiString Caption;
    MB_TASK(){Type=taskNONE;PeerType=maxPeerUser;}
    void operator=(MB_TASK & task);
};
//---------------------------------------------------------------------------
//Список заданий для потока
class MB_TASK_LIST
{
    //Список заданий
    TThreadList * List;
public:
    MB_TASK_LIST();
    ~MB_TASK_LIST();
    bool Get(MB_TASK & task);
    void Put(MB_TASK & task);
    void Clear();
    void Add(MB_TASKTYPE type);
    void AddSendMsg(AnsiString id,AnsiString text,MAX_PEER_TYPE peerType=maxPeerUser);
    void AddReadMsg(void);
    void AddGetMe(void);
    void AddSendPhoto(AnsiString id,AnsiString filename,AnsiString caption,MAX_PEER_TYPE peerType=maxPeerUser);
    void AddSendDoc(AnsiString id,AnsiString filename,AnsiString caption,MAX_PEER_TYPE peerType=maxPeerUser);
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#endif
