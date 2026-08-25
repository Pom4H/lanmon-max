#ifndef maxindyH
#define maxindyH

//---------------------------------------------------------------------------
//C++Builder 2007 в проекте LanMon использует Indy с generated headers Id*.
//MAX transport должен использовать те же классы, что и существующий tgbot.cpp.
//---------------------------------------------------------------------------
#include <IdHTTP.hpp>
#include <IdSSLOpenSSL.hpp>
#include <IdMultipartFormData.hpp>
#include "maxclient.h"

//Production HTTP/HTTPS transport MAX для C++Builder 2007/Indy
class TMaxIndyTransport : public IMaxHttpTransport
{
    //HTTP-клиент Indy
    TIdHTTP * Http;
    //OpenSSL IOHandler
    TIdSSLIOHandlerSocketOpenSSL * Ssl;
    //Ошибка начальной TLS-конфигурации. При ней transport работает fail-closed.
    AnsiString StartupError;
public:
    TMaxIndyTransport();
    virtual ~TMaxIndyTransport();

    virtual MAX_HTTP_RESPONSE Get(const MAX_TEXT & url,const MAX_HTTP_HEADERS & headers);
    virtual MAX_HTTP_RESPONSE Post(const MAX_TEXT & url,const MAX_HTTP_HEADERS & headers,const MAX_TEXT & body);
    virtual MAX_HTTP_RESPONSE PostMultipartFile(const MAX_TEXT & url,const MAX_HTTP_HEADERS & headers,
                                   const MAX_TEXT & fieldName,const MAX_TEXT & filename);
    virtual void SleepMilliseconds(unsigned int milliseconds);

    TIdSSLIOHandlerSocketOpenSSL * SSL(){return Ssl;}
private:
    bool StartupFailed(MAX_HTTP_RESPONSE & response) const;
    void ApplyHeaders(const MAX_HTTP_HEADERS & headers);
    MAX_HTTP_RESPONSE ReadResponse(TMemoryStream * stream,const AnsiString & exceptionText);
};

MAX_TEXT MaxUtf8FromAnsi1251(const AnsiString & text);

#endif
