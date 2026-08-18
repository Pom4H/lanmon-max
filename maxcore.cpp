#include "maxcore.h"
#include <map>
#include <sstream>
#include <cstdlib>
#include <cctype>

namespace {

enum JsonType { jtNull, jtBool, jtNumber, jtString, jtArray, jtObject };

struct JsonValue
{
    JsonType Type;
    bool BoolValue;
    std::string StringValue;
    std::vector<JsonValue> ArrayValue;
    std::map<std::string, JsonValue> ObjectValue;

    JsonValue() : Type(jtNull), BoolValue(false) {}
};

static void AppendUtf8(std::string & out, unsigned long cp)
{
    if(cp <= 0x7F) out += (char)cp;
    else if(cp <= 0x7FF) {
        out += (char)(0xC0 | (cp >> 6));
        out += (char)(0x80 | (cp & 0x3F));
    } else if(cp <= 0xFFFF) {
        out += (char)(0xE0 | (cp >> 12));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    } else {
        out += (char)(0xF0 | (cp >> 18));
        out += (char)(0x80 | ((cp >> 12) & 0x3F));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    }
}

class JsonParser
{
    const std::string & S;
    size_t P;
    std::string Error;
public:
    JsonParser(const std::string & s) : S(s), P(0) {}

    bool Parse(JsonValue & value, std::string & error)
    {
        SkipWs();
        if(!ParseValue(value)) { error=Error; return false; }
        SkipWs();
        if(P != S.size()) { error="Unexpected characters after JSON value"; return false; }
        return true;
    }
private:
    void SkipWs() { while(P<S.size() && std::isspace((unsigned char)S[P])) ++P; }
    bool Fail(const std::string & msg) {
        std::ostringstream os; os << msg << " at byte " << P; Error=os.str(); return false;
    }
    bool ParseValue(JsonValue & v)
    {
        SkipWs();
        if(P>=S.size()) return Fail("Unexpected end of JSON");
        char c=S[P];
        if(c=='{') return ParseObject(v);
        if(c=='[') return ParseArray(v);
        if(c=='\"') { v.Type=jtString; return ParseString(v.StringValue); }
        if(c=='t' && S.compare(P,4,"true")==0) { P+=4; v.Type=jtBool; v.BoolValue=true; return true; }
        if(c=='f' && S.compare(P,5,"false")==0) { P+=5; v.Type=jtBool; v.BoolValue=false; return true; }
        if(c=='n' && S.compare(P,4,"null")==0) { P+=4; v.Type=jtNull; return true; }
        if(c=='-' || (c>='0' && c<='9')) return ParseNumber(v);
        return Fail("Unexpected JSON token");
    }
    bool ParseString(std::string & out)
    {
        if(P>=S.size() || S[P]!='\"') return Fail("Expected string");
        ++P; out.clear();
        while(P<S.size()) {
            unsigned char c=(unsigned char)S[P++];
            if(c=='\"') return true;
            if(c=='\\') {
                if(P>=S.size()) return Fail("Broken escape sequence");
                char e=S[P++];
                switch(e) {
                    case '\"': out+='\"'; break; case '\\': out+='\\'; break; case '/': out+='/'; break;
                    case 'b': out+='\b'; break; case 'f': out+='\f'; break; case 'n': out+='\n'; break;
                    case 'r': out+='\r'; break; case 't': out+='\t'; break;
                    case 'u': {
                        if(P+4>S.size()) return Fail("Short unicode escape");
                        unsigned long cp=0;
                        for(int i=0;i<4;i++) {
                            char h=S[P++]; cp<<=4;
                            if(h>='0'&&h<='9') cp+=(h-'0');
                            else if(h>='a'&&h<='f') cp+=(h-'a'+10);
                            else if(h>='A'&&h<='F') cp+=(h-'A'+10);
                            else return Fail("Invalid unicode escape");
                        }
                        AppendUtf8(out,cp); break;
                    }
                    default: return Fail("Invalid escape sequence");
                }
            } else {
                if(c<0x20) return Fail("Control character in string");
                out+=(char)c;
            }
        }
        return Fail("Unterminated string");
    }
    bool ParseNumber(JsonValue & v)
    {
        size_t start=P;
        if(S[P]=='-') ++P;
        if(P>=S.size()) return Fail("Invalid number");
        if(S[P]=='0') ++P;
        else {
            if(!(S[P]>='1'&&S[P]<='9')) return Fail("Invalid number");
            while(P<S.size() && std::isdigit((unsigned char)S[P])) ++P;
        }
        if(P<S.size() && S[P]=='.') { ++P; while(P<S.size() && std::isdigit((unsigned char)S[P])) ++P; }
        if(P<S.size() && (S[P]=='e'||S[P]=='E')) {
            ++P; if(P<S.size()&&(S[P]=='+'||S[P]=='-')) ++P;
            while(P<S.size() && std::isdigit((unsigned char)S[P])) ++P;
        }
        v.Type=jtNumber; v.StringValue=S.substr(start,P-start); return true;
    }
    bool ParseArray(JsonValue & v)
    {
        v.Type=jtArray; v.ArrayValue.clear(); ++P; SkipWs();
        if(P<S.size() && S[P]==']') { ++P; return true; }
        for(;;) {
            JsonValue item; if(!ParseValue(item)) return false; v.ArrayValue.push_back(item); SkipWs();
            if(P>=S.size()) return Fail("Unterminated array");
            if(S[P]==']') { ++P; return true; }
            if(S[P]!=',') return Fail("Expected comma in array");
            ++P;
        }
    }
    bool ParseObject(JsonValue & v)
    {
        v.Type=jtObject; v.ObjectValue.clear(); ++P; SkipWs();
        if(P<S.size() && S[P]=='}') { ++P; return true; }
        for(;;) {
            SkipWs(); std::string key; if(!ParseString(key)) return false; SkipWs();
            if(P>=S.size() || S[P]!=':') return Fail("Expected colon in object");
            ++P;
            JsonValue item; if(!ParseValue(item)) return false; v.ObjectValue[key]=item; SkipWs();
            if(P>=S.size()) return Fail("Unterminated object");
            if(S[P]=='}') { ++P; return true; }
            if(S[P]!=',') return Fail("Expected comma in object");
            ++P;
        }
    }
};

static const JsonValue * Field(const JsonValue & o, const char * key)
{
    if(o.Type!=jtObject) return 0;
    std::map<std::string,JsonValue>::const_iterator it=o.ObjectValue.find(key);
    return it==o.ObjectValue.end()?0:&it->second;
}
static std::string Str(const JsonValue & o,const char * key)
{
    const JsonValue *v=Field(o,key); return (v&&v->Type==jtString)?v->StringValue:"";
}
static max_int64 Int64(const JsonValue & o,const char * key,max_int64 def=0)
{
    const JsonValue *v=Field(o,key); if(!v || v->Type!=jtNumber) return def;
#ifdef __BORLANDC__
    return _atoi64(v->StringValue.c_str());
#else
    return (max_int64)std::strtoll(v->StringValue.c_str(),0,10);
#endif
}
static bool Bool(const JsonValue & o,const char * key,bool def=false)
{
    const JsonValue *v=Field(o,key); return (v&&v->Type==jtBool)?v->BoolValue:def;
}
static std::string I64(max_int64 v)
{
    std::ostringstream os; os << v; return os.str();
}

}

