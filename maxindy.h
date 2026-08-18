#ifndef maxindyH
#define maxindyH

#include <IdHTTP.hpp>
#include <IdSSLOpenSSL.hpp>
#include <IdMultipartFormData.hpp>
#include "maxclient.h"

class TMaxIndyTransport : public IMaxHttpTransport
{
    TIdHTTP * Http;
    TIdSSLIOHandlerSocketOpenSSL * Ssl;
public:
    TMaxIndyTransport();
    virtual ~TMaxIndyTransport();

    virtual MAX_HTTP_RESPONSE Get(const std::string & url,
                                  const std::map<std::string,std::string> & headers);
    virtual MAX_HTTP_RESPONSE Post(const std::string & url,
                                   const std::map<std::string,std::string> & headers,
                                   const std::string & body);
    virtual MAX_HTTP_RESPONSE PostMultipartFile(const std::string & url,
                                   const std::map<std::string,std::string> & headers,
                                   const std::string & fieldName,
                                   const std::string & filename);

    TIdSSLIOHandlerSocketOpenSSL * SSL() { return Ssl; }
private:
    void ApplyHeaders(const std::map<std::string,std::string> & headers);
    MAX_HTTP_RESPONSE ReadResponse(TMemoryStream * stream, const AnsiString & exceptionText);
};

// LanMon uses CP1251 AnsiString. This converts it to UTF-8 before maxcore serializes JSON.
std::string MaxUtf8FromAnsi1251(const AnsiString & text);

#endif
