// Unit tests MAX_API_CLIENT behavior through an in-memory HTTP transport.
#include "../api/maxclient.h"
#include <iostream>
#include <vector>

// One captured HTTP call made by MAX_API_CLIENT.
struct Call
{
    std::string Method,Url,Body;
    std::map<std::string,std::string> Headers;
};

// Deterministic transport: returns queued responses and records every request.
class Mock : public IMaxHttpTransport
{
public:
    std::vector<Call> Calls;
    std::vector<MAX_HTTP_RESPONSE> Responses;

    MAX_HTTP_RESPONSE Next(const char *method,const std::string &url,
        const std::map<std::string,std::string> &headers,const std::string &body)
    {
        Call c;
        c.Method=method;c.Url=url;c.Headers=headers;c.Body=body;
        Calls.push_back(c);
        MAX_HTTP_RESPONSE r=Responses.front();
        Responses.erase(Responses.begin());
        return r;
    }

    MAX_HTTP_RESPONSE Get(const std::string &url,
        const std::map<std::string,std::string> &headers)
    { return Next("GET",url,headers,""); }

    MAX_HTTP_RESPONSE Post(const std::string &url,
        const std::map<std::string,std::string> &headers,const std::string &body)
    { return Next("POST",url,headers,body); }
};

static int failures=0;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL line "<<__LINE__<<": "#x"\n";++failures;}}while(0)

// Helper for a synthetic HTTP response.
static MAX_HTTP_RESPONSE Response(int code,const char *body)
{
    MAX_HTTP_RESPONSE r;r.StatusCode=code;r.Body=body;return r;
}

int main()
{
    Mock mock;
    MAX_API_CLIENT api(&mock,"secret-token");
    std::string error;

    // GET /me: parse bot identity and send token only in Authorization header.
    mock.Responses.push_back(Response(200,"{\"user_id\":77,\"first_name\":\"LanMon\",\"is_bot\":true}"));
    MAX_BOT_INFO bot;
    CHECK(api.GetMe(bot,error));
    CHECK(bot.Id==77);
    CHECK(mock.Calls[0].Headers["Authorization"]=="secret-token");
    CHECK(mock.Calls[0].Url=="https://platform-api2.max.ru/me");

    // First Long Poll stores marker returned by MAX.
    mock.Responses.push_back(Response(200,"{\"updates\":[],\"marker\":500}"));
    MAX_UPDATES updates;
    CHECK(api.Poll(updates,error,30,100));
    CHECK(api.MarkerValid()&&api.CurrentMarker()==500);

    // Next Long Poll must send previous marker and advance it to the new value.
    mock.Responses.push_back(Response(200,"{\"updates\":[],\"marker\":501}"));
    CHECK(api.Poll(updates,error,30,100));
    CHECK(mock.Calls[2].Url.find("marker=500")!=std::string::npos);
    CHECK(api.CurrentMarker()==501);

    // Text send: POST JSON body with Content-Type and correct escaping.
    mock.Responses.push_back(Response(200,"{\"message\":{}}"));
    CHECK(api.SendMessage(MAX_PEER(maxPeerUser,12),"hello \"x\"\n",error));
    CHECK(mock.Calls[3].Method=="POST");
    CHECK(mock.Calls[3].Headers["Content-Type"]=="application/json");
    CHECK(mock.Calls[3].Body=="{\"text\":\"hello \\\"x\\\"\\n\"}");

    // Non-2xx response must propagate as a MAX HTTP diagnostic.
    mock.Responses.push_back(Response(401,"{\"code\":\"unauthorized\"}"));
    CHECK(!api.SendMessage(MAX_PEER(maxPeerChat,33),"x",error));
    CHECK(error.find("MAX HTTP 401")!=std::string::npos);

    std::cout<<(failures?"maxclient tests FAILED":"All maxclient tests passed")<<"\n";
    return failures?1:0;
}
