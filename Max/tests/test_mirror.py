#!/usr/bin/env python3
from pathlib import Path
import sys

root=Path(__file__).resolve().parents[1]
errors=[]

def need(path, *tokens):
    text=(root/path).read_text(encoding='utf-8')
    for token in tokens:
        if token not in text:
            errors.append(f'{path}: missing {token!r}')
    return text

h=need('maxbot.h',
    'class TMaxBotThread', 'class MAX_BOT', 'MB_TASK_LIST TaskList',
    'void SendMessage(AnsiString id,AnsiString msg);',
    'void OnMessages(MaxMessage_LIST & msglist);',
    '//Состояние потока работы с MAX', '//Поток для работы с MAX',
    '//Класс для работы с MAX Bot', '//Добавление заданий потоку')
cpp=need('maxbot.cpp',
    'while(TaskList.Get(Task))', 'OnTgMessage(',
    'SCREEN', 'ЭКРАН', 'MAP', 'КАРТА', 'STOP', 'СТОП',
    'LOGXLS', 'ЖУРНАЛ', 'ALARM', 'ТРЕВОГ', 'HELP',
    'SendMessageByAlias', 'SendPhotoByAlias', 'SendDocByAlias',
    'TFastIniFile ini', 'UserList->Load(&ini)', 'UserList->Save(&ini)')
need('maxtask.h', 'struct MB_TASK', 'class MB_TASK_LIST')
need('maxtask.cpp', '//Извлечение одного задания из списка', 'MB_TASK_LIST::AddSendDoc')
need('maxmsg.h', 'class MaxMessage', 'class MaxUser', 'class MaxUser_LIST', 'class MaxBotInfo')
need('maxmsg.cpp', 'MaxUser::HasValidAlias', 'MaxUser_LIST::GetUsersByAlias', 'MaxMessage_LIST::CopyFrom')

for forbidden in ('lanmon_bot.cpp','lanmon_bot.h','lanmon_commands.cpp','lanmon_commands.h','maxsettings.cpp','maxsettings.h','maxusers.cpp','maxusers.h','maxuser.h'):
    if (root/forbidden).exists():
        errors.append(f'forbidden alternative architecture file remains: {forbidden}')

for required in (
    'UFMaxBot.cpp','UFMaxBot.h','UFMaxBot.dfm',
    'UFMaxBotApi.cpp','UFMaxBotApi.h','UFMaxBotApi.dfm',
    'UFMaxUserEdit.cpp','UFMaxUserEdit.h','UFMaxUserEdit.dfm',
    'UFMaxMsg.cpp','UFMaxMsg.h','UFMaxMsg.dfm'):
    if not (root/required).exists():
        errors.append(f'missing mirrored VCL file: {required}')

for ui_header in ('UFMaxBot.h','UFMaxBotApi.h','UFMaxUserEdit.h'):
    if not (root/ui_header).exists():
        continue
    text=(root/ui_header).read_text(encoding='utf-8')
    if '#include "maxbot.h"' not in text:
        errors.append(f'{ui_header}: must include maxbot.h')
    if 'tgbot.h' in text:
        errors.append(f'{ui_header}: Telegram include remains')

required_comments=[
    '//Состояние потока работы с MAX', '//Поток для работы с MAX',
    '//Прочитанные сообщения с сервера', '//Выполнение заданий',
    '//Чтение сообщений', '//Передача сообщения', '//Информация о себе',
    '//Посылка файла картинки', '//Посылка файла документа',
    '//Класс для работы с MAX Bot', '//Загрузка бота из файла',
    '//Сохранение бота в файл', '//Список пользователей, используемых в программе',
    '//Добавление заданий потоку', '//Установка обработчиков событий',
    '//Возникла новая авария LanMon', '//Получены новые события MAX',
    '//Передача сообщения по alias (из LanMon)', '//статистика'
]
for comment in required_comments:
    if comment not in h:
        errors.append(f'maxbot.h: missing mirrored comment {comment!r}')

if errors:
    print('MIRROR CHECK FAILED')
    for e in errors: print(' -',e)
    sys.exit(1)
print('Telegram-style MAX mirror source check passed')
