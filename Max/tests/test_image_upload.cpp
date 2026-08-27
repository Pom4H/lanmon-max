// Regression test for the live MAX image upload response used by iu.oneme.ru.
#include "../api/maxclient.h"
#include <iostream>
#include <map>
#include <string>
#include <vector>

static int fails=0;
#define CHECK(x) do { if(!(x)){ std::cerr << "FAIL line " << __LINE__ << ": " #x "\n"; ++fails; } } while(0)

static MAX_HTTP_RESPONSE Response(int code,const char * body)
{
    MAX_HTTP_RESPONSE r;
    r.StatusCode=code;
    r.Body=body;
    return r;
}

class ImagePhotosTransport : public IMaxHttpTransport
{
public:
    int Step;
    std::string FinalBody;
    ImagePhotosTransport() : Step(0) {}

    virtual MAX_HTTP_RESPONSE Get(const std::string &,
        const std::map<std::string,std::string> &)
    {
        MAX_HTTP_RESPONSE r;
        r.Error="unexpected GET";
        return r;
    }

    virtual MAX_HTTP_RESPONSE Post(const std::string & url,
        const std::map<std::string,std::string> & headers,const std::string & body)
    {
        ++Step;
        if(Step==1)
        {
            CHECK(url=="https://platform-api2.max.ru/uploads?type=image");
            CHECK(headers.find("Authorization")!=headers.end());
            if(headers.find("Authorization")!=headers.end())
                CHECK(headers.find("Authorization")->second=="secret-token");
            CHECK(body.empty());
            return Response(200,
                "{\"url\":\"https://iu.oneme.ru/uploadImage?apiToken=upload-ticket&photoIds=1\"}");
        }
        if(Step==3)
        {
            CHECK(url=="https://platform-api2.max.ru/messages?chat_id=-777");
            CHECK(headers.find("Authorization")!=headers.end());
            CHECK(headers.find("Content-Type")!=headers.end());
            FinalBody=body;
            return Response(200,"{\"message\":{\"body\":{\"mid\":\"ok\"}}}");
        }
        MAX_HTTP_RESPONSE r;
        r.Error="unexpected POST step";
        return r;
    }

    virtual MAX_HTTP_RESPONSE PostMultipartFile(const std::string & url,
        const std::map<std::string,std::string> & headers,const std::string & fieldName,
        const std::string & filename)
    {
        ++Step;
        CHECK(Step==2);
        CHECK(url=="https://iu.oneme.ru/uploadImage?apiToken=upload-ticket&photoIds=1");
        CHECK(headers.find("Authorization")==headers.end());
        CHECK(fieldName=="data");
        CHECK(filename=="map.png");

        // Since July 2026 the live image host may return photos instead of a top-level token.
        return Response(200,
            "{\"photos\":{\"6l2NA9+Fc/K8m2DdCKWH51CJ7+uXuAcfGXqB+DPMl66HNRlsEU31LA==\":"
            "{\"token\":\"pWlNfbX8s81xW218OZTNSu99ixgVWM/jH7X/eOrpP+s=\"}}}");
    }
};

int main()
{
    ImagePhotosTransport transport;
    MAX_API_CLIENT api(&transport,"secret-token");
    std::string error;

    CHECK(api.SendImage(MAX_PEER(maxPeerChat,-777),"map.png","Карта 1",error));
    CHECK(error.empty());
    CHECK(transport.Step==3);

    const std::string expectedPayload=
        "\"payload\":{\"photos\":{\"6l2NA9+Fc/K8m2DdCKWH51CJ7+uXuAcfGXqB+DPMl66HNRlsEU31LA==\":"
        "{\"token\":\"pWlNfbX8s81xW218OZTNSu99ixgVWM/jH7X/eOrpP+s=\"}}}";
    CHECK(transport.FinalBody.find("\"type\":\"image\"")!=std::string::npos);
    CHECK(transport.FinalBody.find(expectedPayload)!=std::string::npos);
    CHECK(transport.FinalBody.find("\"payload\":{\"token\":") == std::string::npos);

    if(fails)
    {
        std::cerr << fails << " image upload regression test(s) failed\n";
        return 1;
    }
    std::cout << "MAX live image photos payload regression passed\n";
    return 0;
}
