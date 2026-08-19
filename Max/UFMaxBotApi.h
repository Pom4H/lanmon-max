//---------------------------------------------------------------------------
#ifndef UFMaxBotApiH
#define UFMaxBotApiH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include "maxbot.h"
//---------------------------------------------------------------------------
//Форма ввода и проверки MAX BotApi/token
class TFormMaxBotApi : public TForm
{
__published:
    TEdit *EditBotApi;
    TEdit *EditId;
    TEdit *EditFirstName;
    TEdit *EditUsername;
    TButton *ButtonTestBotApi;
    TButton *ButtonOk;
    TButton *ButtonCancel;
    TLabel *LabelHelp;
    //Инициализация формы текущими данными бота
    void __fastcall FormCreate(TObject *Sender);
    //Изменён BotApi/token — требуется повторная проверка
    void __fastcall EditBotApiChange(TObject *Sender);
    //Проверить BotApi запросом GetMe
    void __fastcall ButtonTestBotApiClick(TObject *Sender);
private:
public:
    __fastcall TFormMaxBotApi(TComponent* Owner);
    //Данные MAX бота
    AnsiString OldBotApi;
    AnsiString NewBotApi;
    MaxBotInfo NewMyBotInfo;
    //Получен ответ GetMe
    void __fastcall OnGetMe(MaxBotInfo & botinfo);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormMaxBotApi *FormMaxBotApi;
//---------------------------------------------------------------------------
#endif
