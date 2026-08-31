// Unit tests protocol-level MAX functions without network or VCL dependencies.
#include "../api/maxcore.h"
#include <iostream>
#include <cstdlib>

static int fails=0;
#define CHECK(x) do { if(!(x)){ std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " #x "\n"; ++fails; } } while(0)

int main()
{
    const char * updateTypes="&types=message_created,bot_added,user_added";

    // URL GET /updates: defaults, marker and both API range boundaries.
    CHECK(MaxBuildUpdatesUrl(false,0,30,100)==
        std::string("https://platform-api2.max.ru/updates?timeout=30&limit=100")+updateTypes);
    CHECK(MaxBuildUpdatesUrl(true,922337203685477000LL,120,5000)==
        std::string("https://platform-api2.max.ru/updates?timeout=90&limit=1000&marker=922337203685477000")+updateTypes);
    CHECK(MaxBuildUpdatesUrl(false,0,-10,0)==
        std::string("https://platform-api2.max.ru/updates?timeout=0&limit=1")+updateTypes);
    CHECK(MaxBuildUpdatesUrl(false,0,90,1000)==
        std::string("https://platform-api2.max.ru/updates?timeout=90&limit=1000")+updateTypes);

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

    // /updates: message + Telegram invitation/new-participant equivalents.
    const char * updatesJson=
        "{\"updates\":["
        "{\"update_type\":\"message_created\",\"timestamp\":1720000000123,\"message\":{"
        "\"sender\":{\"user_id\":100,\"first_name\":\"Ivan\",\"last_name\":\"Petrov\",\"username\":\"ivan\",\"is_bot\":false},"
        "\"recipient\":{\"chat_id\":-555,\"chat_type\":\"chat\",\"user_id\":200},"
        "\"timestamp\":1720000000000,\"body\":{\"mid\":\"mid1\",\"seq\":1,\"text\":\"hello \\\"MAX\\\"\\nline2\"}}},"
        "{\"update_type\":\"bot_added\",\"timestamp\":1720000001000,\"chat_id\":-777,"
        "\"user\":{\"user_id\":201,\"first_name\":\"Admin\",\"is_bot\":false},\"is_channel\":false},"
        "{\"update_type\":\"user_added\",\"timestamp\":1720000002000,\"chat_id\":-777,"
        "\"user\":{\"user_id\":202,\"first_name\":\"New\",\"last_name\":\"User\",\"is_bot\":false},\"is_channel\":false},"
        "{\"update_type\":\"bot_started\",\"timestamp\":1720000003000,\"user\":{\"user_id\":101}}"
        "],\"marker\":9876543210123}";
    MAX_UPDATES up;
    CHECK(MaxParseUpdates(updatesJson,up,err));
    CHECK(up.HasMarker && up.Marker==9876543210123LL);
    CHECK(up.Messages.size()==3);
    if(up.Messages.size()==3)
    {
        CHECK(up.Messages[0].UpdateType=="message_created");
        CHECK(up.Messages[0].UserId==100);
        CHECK(up.Messages[0].ChatId==-555);
        CHECK(up.Messages[0].ChatType=="chat");
        CHECK(up.Messages[0].MessageId=="mid1");
        CHECK(up.Messages[0].Text=="hello \"MAX\"\nline2");
        CHECK(up.Messages[0].FirstName=="Ivan");
        CHECK(up.Messages[0].LastName=="Petrov");
        CHECK(!up.Messages[0].SenderIsBot);

        CHECK(up.Messages[1].UpdateType=="bot_added");
        CHECK(up.Messages[1].ChatId==-777);
        CHECK(up.Messages[1].ChatType=="chat");
        CHECK(up.Messages[1].UserId==201);
        CHECK(up.Messages[1].FirstName=="Admin");
        CHECK(up.Messages[1].MessageTimestamp==1720000001000LL);

        CHECK(up.Messages[2].UpdateType=="user_added");
        CHECK(up.Messages[2].ChatId==-777);
        CHECK(up.Messages[2].UserId==202);
        CHECK(up.Messages[2].FirstName=="New");
        CHECK(up.Messages[2].LastName=="User");
    }

    // Channel membership preserves channel addressing.
    CHECK(MaxParseUpdates(
        "{\"updates\":[{\"update_type\":\"bot_added\",\"timestamp\":1,\"chat_id\":-9,"
        "\"user\":{\"user_id\":1},\"is_channel\":true}],\"marker\":2}",up,err));
    CHECK(up.Messages.size()==1);
    CHECK(up.Messages[0].ChatType=="channel");

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

    // Membership event without chat_id is unusable and ignored safely.
    CHECK(MaxParseUpdates(
        "{\"updates\":[{\"update_type\":\"bot_added\",\"timestamp\":1,\"user\":{\"user_id\":1}}],\"marker\":2}",
        up,err));
    CHECK(up.Messages.empty());

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

    // JSON number grammar must be strict. These forms are forbidden by RFC JSON grammar.
    CHECK(!MaxParseUpdates("{\"updates\":[],\"marker\":1.}",up,err));
    CHECK(!MaxParseUpdates("{\"updates\":[],\"marker\":1e}",up,err));
    CHECK(!MaxParseUpdates("{\"updates\":[],\"marker\":1e+}",up,err));
    CHECK(!MaxParseUpdates("{\"updates\":[],\"marker\":01}",up,err));
    CHECK(!MaxParseUpdates("{\"updates\":[],\"marker\":-}",up,err));

    // JSON \uXXXX escapes from MAX are decoded to UTF-8.
    std::string unicodeJson=
        "{\"updates\":[{\"update_type\":\"message_created\",\"timestamp\":1,\"message\":{"
        "\"sender\":{\"user_id\":1},\"recipient\":{\"chat_id\":2,\"chat_type\":\"dialog\"},"
        "\"timestamp\":1,\"body\":{\"mid\":\"m\",\"text\":\"\\u041f\\u0440\\u0438\\u0432\\u0435\\u0442\"}}}],\"marker\":2}";
    CHECK(MaxParseUpdates(unicodeJson,up,err));
    CHECK(up.Messages.size()==1 && up.Messages[0].Text=="Привет");

    // UTF-16 surrogate pair in JSON must become one Unicode code point, not invalid UTF-8.
    std::string surrogateJson=
        "{\"updates\":[{\"update_type\":\"message_created\",\"timestamp\":1,\"message\":{"
        "\"sender\":{\"user_id\":1},\"recipient\":{\"chat_id\":2,\"chat_type\":\"dialog\"},"
        "\"timestamp\":1,\"body\":{\"mid\":\"m\",\"text\":\"emoji \\uD83D\\uDE00\"}}}],\"marker\":2}";
    CHECK(MaxParseUpdates(surrogateJson,up,err));
    CHECK(up.Messages.size()==1 && up.Messages[0].Text=="emoji 😀");

    // Lone or mismatched UTF-16 surrogate escapes are invalid JSON strings for our UTF-8 output.
    CHECK(!MaxParseUpdates(
        "{\"updates\":[{\"update_type\":\"message_created\",\"message\":{\"body\":{\"text\":\"\\uD83D\"}}}]}",
        up,err));
    CHECK(!MaxParseUpdates(
        "{\"updates\":[{\"update_type\":\"message_created\",\"message\":{\"body\":{\"text\":\"\\uDE00\"}}}]}",
        up,err));
    CHECK(!MaxParseUpdates(
        "{\"updates\":[{\"update_type\":\"message_created\",\"message\":{\"body\":{\"text\":\"\\uD83D\\u0041\"}}}]}",
        up,err));

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
