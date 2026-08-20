#ifndef maxindyH
#define maxindyH

//Фактическое окружение LanMon: C++Builder 2007 с обновлённым Indy 10.6.2.
//Присланные generated headers имеют современные Id* имена и TLS 1.2 enum.
#include <IdHTTP.hpp>
#include <IdSSLOpenSSL.hpp>
#include <IdMultipartFormData.hpp>
#include "maxclient.h"

//Production HTTP/HTTPS transport MAX для C++Builder 2007 + Indy 10.6.2
class TMaxIndyTransport : public IMaxHttpTransport
{
    //HTTP-клиент обновлённого Indy
    TIdHTTP * Http;
    //OpenSSL IOHandler обновлённого Indy
    TIdSSLIOHandlerSocketOpenSSL * Ssl;
    //Ошибка начальной TLS-конфигурации. При ней transport работает fail-closed.
    AnsiString StartupError;
public:
    TMaxIndyTransport();
    virtual ~TMaxIndyTransport();

    //HTTP GET
    virtual MAX_HTTP_RESPONSE Get(const MAX_TEXT & url,const MAX_HTTP_HEADERS & headers);
    //HTTP POST
    virtual MAX_HTTP_RESPONSE Post(const MAX_TEXT & url,const MAX_HTTP_HEADERS & headers,const MAX_TEXT & body);
    //Multipart upload файла в URL, полученный от MAX /uploads
    virtual MAX_HTTP_RESPONSE PostMultipartFile(const MAX_TEXT & url,const MAX_HTTP_HEADERS & headers,
                                   const MAX_TEXT & fieldName,const MAX_TEXT & filename);
    //Реальная пауза для retry/backoff attachment.not.ready
    virtual void SleepMilliseconds(unsigned int milliseconds);

    //Доступ к SSL-настройкам для интеграции/диагностики LanMon
    TIdSSLIOHandlerSocketOpenSSL * SSL(){return Ssl;}
private:
    //Если обязательный CA bundle/OpenSSL TLS 1.2 не настроены, вернуть ошибку без запроса
    bool StartupFailed(MAX_HTTP_RESPONSE & response) const;
    //Перенести VCL-список заголовков MAX в TIdHTTP::Request
    void ApplyHeaders(const MAX_HTTP_HEADERS & headers);
    //Собрать единый MAX_HTTP_RESPONSE из Indy response/exception
    MAX_HTTP_RESPONSE ReadResponse(TMemoryStream * stream,const AnsiString & exceptionText);
};

//LanMon использует CP1251 AnsiString. Преобразуем его в UTF-8 перед сериализацией JSON MAX.
MAX_TEXT MaxUtf8FromAnsi1251(const AnsiString & text);

#endif
