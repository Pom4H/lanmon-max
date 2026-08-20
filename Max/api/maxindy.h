#ifndef maxindyH
#define maxindyH

//C++Builder 2007 поставляется с Indy, чьи generated headers называются In*.
//Не использовать здесь IdHTTP.hpp/IdSSLOpenSSL.hpp из более новых Builder.
#include <InHTTP.hpp>
#include <InSSLOpenSSL.hpp>
#include <InMultipartFormData.hpp>
#include "maxclient.h"

//Production HTTP/HTTPS transport MAX для старого C++Builder/Indy
class TMaxIndyTransport : public IMaxHttpTransport
{
    //HTTP-клиент Indy из фактической поставки BCB2007
    TInHTTP * Http;
    //OpenSSL IOHandler той же поставки
    TInSSLIOHandlerSocketOpenSSL * Ssl;
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
    TInSSLIOHandlerSocketOpenSSL * SSL(){return Ssl;}
private:
    //Если обязательный CA bundle не настроен, вернуть ошибку без сетевого запроса
    bool StartupFailed(MAX_HTTP_RESPONSE & response) const;
    //Перенести VCL-список заголовков MAX в TInHTTP::Request
    void ApplyHeaders(const MAX_HTTP_HEADERS & headers);
    //Собрать единый MAX_HTTP_RESPONSE из Indy response/exception
    MAX_HTTP_RESPONSE ReadResponse(TMemoryStream * stream,const AnsiString & exceptionText);
};

//LanMon использует CP1251 AnsiString. Преобразуем его в UTF-8 перед сериализацией JSON MAX.
MAX_TEXT MaxUtf8FromAnsi1251(const AnsiString & text);

#endif
