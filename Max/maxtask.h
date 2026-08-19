//---------------------------------------------------------------------------
#ifndef maxtaskH
#define maxtaskH
//---------------------------------------------------------------------------
#include <vcl.h>
#include "maxcore.h"
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
    __int64 Id;
    MAX_PEER_TYPE PeerType;
    //Содержит текст сообщения или имя файла
    AnsiString Text;
    //Подпись
    AnsiString Caption;
    MB_TASK(){Type=taskNONE;Id=0;PeerType=maxPeerUser;}
    void operator=(MB_TASK & task)
    {
        Type=task.Type;Id=task.Id;PeerType=task.PeerType;Text=task.Text;Caption=task.Caption;
    }
};
//---------------------------------------------------------------------------
//Список заданий для потока
class MB_TASK_LIST
{
    //Список заданий
    TThreadList * List;
public:
    MB_TASK_LIST(){List=new TThreadList;}
    ~MB_TASK_LIST(){Clear();delete List;}
    bool Get(MB_TASK & task)
    {
        TList *list=List->LockList();
        bool ok=list->Count>0;
        if(ok){MB_TASK *t=(MB_TASK*)list->Items[0];task=*t;delete t;list->Delete(0);}
        List->UnlockList();return ok;
    }
    void Put(MB_TASK & task)
    {
        MB_TASK *t=new MB_TASK;t->Type=task.Type;t->Id=task.Id;t->PeerType=task.PeerType;t->Text=task.Text;t->Caption=task.Caption;
        List->Add(t);
    }
    void Clear(){MB_TASK t;while(Get(t)){};}
    void Add(MB_TASKTYPE type){MB_TASK t;t.Type=type;Put(t);}
    void AddReadMsg(void){Add(taskREADMSG);}
    void AddGetMe(void){Add(taskGETME);}
    void AddSendMsg(MAX_PEER peer,AnsiString text){MB_TASK t;t.Type=taskSENDMSG;t.Id=peer.Id;t.PeerType=peer.Type;t.Text=text;Put(t);}
    void AddSendPhoto(MAX_PEER peer,AnsiString filename,AnsiString caption){MB_TASK t;t.Type=taskSENDPHOTO;t.Id=peer.Id;t.PeerType=peer.Type;t.Text=filename;t.Caption=caption;Put(t);}
    void AddSendDoc(MAX_PEER peer,AnsiString filename,AnsiString caption){MB_TASK t;t.Type=taskSENDDOC;t.Id=peer.Id;t.PeerType=peer.Type;t.Text=filename;t.Caption=caption;Put(t);}
};
//---------------------------------------------------------------------------
#endif
