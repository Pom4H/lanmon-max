#ifndef max_posix_http_transportH
#define max_posix_http_transportH

#include "../api/maxclient.h"

// Minimal plain-HTTP transport used only by Linux E2E tests.
// Production LanMon uses TMaxIndyTransport over HTTPS/OpenSSL.
class TPosixHttpTransport : public IMaxHttpTransport
{
public:
    // HTTP GET to the local mock MAX server.
    virtual MAX_HTTP_RESPONSE Get(const std::string & url,
        const std::map<std::string,std::string> & headers);
    // HTTP POST with a text/JSON body.
    virtual MAX_HTTP_RESPONSE Post(const std::string & url,
        const std::map<std::string,std::string> & headers,
        const std::string & body);
    // Multipart upload used to exercise the same attachment flow as production.
    virtual MAX_HTTP_RESPONSE PostMultipartFile(const std::string & url,
        const std::map<std::string,std::string> & headers,
        const std::string & fieldName,
        const std::string & filename);
private:
    // Shared socket-level HTTP/1.1 implementation for the local E2E server.
    MAX_HTTP_RESPONSE Request(const std::string & method,
        const std::string & url,
        const std::map<std::string,std::string> & headers,
        const std::string & body);
};

#endif
