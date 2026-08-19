# Сертификат MAX / Минцифры

`max-ca.pem` — доверенная цепочка, которую `TMaxIndyTransport` использует через `TIdSSLIOHandlerSocketOpenSSL::SSLOptions->RootCertFile`.

В bundle находятся два сертификата:

| Сертификат | SHA-256 DER fingerprint | Срок действия |
|---|---|---|
| Russian Trusted Sub CA | `BBBDE2103E790B999EC62BD03CF625A5A2E7C316E10AFE6A490EEDEAD8B3FD9B` | до 2027-03-06 |
| Russian Trusted Root CA | `D26D2D0231B7C39F92CC738512BA54103519E4405D68B5BD703E9788CA8ECF31` | до 2032-02-27 |

Официальные исходные файлы публикуются инфраструктурой Госуслуг:

```text
https://gu-st.ru/content/Other/doc/russian_trusted_sub_ca.cer
https://gu-st.ru/content/Other/doc/russian_trusted_root_ca.cer
```

Также официальный combined PEM historically публикуется как:

```text
https://gu-st.ru/content/Other/doc/russiantrustedca.pem
```

## Проверка

В репозитории есть автоматическая проверка:

```bash
bash Max/tests/test_cert.sh
```

Она не доверяет файлу только потому, что он называется `max-ca.pem`. Проверяются:

- ровно два PEM-сертификата;
- SHA-256 каждого DER-сертификата;
- CN `Russian Trusted Sub CA` и `Russian Trusted Root CA`;
- срок действия;
- криптографическая цепочка Sub CA → Root CA;
- пригодность combined bundle как OpenSSL `-CAfile`.

GitHub Actions дополнительно выполняет настоящий TLS 1.2 handshake к `platform-api2.max.ru` с:

```bash
openssl s_client \
  -connect platform-api2.max.ru:443 \
  -servername platform-api2.max.ru \
  -verify_hostname platform-api2.max.ru \
  -verify_return_error \
  -tls1_2 \
  -CAfile Max/certs/max-ca.pem
```

## Установка вместе с LanMon

Production transport ищет файл относительно **каталога `lanmon4.exe`**, а не относительно исходников:

```text
LanMon4/
  lanmon4.exe
  certs/
    max-ca.pem
```

При сборке/установке скопировать:

```text
Max/certs/max-ca.pem
```

в:

```text
<каталог lanmon4.exe>\certs\max-ca.pem
```

Если файла нет, `TMaxIndyTransport` теперь работает fail-closed: сетевой запрос не выполняется, а LanMon получает ошибку `MAX CA bundle not found: ...`.

## Обновление сертификата

Не заменять bundle молча. При официальной ротации:

1. скачать сертификаты из официального источника;
2. проверить subject/issuer и сроки;
3. вычислить DER SHA-256;
4. заменить `max-ca.pem`;
5. обновить fingerprints в `Max/tests/test_cert.sh` и в этом README;
6. прогнать unit + live TLS CI.

Особенно важно обновить Sub CA до истечения текущего сертификата в марте 2027 года, если Минцифры выпустит новую цепочку.
