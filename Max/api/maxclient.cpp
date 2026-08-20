#include "maxclient.h"

//---------------------------------------------------------------------------
//Локальные строковые helpers позволяют одному client-коду собираться и с
//AnsiString (BCB2007), и со std::string (Linux CI).
//---------------------------------------------------------------------------
static int ClientTextLength(const MAX_TEXT & s)
{
#ifdef __BORLANDC__
    return s.Length();
#else
    return (int)s.size();
#endif
}

static bool ClientTextEmpty(const MAX_TEXT & s){return ClientTextLength(s)==0;}

static void ClientTextClear(MAX_TEXT & s)
{
#ifdef __BORLANDC__
    s="";
#else
    s.clear();
#endif
}

static bool ClientTextStartsWith(const MAX_TEXT & value,const char * prefix)
{
    int plen=0;while(prefix[plen])++plen;
    if(ClientTextLength(value)<plen)return false;
    const char * p=value.c_str();
    for(int i=0;i<plen;i++)if(p[i]!=prefix[i])return false;
    return true;
}

static MAX_TEXT ClientTextSubstr(const MAX_TEXT & value,int start)
{
#ifdef __BORLANDC__
    int len=value.Length()-start;
    return len>0?value.SubString(start+1,len):MAX_TEXT("");
#else
    return value.substr((size_t)start);
#endif
}

static bool ClientTextContains(const MAX_TEXT & value,const char * needle)
{
#ifdef __BORLANDC__
    return value.Pos(needle)>0;
#else
    return value.find(needle)!=MAX_TEXT::npos;
#endif
}

static void ClientTextRemoveLast(MAX_TEXT & value)
{
#ifdef __BORLANDC__
    if(value.Length())value.Delete(value.Length(),1);
#else
    if(!value.empty())value.erase(value.size()-1);
#endif
}

#ifdef __BORLANDC__
//---------------------------------------------------------------------------
//TStringList-backed headers for C++Builder 2007.
MAX_HTTP_HEADERS::MAX_HTTP_HEADERS(){Items=new TStringList;}
MAX_HTTP_HEADERS::MAX_HTTP_HEADERS(const MAX_HTTP_HEADERS & source){Items=new TStringList;Items->Assign(source.Items);}
MAX_HTTP_HEADERS::~MAX_HTTP_HEADERS(){delete Items;}
MAX_HTTP_HEADERS & MAX_HTTP_HEADERS::operator=(const MAX_HTTP_HEADERS & source)
{
    if(this!=&source)Items->Assign(source.Items);
    return *this;
}
void MAX_HTTP_HEADERS::Clear(){Items->Clear();}
void MAX_HTTP_HEADERS::Set(const MAX_TEXT & name,const MAX_TEXT & value){Items->Values[name]=value;}
bool MAX_HTTP_HEADERS::Has(const MAX_TEXT & name) const{return Items->IndexOfName(name)>=0;}
MAX_TEXT MAX_HTTP_HEADERS::Get(const MAX_TEXT & name) const{return Items->Values[name];}
int MAX_HTTP_HEADERS::Count() const{return Items->Count;}
MAX_TEXT MAX_HTTP_HEADERS::Name(int index) const{return Items->Names[index];}
MAX_TEXT MAX_HTTP_HEADERS::Value(int index) const{return Items->ValueFromIndex[index];}
#endif

static void HeaderSet(MAX_HTTP_HEADERS & headers,const MAX_TEXT & name,const MAX_TEXT & value)
{
#ifdef __BORLANDC__
    headers.Set(name,value);
#else
    headers[name]=value;
#endif
}

//Сформировать JSON сообщения с attachment token, полученным после upload
static MAX_TEXT BuildAttachmentMessageBody(const MAX_TEXT & text,const MAX_TEXT & type,const MAX_TEXT & token)
{
    return MAX_TEXT("{\"text\":\"")+MaxJsonEscape(text)+
        "\",\"attachments\":[{\"type\":\""+MaxJsonEscape(type)+
        "\",\"payload\":{\"token\":\""+MaxJsonEscape(token)+"\"}}]}";
}

