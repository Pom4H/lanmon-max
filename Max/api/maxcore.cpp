#include "maxcore.h"
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <cstring>
#ifndef __BORLANDC__
#include <vector>
#endif

//---------------------------------------------------------------------------
//Небольшие операции над строкой скрывают отличие AnsiString от std::string.
//В Borland-ветке здесь нет ни одного STL-контейнера.
//---------------------------------------------------------------------------
static int TextLength(const MAX_TEXT & s)
{
#ifdef __BORLANDC__
    return s.Length();
#else
    return (int)s.size();
#endif
}

static bool TextEmpty(const MAX_TEXT & s)
{
    return TextLength(s)==0;
}

static void TextClear(MAX_TEXT & s)
{
#ifdef __BORLANDC__
    s="";
#else
    s.clear();
#endif
}

static MAX_TEXT TextFromBytes(const char * p,int n)
{
#ifdef __BORLANDC__
    MAX_TEXT s;
    if(n<=0)return s;
    s.SetLength(n);
    std::memcpy(&s[1],p,n);
    return s;
#else
    return MAX_TEXT(p,(size_t)n);
#endif
}

static MAX_TEXT IntToText(int value)
{
    char buf[32];
    std::sprintf(buf,"%d",value);
    return MAX_TEXT(buf);
}

//---------------------------------------------------------------------------
//Минимальный JSON parser для C++98.
//В BCB2007 object/array хранятся через TList, а строки через AnsiString.
//Это убирает std::map/std::vector/std::string из production toolchain.
//---------------------------------------------------------------------------
namespace {

enum JsonType { jtNull, jtBool, jtNumber, jtString, jtArray, jtObject };

//Одинаковый список указателей: VCL TList в Builder и std::vector только в Linux CI.
class PtrList
{
#ifdef __BORLANDC__
    TList * FList;
#else
    std::vector<void *> FList;
#endif
public:
    PtrList()
    {
#ifdef __BORLANDC__
        FList=new TList;
#endif
    }
    ~PtrList()
    {
#ifdef __BORLANDC__
        delete FList;
#endif
    }
    int Count() const
    {
#ifdef __BORLANDC__
        return FList->Count;
#else
        return (int)FList.size();
#endif
    }
    void * Get(int index) const
    {
#ifdef __BORLANDC__
        return FList->Items[index];
#else
        return FList[(size_t)index];
#endif
    }
    void Add(void * value)
    {
#ifdef __BORLANDC__
        FList->Add(value);
#else
        FList.push_back(value);
#endif
    }
    void Delete(int index)
    {
#ifdef __BORLANDC__
        FList->Delete(index);
#else
        FList.erase(FList.begin()+index);
#endif
    }
};

struct JsonValue;

struct JsonMember
{
    MAX_TEXT Key;
    JsonValue * Value;
    JsonMember(const MAX_TEXT & key,JsonValue * value) : Key(key),Value(value) {}
};

struct JsonValue
{
    JsonType Type;
    bool BoolValue;
    MAX_TEXT StringValue;
    PtrList ArrayValue;  //JsonValue*
    PtrList ObjectValue; //JsonMember*

    JsonValue() : Type(jtNull), BoolValue(false) {}
    ~JsonValue()
    {
        int i;
        for(i=0;i<ArrayValue.Count();++i)delete (JsonValue *)ArrayValue.Get(i);
        for(i=0;i<ObjectValue.Count();++i)
        {
            JsonMember * m=(JsonMember *)ObjectValue.Get(i);
            delete m->Value;
            delete m;
        }
    }
};

//Добавить Unicode code point в UTF-8 строку
static void AppendUtf8(MAX_TEXT & out,unsigned long cp)
{
    if(cp<=0x7F)out+=(char)cp;
    else if(cp<=0x7FF)
    {
        out+=(char)(0xC0|(cp>>6));
        out+=(char)(0x80|(cp&0x3F));
    }
    else if(cp<=0xFFFF)
    {
        out+=(char)(0xE0|(cp>>12));
        out+=(char)(0x80|((cp>>6)&0x3F));
        out+=(char)(0x80|(cp&0x3F));
    }
    else
    {
        out+=(char)(0xF0|(cp>>18));
        out+=(char)(0x80|((cp>>12)&0x3F));
        out+=(char)(0x80|((cp>>6)&0x3F));
        out+=(char)(0x80|(cp&0x3F));
    }
}

class JsonParser
{
    const char * S;
    int N;
    int P;
    MAX_TEXT Error;
public:
    JsonParser(const MAX_TEXT & s) : S(s.c_str()),N(TextLength(s)),P(0) {}

