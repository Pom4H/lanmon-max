object FormMaxBot: TFormMaxBot
  Left = 0
  Top = 0
  Caption = 'MAX бот'
  ClientHeight = 560
  ClientWidth = 900
  OnClose = FormClose
  OnShow = FormShow
  object PageControl1: TPageControl
    Left = 0
    Top = 0
    Width = 900
    Height = 530
    Align = alClient
    ActivePage = TabSheetBotApi
    object TabSheetBotApi: TTabSheet
      Caption = 'Bot API'
      object EditBotApi: TEdit
        Left = 16
        Top = 24
        Width = 620
        Height = 24
        PasswordChar = '*'
        ReadOnly = True
      end
      object ButtonEditBotApi: TButton
        Left = 648
        Top = 24
        Width = 120
        Height = 25
        Caption = 'Изменить'
        OnClick = ButtonEditBotApiClick
      end
      object CheckBoxShowBotApi: TCheckBox
        Left = 16
        Top = 56
        Width = 160
        Height = 17
        Caption = 'Показать токен'
        OnClick = CheckBoxShowBotApiClick
      end
      object CheckBoxActive: TCheckBox
        Left = 16
        Top = 96
        Width = 180
        Height = 17
        Caption = 'Разрешить работу MAX'
        OnClick = CheckBoxActiveClick
      end
      object EditId: TEdit
        Left = 16
        Top = 136
        Width = 240
        Height = 24
        ReadOnly = True
      end
      object EditFirstName: TEdit
        Left = 16
        Top = 168
        Width = 240
        Height = 24
        ReadOnly = True
      end
      object EditUsername: TEdit
        Left = 16
        Top = 200
        Width = 240
        Height = 24
        ReadOnly = True
      end
      object CheckBoxPeriodReadMessages: TCheckBox
        Left = 16
        Top = 248
        Width = 230
        Height = 17
        Caption = 'Периодически читать сообщения'
        OnClick = CheckBoxPeriodReadMessagesClick
      end
      object EditPeriodReadMessages: TEdit
        Left = 256
        Top = 244
        Width = 80
        Height = 24
        Text = '10'
        OnChange = EditPeriodReadMessagesChange
      end
    end
    object TabSheetUsers: TTabSheet
      Caption = 'Пользователи'
      object ListViewUsers: TListView
        Left = 0
        Top = 0
        Width = 876
        Height = 420
        Align = alClient
        Columns = <
          item Caption = '#' Width = 40 end
          item Caption = 'Имя' Width = 180 end
          item Caption = 'Id' Width = 160 end
          item Caption = 'Alias' Width = 100 end
          item Caption = 'In' Width = 60 end
          item Caption = 'Out' Width = 60 end
          item Caption = 'Peer' Width = 70 end>
        ReadOnly = True
        RowSelect = True
        ViewStyle = vsReport
      end
      object ButtonEditUser: TButton
        Left = 16
        Top = 432
        Width = 120
        Height = 25
        Caption = 'Изменить'
        OnClick = ButtonEditUserClick
      end
      object ButtonDeleteUser: TButton
        Left = 144
        Top = 432
        Width = 120
        Height = 25
        Caption = 'Удалить'
        OnClick = ButtonDeleteUserClick
      end
    end
    object TabSheetSend: TTabSheet
      Caption = 'Настройки'
      object CheckBoxFlagSendAlarms: TCheckBox
        Left = 16
        Top = 24
        Width = 240
        Height = 17
        Caption = 'Отсылка алармов'
        OnClick = CheckBoxFlagSendAlarmsClick
      end
      object CheckBoxFlagSendAlarmsEnd: TCheckBox
        Left = 16
        Top = 48
        Width = 280
        Height = 17
        Caption = 'Сообщать о завершении аварии'
        OnClick = CheckBoxFlagSendAlarmsEndClick
      end
      object CheckBoxFlagOperatorAlarm: TCheckBox
        Left = 16
        Top = 72
        Width = 300
        Height = 17
        Caption = 'Сообщать о подтверждении аларма'
        OnClick = CheckBoxFlagOperatorAlarmClick
      end
      object EditAlarmAlias: TEdit
        Left = 16
        Top = 112
        Width = 220
        Height = 24
        OnChange = EditAlarmAliasChange
      end
      object EditRequestAlias: TEdit
        Left = 16
        Top = 152
        Width = 220
        Height = 24
        OnChange = EditRequestAliasChange
      end
      object CheckBoxFlagSendMaps: TCheckBox
        Left = 16
        Top = 200
        Width = 300
        Height = 17
        Caption = 'Разрешить встроенные запросы'
        OnClick = CheckBoxFlagSendMapsClick
      end
      object CheckBoxUseLanmonLog: TCheckBox
        Left = 16
        Top = 224
        Width = 300
        Height = 17
        Caption = 'Записывать сообщения в lanmon.log'
        OnClick = CheckBoxUseLanmonLogClick
      end
      object ButtonReadMessages: TButton
        Left = 16
        Top = 272
        Width = 180
        Height = 25
        Caption = 'Прочитать сообщения'
        OnClick = ButtonReadMessagesClick
      end
      object ListViewMsg: TListView
        Left = 328
        Top = 16
        Width = 520
        Height = 430
        Columns = <
          item Caption = '#' Width = 40 end
          item Caption = 'Время' Width = 120 end
          item Caption = 'Текст' Width = 180 end
          item Caption = 'От кого' Width = 100 end
          item Caption = 'Id' Width = 100 end>
        ReadOnly = True
        RowSelect = True
        ViewStyle = vsReport
      end
    end
    object TabSheetLog: TTabSheet
      Caption = 'Log'
      object ListBoxLog: TListBox
        Left = 0
        Top = 0
        Width = 876
        Height = 470
        Align = alClient
      end
    end
    object TabSheetJson: TTabSheet
      Caption = 'JSON'
      OnShow = TabSheetJsonShow
      object MemoJson: TMemo
        Left = 0
        Top = 0
        Width = 876
        Height = 470
        Align = alClient
        ScrollBars = ssBoth
      end
    end
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 530
    Width = 900
    Height = 30
    Panels = <
      item Width = 220 end
      item Width = 220 end
      item Width = 220 end>
  end
  object Timer1: TTimer
    Interval = 1000
    OnTimer = Timer1Timer
    Left = 840
    Top = 8
  end
end
