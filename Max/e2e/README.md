# MAX E2E harness

Этот тест прогоняет настоящий `MAX_API_CLIENT` через локальное TCP/HTTP-соединение.
`IMaxHttpTransport` внутри процесса не мокается: C++98-клиент реально открывает socket, пишет HTTP и читает ответ Python-сервера.

## Что проверяется

1. `GET /me` с токеном только в заголовке `Authorization`.
2. Первый `GET /updates` без `marker`.
3. Разбор `message_created`, включая UTF-8, кавычки и перевод строки.
4. Сохранение `marker=101`.
5. `POST /messages?chat_id=777` и точное сохранение русского текста/JSON escaping.
6. Второй `GET /updates` уже с `marker=101`.
7. Переход на новый `marker=102`.
8. Полный цикл изображения:
   - `POST /uploads?type=image`;
   - использование возвращённого URL без переписывания;
   - реальный `multipart/form-data` с полем `data`;
   - отсутствие bot token на upload-host;
   - получение `token`;
   - `POST /messages?chat_id=-777` с `attachment.type=image`.
9. Полный цикл документа через `type=file` и `attachment.type=file`.
10. Раздельная адресация `chat_id` и `user_id`.
11. Отрицательный `chat_id`.
12. HTTP 401 и передача тела ошибки вызывающему коду.

Mock server специально возвращает ошибку, если multipart-запрос на upload-host содержит `Authorization`. Это защищает от случайной утечки bot token на хост загрузки.

## Запуск

```bash
./run_e2e.sh
```

Другой порт:

```bash
MAX_E2E_PORT=18081 ./run_e2e.sh
```

Скрипт собирает harness с:

```text
g++ -std=gnu++98 -Wall -Wextra -Werror
```

после чего запускает Python mock server, выполняет C++ E2E и завершает сервер.

## Подмена API host только для теста

`MAX_API_CLIENT` принимает необязательный третий аргумент `baseUrl`:

```cpp
MAX_API_CLIENT api(transport, token); // production: platform-api2.max.ru
MAX_API_CLIENT testApi(transport, token, "http://127.0.0.1:18080");
```

Production call sites менять не нужно. URL, возвращённый `POST /uploads`, через `baseUrl` не переписывается — это отдельное свойство MAX upload flow, которое E2E тоже проверяет.
