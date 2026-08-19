// Unit tests protocol-level MAX functions without network or VCL dependencies.
#include "../api/maxcore.h"
#include <iostream>
#include <cstdlib>

static int fails=0;
#define CHECK(x) do { if(!(x)){ std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " #x "\n"; ++fails; } } while(0)

int main()
{
    // URL GET /updates: defaults, marker and both API range boundaries.
    CHECK(MaxBuildUpdatesUrl(false,0,30,100)==
        "https://platform-api2.max.ru/updates?timeout=30&limit=100&types=message_created");
    CHECK(MaxBuildUpdatesUrl(true,922337203685477000LL,120,5000)==
        "https://platform-api2.max.ru/updates?timeout=90&limit=1000&marker=922337203685477000&types=message_created");
    CHECK(MaxBuildUpdatesUrl(false,0,-10,0)==
        "https://platform-api2.max.ru/updates?timeout=0&limit=1&types=message_created");
    CHECK(MaxBuildUpdatesUrl(false,0,90,1000)==
        "https://platform-api2.max.ru/updates?timeout=90&limit=1000&types=message_created");

    // MAX has two address spaces: personal user_id and signed chat_id.
    CHECK(MaxBuildSendMessageUrl(MAX_PEER(maxPeerUser,123))==
        "https://platform-api2.max.ru/messages?user_id=123");
    CHECK(MaxBuildSendMessageUrl(MAX_PEER(maxPeerChat,456))==
        "https://platform-api2.max.ru/messages?chat_id=456");
    CHECK(MaxBuildSendMessageUrl(MAX_PEER(maxPeerChat,-9876543210LL))==
        "https://platform-api2.max.ru/messages?chat_id=-9876543210");
    CHECK(MaxInt64ToString(-922337203685477000LL)=="-922337203685477000");

    // Legacy LanMon text is CP1251; MAX JSON is UTF-8.
    const char cp1251Privet[]={
        (char)0xCF,(char)0xF0,(char)0xE8,(char)0xE2,(char)0xE5,(char)0xF2,0};
    CHECK(MaxUtf8FromCp1251(cp1251Privet)=="Привет");
    const char cp1251Yo[]={ (char)0xA8,(char)0xB8,0 };
    CHECK(MaxUtf8FromCp1251(cp1251Yo)=="Ёё");
    CHECK(MaxUtf8FromCp1251("ASCII 123")=="ASCII 123");

    // JSON escaping must preserve data and escape JSON syntax/control characters.
    std::string body=MaxBuildSendMessageBody("Alarm \"Pump #1\"\\path\nline2\t!");
    CHECK(body=="{\"text\":\"Alarm \\\"Pump #1\\\"\\\\path\\nline2\\t!\"}");
    std::string control;
    control+=(char)1;
    CHECK(MaxJsonEscape(control)=="\\u0001");
    CHECK(MaxBuildSendMessageBody("")=="{\"text\":\"\"}");

    // Attachment JSON must escape caption and token independently.
    std::string imageBody=MaxBuildImageMessageBody("Карта \"1\"","tok\\\"en");
    CHECK(imageBody.find("\"type\":\"image\"")!=std::string::npos);
    CHECK(imageBody.find("Карта \\\"1\\\"")!=std::string::npos);
    CHECK(imageBody.find("tok\\\\\\\"en")!=std::string::npos);

    // GET /me response -> MAX_BOT_INFO, including optional fields.
    MAX_BOT_INFO bi;
    std::string err;
    CHECK(MaxParseBotInfo(
        "{\"user_id\":42,\"first_name\":\"LanMon\",\"last_name\":\"Bot\","
        "\"username\":\"lanmon_bot\",\"is_bot\":true}",bi,err));
    CHECK(bi.Id==42);
    CHECK(bi.FirstName=="LanMon");
    CHECK(bi.LastName=="Bot");
    CHECK(bi.UserName=="lanmon_bot");
    CHECK(bi.IsBot);

    // Missing identity is not a valid /me result.
    CHECK(!MaxParseBotInfo("{\"first_name\":\"LanMon\"}",bi,err));
    CHECK(err.find("user_id")!=std::string::npos);
    CHECK(!MaxParseBotInfo("[]",bi,err));

    // /updates: keep only message_created, preserve 64-bit marker and message fields.
    const char * updatesJson=
        "{\"updates\":["
        "{\"update_type\":\"message_created\",\"timestamp\":1720000000123,\"message\":{"
        "\"sender\":{\"user_id\":100,\"first_name\":\"Ivan\",\"last_name\":\"Petrov\",\"username\":\"ivan\",\"is_bot\":false},"
        "\"recipient\":{\"chat_id\":-555,\"chat_type\":\"chat\",\"user_id\":200},"
        "\"timestamp\":1720000000000,\"body\":{\"mid\":\"mid1\",\"seq\":1,\"text\":\"hello \\\"MAX\\\"\\nline2\"}}},"
        "{\"update_type\":\"bot_started\",\"timestamp\":1720000000999,\"user\":{\"user_id\":101}}"
        "],\"marker\":9876543210123}";
    MAX_UPDATES up;
    CHECK(MaxParseUpdates(updatesJson,up,err));
    CHECK(up.HasMarker && up.Marker==9876543210123LL);
    CHECK(up.Messages.size()==1);
    if(up.Messages.size()==1)
    {
        CHECK(up.Messages[0].UserId==100);
        CHECK(up.Messages[0].ChatId==-555);
        CHECK(up.Messages[0].ChatType=="chat");
        CHECK(up.Messages[0].MessageId=="mid1");
        CHECK(up.Messages[0].Text=="hello \"MAX\"\nline2");
        CHECK(up.Messages[0].FirstName=="Ivan");
        CHECK(up.Messages[0].LastName=="Petrov");
        CHECK(!up.Messages[0].SenderIsBot);
    }

    // Empty update page without marker is valid and keeps HasMarker=false.
    CHECK(MaxParseUpdates("{\"updates\":[]}",up,err));
    CHECK(up.Messages.empty());
    CHECK(!up.HasMarker);

    // Explicit nullable marker has the same meaning for the parser.
    CHECK(MaxParseUpdates("{\"updates\":[],\"marker\":null}",up,err));
    CHECK(!up.HasMarker);

    // Unknown event types are ignored rather than converted into fake messages.
    CHECK(MaxParseUpdates(
        "{\"updates\":[{\"update_type\":\"message_callback\",\"timestamp\":1}],\"marker\":2}",
        up,err));
    CHECK(up.Messages.empty());
    CHECK(up.HasMarker && up.Marker==2);

    // A message_created event with no message object is ignored safely.
    CHECK(MaxParseUpdates(
        "{\"updates\":[{\"update_type\":\"message_created\",\"timestamp\":1}],\"marker\":3}",
        up,err));
    CHECK(up.Messages.empty());

    // Malformed/top-level-invalid responses must fail with diagnostics.
    CHECK(!MaxParseUpdates("{broken",up,err));
    CHECK(!err.empty());
    CHECK(!MaxParseUpdates("[]",up,err));
    CHECK(!MaxParseUpdates("{\"marker\":1}",up,err));

    // JSON \uXXXX escapes from MAX are decoded to UTF-8.
    std::string unicodeJson=
        "{\"updates\":[{\"update_type\":\"message_created\",\"timestamp\":1,\"message\":{"
        "\"sender\":{\"user_id\":1},\"recipient\":{\"chat_id\":2,\"chat_type\":\"dialog\"},"
        "\"timestamp\":1,\"body\":{\"mid\":\"m\",\"text\":\"\\u041f\\u0440\\u0438\\u0432\\u0435\\u0442\"}}}],\"marker\":2}";
    CHECK(MaxParseUpdates(unicodeJson,up,err));
    CHECK(up.Messages.size()==1 && up.Messages[0].Text=="Привет");

    // Raw UTF-8 from a normal MAX response must be preserved, including emoji.
    std::string utf8Json=
        "{\"updates\":[{\"update_type\":\"message_created\",\"timestamp\":1,\"message\":{"
        "\"sender\":{\"user_id\":1},\"recipient\":{\"chat_id\":2,\"chat_type\":\"dialog\"},"
        "\"timestamp\":1,\"body\":{\"mid\":\"m\",\"text\":\"Привет 😀\"}}}],\"marker\":2}";
    CHECK(MaxParseUpdates(utf8Json,up,err));
    CHECK(up.Messages.size()==1 && up.Messages[0].Text=="Привет 😀");

    // Upload response parsing: exact URL/token and required-field failures.
    std::string uploadUrl;
    CHECK(MaxParseUploadUrl(
        "{\"url\":\"https://iu.oneme.ru/upload.do?sig=a%2Bb&x=1\"}",uploadUrl,err));
    CHECK(uploadUrl=="https://iu.oneme.ru/upload.do?sig=a%2Bb&x=1");
    CHECK(!MaxParseUploadUrl("{}",uploadUrl,err));
    CHECK(err.find("url")!=std::string::npos);

    std::string token;
    CHECK(MaxParseUploadToken("{\"token\":\"abc_DEF-123\"}",token,err));
    CHECK(token=="abc_DEF-123");
    CHECK(!MaxParseUploadToken("{\"retval\":true}",token,err));
    CHECK(err.find("token")!=std::string::npos);

    if(fails)
    {
        std::cerr << fails << " test(s) failed\n";
        return 1;
    }
    std::cout << "All maxcore tests passed\n";
    return 0;
}