//MAX может временно вернуть attachment.not.ready сразу после upload
static bool IsAttachmentNotReady(const MAX_HTTP_RESPONSE & response)
{
    return ClientTextEmpty(response.Error) && ClientTextContains(response.Body,"attachment.not.ready");
}

//Транспорт по умолчанию не умеет multipart; production VCL transport переопределяет этот метод
MAX_HTTP_RESPONSE IMaxHttpTransport::PostMultipartFile(const MAX_TEXT &,
    const MAX_HTTP_HEADERS &,const MAX_TEXT &,const MAX_TEXT &)
{
    MAX_HTTP_RESPONSE r;
    r.Error="multipart upload is not supported by this transport";
    return r;
}

//По умолчанию тестовый transport не обязан реально ждать
void IMaxHttpTransport::SleepMilliseconds(unsigned int)
{
}

//Создание клиента MAX API
MAX_API_CLIENT::MAX_API_CLIENT(IMaxHttpTransport * transport,const MAX_TEXT & token,const MAX_TEXT & baseUrl)
    : Token(token),Transport(transport),HasMarker(false),Marker(0),BaseUrl(baseUrl),LastStatusCode(0)
{
    //Храним BaseUrl без завершающего '/', чтобы URL собирались одинаково
    while(ClientTextLength(BaseUrl)>1 && BaseUrl.c_str()[ClientTextLength(BaseUrl)-1]=='/')ClientTextRemoveLast(BaseUrl);
}

//Подмена production host на тестовый BaseUrl
MAX_TEXT MAX_API_CLIENT::WithBaseUrl(const MAX_TEXT & url) const
{
    const char * production="https://platform-api2.max.ru";
    const int productionLength=28;
    if(ClientTextStartsWith(url,production))return BaseUrl+ClientTextSubstr(url,productionLength);
    //Upload URL MAX может вести на другой host и должен использоваться без изменений
    return url;
}

//Обязательные HTTP-заголовки MAX Bot API
MAX_HTTP_HEADERS MAX_API_CLIENT::Headers(bool json) const
{
    MAX_HTTP_HEADERS h;
    //MAX принимает token только через Authorization
    HeaderSet(h,"Authorization",Token);
    if(json)HeaderSet(h,"Content-Type","application/json");
    return h;
}

//Проверка transport/HTTP результата
bool MAX_API_CLIENT::CheckResponse(const MAX_HTTP_RESPONSE & r,MAX_TEXT & error)
{
    //Успешная новая операция не должна оставлять текст старой ошибки
    ClientTextClear(error);
    //Сохраняем последний ответ для свойства MAX_BOT::Json и диагностики UI
    LastStatusCode=r.StatusCode;
    LastResponseBody=r.Body;
    if(!ClientTextEmpty(r.Error))
    {
        error=r.Error;
        return false;
    }
    if(r.StatusCode<200 || r.StatusCode>=300)
    {
        error=MAX_TEXT("MAX HTTP ")+MaxInt64ToString((max_int64)r.StatusCode);
        if(!ClientTextEmpty(r.Body))error+=MAX_TEXT(": ")+r.Body;
        return false;
    }
    return true;
}

//Получить информацию о боте
bool MAX_API_CLIENT::GetMe(MAX_BOT_INFO & info,MAX_TEXT & error)
{
    if(!Transport)
    {
        error="MAX transport is null";
        LastStatusCode=0;
        ClientTextClear(LastResponseBody);
        return false;
    }
    MAX_HTTP_RESPONSE r=Transport->Get(WithBaseUrl("https://platform-api2.max.ru/me"),Headers(false));
    if(!CheckResponse(r,error))return false;
    //Декодирование JSON ответа /me
    return MaxParseBotInfo(r.Body,info,error);
}

