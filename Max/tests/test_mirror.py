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

def require_comment_count(path, minimum):
    text=(root/path).read_text(encoding='utf-8')
    count=sum(1 for line in text.splitlines() if line.strip().startswith('//'))
    if count < minimum:
        errors.append(f'{path}: only {count} // comment lines, expected at least {minimum}')

def require_hash_comment_count(path, minimum):
    text=(root/path).read_text(encoding='utf-8')
    # Do not count the shebang as documentation.
    count=sum(1 for line in text.splitlines()
              if line.strip().startswith('#') and not line.strip().startswith('#!'))
    if count < minimum:
        errors.append(f'{path}: only {count} # comment lines, expected at least {minimum}')

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

need('maxtask.h',
    'struct MB_TASK', 'class MB_TASK_LIST',
    '//Типы заданий', '//Задание для потока', '//Список заданий для потока',
    '//Посылка сообщения', '//Чтение сообщений', '//Информация о себе')
need('maxtask.cpp',
    '//Конструктор', '//Деструктор', '//Извлечение одного задания из списка',
    '//Есть задания - выбрать нулевое', '//Очистить все задания',
    '//Посылка файла картинки', '//Посылка файла документа',
    'MB_TASK_LIST::AddSendDoc')

need('maxmsg.h',
    'class MaxMessage', 'class MaxUser', 'class MaxUser_LIST', 'class MaxBotInfo',
    '//От какого пользователя пришло сообщение', '//Объект Chat', '//Сообщение MAX',
    '//Распознаваемые виды сообщений', '//Список сообщений',
    '//Это класс пользователей, используемых в программе',
    '//Список пользователей, используемых в программе',
    '//Проверка, что пользователь соответствует маске псевдонима')
need('maxmsg.cpp',
    'MaxUser::HasValidAlias', 'MaxUser_LIST::GetUsersByAlias', 'MaxMessage_LIST::CopyFrom',
    '//Список сообщений', '//Конструктор', '//Деструктор',
    '//Доступ по индексу к сообщениям', '//Удалить сообщение по индексу',
    '//Поиск сообщения', '//Добавление сообщения', '//Копирование',
    '//Сохранить', '//Загрузить', '//Список пользователей',
    '//Поиск пользователя по идентификатору Id',
    '//Получить свободный псевдоним автоматически')

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

need('UFMaxBot.cpp',
    '//Рабочий каталог проекта', '//Разрешить работу MAX бота',
    '//Идентификатор разработчика бота', '//Данные MAX бота',
    '//Отсылка алармов', '//Маска отсылки алармов',
    '//Маска пользователей, которым разрешено делать запросы',
    '//Список пользователей, используемых в программе',
    '//0 - номер ПП', '//1 Пользователь/Чат', '//2 Идентификатор',
    '//3 Псевдоним', '//4 Принято', '//5 Послано',
    '//Показать сообщения', '//Прочитаны сообщения', '//Получен ответ GetMe',
    '//Ошибки', '//Изменить пользователя', '//Удалить пользователя',
    '//Записывать отладочные сообщения бота в lanmon.log',
    '//Получить состояние потока')
need('UFMaxBot.h',
    '//Форма настройки и диагностики MAX бота',
    '//Показать сообщения', '//Прочитаны сообщения', '//Получен ответ GetMe', '//Ошибки')
need('UFMaxBotApi.cpp',
    '//Данные MAX бота', '//Проверить BotApi запросом GetMe', '//Получен ответ GetMe')
need('UFMaxBotApi.h',
    '//Форма ввода и проверки MAX BotApi/token', '//Данные MAX бота', '//Получен ответ GetMe')
need('UFMaxUserEdit.cpp',
    '//Заполнить форму данными пользователя', '//Сохранить изменения пользователя',
    '//MAX различает user_id и chat_id')
need('UFMaxUserEdit.h', '//Форма редактирования пользователя/чата MAX')
need('UFMaxMsg.cpp', '//Конструктор универсальной формы ввода текста')
need('UFMaxMsg.h', '//Универсальная форма ввода текста для операций MAX')

# MAX-specific internal API has no Telegram source counterpart, but it must remain
# documented because it contains the differences a LanMon developer must understand.
need('api/maxcore.h',
    '//Тип адресата MAX: личный пользователь или чат',
    '//Ответ GET /updates', '//Преобразование CP1251 LanMon -> UTF-8 MAX')
need('api/maxcore.cpp',
    '//Минимальный JSON parser для C++98.',
    '//Построить URL Long Polling GET /updates',
    '//Получение сообщений из JSON ответа GET /updates')
need('api/maxclient.h',
    '//Клиент MAX Bot API, независимый от конкретной HTTP-библиотеки',
    '//Long Polling marker', '//Общий MAX upload flow')