    //Разобрать один полный JSON документ
    bool Parse(JsonValue & value,MAX_TEXT & error)
    {
        SkipWs();
        if(!ParseValue(&value)){error=Error;return false;}
        SkipWs();
        if(P!=N){error="Unexpected characters after JSON value";return false;}
        return true;
    }
private:
    void SkipWs()
    {
        while(P<N && std::isspace((unsigned char)S[P]))++P;
    }
    bool Fail(const char * msg)
    {
        Error=MAX_TEXT(msg)+" at byte "+IntToText(P);
        return false;
    }
    bool ParseHex4(unsigned long & cp)
    {
        if(P+4>N)return Fail("Short unicode escape");
        cp=0;
        for(int i=0;i<4;i++)
        {
            char h=S[P++];
            cp<<=4;
            if(h>='0'&&h<='9')cp+=(h-'0');
            else if(h>='a'&&h<='f')cp+=(h-'a'+10);
            else if(h>='A'&&h<='F')cp+=(h-'A'+10);
            else return Fail("Invalid unicode escape");
        }
        return true;
    }
    bool ParseValue(JsonValue * v)
    {
        SkipWs();
        if(P>=N)return Fail("Unexpected end of JSON");
        char c=S[P];
        if(c=='{')return ParseObject(v);
        if(c=='[')return ParseArray(v);
        if(c=='\"'){v->Type=jtString;return ParseString(v->StringValue);}
        if(c=='t' && P+4<=N && std::memcmp(S+P,"true",4)==0)
        {P+=4;v->Type=jtBool;v->BoolValue=true;return true;}
        if(c=='f' && P+5<=N && std::memcmp(S+P,"false",5)==0)
        {P+=5;v->Type=jtBool;v->BoolValue=false;return true;}
        if(c=='n' && P+4<=N && std::memcmp(S+P,"null",4)==0)
        {P+=4;v->Type=jtNull;return true;}
        if(c=='-' || (c>='0'&&c<='9'))return ParseNumber(v);
        return Fail("Unexpected JSON token");
    }
    bool ParseString(MAX_TEXT & out)
    {
        if(P>=N || S[P]!='\"')return Fail("Expected string");
        ++P;TextClear(out);
        while(P<N)
        {
            unsigned char c=(unsigned char)S[P++];
            if(c=='\"')return true;
            if(c=='\\')
            {
                if(P>=N)return Fail("Broken escape sequence");
                char e=S[P++];
                switch(e)
                {
                    case '\"':out+='\"';break;
                    case '\\':out+='\\';break;
                    case '/':out+='/';break;
                    case 'b':out+='\b';break;
                    case 'f':out+='\f';break;
                    case 'n':out+='\n';break;
                    case 'r':out+='\r';break;
                    case 't':out+='\t';break;
                    case 'u':
                    {
                        unsigned long cp=0;
                        if(!ParseHex4(cp))return false;
                        if(cp>=0xD800 && cp<=0xDBFF)
                        {
                            if(P+2>N || S[P]!='\\' || S[P+1]!='u')return Fail("Missing low surrogate");
                            P+=2;
                            unsigned long low=0;
                            if(!ParseHex4(low))return false;
                            if(low<0xDC00 || low>0xDFFF)return Fail("Invalid low surrogate");
                            cp=0x10000+((cp-0xD800)<<10)+(low-0xDC00);
                        }
                        else if(cp>=0xDC00 && cp<=0xDFFF)return Fail("Unexpected low surrogate");
                        if(cp>0x10FFFF)return Fail("Unicode code point out of range");
                        AppendUtf8(out,cp);
                        break;
                    }
                    default:return Fail("Invalid escape sequence");
                }
            }
            else
            {
                if(c<0x20)return Fail("Control character in string");
                out+=(char)c;
            }
        }
        return Fail("Unterminated string");
    }
    bool ParseNumber(JsonValue * v)
    {
        int start=P;
        if(S[P]=='-')
        {
            ++P;
            if(P>=N)return Fail("Invalid number");
        }
        if(S[P]=='0')
        {
            ++P;
            if(P<N && std::isdigit((unsigned char)S[P]))return Fail("Leading zero in number");
        }
        else
        {
            if(!(S[P]>='1'&&S[P]<='9'))return Fail("Invalid number");
            while(P<N && std::isdigit((unsigned char)S[P]))++P;
        }
        if(P<N && S[P]=='.')
        {
            ++P;
            if(P>=N || !std::isdigit((unsigned char)S[P]))return Fail("Invalid fraction");
            while(P<N && std::isdigit((unsigned char)S[P]))++P;
        }
        if(P<N && (S[P]=='e'||S[P]=='E'))
        {
            ++P;
            if(P<N && (S[P]=='+'||S[P]=='-'))++P;
            if(P>=N || !std::isdigit((unsigned char)S[P]))return Fail("Invalid exponent");
            while(P<N && std::isdigit((unsigned char)S[P]))++P;
        }
        v->Type=jtNumber;
        v->StringValue=TextFromBytes(S+start,P-start);
        return true;
    }
    bool ParseArray(JsonValue * v)
    {
        v->Type=jtArray;++P;SkipWs();
        if(P<N && S[P]==']'){++P;return true;}
        for(;;)
        {
            JsonValue * item=new JsonValue;
            if(!ParseValue(item)){delete item;return false;}
            v->ArrayValue.Add(item);SkipWs();
            if(P>=N)return Fail("Unterminated array");
            if(S[P]==']'){++P;return true;}
            if(S[P]!=',')return Fail("Expected comma in array");
            ++P;
        }
    }
    bool ParseObject(JsonValue * v)
    {
        v->Type=jtObject;++P;SkipWs();
        if(P<N && S[P]=='}'){++P;return true;}
        for(;;)
        {
            SkipWs();MAX_TEXT key;
            if(!ParseString(key))return false;
            SkipWs();
            if(P>=N || S[P]!=':')return Fail("Expected colon in object");
            ++P;
            JsonValue * item=new JsonValue;
            if(!ParseValue(item)){delete item;return false;}
            v->ObjectValue.Add(new JsonMember(key,item));SkipWs();
            if(P>=N)return Fail("Unterminated object");
            if(S[P]=='}'){++P;return true;}
            if(S[P]!=',')return Fail("Expected comma in object");
            ++P;
        }
    }
};

static const JsonValue * Field(const JsonValue & o,const char * key)
{
    if(o.Type!=jtObject)return 0;
    for(int i=0;i<o.ObjectValue.Count();++i)
    {
        const JsonMember * m=(const JsonMember *)o.ObjectValue.Get(i);
        if(m->Key==key)return m->Value;
    }
    return 0;
}

static MAX_TEXT Str(const JsonValue & o,const char * key)
{
    const JsonValue * v=Field(o,key);
    return (v&&v->Type==jtString)?v->StringValue:MAX_TEXT("");
}

static max_int64 Int64(const JsonValue & o,const char * key,max_int64 def=0)
{
    const JsonValue * v=Field(o,key);
    if(!v || v->Type!=jtNumber)return def;
#ifdef __BORLANDC__
    return _atoi64(v->StringValue.c_str());
#else
    return (max_int64)::strtoll(v->StringValue.c_str(),0,10);
#endif
}

static bool Bool(const JsonValue & o,const char * key,bool def=false)
{
    const JsonValue * v=Field(o,key);
    return (v&&v->Type==jtBool)?v->BoolValue:def;
}

//Скопировать объект User из update/message в нормализованный MAX_MESSAGE.
static void ReadUser(const JsonValue * user,MAX_MESSAGE & message)
{
    if(!user || user->Type!=jtObject)return;
    message.UserId=Int64(*user,"user_id");
    message.FirstName=Str(*user,"first_name");
    message.LastName=Str(*user,"last_name");
    message.UserName=Str(*user,"username");
    message.SenderIsBot=Bool(*user,"is_bot");
}

}
//---------------------------------------------------------------------------
//DTO lifecycle
void MAX_BOT_INFO::Clear()
{
    Id=0;TextClear(FirstName);TextClear(LastName);TextClear(UserName);IsBot=false;
}