//Получить обновления через Long Polling
bool MAX_API_CLIENT::Poll(MAX_UPDATES & updates,MAX_TEXT & error,int timeoutSeconds,int limit)
{
    if(!Transport)
    {
        error="MAX transport is null";
        LastStatusCode=0;
        ClientTextClear(LastResponseBody);
        return false;
    }
    MAX_TEXT url=MaxBuildUpdatesUrl(HasMarker,Marker,timeoutSeconds,limit);
    MAX_HTTP_RESPONSE r=Transport->Get(WithBaseUrl(url),Headers(false));
    if(!CheckResponse(r,error))return false;
    if(!MaxParseUpdates(r.Body,updates,error))return false;
    if(updates.HasMarker){HasMarker=true;Marker=updates.Marker;}
    return true;
}

//Послать текстовое сообщение
bool MAX_API_CLIENT::SendMessage(const MAX_PEER & peer,const MAX_TEXT & utf8Text,MAX_TEXT & error)
{
    if(!Transport)
    {
        error="MAX transport is null";
        LastStatusCode=0;
        ClientTextClear(LastResponseBody);
        return false;
    }
    MAX_HTTP_RESPONSE r=Transport->Post(
        WithBaseUrl(MaxBuildSendMessageUrl(peer)),Headers(true),MaxBuildSendMessageBody(utf8Text));
    return CheckResponse(r,error);
}

//Общий MAX upload flow для изображения и документа
bool MAX_API_CLIENT::SendUploadedAttachment(const MAX_PEER & peer,const MAX_TEXT & filename,
    const MAX_TEXT & utf8Caption,const MAX_TEXT & uploadType,const MAX_TEXT & attachmentType,MAX_TEXT & error)
{
    if(!Transport)
    {
        error="MAX transport is null";
        LastStatusCode=0;
        ClientTextClear(LastResponseBody);
        return false;
    }

    //1. Запросить у MAX URL для загрузки файла
    MAX_HTTP_RESPONSE prepare=Transport->Post(
        WithBaseUrl(MAX_TEXT("https://platform-api2.max.ru/uploads?type=")+uploadType),Headers(false),"");
    if(!CheckResponse(prepare,error))return false;

    //2. Получить upload URL из ответа MAX
    MAX_TEXT uploadUrl;
    if(!MaxParseUploadUrl(prepare.Body,uploadUrl,error))return false;

    //3. Послать файл multipart/form-data. Bot token на upload-host не передаётся.
    MAX_HTTP_HEADERS uploadHeaders;
    MAX_HTTP_RESPONSE uploaded=Transport->PostMultipartFile(uploadUrl,uploadHeaders,"data",filename);
    if(!CheckResponse(uploaded,error))return false;

    //4. Получить token загруженного файла
    MAX_TEXT token;
    if(!MaxParseUploadToken(uploaded.Body,token,error))return false;

    //5. Отправить сообщение с attachment.payload.token.
    const unsigned int retryDelayMs[]={500,1000,2000};
    const int maxAttempts=4;
    MAX_TEXT body=BuildAttachmentMessageBody(utf8Caption,attachmentType,token);
    for(int attempt=0;attempt<maxAttempts;attempt++)
    {
        MAX_HTTP_RESPONSE sent=Transport->Post(WithBaseUrl(MaxBuildSendMessageUrl(peer)),Headers(true),body);
        if(CheckResponse(sent,error))return true;
        if(!IsAttachmentNotReady(sent) || attempt==maxAttempts-1)return false;
        //Интервал растёт между повторами: 0.5s -> 1s -> 2s
        Transport->SleepMilliseconds(retryDelayMs[attempt]);
    }
    return false;
}

//Посылка файла картинки
bool MAX_API_CLIENT::SendImage(const MAX_PEER & peer,const MAX_TEXT & filename,const MAX_TEXT & utf8Caption,MAX_TEXT & error)
{
    return SendUploadedAttachment(peer,filename,utf8Caption,"image","image",error);
}

//Посылка файла документа
bool MAX_API_CLIENT::SendFile(const MAX_PEER & peer,const MAX_TEXT & filename,const MAX_TEXT & utf8Caption,MAX_TEXT & error)
{
    return SendUploadedAttachment(peer,filename,utf8Caption,"file","file",error);
}
