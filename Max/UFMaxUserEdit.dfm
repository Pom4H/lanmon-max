object FormMaxUserEdit: TFormMaxUserEdit
  Left = 0
  Top = 0
  Caption = #1055#1086#1083#1100#1079#1086#1074#1072#1090#1077#1083#1100' MAX'
  ClientHeight = 330
  ClientWidth = 500
  Position = poOwnerFormCenter
  OnShow = FormShow
  object LabelName: TLabel
    Left = 16
    Top = 20
    Width = 113
    Height = 13
    Caption = #1048#1084#1103
  end
  object LabelId: TLabel
    Left = 16
    Top = 52
    Width = 113
    Height = 13
    Caption = 'Id'
  end
  object LabelAlias: TLabel
    Left = 16
    Top = 84
    Width = 113
    Height = 13
    Caption = 'Alias'
  end
  object LabelComment: TLabel
    Left = 16
    Top = 116
    Width = 113
    Height = 13
    Caption = #1050#1086#1084#1084#1077#1085#1090#1072#1088#1080#1081
  end
  object LabelInCount: TLabel
    Left = 16
    Top = 148
    Width = 113
    Height = 13
    Caption = 'In'
  end
  object LabelOutCount: TLabel
    Left = 16
    Top = 180
    Width = 113
    Height = 13
    Caption = 'Out'
  end
  object LabelTag: TLabel
    Left = 16
    Top = 212
    Width = 113
    Height = 13
    Caption = 'Tag'
  end
  object LabelPeerType: TLabel
    Left = 16
    Top = 244
    Width = 113
    Height = 13
    Caption = 'Peer type'
  end
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
  object CheckBoxIsBot: TCheckBox
    Left = 300 Top = 242 Width = 120 Height = 17
    Caption = #1041#1086#1090
  end
  object ButtonOk: TButton Left = 260 Top = 288 Width = 100 Height = 25 Caption = 'OK' OnClick = ButtonOkClick end
  object ButtonCancel: TButton
    Left = 370 Top = 288 Width = 100 Height = 25
    Caption = #1054#1090#1084#1077#1085#1072
    ModalResult = 2
  end
end
