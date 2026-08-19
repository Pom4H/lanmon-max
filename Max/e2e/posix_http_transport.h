#ifndef max_posix_http_transportH
#define max_posix_http_transportH

#include "../api/maxclient.h"

class TPosixHttpTransport : public IMaxHttpTransport
{
public:
    virtual MAX_HTTP_RESPONSE Get(const std::string & url,
        const std::map<std::string,std::string> & headers);
    virtual MAX_HTTP_RESPONSE Post(const std::string & url,
        const std::map<std::string,std::string> & headers,
        const std::string & body);
    virtual MAX_HTTP_RESPONSE PostMultipartFile(const std::string & url,
        const std::map<std::string,std::string> & headers,
        const std::string & fieldName,
        const std::string & filename);
private:
    MAX_HTTP_RESPONSE Request(const std::string & method,
        const std::string & url,
        const std::map<std::string,std::string> & headers,
        const std::string & body);
};

#endif
