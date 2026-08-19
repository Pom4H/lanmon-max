#include "../api/maxcore.h"
#include <iostream>
#include <cstdlib>

static int fails=0;
#define CHECK(x) do { if(!(x)){ std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " #x "\n"; ++fails; } } while(0)

int main()
{
    CHECK(MaxBuildUpdatesUrl(false,0,30,100)=="https://platform-api2.max.ru/updates?timeout=30&limit=100&types=message_created");
    CHECK(MaxBuildUpdatesUrl(true,922337203685477000LL,120,5000)=="https://platform-api2.max.ru/updates?timeout=90&limit=1000&marker=922337203685477000&types=message_created");
    CHECK(MaxBuildSendMessageUrl(MAX_PEER(maxPeerUser,123))=="https://platform-api2.max.ru/messages?user_id=123");
    CHECK(MaxBuildSendMessageUrl(MAX_PEER(maxPeerChat,456))=="https://platform-api2.max.ru/messages?chat_id=456");

    const char cp1251Privet[]={(char)0xCF,(char)0xF0,(char)0xE8,(char)0xE2,(char)0xE5,(char)0xF2,0};
    CHECK(MaxUtf8FromCp1251(cp1251Privet)=="Привет");

    std::string body=MaxBuildSendMessageBody("Alarm \"Pump #1\"\\path\nline2\t!");
    CHECK(body=="{\"text\":\"Alarm \\\"Pump #1\\\"\\\\path\\nline2\\t!\"}");

    MAX_BOT_INFO bi; std::string err;
    CHECK(MaxParseBotInfo("{\"user_id\":42,\"first_name\":\"LanMon\",\"username\":\"lanmon_bot\",\"is_bot\":true}",bi,err));
    CHECK(bi.Id==42 && bi.FirstName=="LanMon" && bi.UserName=="lanmon_bot" && bi.IsBot);

    const char * updatesJson=
        "{\"updates\":["
        "{\"update_type\":\"message_created\",\"timestamp\":1720000000123,\"message\":{"
        "\"sender\":{\"user_id\":100,\"first_name\":\"Ivan\",\"last_name\":\"Petrov\",\"username\":\"ivan\",\"is_bot\":false},"
        "\"recipient\":{\"chat_id\":555,\"chat_type\":\"dialog\",\"user_id\":200},"
        "\"timestamp\":1720000000000,\"body\":{\"mid\":\"mid1\",\"seq\":1,\"text\":\"hello \\\"MAX\\\"\\nline2\"}}},"
        "{\"update_type\":\"bot_started\",\"timestamp\":1720000000999,\"user\":{\"user_id\":101}}"
        "],\"marker\":9876543210123}";
    MAX_UPDATES up;
    CHECK(MaxParseUpdates(updatesJson,up,err));
    CHECK(up.HasMarker && up.Marker==9876543210123LL);
    CHECK(up.Messages.size()==1);
    if(up.Messages.size()==1) {
        CHECK(up.Messages[0].UserId==100);
        CHECK(up.Messages[0].ChatId==555);
        CHECK(up.Messages[0].MessageId=="mid1");
        CHECK(up.Messages[0].Text=="hello \"MAX\"\nline2");
        CHECK(up.Messages[0].FirstName=="Ivan");
    }

    CHECK(!MaxParseUpdates("{broken",up,err));
    CHECK(!err.empty());

    std::string unicodeJson="{\"updates\":[{\"update_type\":\"message_created\",\"timestamp\":1,\"message\":{\"sender\":{\"user_id\":1},\"recipient\":{\"chat_id\":2,\"chat_type\":\"dialog\"},\"timestamp\":1,\"body\":{\"mid\":\"m\",\"text\":\"\\u041f\\u0440\\u0438\\u0432\\u0435\\u0442\"}}}],\"marker\":2}";
    CHECK(MaxParseUpdates(unicodeJson,up,err));
    CHECK(up.Messages.size()==1 && up.Messages[0].Text=="Привет");

    if(fails) { std::cerr << fails << " test(s) failed\n"; return 1; }
    std::cout << "All maxcore tests passed\n";
    return 0;
}
