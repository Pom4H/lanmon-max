#ifndef maxclientH
#define maxclientH

#include "maxcore.h"
#include <string>
#include <map>

struct MAX_HTTP_RESPONSE
{
    int StatusCode;
    std::string Body;
    std::string Error;
    MAX_HTTP_RESPONSE() : StatusCode(0) {}
};

class IMaxHttpTransport
{
public:
    virtual ~IMaxHttpTransport() {}
    virtual MAX_HTTP_RESPONSE Get(const std::string & url,
                                  const std::map<std::string,std::string> & headers)=0;
    virtual MAX_HTTP_RESPONSE Post(const std::string & url,
                                   const std::map<std::string,std::string> & headers,
                                   const std::string & body)=0;
    virtual MAX_HTTP_RESPONSE PostMultipartFile(const std::string & url,
                                   const std::map<std::string,std::string> & headers,
                                   const std::string & fieldName,
                                   const std::string & filename);
};

class MAX_API_CLIENT
{
    std::string Token;
    IMaxHttpTransport * Transport;
    bool HasMarker;
    max_int64 Marker;
    std::string BaseUrl;
public:
    MAX_API_CLIENT(IMaxHttpTransport * transport, const std::string & token,
                   const std::string & baseUrl="https://platform-api2.max.ru");
    void SetToken(const std::string & token) { Token=token; }
    bool GetMe(MAX_BOT_INFO & info, std::string & error);
    bool Poll(MAX_UPDATES & updates, std::string & error, int timeoutSeconds=30, int limit=100);
    bool SendMessage(const MAX_PEER & peer, const std::string & utf8Text, std::string & error);
    bool SendImage(const MAX_PEER & peer, const std::string & filename, const std::string & utf8Caption, std::string & error);
    bool MarkerValid() const { return HasMarker; }
    max_int64 CurrentMarker() const { return Marker; }
    void ResetMarker() { HasMarker=false; Marker=0; }
    const std::string & GetBaseUrl() const { return BaseUrl; }
private:
    std::string WithBaseUrl(const std::string & url) const;
    std::map<std::string,std::string> Headers(bool json) const;
    bool CheckResponse(const MAX_HTTP_RESPONSE & r, std::string & error) const;
};

#endif
