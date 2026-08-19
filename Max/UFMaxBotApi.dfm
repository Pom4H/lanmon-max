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
    Caption = 'MAX: получите токен в платформе MAX для партнёров и введите его ниже.'
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
    Caption = 'Проверка'
    OnClick = ButtonTestBotApiClick
  end
  object EditId: TEdit
    Left = 16
    Top = 128
    Width = 190
    Height = 24
    ReadOnly = True
  end
  object EditFirstName: TEdit
    Left = 216
    Top = 128
    Width = 190
    Height = 24
    ReadOnly = True
  end
  object EditUsername: TEdit
    Left = 416
    Top = 128
    Width = 210
    Height = 24
    ReadOnly = True
  end
  object ButtonOk: TButton
    Left = 416
    Top = 200
    Width = 100
    Height = 25
    Caption = 'OK'
    ModalResult = 1
  end
  object ButtonCancel: TButton
    Left = 526
    Top = 200
    Width = 100
    Height = 25
    Caption = 'Отмена'
    ModalResult = 2
  end
end
