#include "../maxclient.h"
#include "../lanmon_commands.h"
#include "../e2e/posix_http_transport.h"
#include <iostream>
#include <fstream>
#include <cstdio>

static int failures=0;
#define CHECK(x) do { if(!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " #x "\n"; ++failures; } else { std::cout << "PASS " #x "\n"; } } while(0)

class TestLanMonActions : public ILanMonCommandActions
{
public:
    int CloseCount;
    int MapCount;
    int LastMapIndex;
    std::string TempFile;
    TestLanMonActions() : CloseCount(0), MapCount(0), LastMapIndex(-1), TempFile("/tmp/lanmon_max_map_2.png") {}
    ~TestLanMonActions() { std::remove(TempFile.c_str()); }

    virtual bool CloseAlarmWindow(std::string &)
    {
        ++CloseCount;
        return true;
    }

    virtual bool CreateMapImage(int zeroBasedMapIndex, std::string & filename, std::string & error)
    {
        ++MapCount; LastMapIndex=zeroBasedMapIndex;
        std::ofstream f(TempFile.c_str(),std::ios::out|std::ios::binary);
        if(!f) { error="cannot create fake map image"; return false; }
        const char bytes[]="\x89PNG\r\n\x1a\nLANMON_FAKE_PNG_MAP_2";
        f.write(bytes,sizeof(bytes)-1); f.close();
        filename=TempFile;
        return true;
    }
};

int main(int argc,char **argv)
{
    std::string base=argc>1?argv[1]:"http://127.0.0.1:18081";
    TPosixHttpTransport transport;
    MAX_API_CLIENT api(&transport,"lanmon-e2e-token",base);
    TestLanMonActions actions;
    LANMON_MAX_COMMAND_ROUTER router(&api,&actions);
    std::string error;

    std::cout << "LanMon/MAX command E2E -> " << base << "\n";

    for(int command=0;command<3;++command) {
        MAX_UPDATES updates; error.clear();
        CHECK(api.Poll(updates,error,5,10));
        CHECK(updates.Messages.size()==1);
        if(updates.Messages.size()==1) {
            CHECK(router.Handle(updates.Messages[0],error));
            if(!error.empty()) std::cerr << "router error: " << error << "\n";
        }
    }

    CHECK(actions.CloseCount==1);
    CHECK(actions.MapCount==1);
    CHECK(actions.LastMapIndex==1);
    CHECK(api.MarkerValid());
    CHECK(api.CurrentMarker()==203);

    MAX_UPDATES empty; error.clear();
    CHECK(api.Poll(empty,error,5,10));
    CHECK(empty.Messages.empty());

    std::cout << (failures?"LANMON/MAX E2E FAILED":"LANMON/MAX E2E PASSED") << "\n";
    return failures?1:0;
}
