#include "lanmon_commands.h"
#include <cctype>
#include <cstdlib>

static std::string StopDoneText()
{
    return MaxUtf8FromCp1251("\xCA\xEE\xEC\xE0\xED\xE4\xE0 \xD1\xD2\xCE\xCF \xE2\xFB\xEF\xEE\xEB\xED\xE5\xED\xE0");
}

static std::string MapCaption(int mapIndex)
{
    return MaxUtf8FromCp1251("\xCA\xE0\xF0\xF2\xE0 ") + MaxInt64ToString(mapIndex);
}

static std::string HelpText()
{
    return MaxUtf8FromCp1251(
        "\xC2\xEE\xE7\xEC\xEE\xE6\xED\xFB\xE5 \xE7\xE0\xEF\xF0\xEE\xF1\xFB:\n"
        "MAP x - \xEA\xE0\xF0\xF2\xE0 \xED\xEE\xEC\xE5\xF0 x\n"
        "STOP - \xE7\xE0\xEA\xF0\xFB\xF2\xFC \xEE\xEA\xED\xEE \xE0\xE2\xE0\xF0\xE8\xE9\n"
        "HELP (?) - \xEF\xEE\xEC\xEE\xF9\xFC");
}

std::string LANMON_MAX_COMMAND_ROUTER::TrimUpperAscii(const std::string & text)
{
    size_t begin=0;
    while(begin<text.size() && std::isspace((unsigned char)text[begin])) ++begin;
    size_t end=text.size();
    while(end>begin && std::isspace((unsigned char)text[end-1])) --end;
    std::string out=text.substr(begin,end-begin);
    for(size_t i=0;i<out.size();++i) {
        unsigned char c=(unsigned char)out[i];
        if(c>='a' && c<='z') out[i]=(char)(c-'a'+'A');
    }
    return out;
}

int LANMON_MAX_COMMAND_ROUTER::ParsePositiveIndex(const std::string & text, size_t prefixLen)
{
    if(text.size()<=prefixLen) return 0;
    const char * p=text.c_str()+prefixLen;
    while(*p && std::isspace((unsigned char)*p)) ++p;
    int value=std::atoi(p);
    return value>0?value:0;
}

MAX_PEER LANMON_MAX_COMMAND_ROUTER::PeerFor(const MAX_MESSAGE & msg) const
{
    if(msg.ChatId!=0) return MAX_PEER(maxPeerChat,msg.ChatId);
    return MAX_PEER(maxPeerUser,msg.UserId);
}

bool LANMON_MAX_COMMAND_ROUTER::Handle(const MAX_MESSAGE & msg, std::string & error)
{
    if(!Api || !Actions) { error="LanMon MAX router is not initialized"; return false; }
    std::string text=TrimUpperAscii(msg.Text);
    MAX_PEER peer=PeerFor(msg);

    if(text.compare(0,4,"STOP")==0) {
        if(!Actions->CloseAlarmWindow(error)) return false;
        return Api->SendMessage(peer,StopDoneText(),error);
    }

    if(text.compare(0,3,"MAP")==0) {
        int mapIndex=ParsePositiveIndex(text,3);
        if(mapIndex<=0) return true;
        std::string filename;
        if(!Actions->CreateMapImage(mapIndex-1,filename,error)) return false;
        return Api->SendImage(peer,filename,MapCaption(mapIndex),error);
    }

    if(text.compare(0,4,"HELP")==0 || (!text.empty() && text[0]=='?'))
        return Api->SendMessage(peer,HelpText(),error);

    return true;
}
