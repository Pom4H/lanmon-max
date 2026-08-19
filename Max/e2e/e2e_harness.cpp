// End-to-end test of MAX_API_CLIENT over a real local TCP/HTTP connection.
#include "../api/maxclient.h"
#include "posix_http_transport.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>

static int failures=0;
#define CHECK(x) do { if(!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " #x "\n"; ++failures; } else { std::cout << "PASS " #x "\n"; } } while(0)

// Create a tiny local file so multipart upload uses real filesystem bytes.
static bool WriteFile(const char * filename,const char * contents)
{
    std::ofstream out(filename,std::ios::out|std::ios::binary);
    if(!out)return false;
    out << contents;
    return out.good();
}

int main(int argc,char **argv)
{
    // CI points BaseUrl to mock_max_server.py instead of production MAX.
    std::string base=argc>1?argv[1]:"http://127.0.0.1:18080";
    TPosixHttpTransport transport;
    MAX_API_CLIENT api(&transport,"e2e-secret-token",base);
    std::string error;

    std::cout << "MAX E2E harness -> " << base << "\n";

    // 1. Real HTTP GET /me through the POSIX transport.
    MAX_BOT_INFO me;
    CHECK(api.GetMe(me,error));
    CHECK(me.Id==9001);
    CHECK(me.UserName=="lanmon_e2e_bot");

    // 2. First Long Poll returns one Russian message and marker=101.
    MAX_UPDATES updates;
    error.clear();
    CHECK(api.Poll(updates,error,30,100));
    CHECK(updates.Messages.size()==1);
    CHECK(api.MarkerValid());
    CHECK(api.CurrentMarker()==101);
    if(updates.Messages.size()==1)
    {
        CHECK(updates.Messages[0].UserId==42);
        CHECK(updates.Messages[0].ChatId==777);
        CHECK(updates.Messages[0].Text=="PING \"MAX\"\nПривет");

        // 3. Reply to the same MAX chat and verify UTF-8/JSON across real TCP.
        error.clear();
        std::string reply="PONG \"LanMon\"\nПривет из E2E";
        CHECK(api.SendMessage(MAX_PEER(maxPeerChat,updates.Messages[0].ChatId),reply,error));
    }

    // 4. Second Long Poll must send marker=101; server advances it to 102.
    error.clear();
    updates.Clear();
    CHECK(api.Poll(updates,error,30,100));
    CHECK(updates.Messages.empty());
    CHECK(api.CurrentMarker()==102);

    // 5. Full image flow over TCP: /uploads -> multipart -> /messages attachment.
    const char * imageFile="e2e-map.png";
    CHECK(WriteFile(imageFile,"PNG-E2E-BYTES\n"));
    error.clear();
    CHECK(api.SendImage(MAX_PEER(maxPeerChat,-777),imageFile,"Карта \"1\"",error));
    std::remove(imageFile);

    // 6. Full document flow over TCP with a different recipient namespace.
    const char * docFile="e2e-alarm.pdf";
    CHECK(WriteFile(docFile,"PDF-E2E-BYTES\n"));
    error.clear();
    CHECK(api.SendFile(MAX_PEER(maxPeerUser,42),docFile,"Тревоги E2E",error));
    std::remove(docFile);

    // 7. HTTP error path must propagate server status and body context.
    error.clear();
    CHECK(!api.SendMessage(MAX_PEER(maxPeerUser,401),"must fail",error));
    CHECK(error.find("MAX HTTP 401")!=std::string::npos);
    CHECK(error.find("unauthorized_test")!=std::string::npos);

    std::cout << (failures?"MAX E2E FAILED":"MAX E2E PASSED") << "\n";
    return failures?1:0;
}
