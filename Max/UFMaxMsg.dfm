object FormMaxMsg: TFormMaxMsg
  Left = 0
  Top = 0
  Caption = 'MAX'
  ClientHeight = 150
  ClientWidth = 520
  Position = poOwnerFormCenter
  object LabelHdr: TLabel Left = 16 Top = 16 Width = 480 Height = 20 Caption = 'Сообщение' end
  object EditText: TEdit Left = 16 Top = 48 Width = 488 Height = 24 end
  object ButtonOk: TButton Left = 294 Top = 104 Width = 100 Height = 25 Caption = 'OK' ModalResult = 1 end
  object ButtonCancel: TButton Left = 404 Top = 104 Width = 100 Height = 25 Caption = 'Отмена' ModalResult = 2 end
end