void MAX_BOT_INFO::Clear() { Id=0; FirstName.clear(); LastName.clear(); UserName.clear(); IsBot=false; }
MAX_MESSAGE::MAX_MESSAGE() : UpdateTimestamp(0), MessageTimestamp(0), ChatId(0), UserId(0), SenderIsBot(false) {}
void MAX_UPDATES::Clear() { HasMarker=false; Marker=0; Messages.clear(); }

static unsigned short Cp1251ToUnicodeCore(unsigned char c)
{
    if(c<0x80) return c;
    if(c>=0xC0) return (unsigned short)(0x0410 + (c-0xC0));
    switch(c) {
        case 0xA8: return 0x0401; case 0xB8: return 0x0451;
        case 0x80: return 0x0402; case 0x81: return 0x0403; case 0x82: return 0x201A;
        case 0x83: return 0x0453; case 0x84: return 0x201E; case 0x85: return 0x2026;
        case 0x86: return 0x2020; case 0x87: return 0x2021; case 0x88: return 0x20AC;
        case 0x89: return 0x2030; case 0x8A: return 0x0409; case 0x8B: return 0x2039;
        case 0x8C: return 0x040A; case 0x8D: return 0x040C; case 0x8E: return 0x040B;
        case 0x8F: return 0x040F; case 0x90: return 0x0452; case 0x91: return 0x2018;
        case 0x92: return 0x2019; case 0x93: return 0x201C; case 0x94: return 0x201D;
        case 0x95: return 0x2022; case 0x96: return 0x2013; case 0x97: return 0x2014;
        case 0x99: return 0x2122; case 0x9A: return 0x0459; case 0x9B: return 0x203A;
        case 0x9C: return 0x045A; case 0x9D: return 0x045C; case 0x9E: return 0x045B;
        case 0x9F: return 0x045F; case 0xA0: return 0x00A0; case 0xA1: return 0x040E;
        case 0xA2: return 0x045E; case 0xA3: return 0x0408; case 0xA4: return 0x00A4;
        case 0xA5: return 0x0490; case 0xA6: return 0x00A6; case 0xA7: return 0x00A7;
        case 0xA9: return 0x00A9; case 0xAA: return 0x0404; case 0xAB: return 0x00AB;
        case 0xAC: return 0x00AC; case 0xAD: return 0x00AD; case 0xAE: return 0x00AE;
        case 0xAF: return 0x0407; case 0xB0: return 0x00B0; case 0xB1: return 0x00B1;
        case 0xB2: return 0x0406; case 0xB3: return 0x0456; case 0xB4: return 0x0491;
        case 0xB5: return 0x00B5; case 0xB6: return 0x00B6; case 0xB7: return 0x00B7;
        case 0xB9: return 0x2116; case 0xBA: return 0x0454; case 0xBB: return 0x00BB;
        case 0xBC: return 0x0458; case 0xBD: return 0x0405; case 0xBE: return 0x0455;
        case 0xBF: return 0x0457;
    }
    return '?';
}

