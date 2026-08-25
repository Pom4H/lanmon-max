object FormMaxMsg: TFormMaxMsg
  Left = 0
  Top = 0
  Caption = 'MAX'
  ClientHeight = 150
  ClientWidth = 520
  Position = poOwnerFormCenter
  object LabelHdr: TLabel
    Left = 16
    Top = 16
    Width = 480
    Height = 20
    Caption = #1057#1086#1086#1073#1097#1077#1085#1080#1077
  end
  object EditText: TEdit Left = 16 Top = 48 Width = 488 Height = 24 end
  object ButtonOk: TButton Left = 294 Top = 104 Width = 100 Height = 25 Caption = 'OK' ModalResult = 1 end
  object ButtonCancel: TButton
    Left = 404 Top = 104 Width = 100 Height = 25
    Caption = #1054#1090#1084#1077#1085#1072
    ModalResult = 2
  end
end