MAX_MESSAGE::MAX_MESSAGE()
    : UpdateTimestamp(0),MessageTimestamp(0),ChatId(0),UserId(0),SenderIsBot(false)
{
}

#ifdef __BORLANDC__
MAX_MESSAGE_ARRAY::MAX_MESSAGE_ARRAY(){List=new TList;}
MAX_MESSAGE_ARRAY::MAX_MESSAGE_ARRAY(const MAX_MESSAGE_ARRAY & source){List=new TList;*this=source;}
MAX_MESSAGE_ARRAY::~MAX_MESSAGE_ARRAY(){clear();delete List;}
MAX_MESSAGE_ARRAY & MAX_MESSAGE_ARRAY::operator=(const MAX_MESSAGE_ARRAY & source)
{
    if(this==&source)return *this;
    clear();
    for(int i=0;i<source.size();++i)push_back(source[i]);
    return *this;
}
int MAX_MESSAGE_ARRAY::size() const{return List->Count;}
bool MAX_MESSAGE_ARRAY::empty() const{return List->Count==0;}
void MAX_MESSAGE_ARRAY::clear()
{
    while(List->Count)
    {
        delete (MAX_MESSAGE *)List->Items[0];
        List->Delete(0);
    }
}
void MAX_MESSAGE_ARRAY::push_back(const MAX_MESSAGE & message){List->Add(new MAX_MESSAGE(message));}
MAX_MESSAGE & MAX_MESSAGE_ARRAY::operator[](int index){return *(MAX_MESSAGE *)List->Items[index];}
const MAX_MESSAGE & MAX_MESSAGE_ARRAY::operator[](int index) const{return *(MAX_MESSAGE *)List->Items[index];}
#endif

