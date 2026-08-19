object FormMaxUserEdit: TFormMaxUserEdit
  Left = 0
  Top = 0
  Caption = 'Пользователь MAX'
  ClientHeight = 330
  ClientWidth = 500
  Position = poOwnerFormCenter
  OnShow = FormShow
  object EditName: TEdit Left = 150 Top = 16 Width = 320 Height = 24 end
  object EditId: TEdit Left = 150 Top = 48 Width = 320 Height = 24 end
  object EditAlias: TEdit Left = 150 Top = 80 Width = 320 Height = 24 end
  object EditComment: TEdit Left = 150 Top = 112 Width = 320 Height = 24 end
  object EditInCount: TEdit Left = 150 Top = 144 Width = 100 Height = 24 end
  object EditOutCount: TEdit Left = 150 Top = 176 Width = 100 Height = 24 end
  object EditTag: TEdit Left = 150 Top = 208 Width = 100 Height = 24 end
  object ComboPeerType: TComboBox
    Left = 150 Top = 240 Width = 120 Height = 24
    Style = csDropDownList
    Items.Strings = ('user' 'chat')
  end
  object CheckBoxIsBot: TCheckBox Left = 300 Top = 242 Width = 120 Height = 17 Caption = 'Бот' end
  object ButtonOk: TButton Left = 260 Top = 288 Width = 100 Height = 25 Caption = 'OK' OnClick = ButtonOkClick end
  object ButtonCancel: TButton Left = 370 Top = 288 Width = 100 Height = 25 Caption = 'Отмена' ModalResult = 2 end
end
