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
    Http=new TInHTTP(NULL);
    Ssl=new TInSSLIOHandlerSocketOpenSSL(NULL);

    //BCB2007 enum не содержит отдельного значения для TLS 1.2. sslvSSLv23
    //здесь означает OpenSSL negotiation method, а не принудительный SSLv2/3:
    //с ABI-совместимым OpenSSL runtime 1.0.x он договаривается о TLS 1.2,
    //который требует MAX. Старые протоколы MAX server всё равно не принимает.
    Ssl->SSLOptions->Method=sslvSSLv23;

    //MAX требует доверять цепочке Минцифры. Bundle лежит рядом с executable.
    AnsiString rootCert=ExtractFilePath(Application->ExeName)+"certs\\max-ca.pem";
    if(FileExists(rootCert))
    {
        Ssl->SSLOptions->RootCertFile=rootCert;
        //Не отключаем проверку сертификата сервера.
        Ssl->SSLOptions->VerifyMode=TInSSLVerifyModeSet()<<sslvrfPeer;
        Ssl->SSLOptions->VerifyDepth=9;
    }
    else
    {
        //Fail-closed: без trust bundle нельзя тихо переходить к непроверенному TLS.
        StartupError="MAX CA bundle not found: "+rootCert;
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

//Перенести заголовки MAX_API_CLIENT в TInHTTP
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
        TStringList * sl=new TStringList;
        sl->LoadFromStream(stream);
        r.Body=sl->Text;
        delete sl;
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
    TInMultipartFormDataStream * form=new TInMultipartFormDataStream;
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
