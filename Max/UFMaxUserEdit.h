//---------------------------------------------------------------------------
#ifndef UFMaxUserEditH
#define UFMaxUserEditH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "maxbot.h"
//---------------------------------------------------------------------------
class TFormMaxUserEdit : public TForm
{
__published:
    TLabel *LabelName;
    TLabel *LabelId;
    TLabel *LabelAlias;
    TLabel *LabelComment;
    TLabel *LabelInCount;
    TLabel *LabelOutCount;
    TLabel *LabelTag;
    TLabel *LabelPeerType;
    TEdit *EditName;
    TEdit *EditId;
    TEdit *EditAlias;
    TEdit *EditComment;
    TEdit *EditInCount;
    TEdit *EditOutCount;
    TEdit *EditTag;
    TComboBox *ComboPeerType;
    TCheckBox *CheckBoxIsBot;
    TButton *ButtonOk;
    TButton *ButtonCancel;
    void __fastcall FormShow(TObject *Sender);
    void __fastcall ButtonOkClick(TObject *Sender);
private:
    MaxUser *User;
public:
    __fastcall TFormMaxUserEdit(TComponent* Owner,MaxUser *user);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormMaxUserEdit *FormMaxUserEdit;
//---------------------------------------------------------------------------
#endif
