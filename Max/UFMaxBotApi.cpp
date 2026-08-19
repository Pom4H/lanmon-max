//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "UFMaxBotApi.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormMaxBotApi *FormMaxBotApi;
//---------------------------------------------------------------------------
__fastcall TFormMaxBotApi::TFormMaxBotApi(TComponent* Owner):TForm(Owner){}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBotApi::FormCreate(TObject *Sender)
{
    OldBotApi=MaxBot.BotApi;
    EditBotApi->Text=OldBotApi;
    EditId->Text=MaxBot.MyBotInfo.Id;
    EditFirstName->Text=MaxBot.MyBotInfo.first_name;
    EditUsername->Text=MaxBot.MyBotInfo.username;
    ButtonTestBotApi->Enabled=EditBotApi->Text.Length()!=0;
    ButtonOk->Enabled=false;
}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBotApi::EditBotApiChange(TObject *Sender)
{ButtonTestBotApi->Enabled=EditBotApi->Text.Length()!=0;ButtonOk->Enabled=false;}
//---------------------------------------------------------------------------
void __fastcall TFormMaxBotApi::ButtonTestBotApiClick(TObject *Sender)
{
    NewMyBotInfo.Clear();
    NewBotApi=EditBotApi->Text;
    MaxBot.BotApi=NewBotApi;
    MaxBot.GetMe();
}
//---------------------------------------------------------------------------
//Получен ответ GetMe
void __fastcall TFormMaxBotApi::OnGetMe(MaxBotInfo &botinfo)
{
    if(!botinfo.Valid){ButtonOk->Enabled=false;return;}
    NewMyBotInfo=botinfo;
    EditId->Text=botinfo.Id;
    EditFirstName->Text=botinfo.first_name;
    EditUsername->Text=botinfo.username;
    ButtonOk->Enabled=true;
}
//---------------------------------------------------------------------------
