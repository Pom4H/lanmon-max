//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------
#include "UFMaxBot.h"
#include "UFMaxBotApi.h"
#include "UFMaxUserEdit.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormMaxBot *FormMaxBot;
//---------------------------------------------------------------------------
extern AnsiString WorkDir;
//---------------------------------------------------------------------------
__fastcall TFormMaxBot::TFormMaxBot(TComponent* Owner)
    : TForm(Owner)
{
    Busy=false;
    MaxBot.PeriodicReadMessagesPaused=true;
}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBot::FormShow(TObject *Sender)
{
    Busy=true;
    //Разрешить работу MAX бота
    CheckBoxActive->Checked=MaxBot.Active;
    //Идентификатор разработчика бота
    EditBotApi->Text=MaxBot.BotApi;
    //Данные MAX бота
    EditId->Text=MaxBot.MyBotInfo.Id;
    EditFirstName->Text=MaxBot.MyBotInfo.first_name;
    EditUsername->Text=MaxBot.MyBotInfo.username;
    //Отсылка алармов
    CheckBoxFlagSendAlarms->Checked=MaxBot.FlagSendAlarms;
    //Посылать сообщения о завершении аварии
    CheckBoxFlagSendAlarmsEnd->Checked=MaxBot.FlagSendAlarmsEnd;
    //Сообщать о подтверждении срабатывания аларма
    CheckBoxFlagOperatorAlarm->Checked=MaxBot.FlagOperatorAlarm;
    //Маска отсылки алармов
    EditAlarmAlias->Text=MaxBot.AlarmAlias;
    //Маска пользователей, которым разрешено делать запросы
    EditRequestAlias->Text=MaxBot.RequestAlias;
    //Разрешение отсылка картинок карт по запросу MAP
    CheckBoxFlagSendMaps->Checked=MaxBot.FlagSendMaps;
    //Записывать отладочные сообщения бота в lanmon.log
    CheckBoxUseLanmonLog->Checked=MaxBot.UseLanmonLog;
    UINT period=MaxBot.PeriodReadMessages;
    CheckBoxPeriodReadMessages->Checked=period!=0;
    EditPeriodReadMessages->Text=IntToStr(period?period:10);
    Busy=false;
    FillUsers();
}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBot::FormClose(TObject *Sender, TCloseAction &Action)
{
    //сохранение объекта MAX бот
    MaxBot.Save(WorkDir+"MaxBot.ini");
    MaxBot.PeriodicReadMessagesPaused=false;
    Action=caFree;
    FormMaxBot=NULL;
}
//---------------------------------------------------------------------------
void TFormMaxBot::FillUsers(void)
{
    ListViewUsers->Items->BeginUpdate();
    ListViewUsers->Items->Clear();
    for(int i=0;i<MaxBot.UserList->Count;i++)
    {
        //Список пользователей, используемых в программе
        MaxUser *user=MaxBot.UserList->Get(i);
        TListItem *li=ListViewUsers->Items->Add();
        li->Caption=IntToStr(i+1);
        li->SubItems->Add(user->Name);
        li->SubItems->Add(user->Id);
        li->SubItems->Add(user->Alias);
        li->SubItems->Add(IntToStr((__int64)user->InCount));
        li->SubItems->Add(IntToStr((__int64)user->OutCount));
        li->SubItems->Add(user->PeerType==maxPeerChat?"chat":"user");
        li->Data=user;
    }
    ListViewUsers->Items->EndUpdate();
}
//---------------------------------------------------------------------------
//Показать сообщения
void TFormMaxBot::ShowMsgList(void)
{
    ListViewMsg->Items->BeginUpdate();
    ListViewMsg->Items->Clear();
    for(int i=0;i<MsgList.Count;i++)
    {
        MaxMessage *msg=MsgList[i];
        TListItem *li=ListViewMsg->Items->Add();
        li->Caption=IntToStr(i+1);
        li->SubItems->Add(msg->DateText);
        li->SubItems->Add(msg->Text);
        li->SubItems->Add(msg->Chat.FullName);
        li->SubItems->Add(msg->Chat.Id);
    }
    ListViewMsg->Items->EndUpdate();
}
//---------------------------------------------------------------------------
//Прочитаны сообщения
void __fastcall TFormMaxBot::OnTaskReadMessages(MaxMessage_LIST &msglist)
{
    MsgList.CopyFrom(msglist);
    //Статистика ручного чтения — как в Telegram UI
    for(int i=0;i<MsgList.Count;i++)
    {
        MaxUser *user=MaxBot.UserList->Find(MsgList[i]->Chat.Id);
        if(!user)user=MaxBot.UserList->Find(MsgList[i]->From.Id);
        if(user)user->InCount++;
    }
    FillUsers();
    ShowMsgList();
}
//---------------------------------------------------------------------------
//Получен ответ GetMe
void __fastcall TFormMaxBot::OnGetMe(MaxBotInfo &botinfo)
{
    if(FormMaxBotApi)FormMaxBotApi->OnGetMe(botinfo);
}
//---------------------------------------------------------------------------
//Ошибки
void __fastcall TFormMaxBot::OnDebugMessage(AnsiString msg)
{
    ListBoxLog->Items->Add(Now().DateTimeString()+" > "+msg);
}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBot::OnErrorDebugMessage(AnsiString msg)
{
    ListBoxLog->Items->Add(Now().DateTimeString()+" > ERROR: "+msg);
    PageControl1->ActivePage=TabSheetLog;
}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBot::ButtonEditBotApiClick(TObject *Sender)
{
    TFormMaxBotApi *form=new TFormMaxBotApi(this);
    if(form->ShowModal()==mrOk)
    {
        MaxBot.BotApi=form->NewBotApi;
        MaxBot.MyBotInfo=form->NewMyBotInfo;
        EditBotApi->Text=MaxBot.BotApi;
        EditId->Text=MaxBot.MyBotInfo.Id;
        EditFirstName->Text=MaxBot.MyBotInfo.first_name;
        EditUsername->Text=MaxBot.MyBotInfo.username;
    }
    else MaxBot.BotApi=form->OldBotApi;
    delete form;
}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBot::ButtonReadMessagesClick(TObject *Sender)
{
    MaxBot.ReadMessages();
}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBot::ButtonEditUserClick(TObject *Sender)
{
    if(!ListViewUsers->Selected)return;
    MaxUser *user=(MaxUser *)ListViewUsers->Selected->Data;
    if(!user)return;
    MaxUser copy;copy.CopyFrom(user);
    TFormMaxUserEdit *form=new TFormMaxUserEdit(this,&copy);
    if(form->ShowModal()==mrOk){user->CopyFrom(&copy);FillUsers();}
    delete form;
}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBot::ButtonDeleteUserClick(TObject *Sender)
{
    if(!ListViewUsers->Selected)return;
    int index=ListViewUsers->Selected->Index;
    MaxBot.UserList->DeleteUser(index);
    FillUsers();
}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBot::CheckBoxActiveClick(TObject *Sender)
{if(!Busy)MaxBot.Active=CheckBoxActive->Checked;}
void __fastcall TFormMaxBot::CheckBoxShowBotApiClick(TObject *Sender)
{EditBotApi->PasswordChar=CheckBoxShowBotApi->Checked?0:'*';}
void __fastcall TFormMaxBot::CheckBoxFlagSendAlarmsClick(TObject *Sender)
{if(!Busy)MaxBot.FlagSendAlarms=CheckBoxFlagSendAlarms->Checked;}
void __fastcall TFormMaxBot::CheckBoxFlagSendAlarmsEndClick(TObject *Sender)
{if(!Busy)MaxBot.FlagSendAlarmsEnd=CheckBoxFlagSendAlarmsEnd->Checked;}
void __fastcall TFormMaxBot::CheckBoxFlagOperatorAlarmClick(TObject *Sender)
{if(!Busy)MaxBot.FlagOperatorAlarm=CheckBoxFlagOperatorAlarm->Checked;}
void __fastcall TFormMaxBot::CheckBoxFlagSendMapsClick(TObject *Sender)
{if(!Busy)MaxBot.FlagSendMaps=CheckBoxFlagSendMaps->Checked;}
void __fastcall TFormMaxBot::CheckBoxUseLanmonLogClick(TObject *Sender)
{if(!Busy)MaxBot.UseLanmonLog=CheckBoxUseLanmonLog->Checked;}
void __fastcall TFormMaxBot::EditAlarmAliasChange(TObject *Sender)
{if(!Busy)MaxBot.AlarmAlias=EditAlarmAlias->Text;}
void __fastcall TFormMaxBot::EditRequestAliasChange(TObject *Sender)
{if(!Busy)MaxBot.RequestAlias=EditRequestAlias->Text;}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBot::CheckBoxPeriodReadMessagesClick(TObject *Sender)
{
    if(Busy)return;
    MaxBot.PeriodReadMessages=CheckBoxPeriodReadMessages->Checked?
        (UINT)atoi(EditPeriodReadMessages->Text.c_str()):0;
}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBot::EditPeriodReadMessagesChange(TObject *Sender)
{
    if(Busy || !CheckBoxPeriodReadMessages->Checked)return;
    UINT value=atoi(EditPeriodReadMessages->Text.c_str());
    if(value)MaxBot.PeriodReadMessages=value;
}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBot::TabSheetJsonShow(TObject *Sender)
{MemoJson->Text=MaxBot.Json;}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBot::Timer1Timer(TObject *Sender)
{
    if(StatusBar1->Panels->Count<3)return;
    StatusBar1->Panels->Items[0]->Text="Thread: "+IntToStr((int)MaxBot.GetThreadState());
    StatusBar1->Panels->Items[1]->Text="Reads: "+IntToStr((__int64)MaxBot.ReadMessagesCount)+"/"+IntToStr((__int64)MaxBot.ReadMessagesCountOk);
    StatusBar1->Panels->Items[2]->Text="Requests: "+IntToStr((__int64)MaxBot.UserMessageCount);
}
//---------------------------------------------------------------------------
