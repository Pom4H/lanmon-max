//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "UFMaxBotApi.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormMaxBotApi *FormMaxBotApi;
//---------------------------------------------------------------------------
//Конструктор
__fastcall TFormMaxBotApi::TFormMaxBotApi(TComponent* Owner):TForm(Owner){}
//---------------------------------------------------------------------------
//Данные MAX бота
void __fastcall TFormMaxBotApi::FormCreate(TObject *Sender)
{
    OldBotApi=MaxBot.BotApi;
    EditBotApi->Text=OldBotApi;
    EditId->Text=MaxBot.MyBotInfo.Id;
    EditFirstName->Text=MaxBot.MyBotInfo.first_name;
    EditUsername->Text=MaxBot.MyBotInfo.username;
    ButtonTestBotApi->Enabled=EditBotApi->Text.Length()!=0;
    //OK разрешается только после успешного GetMe
    ButtonOk->Enabled=false;
}
//---------------------------------------------------------------------------
//Изменён BotApi/token
void __fastcall TFormMaxBotApi::EditBotApiChange(TObject *Sender)
{
    ButtonTestBotApi->Enabled=EditBotApi->Text.Length()!=0;
    //После изменения токена предыдущий результат проверки больше не действителен
    ButtonOk->Enabled=false;
}
//---------------------------------------------------------------------------
//Проверить BotApi запросом GetMe
void __fastcall TFormMaxBotApi::ButtonTestBotApiClick(TObject *Sender)
{
    NewMyBotInfo.Clear();
    NewBotApi=EditBotApi->Text;
    //Временно передать новый token в MAX_BOT и выполнить GetMe через рабочий поток
    MaxBot.BotApi=NewBotApi;
    MaxBot.GetMe();
}
//---------------------------------------------------------------------------
//Получен ответ GetMe
void __fastcall TFormMaxBotApi::OnGetMe(MaxBotInfo &botinfo)
{
    if(!botinfo.Valid)
    {
        //MAX не вернул валидные данные бота
        ButtonOk->Enabled=false;
        return;
    }
    //Данные MAX бота
    NewMyBotInfo=botinfo;
    EditId->Text=botinfo.Id;
    EditFirstName->Text=botinfo.first_name;
    EditUsername->Text=botinfo.username;
    ButtonOk->Enabled=true;
}
//---------------------------------------------------------------------------
