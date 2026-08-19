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
    void __fastcall FormCreate(TObject *Sender);
    void __fastcall EditBotApiChange(TObject *Sender);
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
