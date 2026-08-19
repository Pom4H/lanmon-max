#include "maxclient.h"
#include <sstream>

//Сформировать JSON сообщения с attachment token, полученным после upload
static std::string BuildAttachmentMessageBody(const std::string & text, const std::string & type, const std::string & token)
{
    return std::string("{\"text\":\"") + MaxJsonEscape(text) +
           "\",\"attachments\":[{\"type\":\"" + MaxJsonEscape(type) +
           "\",\"payload\":{\"token\":\"" + MaxJsonEscape(token) + "\"}}]}";
}

//Транспорт по умолчанию не умеет multipart; production Indy переопределяет этот метод
MAX_HTTP_RESPONSE IMaxHttpTransport::PostMultipartFile(const std::string &,
    const std::map<std::string,std::string> &, const std::string &, const std::string &)
{
    MAX_HTTP_RESPONSE r; r.Error="multipart upload is not supported by this transport"; return r;
}

//Создание клиента MAX API
MAX_API_CLIENT::MAX_API_CLIENT(IMaxHttpTransport * transport, const std::string & token, const std::string & baseUrl)
    : Token(token), Transport(transport), HasMarker(false), Marker(0), BaseUrl(baseUrl), LastStatusCode(0)
{
    //Храним BaseUrl без завершающего '/', чтобы URL собирались одинаково
    while(BaseUrl.size()>1 && BaseUrl[BaseUrl.size()-1]=='/') BaseUrl.erase(BaseUrl.size()-1);
}

//Подмена production host на тестовый BaseUrl
std::string MAX_API_CLIENT::WithBaseUrl(const std::string & url) const
{
    const std::string production="https://platform-api2.max.ru";
    if(url.compare(0,production.size(),production)==0)
        return BaseUrl + url.substr(production.size());
    //Upload URL MAX может вести на другой host и должен использоваться без изменений
    return url;
}

//Обязательные HTTP-заголовки MAX Bot API
std::map<std::string,std::string> MAX_API_CLIENT::Headers(bool json) const
{
    std::map<std::string,std::string> h;
    //MAX принимает token только через Authorization
    h["Authorization"]=Token;
    if(json) h["Content-Type"]="application/json";
    return h;
}

//Проверка transport/HTTP результата
bool MAX_API_CLIENT::CheckResponse(const MAX_HTTP_RESPONSE & r, std::string & error)
{
    //Сохраняем последний ответ для свойства MAX_BOT::Json и диагностики UI
    LastStatusCode=r.StatusCode;
    LastResponseBody=r.Body;
    if(!r.Error.empty()) { error=r.Error; return false; }
    if(r.StatusCode<200 || r.StatusCode>=300) {
        std::ostringstream os; os << "MAX HTTP " << r.StatusCode;
        if(!r.Body.empty()) os << ": " << r.Body;
        error=os.str(); return false;
    }
    return true;
}

//Получить информацию о боте
bool MAX_API_CLIENT::GetMe(MAX_BOT_INFO & info, std::string & error)
{
    if(!Transport) { error="MAX transport is null"; LastStatusCode=0; LastResponseBody.clear(); return false; }
    MAX_HTTP_RESPONSE r=Transport->Get(WithBaseUrl("https://platform-api2.max.ru/me"),Headers(false));
    if(!CheckResponse(r,error)) return false;
    //Декодирование JSON ответа /me
    return MaxParseBotInfo(r.Body,info,error);
}

//Получить обновления через Long Polling
bool MAX_API_CLIENT::Poll(MAX_UPDATES & updates, std::string & error, int timeoutSeconds, int limit)
{
    if(!Transport) { error="MAX transport is null"; LastStatusCode=0; LastResponseBody.clear(); return false; }
    //Если marker уже получен, передаём его серверу как курсор следующего чтения
    std::string url=MaxBuildUpdatesUrl(HasMarker,Marker,timeoutSeconds,limit);
    MAX_HTTP_RESPONSE r=Transport->Get(WithBaseUrl(url),Headers(false));
    if(!CheckResponse(r,error)) return false;
    //Получение сообщений из JSON ответа сервера
    if(!MaxParseUpdates(r.Body,updates,error)) return false;
    //Сохранить marker для следующего чтения
    if(updates.HasMarker) { HasMarker=true; Marker=updates.Marker; }
    return true;
}

//Послать текстовое сообщение
bool MAX_API_CLIENT::SendMessage(const MAX_PEER & peer, const std::string & utf8Text, std::string & error)
{
    if(!Transport) { error="MAX transport is null"; LastStatusCode=0; LastResponseBody.clear(); return false; }
    MAX_HTTP_RESPONSE r=Transport->Post(WithBaseUrl(MaxBuildSendMessageUrl(peer)),Headers(true),MaxBuildSendMessageBody(utf8Text));
    return CheckResponse(r,error);
}

//Общий MAX upload flow для изображения и документа
bool MAX_API_CLIENT::SendUploadedAttachment(const MAX_PEER & peer, const std::string & filename,
                               const std::string & utf8Caption, const std::string & uploadType,
                               const std::string & attachmentType, std::string & error)
{
    if(!Transport) { error="MAX transport is null"; LastStatusCode=0; LastResponseBody.clear(); return false; }

    //1. Запросить у MAX URL для загрузки файла
    MAX_HTTP_RESPONSE prepare=Transport->Post(
        WithBaseUrl(std::string("https://platform-api2.max.ru/uploads?type=")+uploadType),Headers(false),"");
    if(!CheckResponse(prepare,error)) return false;

    //2. Получить upload URL из ответа MAX
    std::string uploadUrl;
    if(!MaxParseUploadUrl(prepare.Body,uploadUrl,error)) return false;

    //3. Послать файл multipart/form-data, поле называется "data"
    //Upload URL используем без WithBaseUrl: MAX может вернуть отдельный upload-host
    MAX_HTTP_RESPONSE uploaded=Transport->PostMultipartFile(
        uploadUrl,Headers(false),"data",filename);
    if(!CheckResponse(uploaded,error)) return false;

    //4. Получить token загруженного файла
    std::string token;
    if(!MaxParseUploadToken(uploaded.Body,token,error)) return false;

    //5. Отправить сообщение с attachment.payload.token
    MAX_HTTP_RESPONSE sent=Transport->Post(
        WithBaseUrl(MaxBuildSendMessageUrl(peer)),Headers(true),
        BuildAttachmentMessageBody(utf8Caption,attachmentType,token));
    return CheckResponse(sent,error);
}

//Посылка файла картинки
bool MAX_API_CLIENT::SendImage(const MAX_PEER & peer, const std::string & filename,
                               const std::string & utf8Caption, std::string & error)
{
    //MAX использует upload type=image и attachment type=image
    return SendUploadedAttachment(peer,filename,utf8Caption,"image","image",error);
}

//Посылка файла документа
bool MAX_API_CLIENT::SendFile(const MAX_PEER & peer, const std::string & filename,
                              const std::string & utf8Caption, std::string & error)
{
    //Произвольный документ загружается как file
    return SendUploadedAttachment(peer,filename,utf8Caption,"file","file",error);
}
