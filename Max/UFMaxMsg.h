//---------------------------------------------------------------------------
#ifndef UFMaxMsgH
#define UFMaxMsgH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
//Универсальная форма ввода текста для операций MAX
class TFormMaxMsg : public TForm
{
__published:
    TLabel *LabelHdr;
    TEdit *EditText;
    TButton *ButtonOk;
    TButton *ButtonCancel;
private:
public:
    //Конструктор
    __fastcall TFormMaxMsg(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormMaxMsg *FormMaxMsg;
//---------------------------------------------------------------------------
#endif