void MAX_UPDATES::Clear(){HasMarker=false;Marker=0;Messages.clear();}

//Преобразование одного байта Windows-1251 в Unicode code point
static unsigned short Cp1251ToUnicodeCore(unsigned char c)
{
    if(c<0x80)return c;
    if(c>=0xC0)return (unsigned short)(0x0410+(c-0xC0));
    switch(c)
    {
        case 0xA8:return 0x0401;case 0xB8:return 0x0451;
        case 0x80:return 0x0402;case 0x81:return 0x0403;case 0x82:return 0x201A;
        case 0x83:return 0x0453;case 0x84:return 0x201E;case 0x85:return 0x2026;
        case 0x86:return 0x2020;case 0x87:return 0x2021;case 0x88:return 0x20AC;
        case 0x89:return 0x2030;case 0x8A:return 0x0409;case 0x8B:return 0x2039;
        case 0x8C:return 0x040A;case 0x8D:return 0x040C;case 0x8E:return 0x040B;
        case 0x8F:return 0x040F;case 0x90:return 0x0452;case 0x91:return 0x2018;
        case 0x92:return 0x2019;case 0x93:return 0x201C;case 0x94:return 0x201D;
        case 0x95:return 0x2022;case 0x96:return 0x2013;case 0x97:return 0x2014;
        case 0x99:return 0x2122;case 0x9A:return 0x0459;case 0x9B:return 0x203A;
        case 0x9C:return 0x045A;case 0x9D:return 0x045C;case 0x9E:return 0x045B;
        case 0x9F:return 0x045F;case 0xA0:return 0x00A0;case 0xA1:return 0x040E;
        case 0xA2:return 0x045E;case 0xA3:return 0x0408;case 0xA4:return 0x00A4;
        case 0xA5:return 0x0490;case 0xA6:return 0x00A6;case 0xA7:return 0x00A7;
        case 0xA9:return 0x00A9;case 0xAA:return 0x0404;case 0xAB:return 0x00AB;
        case 0xAC:return 0x00AC;case 0xAD:return 0x00AD;case 0xAE:return 0x00AE;
        case 0xAF:return 0x0407;case 0xB0:return 0x00B0;case 0xB1:return 0x00B1;
        case 0xB2:return 0x0406;case 0xB3:return 0x0456;case 0xB4:return 0x0491;
        case 0xB5:return 0x00B5;case 0xB6:return 0x00B6;case 0xB7:return 0x00B7;
        case 0xB9:return 0x2116;case 0xBA:return 0x0454;case 0xBB:return 0x00BB;
        case 0xBC:return 0x0458;case 0xBD:return 0x0405;case 0xBE:return 0x0455;
        case 0xBF:return 0x0457;
    }
    return '?';
}

