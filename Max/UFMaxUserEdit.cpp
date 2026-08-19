//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "UFMaxUserEdit.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormMaxUserEdit *FormMaxUserEdit;
//---------------------------------------------------------------------------
//Конструктор
__fastcall TFormMaxUserEdit::TFormMaxUserEdit(TComponent* Owner,MaxUser *user)
    :TForm(Owner),User(user){}
//---------------------------------------------------------------------------
//Заполнить форму данными пользователя
void __fastcall TFormMaxUserEdit::FormShow(TObject *Sender)
{
    if(!User)return;
    //Основные данные пользователя/чата
    EditName->Text=User->Name;
    EditId->Text=User->Id;
    EditAlias->Text=User->Alias;
    EditComment->Text=User->Comment;
    //Статистика и пользовательский Tag
    EditInCount->Text=IntToStr((__int64)User->InCount);
    EditOutCount->Text=IntToStr((__int64)User->OutCount);
    EditTag->Text=IntToStr(User->Tag);
    //Признак бота
    CheckBoxIsBot->Checked=User->IsBot;
    //MAX различает user_id и chat_id
    ComboPeerType->ItemIndex=User->PeerType==maxPeerChat?1:0;
}
//---------------------------------------------------------------------------
//Сохранить изменения пользователя
void __fastcall TFormMaxUserEdit::ButtonOkClick(TObject *Sender)
{
    if(!User)return;
    //Основные данные пользователя/чата
    User->Name=EditName->Text;
    User->Id=EditId->Text;
    User->Alias=EditAlias->Text;
    User->Comment=EditComment->Text;
    //Статистика и пользовательский Tag
    User->InCount=(UINT)_atoi64(EditInCount->Text.c_str());
    User->OutCount=(UINT)_atoi64(EditOutCount->Text.c_str());
    User->Tag=atoi(EditTag->Text.c_str());
    User->IsBot=CheckBoxIsBot->Checked;
    //Тип адресата определяет user_id/chat_id при отправке через MAX API
    User->PeerType=ComboPeerType->ItemIndex==1?maxPeerChat:maxPeerUser;
    ModalResult=mrOk;
}
//---------------------------------------------------------------------------
