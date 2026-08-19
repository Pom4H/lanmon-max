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
//Форма редактирования пользователя/чата MAX
class TFormMaxUserEdit : public TForm
{
__published:
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
    //Заполнить форму данными пользователя
    void __fastcall FormShow(TObject *Sender);
    //Сохранить изменения пользователя
    void __fastcall ButtonOkClick(TObject *Sender);
private:
    //Редактируемый объект пользователя
    MaxUser *User;
public:
    __fastcall TFormMaxUserEdit(TComponent* Owner,MaxUser *user);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormMaxUserEdit *FormMaxUserEdit;
//---------------------------------------------------------------------------
#endif
