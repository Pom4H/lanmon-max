#include <vcl.h>
#pragma hdrstop
#include "maxindy.h"
#include <SysUtils.hpp>

#pragma package(smart_init)

MAX_TEXT MaxUtf8FromAnsi1251(const AnsiString & text)
{
    return MaxUtf8FromCp1251(text);
}

TMaxIndyTransport::TMaxIndyTransport()
{
    Http=new TIdHTTP(NULL);
    Ssl=new TIdSSLIOHandlerSocketOpenSSL(NULL);
    Ssl->SSLOptions->Method=sslvTLSv1_2;

    AnsiString rootCert=ExtractFilePath(Application->ExeName)+"certs\\max-ca.pem";
    if(FileExists(rootCert))
    {
        Ssl->SSLOptions->RootCertFile=rootCert;
        Ssl->SSLOptions->VerifyMode=TIdSSLVerifyModeSet()<<sslvrfPeer;
        Ssl->SSLOptions->VerifyDepth=9;
    }
    else
    {
        StartupError="MAX CA bundle not found: "+rootCert;
    }

    Http->IOHandler=Ssl;
    Http->HandleRedirects=true;
    Http->Request->UserAgent="LanMon MAX adapter";
}

TMaxIndyTransport::~TMaxIndyTransport()
{
    delete Http;
    delete Ssl;
}

bool TMaxIndyTransport::StartupFailed(MAX_HTTP_RESPONSE & response) const
{
    if(!StartupError.Length())return false;
    response.StatusCode=0;
    response.Error=StartupError;
    return true;
}

void TMaxIndyTransport::SleepMilliseconds(unsigned int milliseconds)
{
    ::Sleep(milliseconds);
}

void TMaxIndyTransport::ApplyHeaders(const MAX_HTTP_HEADERS & headers)
{
    Http->Request->CustomHeaders->Clear();
    Http->Request->ContentType="";
    for(int i=0;i<headers.Count();++i)
    {
        AnsiString name=headers.Name(i);
        AnsiString value=headers.Value(i);
        if(name=="Content-Type")Http->Request->ContentType=value;
        else Http->Request->CustomHeaders->Values[name]=value;
    }
}

MAX_HTTP_RESPONSE TMaxIndyTransport::ReadResponse(TMemoryStream * stream,const AnsiString & exceptionText)
{
    MAX_HTTP_RESPONSE r;
    r.StatusCode=Http->ResponseCode;
    if(exceptionText.Length())r.Error=exceptionText;
    if(stream)
    {
        stream->Position=0;
        TStringList * sl=new TStringList;
        sl->LoadFromStream(stream);
        r.Body=sl->Text;
        delete sl;
    }
    return r;
}

// Indy превращает HTTP 4xx/5xx в EIdHTTPProtocolException. Для MAX это не
// transport error: тело JSON содержит машинный code, например attachment.not.ready.
// Сохраняем ErrorCode/ErrorMessage как обычный HTTP response, чтобы клиент мог
// принять решение о retry по коду MAX, а не повторять любую ошибку подряд.
static void ApplyHttpProtocolError(MAX_HTTP_RESPONSE & response,int status,const AnsiString & body)
{
    if(status>0)response.StatusCode=status;
    if(body.Length())response.Body=body;
    response.Error="";
}

MAX_HTTP_RESPONSE TMaxIndyTransport::Get(const MAX_TEXT & url,const MAX_HTTP_HEADERS & headers)
{
    MAX_HTTP_RESPONSE blocked;
    if(StartupFailed(blocked))return blocked;

    ApplyHeaders(headers);
    TMemoryStream * stream=new TMemoryStream;
    AnsiString ex="";
    int protocolStatus=0;
    AnsiString protocolBody="";
    try
    {
        Http->Get(url,stream);
    }
    catch(EIdHTTPProtocolException & E)
    {
        protocolStatus=E.ErrorCode;
        protocolBody=E.ErrorMessage;
    }
    catch(Exception & E)
    {
        ex=E.Message;
    }
    MAX_HTTP_RESPONSE r=ReadResponse(stream,ex);
    if(protocolStatus)ApplyHttpProtocolError(r,protocolStatus,protocolBody);
    delete stream;
    Http->Disconnect();
    return r;
}

MAX_HTTP_RESPONSE TMaxIndyTransport::Post(const MAX_TEXT & url,const MAX_HTTP_HEADERS & headers,const MAX_TEXT & body)
{
    MAX_HTTP_RESPONSE blocked;
    if(StartupFailed(blocked))return blocked;

    ApplyHeaders(headers);
    TStringStream * input=new TStringStream(body);
    TMemoryStream * output=new TMemoryStream;
    AnsiString ex="";
    int protocolStatus=0;
    AnsiString protocolBody="";
    try
    {
        Http->Post(url,input,output);
    }
    catch(EIdHTTPProtocolException & E)
    {
        protocolStatus=E.ErrorCode;
        protocolBody=E.ErrorMessage;
    }
    catch(Exception & E)
    {
        ex=E.Message;
    }
    MAX_HTTP_RESPONSE r=ReadResponse(output,ex);
    if(protocolStatus)ApplyHttpProtocolError(r,protocolStatus,protocolBody);
    delete output;
    delete input;
    Http->Disconnect();
    return r;
}

MAX_HTTP_RESPONSE TMaxIndyTransport::PostMultipartFile(const MAX_TEXT & url,
    const MAX_HTTP_HEADERS & headers,const MAX_TEXT & fieldName,const MAX_TEXT & filename)
{
    MAX_HTTP_RESPONSE blocked;
    if(StartupFailed(blocked))return blocked;

    ApplyHeaders(headers);
    TIdMultiPartFormDataStream * form=new TIdMultiPartFormDataStream;
    TMemoryStream * output=new TMemoryStream;
    AnsiString ex="";
    int protocolStatus=0;
    AnsiString protocolBody="";
    try
    {
        form->AddFile(fieldName,filename,"application/octet-stream");
        Http->Post(url,form,output);
    }
    catch(EIdHTTPProtocolException & E)
    {
        protocolStatus=E.ErrorCode;
        protocolBody=E.ErrorMessage;
    }
    catch(Exception & E)
    {
        ex=E.Message;
    }
    MAX_HTTP_RESPONSE r=ReadResponse(output,ex);
    if(protocolStatus)ApplyHttpProtocolError(r,protocolStatus,protocolBody);
    delete output;
    delete form;
    Http->Disconnect();
    return r;
}
