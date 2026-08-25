object FormMaxBotApi: TFormMaxBotApi
  Left = 0
  Top = 0
  Caption = 'MAX Bot API'
  ClientHeight = 250
  ClientWidth = 650
  Position = poOwnerFormCenter
  OnCreate = FormCreate
  object LabelHelp: TLabel
    Left = 16
    Top = 12
    Width = 610
    Height = 32
    AutoSize = False
    Caption = 'MAX: '#1087#1086#1083#1091#1095#1080#1090#1077' '#1090#1086#1082#1077#1085' '#1074' '#1087#1083#1072#1090#1092#1086#1088#1084#1077' MAX '#1076#1083#1103' '#1087#1072#1088#1090#1085#1105#1088#1086#1074' '#1080' '#1074#1074#1077#1076#1080#1090#1077' '#1077#1075#1086' '#1085#1080#1078#1077'.'
    WordWrap = True
  end
  object EditBotApi: TEdit
    Left = 16
    Top = 56
    Width = 610
    Height = 24
    OnChange = EditBotApiChange
  end
  object ButtonTestBotApi: TButton
    Left = 496
    Top = 88
    Width = 130
    Height = 25
    Caption = #1055#1088#1086#1074#1077#1088#1082#1072
    OnClick = ButtonTestBotApiClick
  end
  object EditId: TEdit Left = 16 Top = 128 Width = 190 Height = 24 ReadOnly = True end
  object EditFirstName: TEdit Left = 216 Top = 128 Width = 190 Height = 24 ReadOnly = True end
  object EditUsername: TEdit Left = 416 Top = 128 Width = 210 Height = 24 ReadOnly = True end
  object ButtonOk: TButton Left = 416 Top = 200 Width = 100 Height = 25 Caption = 'OK' ModalResult = 1 end
  object ButtonCancel: TButton
    Left = 526 Top = 200 Width = 100 Height = 25
    Caption = #1054#1090#1084#1077#1085#1072
    ModalResult = 2
  end
end
