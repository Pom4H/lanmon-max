#include "maxclient.h"
#include <sstream>

static std::string BuildAttachmentMessageBody(const std::string & text, const std::string & type, const std::string & token)
{
    return std::string("{\"text\":\"") + MaxJsonEscape(text) +
           "\",\"attachments\":[{\"type\":\"" + MaxJsonEscape(type) +
           "\",\"payload\":{\"token\":\"" + MaxJsonEscape(token) + "\"}}]}";
}

MAX_HTTP_RESPONSE IMaxHttpTransport::PostMultipartFile(const std::string &,
    const std::map<std::string,std::string> &, const std::string &, const std::string &)
{
    MAX_HTTP_RESPONSE r; r.Error="multipart upload is not supported by this transport"; return r;
}

MAX_API_CLIENT::MAX_API_CLIENT(IMaxHttpTransport * transport, const std::string & token, const std::string & baseUrl)
    : Token(token), Transport(transport), HasMarker(false), Marker(0), BaseUrl(baseUrl)
{
    while(BaseUrl.size()>1 && BaseUrl[BaseUrl.size()-1]=='/') BaseUrl.erase(BaseUrl.size()-1);
}

std::string MAX_API_CLIENT::WithBaseUrl(const std::string & url) const
{
    const std::string production="https://platform-api2.max.ru";
    if(url.compare(0,production.size(),production)==0)
        return BaseUrl + url.substr(production.size());
    return url;
}

std::map<std::string,std::string> MAX_API_CLIENT::Headers(bool json) const
{
    std::map<std::string,std::string> h;
    h["Authorization"]=Token;
    if(json) h["Content-Type"]="application/json";
    return h;
}

bool MAX_API_CLIENT::CheckResponse(const MAX_HTTP_RESPONSE & r, std::string & error) const
{
    if(!r.Error.empty()) { error=r.Error; return false; }
    if(r.StatusCode<200 || r.StatusCode>=300) {
        std::ostringstream os; os << "MAX HTTP " << r.StatusCode;
        if(!r.Body.empty()) os << ": " << r.Body;
        error=os.str(); return false;
    }
    return true;
}

bool MAX_API_CLIENT::GetMe(MAX_BOT_INFO & info, std::string & error)
{
    if(!Transport) { error="MAX transport is null"; return false; }
    MAX_HTTP_RESPONSE r=Transport->Get(WithBaseUrl("https://platform-api2.max.ru/me"),Headers(false));
    if(!CheckResponse(r,error)) return false;
    return MaxParseBotInfo(r.Body,info,error);
}

bool MAX_API_CLIENT::Poll(MAX_UPDATES & updates, std::string & error, int timeoutSeconds, int limit)
{
    if(!Transport) { error="MAX transport is null"; return false; }
    std::string url=MaxBuildUpdatesUrl(HasMarker,Marker,timeoutSeconds,limit);
    MAX_HTTP_RESPONSE r=Transport->Get(WithBaseUrl(url),Headers(false));
    if(!CheckResponse(r,error)) return false;
    if(!MaxParseUpdates(r.Body,updates,error)) return false;
    if(updates.HasMarker) { HasMarker=true; Marker=updates.Marker; }
    return true;
}

bool MAX_API_CLIENT::SendMessage(const MAX_PEER & peer, const std::string & utf8Text, std::string & error)
{
    if(!Transport) { error="MAX transport is null"; return false; }
    MAX_HTTP_RESPONSE r=Transport->Post(WithBaseUrl(MaxBuildSendMessageUrl(peer)),Headers(true),MaxBuildSendMessageBody(utf8Text));
    return CheckResponse(r,error);
}

bool MAX_API_CLIENT::SendUploadedAttachment(const MAX_PEER & peer, const std::string & filename,
                               const std::string & utf8Caption, const std::string & uploadType,
                               const std::string & attachmentType, std::string & error)
{
    if(!Transport) { error="MAX transport is null"; return false; }

    MAX_HTTP_RESPONSE prepare=Transport->Post(
        WithBaseUrl(std::string("https://platform-api2.max.ru/uploads?type=")+uploadType),Headers(false),"");
    if(!CheckResponse(prepare,error)) return false;

    std::string uploadUrl;
    if(!MaxParseUploadUrl(prepare.Body,uploadUrl,error)) return false;

    MAX_HTTP_RESPONSE uploaded=Transport->PostMultipartFile(
        uploadUrl,Headers(false),"data",filename);
    if(!CheckResponse(uploaded,error)) return false;

    std::string token;
    if(!MaxParseUploadToken(uploaded.Body,token,error)) return false;

    MAX_HTTP_RESPONSE sent=Transport->Post(
        WithBaseUrl(MaxBuildSendMessageUrl(peer)),Headers(true),
        BuildAttachmentMessageBody(utf8Caption,attachmentType,token));
    return CheckResponse(sent,error);
}

bool MAX_API_CLIENT::SendImage(const MAX_PEER & peer, const std::string & filename,
                               const std::string & utf8Caption, std::string & error)
{
    return SendUploadedAttachment(peer,filename,utf8Caption,"image","image",error);
}

bool MAX_API_CLIENT::SendFile(const MAX_PEER & peer, const std::string & filename,
                              const std::string & utf8Caption, std::string & error)
{
    return SendUploadedAttachment(peer,filename,utf8Caption,"file","file",error);
}
