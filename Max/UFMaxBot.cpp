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
//Рабочий каталог проекта
extern AnsiString WorkDir;
//---------------------------------------------------------------------------
//Конструктор формы настройки MAX бота
__fastcall TFormMaxBot::TFormMaxBot(TComponent* Owner)
    : TForm(Owner)
{
    Busy=false;
    //На время открытой формы останавливаем периодическое чтение сообщений
    MaxBot.PeriodicReadMessagesPaused=true;
}
//---------------------------------------------------------------------------
//Показ формы и заполнение элементов текущими настройками MAX_BOT
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
    //Разрешить запрос картинок командой MAP
    CheckBoxFlagSendMaps->Checked=MaxBot.FlagSendMaps;
    //Записывать отладочные сообщения бота в lanmon.log
    CheckBoxUseLanmonLog->Checked=MaxBot.UseLanmonLog;
    //Период чтения сообщений с MAX сервера, с
    UINT period=MaxBot.PeriodReadMessages;
    CheckBoxPeriodReadMessages->Checked=period!=0;
    EditPeriodReadMessages->Text=IntToStr(period?period:10);
    Busy=false;
    //Показать список пользователей
    FillUsers();
}
//---------------------------------------------------------------------------
//Закрытие формы
void __fastcall TFormMaxBot::FormClose(TObject *Sender, TCloseAction &Action)
{
    //Сохранение объекта MAX бот
    MaxBot.Save(WorkDir+"MaxBot.ini");
    //Возобновить периодическое чтение сообщений
    MaxBot.PeriodicReadMessagesPaused=false;
    Action=caFree;
    FormMaxBot=NULL;
}
//---------------------------------------------------------------------------
//Заполнить список пользователей
void TFormMaxBot::FillUsers(void)
{
    ListViewUsers->Items->BeginUpdate();
    ListViewUsers->Items->Clear();
    for(int i=0;i<MaxBot.UserList->Count;i++)
    {
        //Список пользователей, используемых в программе
        MaxUser *user=MaxBot.UserList->Get(i);
        TListItem *li=ListViewUsers->Items->Add();
        //0 - номер ПП
        li->Caption=IntToStr(i+1);
        //1 Пользователь/Чат
        li->SubItems->Add(user->Name);
        //2 Идентификатор
        li->SubItems->Add(user->Id);
        //3 Псевдоним
        li->SubItems->Add(user->Alias);
        //4 Принято
        li->SubItems->Add(IntToStr((__int64)user->InCount));
        //5 Послано
        li->SubItems->Add(IntToStr((__int64)user->OutCount));
        //6 Тип адресата MAX: user_id/chat_id
        li->SubItems->Add(user->PeerType==maxPeerChat?"chat":"user");
        //Сохраняем указатель на пользователя для команд редактирования
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
        //0 - номер ПП
        li->Caption=IntToStr(i+1);
        //1 Время
        li->SubItems->Add(msg->DateText);
        //2 Текст
        li->SubItems->Add(msg->Text);
        //3 От кого
        li->SubItems->Add(msg->Chat.FullName);
        //4 Идентификатор
        li->SubItems->Add(msg->Chat.Id);
    }
    ListViewMsg->Items->EndUpdate();
}
//---------------------------------------------------------------------------
//Прочитаны сообщения
void __fastcall TFormMaxBot::OnTaskReadMessages(MaxMessage_LIST &msglist)
{
    //Получены сообщения
    MsgList.CopyFrom(msglist);
    //Статистика ручного чтения — как в Telegram UI
    for(int i=0;i<MsgList.Count;i++)
    {
        //Идентификатор пользователя/чата
        MaxUser *user=MaxBot.UserList->Find(MsgList[i]->Chat.Id);
        if(!user)user=MaxBot.UserList->Find(MsgList[i]->From.Id);
        if(user)user->InCount++;
    }
    //Обновить пользователей и показать сообщения
    FillUsers();
    ShowMsgList();
}
//---------------------------------------------------------------------------
//Получен ответ GetMe
void __fastcall TFormMaxBot::OnGetMe(MaxBotInfo &botinfo)
{
    //Если открыта форма проверки BotApi, передать ей данные MAX бота
    if(FormMaxBotApi)FormMaxBotApi->OnGetMe(botinfo);
}
//---------------------------------------------------------------------------
//Отладочные сообщения
void __fastcall TFormMaxBot::OnDebugMessage(AnsiString msg)
{
    ListBoxLog->Items->Add(Now().DateTimeString()+" > "+msg);
}
//---------------------------------------------------------------------------
//Ошибки
void __fastcall TFormMaxBot::OnErrorDebugMessage(AnsiString msg)
{
    ListBoxLog->Items->Add(Now().DateTimeString()+" > ERROR: "+msg);
    //При ошибке сразу показать вкладку лога
    PageControl1->ActivePage=TabSheetLog;
}
//---------------------------------------------------------------------------
//Изменить/проверить идентификатор разработчика бота
void __fastcall TFormMaxBot::ButtonEditBotApiClick(TObject *Sender)
{
    TFormMaxBotApi *form=new TFormMaxBotApi(this);
    if(form->ShowModal()==mrOk)
    {
        //Данные MAX бота обновлены
        MaxBot.BotApi=form->NewBotApi;
        MaxBot.MyBotInfo=form->NewMyBotInfo;
        EditBotApi->Text=MaxBot.BotApi;
        EditId->Text=MaxBot.MyBotInfo.Id;
        EditFirstName->Text=MaxBot.MyBotInfo.first_name;
        EditUsername->Text=MaxBot.MyBotInfo.username;
    }
    else
    {
        //При отмене восстановить прежний BotApi
        MaxBot.BotApi=form->OldBotApi;
    }
    delete form;
}
//---------------------------------------------------------------------------
//Ручное чтение сообщений
void __fastcall TFormMaxBot::ButtonReadMessagesClick(TObject *Sender)
{
    MaxBot.ReadMessages();
}
//---------------------------------------------------------------------------
//Изменить пользователя
void __fastcall TFormMaxBot::ButtonEditUserClick(TObject *Sender)
{
    if(!ListViewUsers->Selected)return;
    //Список пользователей, используемых в программе
    MaxUser *user=(MaxUser *)ListViewUsers->Selected->Data;
    if(!user)return;
    //Редактируем копию и применяем только после OK
    MaxUser copy;
    copy.CopyFrom(user);
    TFormMaxUserEdit *form=new TFormMaxUserEdit(this,&copy);
    if(form->ShowModal()==mrOk)
    {
        user->CopyFrom(&copy);
        FillUsers();
    }
    delete form;
}
//---------------------------------------------------------------------------
//Удалить пользователя
void __fastcall TFormMaxBot::ButtonDeleteUserClick(TObject *Sender)
{
    if(!ListViewUsers->Selected)return;
    int index=ListViewUsers->Selected->Index;
    MaxBot.UserList->DeleteUser(index);
    FillUsers();
}
//---------------------------------------------------------------------------
//Разрешить работу MAX бота
void __fastcall TFormMaxBot::CheckBoxActiveClick(TObject *Sender)
{
    if(!Busy)MaxBot.Active=CheckBoxActive->Checked;
}
//---------------------------------------------------------------------------
//Показать/скрыть BotApi
void __fastcall TFormMaxBot::CheckBoxShowBotApiClick(TObject *Sender)
{
    EditBotApi->PasswordChar=CheckBoxShowBotApi->Checked?0:'*';
}
//---------------------------------------------------------------------------
//Отсылка алармов
void __fastcall TFormMaxBot::CheckBoxFlagSendAlarmsClick(TObject *Sender)
{
    if(!Busy)MaxBot.FlagSendAlarms=CheckBoxFlagSendAlarms->Checked;
}
//---------------------------------------------------------------------------
//Посылать сообщения о завершении аварии
void __fastcall TFormMaxBot::CheckBoxFlagSendAlarmsEndClick(TObject *Sender)
{
    if(!Busy)MaxBot.FlagSendAlarmsEnd=CheckBoxFlagSendAlarmsEnd->Checked;
}
//---------------------------------------------------------------------------
//Сообщать о подтверждении срабатывания аларма
void __fastcall TFormMaxBot::CheckBoxFlagOperatorAlarmClick(TObject *Sender)
{
    if(!Busy)MaxBot.FlagOperatorAlarm=CheckBoxFlagOperatorAlarm->Checked;
}
//---------------------------------------------------------------------------
//Разрешить запрос картинок командой MAP
void __fastcall TFormMaxBot::CheckBoxFlagSendMapsClick(TObject *Sender)
{
    if(!Busy)MaxBot.FlagSendMaps=CheckBoxFlagSendMaps->Checked;
}
//---------------------------------------------------------------------------
//Записывать отладочные сообщения бота в lanmon.log
void __fastcall TFormMaxBot::CheckBoxUseLanmonLogClick(TObject *Sender)
{
    if(!Busy)MaxBot.UseLanmonLog=CheckBoxUseLanmonLog->Checked;
}
//---------------------------------------------------------------------------
//Маска отсылки алармов
void __fastcall TFormMaxBot::EditAlarmAliasChange(TObject *Sender)
{
    if(!Busy)MaxBot.AlarmAlias=EditAlarmAlias->Text;
}
//---------------------------------------------------------------------------
//Маска пользователей, которым разрешено делать запросы
void __fastcall TFormMaxBot::EditRequestAliasChange(TObject *Sender)
{
    if(!Busy)MaxBot.RequestAlias=EditRequestAlias->Text;
}
//---------------------------------------------------------------------------
//Включить/выключить периодическое чтение сообщений
void __fastcall TFormMaxBot::CheckBoxPeriodReadMessagesClick(TObject *Sender)
{
    if(Busy)return;
    //Нулевой период полностью отключает периодическое чтение
    MaxBot.PeriodReadMessages=CheckBoxPeriodReadMessages->Checked?
        (UINT)atoi(EditPeriodReadMessages->Text.c_str()):0;
}
//---------------------------------------------------------------------------
//Изменение периода чтения сообщений с MAX сервера
void __fastcall TFormMaxBot::EditPeriodReadMessagesChange(TObject *Sender)
{
    if(Busy || !CheckBoxPeriodReadMessages->Checked)return;
    UINT value=atoi(EditPeriodReadMessages->Text.c_str());
    if(value)MaxBot.PeriodReadMessages=value;
}
//---------------------------------------------------------------------------
//Показать последний JSON/HTTP ответ MAX
void __fastcall TFormMaxBot::TabSheetJsonShow(TObject *Sender)
{
    MemoJson->Text=MaxBot.Json;
}
//---------------------------------------------------------------------------
//Обновление строки состояния
void __fastcall TFormMaxBot::Timer1Timer(TObject *Sender)
{
    if(StatusBar1->Panels->Count<3)return;
    //Получить состояние потока
    StatusBar1->Panels->Items[0]->Text="Thread: "+IntToStr((int)MaxBot.GetThreadState());
    //Статистика чтения сообщений
    StatusBar1->Panels->Items[1]->Text="Reads: "+IntToStr((__int64)MaxBot.ReadMessagesCount)+"/"+IntToStr((__int64)MaxBot.ReadMessagesCountOk);
    //Количество запросов пользователей
    StatusBar1->Panels->Items[2]->Text="Requests: "+IntToStr((__int64)MaxBot.UserMessageCount);
}
//---------------------------------------------------------------------------
