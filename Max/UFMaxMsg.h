//---------------------------------------------------------------------------
#ifndef UFMaxMsgH
#define UFMaxMsgH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
class TFormMaxMsg : public TForm
{
__published:
    TLabel *LabelHdr;
    TEdit *EditText;
    TButton *ButtonOk;
    TButton *ButtonCancel;
private:
public:
    __fastcall TFormMaxMsg(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormMaxMsg *FormMaxMsg;
//---------------------------------------------------------------------------
#endif
