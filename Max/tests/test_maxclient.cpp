// Unit tests MAX_API_CLIENT behavior through an in-memory HTTP transport.
#include "../api/maxclient.h"
#include <iostream>
#include <vector>

// One captured HTTP call made by MAX_API_CLIENT.
struct Call
{
    std::string Method;
    std::string Url;
    std::string Body;
    std::string FieldName;
    std::string Filename;
    std::map<std::string,std::string> Headers;
};

// Deterministic transport: returns queued responses and records every request.
class Mock : public IMaxHttpTransport
{
public:
    std::vector<Call> Calls;
    std::vector<MAX_HTTP_RESPONSE> Responses;
    std::vector<unsigned int> Sleeps;

    MAX_HTTP_RESPONSE Next(const char * method,const std::string & url,
        const std::map<std::string,std::string> & headers,const std::string & body,
        const std::string & fieldName="",const std::string & filename="")
    {
        Call c;
        c.Method=method;
        c.Url=url;
        c.Headers=headers;
        c.Body=body;
        c.FieldName=fieldName;
        c.Filename=filename;
        Calls.push_back(c);
        if(Responses.empty())
        {
            MAX_HTTP_RESPONSE r;
            r.Error="mock response queue is empty";
            return r;
        }
        MAX_HTTP_RESPONSE r=Responses.front();
        Responses.erase(Responses.begin());
        return r;
    }

    virtual MAX_HTTP_RESPONSE Get(const std::string & url,
        const std::map<std::string,std::string> & headers)
    {
        return Next("GET",url,headers,"");
    }

    virtual MAX_HTTP_RESPONSE Post(const std::string & url,
        const std::map<std::string,std::string> & headers,const std::string & body)
    {
        return Next("POST",url,headers,body);
    }

    virtual MAX_HTTP_RESPONSE PostMultipartFile(const std::string & url,
        const std::map<std::string,std::string> & headers,const std::string & fieldName,
        const std::string & filename)
    {
        return Next("MULTIPART",url,headers,"",fieldName,filename);
    }

    virtual void SleepMilliseconds(unsigned int milliseconds)
    {
        Sleeps.push_back(milliseconds);
    }
};

static int failures=0;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL line "<<__LINE__<<": "#x"\n";++failures;}}while(0)

// Helper for a synthetic HTTP response.
static MAX_HTTP_RESPONSE Response(int code,const char * body)
{
    MAX_HTTP_RESPONSE r;
    r.StatusCode=code;
    r.Body=body;
    return r;
}

// Helper for a transport-level failure without an HTTP response.
static MAX_HTTP_RESPONSE TransportError(const char * text)
{
    MAX_HTTP_RESPONSE r;
    r.Error=text;
    return r;
}

static bool HasHeader(const Call & call,const std::string & name)
{
    return call.Headers.find(name)!=call.Headers.end();
}

