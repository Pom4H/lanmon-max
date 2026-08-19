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
    //Идентификатор чата/пользователя
    AnsiString Id;
    //MAX различает user_id и chat_id
    MAX_PEER_TYPE PeerType;
    //Содержит текст сообщения или имя файла
    AnsiString Text;
    //Подпись
    AnsiString Caption;
    MB_TASK(){Type=taskNONE;PeerType=maxPeerUser;}
    //Копирование задания
    void operator=(MB_TASK & task);
};
//---------------------------------------------------------------------------
//Список заданий для потока
class MB_TASK_LIST
{
    //Список заданий
    TThreadList * List;
public:
    //Конструктор
    MB_TASK_LIST();
    //Деструктор
    ~MB_TASK_LIST();
    //Извлечение одного задания из списка
    bool Get(MB_TASK & task);
    //Добавление готового задания в список
    void Put(MB_TASK & task);
    //Очистить все задания
    void Clear();
    //Добавить задание без параметров
    void Add(MB_TASKTYPE type);
    //Посылка сообщения
    void AddSendMsg(AnsiString id,AnsiString text,MAX_PEER_TYPE peerType=maxPeerUser);
    //Чтение сообщений
    void AddReadMsg(void);
    //Информация о себе
    void AddGetMe(void);
    //Посылка файла картинки
    void AddSendPhoto(AnsiString id,AnsiString filename,AnsiString caption,MAX_PEER_TYPE peerType=maxPeerUser);
    //Посылка файла документа
    void AddSendDoc(AnsiString id,AnsiString filename,AnsiString caption,MAX_PEER_TYPE peerType=maxPeerUser);
};
//---------------------------------------------------------------------------
#endif
