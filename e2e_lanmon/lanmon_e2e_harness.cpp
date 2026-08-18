#include "../maxclient.h"
#include "../lanmon_bot.h"
#include "../e2e/posix_http_transport.h"
#include <iostream>
#include <fstream>

static int failures=0;
#define CHECK(x) do { if(!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " #x "\n"; ++failures; } else { std::cout << "PASS " #x "\n"; } } while(0)

class TestLanMonActions : public ILanMonCommandActions
{
public:
    int CloseCount,MapCount,MonitorCount,DesktopCount,HtmlCount,XlsCount,PdfCount,LastMapIndex,LastMonitorIndex;
    TestLanMonActions():CloseCount(0),MapCount(0),MonitorCount(0),DesktopCount(0),HtmlCount(0),XlsCount(0),PdfCount(0),LastMapIndex(-1),LastMonitorIndex(-2){}
    bool Make(const std::string &name,const std::string &data,std::string &fn,std::string &error){fn="/tmp/"+name;std::ofstream f(fn.c_str(),std::ios::binary);if(!f){error="cannot create "+fn;return false;}f<<data;return true;}
    virtual bool CloseAlarmWindow(std::string &){++CloseCount;return true;}
    virtual bool CreateMapImage(int i,std::string&fn,std::string&e){++MapCount;LastMapIndex=i;return Make("lm_map.png","PNG_MAP",fn,e);}
    virtual bool CreateMonitorImage(int i,std::string&fn,std::string&e){++MonitorCount;LastMonitorIndex=i;if(i<0)return false;return Make("lm_screen.jpg","JPG_SCREEN",fn,e);}
    virtual bool CreateDesktopImage(std::string&fn,std::string&e){++DesktopCount;return Make("lm_desktop.jpg","JPG_DESKTOP",fn,e);}
    virtual bool ExportLogHtml(std::string&fn,std::string&e){++HtmlCount;return Make("lm_log.html","HTML_LOG",fn,e);}
    virtual bool ExportLogXls(std::string&fn,std::string&e){++XlsCount;return Make("lm_log.xls","XLS_LOG",fn,e);}
    virtual bool CreateAlarmsPdf(std::string&fn,std::string&e){++PdfCount;return Make("lm_alarm.pdf","PDF_ALARM",fn,e);}
    virtual std::string CurrentDateTimeText()const{return "18.08.2026 19:00";}
};
class Events:public ILanMonMaxEvents{public:int Messages;Events():Messages(0){}void OnMaxMessage(max_int64,max_int64,const std::string&){++Messages;}};

int main(int argc,char **argv)
{
    std::string base=argc>1?argv[1]:"http://127.0.0.1:18081";
    TPosixHttpTransport transport; MAX_API_CLIENT api(&transport,"lanmon-e2e-token",base); TestLanMonActions actions; Events events;
    LANMON_MAX_BOT bot(&api,&actions,&events); bot.Settings.Active=true;bot.Settings.FlagSendMaps=true;bot.Settings.RequestAlias="OPS!";bot.Settings.AlarmAlias="AL!!";
    MAX_USER op;op.Id=777;op.Name="Operator";op.Alias="OPS1";bot.UserList.Add(op);
    MAX_USER alarm;alarm.Id=778;alarm.Name="Alarm receiver";alarm.Alias="AL01";bot.UserList.Add(alarm);
    std::string error;
    std::cout << "LanMon/MAX full parity E2E -> " << base << "\n";
    for(int command=0;command<8;++command){CHECK(bot.ReadMessages(false,error,5,10));if(!error.empty())std::cerr<<error<<"\n";}
    CHECK(events.Messages==8);CHECK(actions.CloseCount==1);CHECK(actions.MapCount==1);CHECK(actions.LastMapIndex==1);
    CHECK(actions.MonitorCount==2);CHECK(actions.LastMonitorIndex==-1);CHECK(actions.DesktopCount==1);
    CHECK(actions.HtmlCount==1);CHECK(actions.XlsCount==1);CHECK(actions.PdfCount==1);CHECK(bot.UserMessageCount==8);
    CHECK(api.MarkerValid());CHECK(api.CurrentMarker()==208);
    CHECK(bot.OnNewAlarmState("ALARM BROADCAST",error));CHECK(bot.UserList.Get(1)->OutCount==1);
    std::cout << (failures?"LANMON/MAX FULL PARITY E2E FAILED":"LANMON/MAX FULL PARITY E2E PASSED") << "\n";
    return failures?1:0;
}