int main()
{
    std::string error;

    // Null transport must fail predictably instead of dereferencing a null pointer.
    MAX_API_CLIENT noTransport(NULL,"token");
    MAX_BOT_INFO noBot;
    CHECK(!noTransport.GetMe(noBot,error));
    CHECK(error=="MAX transport is null");
    CHECK(noTransport.GetLastStatusCode()==0);

    Mock mock;
    // Trailing slash is removed so all generated URLs are stable.
    MAX_API_CLIENT api(&mock,"secret-token","https://platform-api2.max.ru/");

    // GET /me: parse bot identity and send token only in Authorization header.
    mock.Responses.push_back(Response(200,"{\"user_id\":77,\"first_name\":\"LanMon\",\"is_bot\":true}"));
    MAX_BOT_INFO bot;
    error="stale error";
    CHECK(api.GetMe(bot,error));
    CHECK(error.empty());
    CHECK(bot.Id==77);
    CHECK(mock.Calls.back().Method=="GET");
    CHECK(mock.Calls.back().Url=="https://platform-api2.max.ru/me");
    CHECK(mock.Calls.back().Headers["Authorization"]=="secret-token");
    CHECK(mock.Calls.back().Url.find("secret-token")==std::string::npos);
    CHECK(api.GetLastStatusCode()==200);
    CHECK(api.GetLastResponseBody().find("user_id")!=std::string::npos);

    // A syntactically valid HTTP response with invalid bot JSON must fail at parse stage.
    mock.Responses.push_back(Response(200,"{\"first_name\":\"NoId\"}"));
    CHECK(!api.GetMe(bot,error));
    CHECK(error.find("user_id")!=std::string::npos);
    CHECK(api.GetLastStatusCode()==200);

    // The next successful request must clear the previous parse error.
    mock.Responses.push_back(Response(200,"{\"user_id\":78,\"first_name\":\"Recovered\",\"is_bot\":true}"));
    CHECK(api.GetMe(bot,error));
    CHECK(error.empty());
    CHECK(bot.Id==78);

    // First Long Poll stores marker returned by MAX.
    mock.Responses.push_back(Response(200,"{\"updates\":[],\"marker\":500}"));
    MAX_UPDATES updates;
    CHECK(api.Poll(updates,error,30,100));
    CHECK(api.MarkerValid());
    CHECK(api.CurrentMarker()==500);

    // Broken JSON must not advance a marker that was already committed.
    mock.Responses.push_back(Response(200,"{broken"));
    CHECK(!api.Poll(updates,error,30,100));
    CHECK(api.MarkerValid());
    CHECK(api.CurrentMarker()==500);

    // HTTP failure must also leave marker unchanged and expose response diagnostics.
    mock.Responses.push_back(Response(503,"{\"code\":\"unavailable\"}"));
    CHECK(!api.Poll(updates,error,30,100));
    CHECK(api.CurrentMarker()==500);
    CHECK(api.GetLastStatusCode()==503);
    CHECK(api.GetLastResponseBody().find("unavailable")!=std::string::npos);

    // Next successful Long Poll sends previous marker and advances it.
    mock.Responses.push_back(Response(200,"{\"updates\":[],\"marker\":501}"));
    CHECK(api.Poll(updates,error,30,100));
    CHECK(mock.Calls.back().Url.find("marker=500")!=std::string::npos);
    CHECK(api.CurrentMarker()==501);

    // ResetMarker deliberately returns client to the initial GET /updates behavior.
    api.ResetMarker();
    CHECK(!api.MarkerValid());
    mock.Responses.push_back(Response(200,"{\"updates\":[]}"));
    CHECK(api.Poll(updates,error,30,100));
    CHECK(mock.Calls.back().Url.find("marker=")==std::string::npos);
    CHECK(!api.MarkerValid());

    // Text send to a user: JSON body, Content-Type and user_id routing.
    mock.Responses.push_back(Response(200,"{\"message\":{\"body\":{\"mid\":\"m1\"}}}"));
    CHECK(api.SendMessage(MAX_PEER(maxPeerUser,12),"hello \"x\"\n",error));
    CHECK(mock.Calls.back().Method=="POST");
    CHECK(mock.Calls.back().Url=="https://platform-api2.max.ru/messages?user_id=12");
    CHECK(mock.Calls.back().Headers["Content-Type"]=="application/json");
    CHECK(mock.Calls.back().Body=="{\"text\":\"hello \\\"x\\\"\\n\"}");

    // MAX chat IDs may be signed; preserve the sign and use chat_id namespace.
    mock.Responses.push_back(Response(200,"{\"message\":{}}"));
    CHECK(api.SendMessage(MAX_PEER(maxPeerChat,-33),"chat",error));
    CHECK(mock.Calls.back().Url=="https://platform-api2.max.ru/messages?chat_id=-33");

    // Non-2xx response must propagate status and response body.
    mock.Responses.push_back(Response(401,"{\"code\":\"unauthorized\"}"));
    CHECK(!api.SendMessage(MAX_PEER(maxPeerUser,33),"x",error));
    CHECK(error.find("MAX HTTP 401")!=std::string::npos);
    CHECK(error.find("unauthorized")!=std::string::npos);

    // Transport error must be propagated without inventing an HTTP status.
    mock.Responses.push_back(TransportError("connection reset"));
    CHECK(!api.SendMessage(MAX_PEER(maxPeerUser,33),"x",error));
    CHECK(error=="connection reset");
    CHECK(api.GetLastStatusCode()==0);

    // Image upload: prepare URL -> multipart data -> attachment message.
    size_t imageStart=mock.Calls.size();
    mock.Responses.push_back(Response(200,"{\"url\":\"https://iu.oneme.ru/upload.do?ticket=i-1\"}"));
    mock.Responses.push_back(Response(200,"{\"token\":\"image-token\"}"));
    mock.Responses.push_back(Response(200,"{\"message\":{}}"));
    CHECK(api.SendImage(MAX_PEER(maxPeerChat,-777),"map.png","Карта \"1\"",error));
    CHECK(mock.Calls.size()==imageStart+3);
    CHECK(mock.Calls[imageStart].Url=="https://platform-api2.max.ru/uploads?type=image");
    CHECK(mock.Calls[imageStart].Headers["Authorization"]=="secret-token");
    CHECK(mock.Calls[imageStart+1].Method=="MULTIPART");
    CHECK(mock.Calls[imageStart+1].Url=="https://iu.oneme.ru/upload.do?ticket=i-1");
    CHECK(mock.Calls[imageStart+1].FieldName=="data");
    CHECK(mock.Calls[imageStart+1].Filename=="map.png");
    // Multipart upload-host is separate from Bot API; do not leak the bot token there.
    CHECK(!HasHeader(mock.Calls[imageStart+1],"Authorization"));
    CHECK(mock.Calls[imageStart+2].Url=="https://platform-api2.max.ru/messages?chat_id=-777");
    CHECK(mock.Calls[imageStart+2].Body.find("\"type\":\"image\"")!=std::string::npos);
    CHECK(mock.Calls[imageStart+2].Body.find("image-token")!=std::string::npos);
    CHECK(mock.Calls[imageStart+2].Body.find("Карта \\\"1\\\"")!=std::string::npos);

    // File upload uses type=file and preserves a user recipient.
    size_t fileStart=mock.Calls.size();
    mock.Responses.push_back(Response(200,"{\"url\":\"https://fu.oneme.ru/upload.do?ticket=f-1\"}"));
    mock.Responses.push_back(Response(200,"{\"token\":\"file-token\"}"));
    mock.Responses.push_back(Response(200,"{\"message\":{}}"));
    CHECK(api.SendFile(MAX_PEER(maxPeerUser,42),"alarm.pdf","Тревоги",error));
    CHECK(mock.Calls.size()==fileStart+3);
    CHECK(mock.Calls[fileStart].Url=="https://platform-api2.max.ru/uploads?type=file");
    CHECK(mock.Calls[fileStart+1].Url=="https://fu.oneme.ru/upload.do?ticket=f-1");
    CHECK(!HasHeader(mock.Calls[fileStart+1],"Authorization"));
    CHECK(mock.Calls[fileStart+2].Url=="https://platform-api2.max.ru/messages?user_id=42");
    CHECK(mock.Calls[fileStart+2].Body.find("\"type\":\"file\"")!=std::string::npos);
    CHECK(mock.Calls[fileStart+2].Body.find("file-token")!=std::string::npos);

    // attachment.not.ready retries only the final POST with the same token.
    // The expensive multipart upload must happen exactly once.
    size_t retryStart=mock.Calls.size();
    size_t retrySleepStart=mock.Sleeps.size();
    mock.Responses.push_back(Response(200,"{\"url\":\"https://fu.oneme.ru/retry\"}"));
    mock.Responses.push_back(Response(200,"{\"token\":\"same-token\"}"));
    mock.Responses.push_back(Response(400,"{\"code\":\"attachment.not.ready\"}"));
    mock.Responses.push_back(Response(200,"{\"message\":{}}"));
    CHECK(api.SendFile(MAX_PEER(maxPeerUser,7),"large.pdf","Большой файл",error));
    CHECK(mock.Calls.size()==retryStart+4);
    CHECK(mock.Calls[retryStart].Method=="POST");
    CHECK(mock.Calls[retryStart+1].Method=="MULTIPART");
    CHECK(mock.Calls[retryStart+2].Method=="POST");
    CHECK(mock.Calls[retryStart+3].Method=="POST");
    CHECK(mock.Calls[retryStart+2].Body==mock.Calls[retryStart+3].Body);
    CHECK(mock.Calls[retryStart+2].Body.find("same-token")!=std::string::npos);
    CHECK(mock.Sleeps.size()==retrySleepStart+1);
    CHECK(mock.Sleeps[retrySleepStart]==500);
    CHECK(error.empty());
    CHECK(api.GetLastStatusCode()==200);

    // Persistent attachment.not.ready stops after four sends with 0.5/1/2 second backoff.
    size_t exhaustedStart=mock.Calls.size();
    size_t exhaustedSleepStart=mock.Sleeps.size();
    mock.Responses.push_back(Response(200,"{\"url\":\"https://iu.oneme.ru/exhaust\"}"));
    mock.Responses.push_back(Response(200,"{\"token\":\"never-ready\"}"));
    mock.Responses.push_back(Response(400,"{\"code\":\"attachment.not.ready\"}"));
    mock.Responses.push_back(Response(400,"{\"code\":\"attachment.not.ready\"}"));
    mock.Responses.push_back(Response(400,"{\"code\":\"attachment.not.ready\"}"));
    mock.Responses.push_back(Response(400,"{\"code\":\"attachment.not.ready\"}"));
    CHECK(!api.SendImage(MAX_PEER(maxPeerChat,-8),"large.png","",error));
    CHECK(mock.Calls.size()==exhaustedStart+6);
    CHECK(mock.Calls[exhaustedStart+1].Method=="MULTIPART");
    CHECK(mock.Calls[exhaustedStart+2].Body==mock.Calls[exhaustedStart+3].Body);
    CHECK(mock.Calls[exhaustedStart+3].Body==mock.Calls[exhaustedStart+4].Body);
    CHECK(mock.Calls[exhaustedStart+4].Body==mock.Calls[exhaustedStart+5].Body);
    CHECK(mock.Sleeps.size()==exhaustedSleepStart+3);
    CHECK(mock.Sleeps[exhaustedSleepStart]==500);
    CHECK(mock.Sleeps[exhaustedSleepStart+1]==1000);
    CHECK(mock.Sleeps[exhaustedSleepStart+2]==2000);
    CHECK(api.GetLastStatusCode()==400);
    CHECK(error.find("attachment.not.ready")!=std::string::npos);

    // If POST /uploads fails, multipart and message send must not happen.
    size_t failedPrepareStart=mock.Calls.size();
    mock.Responses.push_back(Response(401,"{\"code\":\"unauthorized\"}"));
    CHECK(!api.SendImage(MAX_PEER(maxPeerUser,1),"x.png","",error));
    CHECK(mock.Calls.size()==failedPrepareStart+1);

    // If upload-host fails, attachment message must not be sent.
    size_t failedUploadStart=mock.Calls.size();
    mock.Responses.push_back(Response(200,"{\"url\":\"https://iu.oneme.ru/u\"}"));
    mock.Responses.push_back(Response(500,"upload failed"));
    CHECK(!api.SendImage(MAX_PEER(maxPeerUser,1),"x.png","",error));
    CHECK(mock.Calls.size()==failedUploadStart+2);
    CHECK(error.find("MAX HTTP 500")!=std::string::npos);

    // A successful upload without token is unusable and must stop before POST /messages.
    size_t missingTokenStart=mock.Calls.size();
    mock.Responses.push_back(Response(200,"{\"url\":\"https://fu.oneme.ru/u\"}"));
    mock.Responses.push_back(Response(200,"{\"retval\":true}"));
    CHECK(!api.SendFile(MAX_PEER(maxPeerUser,1),"x.pdf","",error));
    CHECK(mock.Calls.size()==missingTokenStart+2);
    CHECK(error.find("token")!=std::string::npos);

    // Unrelated final errors such as rate limiting are not mistaken for attachment readiness.
    size_t failedSendStart=mock.Calls.size();
    size_t failedSendSleepStart=mock.Sleeps.size();
    mock.Responses.push_back(Response(200,"{\"url\":\"https://fu.oneme.ru/u2\"}"));
    mock.Responses.push_back(Response(200,"{\"token\":\"ready-later\"}"));
    mock.Responses.push_back(Response(429,"{\"code\":\"too_many_requests\"}"));
    CHECK(!api.SendFile(MAX_PEER(maxPeerUser,1),"x.pdf","",error));
    CHECK(mock.Calls.size()==failedSendStart+3);
    CHECK(mock.Sleeps.size()==failedSendSleepStart);
    CHECK(api.GetLastStatusCode()==429);
    CHECK(error.find("MAX HTTP 429")!=std::string::npos);

    std::cout<<(failures?"maxclient tests FAILED":"All maxclient tests passed")<<"\n";
    return failures?1:0;
}