need('api/maxclient.cpp',
    '//Обязательные HTTP-заголовки MAX Bot API',
    '//Получить обновления через Long Polling',
    '//Общий MAX upload flow для изображения и документа')
need('api/maxindy.h',
    '//Production HTTP/HTTPS transport MAX для старого C++Builder/Indy',
    '//Доступ к SSL-настройкам для интеграции/диагностики LanMon')
need('api/maxindy.cpp',
    '//MAX с 19.07.2026 требует добавить сертификат Минцифры в доверенные.',
    '//HTTP GET', '//HTTP POST', '//Multipart upload файла')

# Tests and E2E are part of the repository code too: each file explains its role/scenario.
need('tests/test_maxcore.cpp',
    '// Unit tests protocol-level MAX functions without network or VCL dependencies.',
    '// URL GET /updates: defaults, marker and API range clamping.',
    '// Legacy LanMon text is CP1251; MAX JSON is UTF-8.',
    '// Malformed JSON must fail with a diagnostic string.')
need('tests/test_maxclient.cpp',
    '// Unit tests MAX_API_CLIENT behavior through an in-memory HTTP transport.',
    '// Deterministic transport: returns queued responses and records every request.',
    '// First Long Poll stores marker returned by MAX.',
    '// Non-2xx response must propagate as a MAX HTTP diagnostic.')
need('e2e/e2e_harness.cpp',
    '// End-to-end test of MAX_API_CLIENT over a real local TCP/HTTP connection.',
    '// 1. Real HTTP GET /me through the POSIX transport.',
    '// 4. Second Long Poll must send marker=101; server advances it to 102.')
need('e2e/posix_http_transport.h',
    '// Minimal plain-HTTP transport used only by Linux E2E tests.',
    '// Production LanMon uses TMaxIndyTransport over HTTPS/OpenSSL.')
need('e2e/posix_http_transport.cpp',
    '// Socket-level HTTP transport used only to test MAX_API_CLIENT end-to-end on Linux CI.',
    '// Execute one HTTP/1.1 request over a real TCP socket and return MAX_HTTP_RESPONSE.',
    '// Normalize chunked body so MAX_API_CLIENT sees the same body shape as with Indy.')
need('e2e/mock_max_server.py',
    '"""Local HTTP model of the MAX endpoints exercised by the C++98 E2E harness."""',
    '# Cross-request state is used to prove Long Poll marker continuity and capture sends.',
    '# GET /updates models two consecutive Long Poll reads.',
    '# Second read proves that MAX_API_CLIENT persisted marker=101.')
need('tests/run.sh',
    '# Проверка protocol core: JSON, URL, marker, CP1251/UTF-8 и DTO MAX.',
    '# Структурная проверка зеркала Telegram и обязательных комментариев.')
need('e2e/run_e2e.sh',
    '# Build the portable MAX client against the real socket-level test transport.',
    '# Start deterministic local MAX HTTP model and always stop it on exit/failure.',
    '# Exercise GET /me, Long Poll marker continuity, send message and HTTP error handling.')

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

# Minimum documentation density protects against a future refactor stripping comments again.
for path, minimum in {
    'maxbot.h': 35,
    'maxbot.cpp': 70,
    'maxmsg.h': 35,
    'maxmsg.cpp': 45,
    'maxtask.h': 15,
    'maxtask.cpp': 20,
    'UFMaxBot.cpp': 35,
    'UFMaxBot.h': 8,
    'UFMaxBotApi.cpp': 8,
    'UFMaxBotApi.h': 5,
    'UFMaxUserEdit.cpp': 8,
    'UFMaxUserEdit.h': 4,
    'UFMaxMsg.cpp': 3,
    'UFMaxMsg.h': 3,
    'api/maxcore.h': 15,
    'api/maxcore.cpp': 25,
    'api/maxclient.h': 15,
    'api/maxclient.cpp': 15,
    'api/maxindy.h': 10,
    'api/maxindy.cpp': 15,
    'tests/test_maxcore.cpp': 8,
    'tests/test_maxclient.cpp': 9,
    'e2e/e2e_harness.cpp': 7,
    'e2e/posix_http_transport.h': 5,
    'e2e/posix_http_transport.cpp': 12,
}.items():
    require_comment_count(path, minimum)

for path, minimum in {
    'tests/run.sh': 4,
    'e2e/run_e2e.sh': 5,
    'e2e/mock_max_server.py': 12,
}.items():
    require_hash_comment_count(path, minimum)

if errors:
    print('MIRROR CHECK FAILED')
    for e in errors: print(' -',e)
    sys.exit(1)
print('Telegram-style MAX mirror source and comments check passed')
