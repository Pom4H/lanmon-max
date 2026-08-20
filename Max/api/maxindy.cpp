#include <vcl.h>
#pragma hdrstop
#include "maxindy.h"
#include <SysUtils.hpp>

#pragma package(smart_init)

//Преобразование исходящего LanMon AnsiString/CP1251 в UTF-8 MAX
MAX_TEXT MaxUtf8FromAnsi1251(const AnsiString & text)
{
    return MaxUtf8FromCp1251(text);
}

//Создание production HTTP/HTTPS транспорта MAX
TMaxIndyTransport::TMaxIndyTransport()
{
    Http=new TIdHTTP(NULL);
    Ssl=new TIdSSLIOHandlerSocketOpenSSL(NULL);

    //У Евгения установлен обновлённый Indy 10.6.2 для C++Builder 2007.
    //В его IdSSLOpenSSL.hpp есть отдельный sslvTLSv1_2, поэтому выбираем
    //нужный MAX протокол явно, без старого sslvSSLv23 negotiation workaround.
    Ssl->SSLOptions->Method=sslvTLSv1_2;

    //Рано проверяем DLL OpenSSL: так несовместимый runtime даёт понятную
    //ошибку до первого HTTP-запроса, а не неочевидный exception из IOHandler.
    if(!LoadOpenSSLLibrary())
    {
        StartupError="OpenSSL DLL load failed: "+WhichFailedToLoad();
    }
    else if(!IsOpenSSL_TLSv1_2_Available())
    {
        StartupError="Loaded OpenSSL runtime does not support TLS 1.2";
    }

    //MAX требует доверять цепочке Минцифры. Bundle лежит рядом с executable.
    AnsiString rootCert=ExtractFilePath(Application->ExeName)+"certs\\max-ca.pem";
    if(!FileExists(rootCert))
    {
        //Fail-closed: без trust bundle нельзя тихо переходить к непроверенному TLS.
        StartupError="MAX CA bundle not found: "+rootCert;
    }
    else
    {
        Ssl->SSLOptions->RootCertFile=rootCert;
        //Не отключаем проверку сертификата сервера.
        Ssl->SSLOptions->VerifyMode=TIdSSLVerifyModeSet()<<sslvrfPeer;
        Ssl->SSLOptions->VerifyDepth=9;
    }

    Http->IOHandler=Ssl;
    Http->HandleRedirects=true;
    Http->Request->UserAgent="LanMon MAX adapter";
}

//Деструктор транспорта
TMaxIndyTransport::~TMaxIndyTransport()
{
    delete Http;
    delete Ssl;
}

//Проверить обязательную TLS-конфигурацию до сетевого запроса
bool TMaxIndyTransport::StartupFailed(MAX_HTTP_RESPONSE & response) const
{
    if(!StartupError.Length())return false;
    response.StatusCode=0;
    response.Error=StartupError;
    return true;
}

//Реальная задержка между повторами MAX attachment.not.ready
void TMaxIndyTransport::SleepMilliseconds(unsigned int milliseconds)
{
    ::Sleep(milliseconds);
}

//Перенести заголовки MAX_API_CLIENT в TIdHTTP
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

//Собрать единый результат HTTP операции для MAX_API_CLIENT
MAX_HTTP_RESPONSE TMaxIndyTransport::ReadResponse(TMemoryStream * stream,const AnsiString & exceptionText)
{
    MAX_HTTP_RESPONSE r;
    r.StatusCode=Http->ResponseCode;
    if(exceptionText.Length())r.Error=exceptionText;
    if(stream)
    {
        stream->Position=0;
        int n=(int)stream->Size;
        if(n>0)
        {
            r.Body.SetLength(n);
            stream->ReadBuffer(&r.Body[1],n);
        }
    }
    return r;
}

//HTTP GET
MAX_HTTP_RESPONSE TMaxIndyTransport::Get(const MAX_TEXT & url,const MAX_HTTP_HEADERS & headers)
{
    MAX_HTTP_RESPONSE blocked;
    if(StartupFailed(blocked))return blocked;

    ApplyHeaders(headers);
    TMemoryStream * stream=new TMemoryStream;
    AnsiString ex="";
    try
    {
        Http->Get(url,stream);
    }
    catch(Exception & E)
    {
        ex=E.Message;
    }
    MAX_HTTP_RESPONSE r=ReadResponse(stream,ex);
    delete stream;
    Http->Disconnect();
    return r;
}

//HTTP POST
MAX_HTTP_RESPONSE TMaxIndyTransport::Post(const MAX_TEXT & url,const MAX_HTTP_HEADERS & headers,const MAX_TEXT & body)
{
    MAX_HTTP_RESPONSE blocked;
    if(StartupFailed(blocked))return blocked;

    ApplyHeaders(headers);
    TStringStream * input=new TStringStream(body);
    TMemoryStream * output=new TMemoryStream;
    AnsiString ex="";
    try
    {
        Http->Post(url,input,output);
    }
    catch(Exception & E)
    {
        ex=E.Message;
    }
    MAX_HTTP_RESPONSE r=ReadResponse(output,ex);
    delete output;
    delete input;
    Http->Disconnect();
    return r;
}

//Multipart upload файла в URL, который вернул MAX /uploads
MAX_HTTP_RESPONSE TMaxIndyTransport::PostMultipartFile(const MAX_TEXT & url,
    const MAX_HTTP_HEADERS & headers,const MAX_TEXT & fieldName,const MAX_TEXT & filename)
{
    MAX_HTTP_RESPONSE blocked;
    if(StartupFailed(blocked))return blocked;

    ApplyHeaders(headers);
    TIdMultiPartFormDataStream * form=new TIdMultiPartFormDataStream;
    TMemoryStream * output=new TMemoryStream;
    AnsiString ex="";
    try
    {
        //MAX ожидает бинарник в multipart поле "data"
        form->AddFile(fieldName,filename,"application/octet-stream");
        //URL используем ровно тот, который вернул MAX; host может отличаться.
        Http->Post(url,form,output);
    }
    catch(Exception & E)
    {
        ex=E.Message;
    }
    MAX_HTTP_RESPONSE r=ReadResponse(output,ex);
    delete output;
    delete form;
    Http->Disconnect();
    return r;
}
