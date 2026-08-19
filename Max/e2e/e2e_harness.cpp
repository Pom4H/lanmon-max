#include "../api/maxclient.h"
#include "posix_http_transport.h"
#include <iostream>
#include <cstdlib>

static int failures=0;
#define CHECK(x) do { if(!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " #x "\n"; ++failures; } else { std::cout << "PASS " #x "\n"; } } while(0)

int main(int argc,char **argv)
{
    std::string base=argc>1?argv[1]:"http://127.0.0.1:18080";
    TPosixHttpTransport transport;
    MAX_API_CLIENT api(&transport,"e2e-secret-token",base);
    std::string error;

    std::cout << "MAX E2E harness -> " << base << "\n";

    MAX_BOT_INFO me;
    CHECK(api.GetMe(me,error));
    CHECK(me.Id==9001);
    CHECK(me.UserName=="lanmon_e2e_bot");

    MAX_UPDATES updates;
    error.clear();
    CHECK(api.Poll(updates,error,30,100));
    CHECK(updates.Messages.size()==1);
    CHECK(api.MarkerValid());
    CHECK(api.CurrentMarker()==101);
    if(updates.Messages.size()==1) {
        CHECK(updates.Messages[0].UserId==42);
        CHECK(updates.Messages[0].ChatId==777);
        CHECK(updates.Messages[0].Text=="PING \"MAX\"\nПривет");

        error.clear();
        std::string reply="PONG \"LanMon\"\nПривет из E2E";
        CHECK(api.SendMessage(MAX_PEER(maxPeerChat,updates.Messages[0].ChatId),reply,error));
    }

    error.clear();
    updates.Clear();
    CHECK(api.Poll(updates,error,30,100));
    CHECK(updates.Messages.empty());
    CHECK(api.CurrentMarker()==102);

    error.clear();
    CHECK(!api.SendMessage(MAX_PEER(maxPeerUser,401),"must fail",error));
    CHECK(error.find("MAX HTTP 401")!=std::string::npos);

    std::cout << (failures?"MAX E2E FAILED":"MAX E2E PASSED") << "\n";
    return failures?1:0;
}
