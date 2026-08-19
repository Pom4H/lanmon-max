//---------------------------------------------------------------------------
#ifndef UFMaxBotH
#define UFMaxBotH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <Dialogs.hpp>
//---------------------------------------------------------------------------
#include "maxbot.h"
//---------------------------------------------------------------------------
class TFormMaxBot : public TForm
{
__published:
    TPageControl *PageControl1;
    TTabSheet *TabSheetBotApi;
    TTabSheet *TabSheetUsers;
    TTabSheet *TabSheetSend;
    TTabSheet *TabSheetLog;
    TTabSheet *TabSheetJson;
    TEdit *EditBotApi;
    TEdit *EditId;
    TEdit *EditFirstName;
    TEdit *EditUsername;
    TEdit *EditPeriodReadMessages;
    TEdit *EditAlarmAlias;
    TEdit *EditRequestAlias;
    TCheckBox *CheckBoxActive;
    TCheckBox *CheckBoxShowBotApi;
    TCheckBox *CheckBoxPeriodReadMessages;
    TCheckBox *CheckBoxFlagSendAlarms;
    TCheckBox *CheckBoxFlagSendAlarmsEnd;
    TCheckBox *CheckBoxFlagOperatorAlarm;
    TCheckBox *CheckBoxFlagSendMaps;
    TCheckBox *CheckBoxUseLanmonLog;
    TButton *ButtonEditBotApi;
    TButton *ButtonReadMessages;
    TButton *ButtonEditUser;
    TButton *ButtonDeleteUser;
    TListView *ListViewUsers;
    TListView *ListViewMsg;
    TListBox *ListBoxLog;
    TMemo *MemoJson;
    TStatusBar *StatusBar1;
    TTimer *Timer1;
    void __fastcall FormShow(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall ButtonEditBotApiClick(TObject *Sender);
    void __fastcall ButtonReadMessagesClick(TObject *Sender);
    void __fastcall ButtonEditUserClick(TObject *Sender);
    void __fastcall ButtonDeleteUserClick(TObject *Sender);
    void __fastcall CheckBoxActiveClick(TObject *Sender);
    void __fastcall CheckBoxShowBotApiClick(TObject *Sender);
    void __fastcall CheckBoxPeriodReadMessagesClick(TObject *Sender);
    void __fastcall EditPeriodReadMessagesChange(TObject *Sender);
    void __fastcall CheckBoxFlagSendAlarmsClick(TObject *Sender);
    void __fastcall CheckBoxFlagSendAlarmsEndClick(TObject *Sender);
    void __fastcall CheckBoxFlagOperatorAlarmClick(TObject *Sender);
    void __fastcall CheckBoxFlagSendMapsClick(TObject *Sender);
    void __fastcall CheckBoxUseLanmonLogClick(TObject *Sender);
    void __fastcall EditAlarmAliasChange(TObject *Sender);
    void __fastcall EditRequestAliasChange(TObject *Sender);
    void __fastcall TabSheetJsonShow(TObject *Sender);
    void __fastcall Timer1Timer(TObject *Sender);
private:
    MaxMessage_LIST MsgList;
    bool Busy;
    void FillUsers(void);
    void ShowMsgList(void);
public:
    __fastcall TFormMaxBot(TComponent* Owner);
    //Прочитаны сообщения
    void __fastcall OnTaskReadMessages(MaxMessage_LIST & msglist);
    //Получен ответ GetMe
    void __fastcall OnGetMe(MaxBotInfo & botinfo);
    //Ошибки
    void __fastcall OnDebugMessage(AnsiString msg);
    void __fastcall OnErrorDebugMessage(AnsiString msg);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormMaxBot *FormMaxBot;
//---------------------------------------------------------------------------
#endif
