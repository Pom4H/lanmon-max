object FormMaxBot: TFormMaxBot
  Left = 0
  Top = 0
  Caption = 'MAX '#1073#1086#1090
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
        Caption = #1048#1079#1084#1077#1085#1080#1090#1100
        OnClick = ButtonEditBotApiClick
      end
      object CheckBoxShowBotApi: TCheckBox
        Left = 16
        Top = 56
        Width = 160
        Height = 17
        Caption = #1055#1086#1082#1072#1079#1072#1090#1100' '#1090#1086#1082#1077#1085
        OnClick = CheckBoxShowBotApiClick
      end
      object CheckBoxActive: TCheckBox
        Left = 16
        Top = 96
        Width = 180
        Height = 17
        Caption = #1056#1072#1079#1088#1077#1096#1080#1090#1100' '#1088#1072#1073#1086#1090#1091' MAX'
        OnClick = CheckBoxActiveClick
      end
      object EditId: TEdit Left = 16 Top = 136 Width = 240 Height = 24 ReadOnly = True end
      object EditFirstName: TEdit Left = 16 Top = 168 Width = 240 Height = 24 ReadOnly = True end
      object EditUsername: TEdit Left = 16 Top = 200 Width = 240 Height = 24 ReadOnly = True end
      object CheckBoxPeriodReadMessages: TCheckBox
        Left = 16
        Top = 248
        Width = 230
        Height = 17
        Caption = #1055#1077#1088#1080#1086#1076#1080#1095#1077#1089#1082#1080' '#1095#1080#1090#1072#1090#1100' '#1089#1086#1086#1073#1097#1077#1085#1080#1103
        OnClick = CheckBoxPeriodReadMessagesClick
      end
      object EditPeriodReadMessages: TEdit
        Left = 256 Top = 244 Width = 80 Height = 24
        Text = '10'
        OnChange = EditPeriodReadMessagesChange
      end
    end
    object TabSheetUsers: TTabSheet
      Caption = #1055#1086#1083#1100#1079#1086#1074#1072#1090#1077#1083#1080
      object ListViewUsers: TListView
        Left = 0 Top = 0 Width = 876 Height = 420 Align = alClient
        Columns = <
          item Caption = '#' Width = 40 end
          item Caption = #1048#1084#1103 Width = 180 end
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
        Left = 16 Top = 432 Width = 120 Height = 25
        Caption = #1048#1079#1084#1077#1085#1080#1090#1100
        OnClick = ButtonEditUserClick
      end
      object ButtonDeleteUser: TButton
        Left = 144 Top = 432 Width = 120 Height = 25
        Caption = #1059#1076#1072#1083#1080#1090#1100
        OnClick = ButtonDeleteUserClick
      end
    end
    object TabSheetSend: TTabSheet
      Caption = #1053#1072#1089#1090#1088#1086#1081#1082#1080
      object CheckBoxFlagSendAlarms: TCheckBox
        Left = 16 Top = 24 Width = 240 Height = 17
        Caption = #1054#1090#1089#1099#1083#1082#1072' '#1072#1083#1072#1088#1084#1086#1074
        OnClick = CheckBoxFlagSendAlarmsClick
      end
      object CheckBoxFlagSendAlarmsEnd: TCheckBox
        Left = 16 Top = 48 Width = 280 Height = 17
        Caption = #1057#1086#1086#1073#1097#1072#1090#1100' '#1086' '#1079#1072#1074#1077#1088#1096#1077#1085#1080#1080' '#1072#1074#1072#1088#1080#1080
        OnClick = CheckBoxFlagSendAlarmsEndClick
      end
      object CheckBoxFlagOperatorAlarm: TCheckBox
        Left = 16 Top = 72 Width = 300 Height = 17
        Caption = #1057#1086#1086#1073#1097#1072#1090#1100' '#1086' '#1087#1086#1076#1090#1074#1077#1088#1078#1076#1077#1085#1080#1080' '#1072#1083#1072#1088#1084#1072
        OnClick = CheckBoxFlagOperatorAlarmClick
      end
      object EditAlarmAlias: TEdit Left = 16 Top = 112 Width = 220 Height = 24 OnChange = EditAlarmAliasChange end
      object EditRequestAlias: TEdit Left = 16 Top = 152 Width = 220 Height = 24 OnChange = EditRequestAliasChange end
      object CheckBoxFlagSendMaps: TCheckBox
        Left = 16 Top = 200 Width = 300 Height = 17
        Caption = #1056#1072#1079#1088#1077#1096#1080#1090#1100' '#1074#1089#1090#1088#1086#1077#1085#1085#1099#1077' '#1079#1072#1087#1088#1086#1089#1099
        OnClick = CheckBoxFlagSendMapsClick
      end
      object CheckBoxUseLanmonLog: TCheckBox
        Left = 16 Top = 224 Width = 300 Height = 17
        Caption = #1047#1072#1087#1080#1089#1099#1074#1072#1090#1100' '#1089#1086#1086#1073#1097#1077#1085#1080#1103' '#1074' lanmon.log'
        OnClick = CheckBoxUseLanmonLogClick
      end
      object ButtonReadMessages: TButton
        Left = 16 Top = 272 Width = 180 Height = 25
        Caption = #1055#1088#1086#1095#1080#1090#1072#1090#1100' '#1089#1086#1086#1073#1097#1077#1085#1080#1103
        OnClick = ButtonReadMessagesClick
      end
      object ListViewMsg: TListView
        Left = 328 Top = 16 Width = 520 Height = 430
        Columns = <
          item Caption = '#' Width = 40 end
          item Caption = #1042#1088#1077#1084#1103 Width = 120 end
          item Caption = #1058#1077#1082#1089#1090 Width = 180 end
          item Caption = #1054#1090' '#1082#1086#1075#1086 Width = 100 end
          item Caption = 'Id' Width = 100 end>
        ReadOnly = True
        RowSelect = True
        ViewStyle = vsReport
      end
    end
    object TabSheetLog: TTabSheet
      Caption = 'Log'
      object ListBoxLog: TListBox Left = 0 Top = 0 Width = 876 Height = 470 Align = alClient end
    end
    object TabSheetJson: TTabSheet
      Caption = 'JSON'
      OnShow = TabSheetJsonShow
      object MemoJson: TMemo Left = 0 Top = 0 Width = 876 Height = 470 Align = alClient ScrollBars = ssBoth end
    end
  end
  object StatusBar1: TStatusBar
    Left = 0 Top = 530 Width = 900 Height = 30
    Panels = <item Width = 220 end item Width = 220 end item Width = 220 end>
  end
  object Timer1: TTimer Interval = 1000 OnTimer = Timer1Timer Left = 840 Top = 8 end
end
