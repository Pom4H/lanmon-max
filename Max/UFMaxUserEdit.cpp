//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "UFMaxUserEdit.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormMaxUserEdit *FormMaxUserEdit;
//---------------------------------------------------------------------------
__fastcall TFormMaxUserEdit::TFormMaxUserEdit(TComponent* Owner,MaxUser *user)
    :TForm(Owner),User(user){}
//---------------------------------------------------------------------------
void __fastcall TFormMaxUserEdit::FormShow(TObject *Sender)
{
    if(!User)return;
    EditName->Text=User->Name;
    EditId->Text=User->Id;
    EditAlias->Text=User->Alias;
    EditComment->Text=User->Comment;
    EditInCount->Text=IntToStr((__int64)User->InCount);
    EditOutCount->Text=IntToStr((__int64)User->OutCount);
    EditTag->Text=IntToStr(User->Tag);
    CheckBoxIsBot->Checked=User->IsBot;
    ComboPeerType->ItemIndex=User->PeerType==maxPeerChat?1:0;
}
//---------------------------------------------------------------------------
void __fastcall TFormMaxUserEdit::ButtonOkClick(TObject *Sender)
{
    if(!User)return;
    User->Name=EditName->Text;
    User->Id=EditId->Text;
    User->Alias=EditAlias->Text;
    User->Comment=EditComment->Text;
    User->InCount=(UINT)_atoi64(EditInCount->Text.c_str());
    User->OutCount=(UINT)_atoi64(EditOutCount->Text.c_str());
    User->Tag=atoi(EditTag->Text.c_str());
    User->IsBot=CheckBoxIsBot->Checked;
    User->PeerType=ComboPeerType->ItemIndex==1?maxPeerChat:maxPeerUser;
    ModalResult=mrOk;
}
//---------------------------------------------------------------------------
