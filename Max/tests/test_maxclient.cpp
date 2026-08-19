#include "../maxclient.h"
#include <iostream>
#include <vector>

struct Call { std::string Method,Url,Body; std::map<std::string,std::string> Headers; };
class Mock : public IMaxHttpTransport {
public:
    std::vector<Call> Calls; std::vector<MAX_HTTP_RESPONSE> Responses;
    MAX_HTTP_RESPONSE Next(const char*m,const std::string&u,const std::map<std::string,std::string>&h,const std::string&b){Call c;c.Method=m;c.Url=u;c.Headers=h;c.Body=b;Calls.push_back(c);MAX_HTTP_RESPONSE r=Responses.front();Responses.erase(Responses.begin());return r;}
    MAX_HTTP_RESPONSE Get(const std::string&u,const std::map<std::string,std::string>&h){return Next("GET",u,h,"");}
    MAX_HTTP_RESPONSE Post(const std::string&u,const std::map<std::string,std::string>&h,const std::string&b){return Next("POST",u,h,b);}
};
static int f=0;
#define C(x) do{if(!(x)){std::cerr<<"FAIL line "<<__LINE__<<": "#x"\n";++f;}}while(0)
static MAX_HTTP_RESPONSE R(int code,const char*body){MAX_HTTP_RESPONSE r;r.StatusCode=code;r.Body=body;return r;}
int main(){
    Mock m; MAX_API_CLIENT api(&m,"secret-token"); std::string e;
    m.Responses.push_back(R(200,"{\"user_id\":77,\"first_name\":\"LanMon\",\"is_bot\":true}")); MAX_BOT_INFO bi;
    C(api.GetMe(bi,e)); C(bi.Id==77); C(m.Calls[0].Headers["Authorization"]=="secret-token"); C(m.Calls[0].Url=="https://platform-api2.max.ru/me");
    m.Responses.push_back(R(200,"{\"updates\":[],\"marker\":500}")); MAX_UPDATES up; C(api.Poll(up,e,30,100)); C(api.MarkerValid()&&api.CurrentMarker()==500);
    m.Responses.push_back(R(200,"{\"updates\":[],\"marker\":501}")); C(api.Poll(up,e,30,100)); C(m.Calls[2].Url.find("marker=500")!=std::string::npos); C(api.CurrentMarker()==501);
    m.Responses.push_back(R(200,"{\"message\":{}}")); C(api.SendMessage(MAX_PEER(maxPeerUser,12),"hello \"x\"\n",e)); C(m.Calls[3].Method=="POST"); C(m.Calls[3].Headers["Content-Type"]=="application/json"); C(m.Calls[3].Body=="{\"text\":\"hello \\\"x\\\"\\n\"}");
    m.Responses.push_back(R(401,"{\"code\":\"unauthorized\"}")); C(!api.SendMessage(MAX_PEER(maxPeerChat,33),"x",e)); C(e.find("MAX HTTP 401")!=std::string::npos);
    std::cout<<(f?"maxclient tests FAILED":"All maxclient tests passed")<<"\n"; return f?1:0;
}
