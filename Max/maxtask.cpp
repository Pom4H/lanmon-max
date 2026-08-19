//---------------------------------------------------------------------------
#include <vcl.h>
//---------------------------------------------------------------------------
#pragma hdrstop
#include "maxtask.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void MB_TASK::operator=(MB_TASK & task)
{
    Type=task.Type;
    Id=task.Id;
    PeerType=task.PeerType;
    Text=task.Text;
    Caption=task.Caption;
}
//---------------------------------------------------------------------------
//Список заданий
//---------------------------------------------------------------------------
MB_TASK_LIST::MB_TASK_LIST()
{
    List=new TThreadList;
}
//---------------------------------------------------------------------------
MB_TASK_LIST::~MB_TASK_LIST()
{
    //Очистить все задания
    Clear();
    delete List;
}
//---------------------------------------------------------------------------
//Извлечение одного задания из списка
bool MB_TASK_LIST::Get(MB_TASK & task)
{
    bool ret=false;
    TList * list = List->LockList();
    if(list->Count)
    {
        //Есть задания - выбрать нулевое
        MB_TASK * pt=(MB_TASK *)list->Items[0];
        list->Delete(0);
        task=*pt;
        delete pt;
        ret=true;
    }
    List->UnlockList();
    return ret;
}
//---------------------------------------------------------------------------
void MB_TASK_LIST::Put(MB_TASK & task)
{
    MB_TASK * pt=new MB_TASK;
    *pt=task;
    List->Add(pt);
}
//---------------------------------------------------------------------------
//Очистить все задания
void MB_TASK_LIST::Clear()
{
    MB_TASK task;
    while(Get(task));
}
//---------------------------------------------------------------------------
void MB_TASK_LIST::Add(MB_TASKTYPE type)
{
    //Задание для потока
    MB_TASK task;
    task.Type=type;
    Put(task);
}
//---------------------------------------------------------------------------
void MB_TASK_LIST::AddSendMsg(AnsiString id,AnsiString text,MAX_PEER_TYPE peerType)
{
    //Задание для потока
    MB_TASK task;
    task.Type=taskSENDMSG;
    task.Id=id;
    task.PeerType=peerType;
    task.Text=text;
    Put(task);
}
//---------------------------------------------------------------------------
void MB_TASK_LIST::AddReadMsg(void)
{
    //Задание для потока
    MB_TASK task;
    task.Type=taskREADMSG;
    Put(task);
}
//---------------------------------------------------------------------------
void MB_TASK_LIST::AddGetMe(void)
{
    //Задание для потока
    MB_TASK task;
    task.Type=taskGETME;
    Put(task);
}
//---------------------------------------------------------------------------
void MB_TASK_LIST::AddSendPhoto(AnsiString id,AnsiString filename,AnsiString caption,MAX_PEER_TYPE peerType)
{
    //Задание для потока
    MB_TASK task;
    task.Type=taskSENDPHOTO;
    task.Id=id;
    task.PeerType=peerType;
    task.Text=filename;
    task.Caption=caption;
    Put(task);
}
//---------------------------------------------------------------------------
void MB_TASK_LIST::AddSendDoc(AnsiString id,AnsiString filename,AnsiString caption,MAX_PEER_TYPE peerType)
{
    //Задание для потока
    MB_TASK task;
    task.Type=taskSENDDOC;
    task.Id=id;
    task.PeerType=peerType;
    task.Text=filename;
    task.Caption=caption;
    Put(task);
}
//---------------------------------------------------------------------------