std::string MaxUtf8FromCp1251(const std::string & value)
{
    std::string out;
    for(size_t i=0;i<value.size();++i) AppendUtf8(out,Cp1251ToUnicodeCore((unsigned char)value[i]));
    return out;
}

std::string MaxJsonEscape(const std::string & value)
{
    static const char hex[]="0123456789abcdef";
    std::string out;
    for(size_t i=0;i<value.size();++i) {
        unsigned char c=(unsigned char)value[i];
        switch(c) {
            case '\"': out+="\\\""; break; case '\\': out+="\\\\"; break;
            case '\b': out+="\\b"; break; case '\f': out+="\\f"; break;
            case '\n': out+="\\n"; break; case '\r': out+="\\r"; break; case '\t': out+="\\t"; break;
            default:
                if(c<0x20) { out+="\\u00"; out+=hex[(c>>4)&15]; out+=hex[c&15]; }
                else out+=(char)c;
        }
    }
    return out;
}

std::string MaxBuildUpdatesUrl(bool hasMarker, max_int64 marker, int timeoutSeconds, int limit)
{
    if(timeoutSeconds<0) timeoutSeconds=0;
    if(timeoutSeconds>90) timeoutSeconds=90;
    if(limit<1) limit=1;
    if(limit>1000) limit=1000;
    std::ostringstream os;
    os << "https://platform-api2.max.ru/updates?timeout=" << timeoutSeconds << "&limit=" << limit;
    if(hasMarker) os << "&marker=" << marker;
    os << "&types=message_created";
    return os.str();
}

std::string MaxBuildSendMessageUrl(const MAX_PEER & peer)
{
    std::string u="https://platform-api2.max.ru/messages?";
    u += peer.Type==maxPeerChat ? "chat_id=" : "user_id=";
    u += I64(peer.Id); return u;
}