//Преобразование строки CP1251 LanMon в UTF-8 MAX
MAX_TEXT MaxUtf8FromCp1251(const MAX_TEXT & value)
{
    MAX_TEXT out;
    const char * p=value.c_str();
    int n=TextLength(value);
    for(int i=0;i<n;++i)AppendUtf8(out,Cp1251ToUnicodeCore((unsigned char)p[i]));
    return out;
}

//Экранирование строки для безопасной вставки в JSON
MAX_TEXT MaxJsonEscape(const MAX_TEXT & value)
{
    static const char hex[]="0123456789abcdef";
    MAX_TEXT out;
    const char * p=value.c_str();
    int n=TextLength(value);
    for(int i=0;i<n;++i)
    {
        unsigned char c=(unsigned char)p[i];
        switch(c)
        {
            case '\"':out+="\\\"";break;case '\\':out+="\\\\";break;
            case '\b':out+="\\b";break;case '\f':out+="\\f";break;
            case '\n':out+="\\n";break;case '\r':out+="\\r";break;case '\t':out+="\\t";break;
            default:
                if(c<0x20){out+="\\u00";out+=hex[(c>>4)&15];out+=hex[c&15];}
                else out+=(char)c;
        }
    }
    return out;
}

//Преобразовать 64-битный идентификатор MAX в строку
MAX_TEXT MaxInt64ToString(max_int64 value)
{
    char buf[64];
#ifdef __BORLANDC__
    _i64toa(value,buf,10);
#else
    std::sprintf(buf,"%lld",(long long)value);
#endif
    return MAX_TEXT(buf);
}

//Построить URL Long Polling GET /updates
MAX_TEXT MaxBuildUpdatesUrl(bool hasMarker,max_int64 marker,int timeoutSeconds,int limit)
{
    if(timeoutSeconds<0)timeoutSeconds=0;
    if(timeoutSeconds>90)timeoutSeconds=90;
    if(limit<1)limit=1;
    if(limit>1000)limit=1000;
    MAX_TEXT url="https://platform-api2.max.ru/updates?timeout=";
    url+=IntToText(timeoutSeconds);
    url+="&limit=";
    url+=IntToText(limit);
    if(hasMarker){url+="&marker=";url+=MaxInt64ToString(marker);}
    //Telegram обрабатывал обычные сообщения, приглашение бота и новых участников.
    //В MAX им соответствуют message_created, bot_added и user_added.
    url+="&types=message_created,bot_added,user_added";
    return url;
}

//Построить URL отправки сообщения с правильной адресацией MAX
MAX_TEXT MaxBuildSendMessageUrl(const MAX_PEER & peer)
{
    MAX_TEXT url="https://platform-api2.max.ru/messages?";
    url+=(peer.Type==maxPeerChat)?"chat_id=":"user_id=";
    url+=MaxInt64ToString(peer.Id);
    return url;
}

//Построить JSON текстового сообщения
MAX_TEXT MaxBuildSendMessageBody(const MAX_TEXT & text)
{
    return MAX_TEXT("{\"text\":\"")+MaxJsonEscape(text)+"\"}";
}

//Построить JSON сообщения с image attachment token
MAX_TEXT MaxBuildImageMessageBody(const MAX_TEXT & text,const MAX_TEXT & token)
{
    return MAX_TEXT("{\"text\":\"")+MaxJsonEscape(text)+
        "\",\"attachments\":[{\"type\":\"image\",\"payload\":{\"token\":\""+
        MaxJsonEscape(token)+"\"}}]}";
}

