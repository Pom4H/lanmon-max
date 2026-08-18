#include "../lanmon_bot.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <cstdio>

struct Call { std::string Method,Url,Body,File; };
class MockTransport: public IMaxHttpTransport {
public:
 std::vector<Call> Calls;
 MAX_HTTP_RESPONSE Ok(const std::string &b="{}") { MAX_HTTP_RESPONSE r;r.StatusCode=200;r.Body=b;return r; }
 virtual MAX_HTTP_RESPONSE Get(const std::string &u,const std::map<std::string,std::string>&){Call c;c.Method="GET";c.Url=u;Calls.push_back(c);return Ok("{\"updates\":[],\"marker\":1}");}
 virtual MAX_HTTP_RESPONSE Post(const std::string &u,const std::map<std::string,std::string>&,const std::string&b){Call c;c.Method="POST";c.Url=u;c.Body=b;Calls.push_back(c);if(u.find("/uploads?")!=std::string::npos)return Ok("{\"url\":\"http://upload.local/x\"}");return Ok();}
 virtual MAX_HTTP_RESPONSE PostMultipartFile(const std::string &u,const std::map<std::string,std::string>&,const std::string&,const std::string&f){Call c;c.Method="MULTIPART";c.Url=u;c.File=f;Calls.push_back(c);return Ok("{\"token\":\"tok\"}");}
};
class Actions: public ILanMonCommandActions {
public:
 int stop,map,monitor,desktop,html,xls,pdf; int last;
 Actions():stop(0),map(0),monitor(0),desktop(0),html(0),xls(0),pdf(0),last(-99){}
 bool File(const char*n,std::string&fn){fn=std::string("/tmp/")+n;std::ofstream f(fn.c_str());f<<"x";return true;}
 virtual bool CloseAlarmWindow(std::string&){++stop;return true;}
 virtual bool CreateMapImage(int i,std::string&fn,std::string&){++map;last=i;return File("map.png",fn);}
 virtual bool CreateMonitorImage(int i,std::string&fn,std::string&){++monitor;last=i;if(i<0)return false;return File("screen.jpg",fn);}
 virtual bool CreateDesktopImage(std::string&fn,std::string&){++desktop;return File("desktop.jpg",fn);}
 virtual bool ExportLogHtml(std::string&fn,std::string&){++html;return File("log.html",fn);}
 virtual bool ExportLogXls(std::string&fn,std::string&){++xls;return File("log.xls",fn);}
 virtual bool CreateAlarmsPdf(std::string&fn,std::string&){++pdf;return File("alarm.pdf",fn);}
 virtual std::string CurrentDateTimeText()const{return "18.08.2026 19:00";}
};
class Events: public ILanMonMaxEvents { public:int messages;std::string text; Events():messages(0){} void OnMaxMessage(max_int64,max_int64,const std::string&t){++messages;text=t;} };
static int F=0;
#define C(x) do{if(!(x)){std::cerr<<"FAIL "<<__LINE__<<": "#x"\n";++F;}}while(0)
static MAX_MESSAGE M(const std::string&t){MAX_MESSAGE m;m.ChatId=777;m.UserId=42;m.Text=t;m.UpdateTimestamp=99;return m;}
int main(){
 MockTransport tr; MAX_API_CLIENT api(&tr,"t"); Actions a; Events ev; LANMON_MAX_BOT bot(&api,&a,&ev); std::string e;
 MAX_USER u;u.Id=777;u.PeerType=maxPeerChat;u.Name="Operator chat";u.Alias="ABCD";bot.UserList.Add(u);
 C(bot.UserList.Get(0)->HasValidAlias(""));C(bot.UserList.Get(0)->HasValidAlias("*"));C(bot.UserList.Get(0)->HasValidAlias("A!!D"));C(!bot.UserList.Get(0)->HasValidAlias("A!!X"));
 bot.Settings.RequestAlias="A!!D";bot.Settings.AlarmAlias="A!!!";bot.Settings.FlagSendMaps=true;
 std::vector<std::string> cmds;cmds.push_back("STOP");cmds.push_back("MAP 2");cmds.push_back("SCREEN 1");cmds.push_back("SCREEN");cmds.push_back("LOG");cmds.push_back("LOGXLS");cmds.push_back("ALARM");cmds.push_back("HELP");cmds.push_back("стоп");cmds.push_back("карта 3");cmds.push_back("экран 1");cmds.push_back("журнал");cmds.push_back("тревоги");
 for(size_t i=0;i<cmds.size();++i){MAX_UPDATES up;up.Messages.push_back(M(cmds[i]));C(bot.OnMessages(up,e));}
 C(ev.messages==(int)cmds.size());C(a.stop==2);C(a.map==2);C(a.monitor==3);C(a.desktop==1);C(a.html==2);C(a.xls==1);C(a.pdf==2);
 C(bot.UserMessageCount==cmds.size());C(bot.UserList.Get(0)->InCount==cmds.size());
 bot.Settings.RequestAlias="NO";MAX_UPDATES unauth;unauth.Messages.push_back(M("STOP"));C(bot.OnMessages(unauth,e));C(ev.messages==(int)cmds.size()+1);C(a.stop==2);
 bot.Settings.RequestAlias="";bot.Settings.FlagSendMaps=false;MAX_UPDATES gated;gated.Messages.push_back(M("STOP"));C(bot.OnMessages(gated,e));C(a.stop==2);bot.Settings.FlagSendMaps=true;
 size_t alarmBefore=tr.Calls.size();C(bot.OnNewAlarmState("ALARM!",e));C(bot.UserList.Get(0)->OutCount>0);bool alarmChat=false;for(size_t i=alarmBefore;i<tr.Calls.size();++i)if(tr.Calls[i].Url.find("chat_id=777")!=std::string::npos)alarmChat=true;C(alarmChat);
 C(bot.UserCanAsk(0));C(bot.UserRcvAlarms(0));C(bot.UserHasValidAlias(0,"A!!D"));C(bot.SetUserTag(0,7));C(bot.GetUser(0)->Tag==7);C(bot.FindUser(777)!=0);C(bot.FindUserIndex(777)==0);C(bot.FindUserAlias("ABCD")!=0);
 size_t before=tr.Calls.size();C(bot.SendDocByAlias("123","/tmp/alarm.pdf","doc",e));bool sawFile=false,sawUser=false;for(size_t i=before;i<tr.Calls.size();++i){if(tr.Calls[i].Url.find("/uploads?type=file")!=std::string::npos)sawFile=true;if(tr.Calls[i].Url.find("user_id=123")!=std::string::npos)sawUser=true;}C(sawFile);C(sawUser);
 const char*ini="/tmp/lanmon_max_parity.ini";bot.Settings.BotToken="secret";C(bot.Save(ini,e));MAX_BOT_SETTINGS s2;MAX_USER_LIST u2;C(MaxLoadIni(ini,s2,u2,e));C(s2.BotToken=="secret");C(s2.RequestAlias==bot.Settings.RequestAlias);C(u2.Count()==1);C(u2.Get(0)->Tag==7);C(u2.Get(0)->PeerType==maxPeerChat);std::remove(ini);
 std::cout<<(F?"parity tests FAILED":"All Telegram parity tests passed")<<"\n";return F?1:0;
}
