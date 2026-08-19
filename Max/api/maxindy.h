#ifndef maxindyH
#define maxindyH

#include <IdHTTP.hpp>
#include <IdSSLOpenSSL.hpp>
#include <IdMultipartFormData.hpp>
#include "maxclient.h"

//Production HTTP/HTTPS transport MAX для старого C++Builder/Indy
class TMaxIndyTransport : public IMaxHttpTransport
{
    //HTTP-клиент Indy
    TIdHTTP * Http;
    //OpenSSL IOHandler; используется также для RootCertFile MAX
    TIdSSLIOHandlerSocketOpenSSL * Ssl;
public:
    TMaxIndyTransport();
    virtual ~TMaxIndyTransport();

    //HTTP GET
    virtual MAX_HTTP_RESPONSE Get(const std::string & url,
                                  const std::map<std::string,std::string> & headers);
    //HTTP POST
    virtual MAX_HTTP_RESPONSE Post(const std::string & url,
                                   const std::map<std::string,std::string> & headers,
                                   const std::string & body);
    //Multipart upload файла в URL, полученный от MAX /uploads
    virtual MAX_HTTP_RESPONSE PostMultipartFile(const std::string & url,
                                   const std::map<std::string,std::string> & headers,
                                   const std::string & fieldName,
                                   const std::string & filename);

    //Доступ к SSL-настройкам для интеграции/диагностики LanMon
    TIdSSLIOHandlerSocketOpenSSL * SSL() { return Ssl; }
private:
    //Перенести map заголовков MAX в TIdHTTP::Request
    void ApplyHeaders(const std::map<std::string,std::string> & headers);
    //Собрать единый MAX_HTTP_RESPONSE из Indy response/exception
    MAX_HTTP_RESPONSE ReadResponse(TMemoryStream * stream, const AnsiString & exceptionText);
};

//LanMon использует CP1251 AnsiString. Преобразуем его в UTF-8 перед сериализацией JSON MAX.
std::string MaxUtf8FromAnsi1251(const AnsiString & text);

#endif