//Получение информации о боте из JSON ответа GET /me
bool MaxParseBotInfo(const MAX_TEXT & json,MAX_BOT_INFO & info,MAX_TEXT & error)
{
    JsonValue root;JsonParser p(json);info.Clear();
    if(!p.Parse(root,error))return false;
    if(root.Type!=jtObject){error="Bot info must be a JSON object";return false;}
    info.Id=Int64(root,"user_id");
    info.FirstName=Str(root,"first_name");
    info.LastName=Str(root,"last_name");
    info.UserName=Str(root,"username");
    info.IsBot=Bool(root,"is_bot",true);
    if(!info.Id){error="Bot info has no user_id";return false;}
    return true;
}

//Получение сообщений и Telegram-equivalent membership events из GET /updates
bool MaxParseUpdates(const MAX_TEXT & json,MAX_UPDATES & updates,MAX_TEXT & error)
{
    JsonValue root;JsonParser p(json);updates.Clear();
    if(!p.Parse(root,error))return false;
    if(root.Type!=jtObject){error="Updates response must be a JSON object";return false;}
    const JsonValue * marker=Field(root,"marker");
    if(marker && marker->Type==jtNumber){updates.HasMarker=true;updates.Marker=Int64(root,"marker");}
    const JsonValue * arr=Field(root,"updates");
    if(!arr || arr->Type!=jtArray){error="Updates response has no updates array";return false;}
    for(int i=0;i<arr->ArrayValue.Count();++i)
    {
        const JsonValue & u=*(const JsonValue *)arr->ArrayValue.Get(i);
        if(u.Type!=jtObject)continue;
        MAX_TEXT updateType=Str(u,"update_type");
        MAX_MESSAGE m;
        m.UpdateType=updateType;
        m.UpdateTimestamp=Int64(u,"timestamp");

        if(updateType=="message_created")
        {
            const JsonValue * msg=Field(u,"message");
            if(!msg || msg->Type!=jtObject)continue;
            m.MessageTimestamp=Int64(*msg,"timestamp");
            ReadUser(Field(*msg,"sender"),m);
            const JsonValue * recipient=Field(*msg,"recipient");
            if(recipient && recipient->Type==jtObject)
            {
                m.ChatId=Int64(*recipient,"chat_id");
                m.ChatType=Str(*recipient,"chat_type");
            }
            const JsonValue * body=Field(*msg,"body");
            if(body && body->Type==jtObject)
            {
                m.MessageId=Str(*body,"mid");
                m.Text=Str(*body,"text");
            }
        }
        else if(updateType=="bot_added" || updateType=="user_added")
        {
            //Эти события заменяют Telegram my_chat_member/new_chat_participant.
            //Оба содержат chat_id и User; у bot_added User — добавивший бота,
            //у user_added — новый участник. Для UI этого достаточно, чтобы
            //показать событие и добавить адресат chat_id в список LanMon.
            m.ChatId=Int64(u,"chat_id");
            m.ChatType=Bool(u,"is_channel",false)?"channel":"chat";
            m.MessageTimestamp=m.UpdateTimestamp;
            ReadUser(Field(u,"user"),m);
            if(!m.ChatId)continue;
        }
        else
        {
            //Остальные MAX events пока не имеют Telegram-usecase в LanMon 4.
            continue;
        }
        updates.Messages.push_back(m);
    }
    return true;
}

//Получить upload URL из ответа POST /uploads
bool MaxParseUploadUrl(const MAX_TEXT & json,MAX_TEXT & url,MAX_TEXT & error)
{
    JsonValue root;JsonParser p(json);TextClear(url);
    if(!p.Parse(root,error))return false;
    if(root.Type!=jtObject){error="Upload URL response must be a JSON object";return false;}
    url=Str(root,"url");
    if(TextEmpty(url)){error="Upload URL response has no url";return false;}
    return true;
}

//Получить attachment token из ответа upload-host
bool MaxParseUploadToken(const MAX_TEXT & json,MAX_TEXT & token,MAX_TEXT & error)
{
    JsonValue root;JsonParser p(json);TextClear(token);
    if(!p.Parse(root,error))return false;
    if(root.Type!=jtObject){error="Upload response must be a JSON object";return false;}
    token=Str(root,"token");
    if(TextEmpty(token)){error="Upload response has no token";return false;}
    return true;
}