std::string MaxBuildSendMessageBody(const std::string & text)
{
    return std::string("{\"text\":\"") + MaxJsonEscape(text) + "\"}";
}

bool MaxParseBotInfo(const std::string & json, MAX_BOT_INFO & info, std::string & error)
{
    JsonValue root; JsonParser p(json); info.Clear();
    if(!p.Parse(root,error)) return false;
    if(root.Type!=jtObject) { error="Bot info must be a JSON object"; return false; }
    info.Id=Int64(root,"user_id");
    info.FirstName=Str(root,"first_name"); info.LastName=Str(root,"last_name"); info.UserName=Str(root,"username");
    info.IsBot=Bool(root,"is_bot",true);
    if(!info.Id) { error="Bot info has no user_id"; return false; }
    return true;
}

bool MaxParseUpdates(const std::string & json, MAX_UPDATES & updates, std::string & error)
{
    JsonValue root; JsonParser p(json); updates.Clear();
    if(!p.Parse(root,error)) return false;
    if(root.Type!=jtObject) { error="Updates response must be a JSON object"; return false; }
    const JsonValue * marker=Field(root,"marker");
    if(marker && marker->Type==jtNumber) { updates.HasMarker=true; updates.Marker=Int64(root,"marker"); }
    const JsonValue * arr=Field(root,"updates");
    if(!arr || arr->Type!=jtArray) { error="Updates response has no updates array"; return false; }
    for(size_t i=0;i<arr->ArrayValue.size();++i) {
        const JsonValue & u=arr->ArrayValue[i];
        if(u.Type!=jtObject) continue;
        if(Str(u,"update_type")!="message_created") continue;
        const JsonValue * msg=Field(u,"message"); if(!msg || msg->Type!=jtObject) continue;
        MAX_MESSAGE m; m.UpdateType="message_created"; m.UpdateTimestamp=Int64(u,"timestamp");
        m.MessageTimestamp=Int64(*msg,"timestamp");
        const JsonValue * sender=Field(*msg,"sender");
        if(sender && sender->Type==jtObject) {
            m.UserId=Int64(*sender,"user_id"); m.FirstName=Str(*sender,"first_name");
            m.LastName=Str(*sender,"last_name"); m.UserName=Str(*sender,"username"); m.SenderIsBot=Bool(*sender,"is_bot");
        }
        const JsonValue * recipient=Field(*msg,"recipient");
        if(recipient && recipient->Type==jtObject) { m.ChatId=Int64(*recipient,"chat_id"); m.ChatType=Str(*recipient,"chat_type"); }
        const JsonValue * body=Field(*msg,"body");
        if(body && body->Type==jtObject) { m.MessageId=Str(*body,"mid"); m.Text=Str(*body,"text"); }
        updates.Messages.push_back(m);
    }
    return true;
}

std::string MaxInt64ToString(max_int64 value)
{
    return I64(value);
}

std::string MaxBuildImageMessageBody(const std::string & text, const std::string & token)
{
    return std::string("{\"text\":\"") + MaxJsonEscape(text) +
           "\",\"attachments\":[{\"type\":\"image\",\"payload\":{\"token\":\"" +
           MaxJsonEscape(token) + "\"}}]}";
}

bool MaxParseUploadUrl(const std::string & json, std::string & url, std::string & error)
{
    JsonValue root; JsonParser p(json); url.clear();
    if(!p.Parse(root,error)) return false;
    if(root.Type!=jtObject) { error="Upload URL response must be a JSON object"; return false; }
    url=Str(root,"url");
    if(url.empty()) { error="Upload URL response has no url"; return false; }
    return true;
}

bool MaxParseUploadToken(const std::string & json, std::string & token, std::string & error)
{
    JsonValue root; JsonParser p(json); token.clear();
    if(!p.Parse(root,error)) return false;
    if(root.Type!=jtObject) { error="Upload response must be a JSON object"; return false; }
    token=Str(root,"token");
    if(token.empty()) { error="Upload response has no token"; return false; }
    return true;
}
