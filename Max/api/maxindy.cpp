#include <vcl.h>
#pragma hdrstop
#include "maxindy.h"
#include <SysUtils.hpp>

#pragma package(smart_init)

std::string MaxUtf8FromAnsi1251(const AnsiString & text)
{
    return MaxUtf8FromCp1251(std::string(text.c_str(), text.Length()));
}

TMaxIndyTransport::TMaxIndyTransport()
{
    Http=new TIdHTTP(NULL);
    Ssl=new TIdSSLIOHandlerSocketOpenSSL(NULL);
    Ssl->SSLOptions->Method=sslvTLSv1_2;

    // MAX с 19.07.2026 требует добавить сертификат Минцифры в доверенные.
    // Для legacy LanMon используем явный PEM bundle рядом с executable:
    //   <каталог lanmon4.exe>\certs\max-ca.pem
    // TIdSSLIOHandlerSocketOpenSSL старых C++Builder/Indy не следует считать
    // автоматически использующим Windows Certificate Store.
    AnsiString rootCert=ExtractFilePath(Application->ExeName)+"certs\\max-ca.pem";
    if(FileExists(rootCert))
    {
        Ssl->SSLOptions->RootCertFile=rootCert;
        Ssl->SSLOptions->VerifyMode=TIdSSLVerifyModeSet()<<sslvrfPeer;
        Ssl->SSLOptions->VerifyDepth=9;
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

void TMaxIndyTransport::ApplyHeaders(const std::map<std::string,std::string> & headers)
{
    Http->Request->CustomHeaders->Clear();
    Http->Request->ContentType="";
    std::map<std::string,std::string>::const_iterator it=headers.begin();
    for(;it!=headers.end();++it) {
        if(it->first=="Content-Type") Http->Request->ContentType=AnsiString(it->second.c_str());
        else Http->Request->CustomHeaders->Values[AnsiString(it->first.c_str())]=AnsiString(it->second.c_str());
    }
}

MAX_HTTP_RESPONSE TMaxIndyTransport::ReadResponse(TMemoryStream * stream, const AnsiString & exceptionText)
{
    MAX_HTTP_RESPONSE r;
    r.StatusCode=Http->ResponseCode;
    if(exceptionText.Length()) r.Error=exceptionText.c_str();
    if(stream) {
        stream->Position=0;
        TStringList * sl=new TStringList;
        sl->LoadFromStream(stream);
        r.Body=sl->Text.c_str();
        delete sl;
    }
    return r;
}

MAX_HTTP_RESPONSE TMaxIndyTransport::Get(const std::string & url,const std::map<std::string,std::string> & headers)
{
    ApplyHeaders(headers);
    TMemoryStream * stream=new TMemoryStream;
    AnsiString ex="";
    try { Http->Get(AnsiString(url.c_str()),stream); }
    catch(EIdException &E) { ex=E.Message; }
    catch(...) { ex="Unknown Indy exception"; }
    MAX_HTTP_RESPONSE r=ReadResponse(stream,ex);
    delete stream;
    Http->Disconnect();
    return r;
}

MAX_HTTP_RESPONSE TMaxIndyTransport::Post(const std::string & url,const std::map<std::string,std::string> & headers,const std::string & body)
{
    ApplyHeaders(headers);
    TStringStream * input=new TStringStream(AnsiString(body.c_str()));
    TMemoryStream * output=new TMemoryStream;
    AnsiString ex="";
    try { Http->Post(AnsiString(url.c_str()),input,output); }
    catch(EIdException &E) { ex=E.Message; }
    catch(...) { ex="Unknown Indy exception"; }
    MAX_HTTP_RESPONSE r=ReadResponse(output,ex);
    delete output;
    delete input;
    Http->Disconnect();
    return r;
}

MAX_HTTP_RESPONSE TMaxIndyTransport::PostMultipartFile(const std::string & url,
    const std::map<std::string,std::string> & headers,const std::string & fieldName,
    const std::string & filename)
{
    ApplyHeaders(headers);
    TIdMultiPartFormDataStream * form=new TIdMultiPartFormDataStream;
    TMemoryStream * output=new TMemoryStream;
    AnsiString ex="";
    try {
        form->AddFile(AnsiString(fieldName.c_str()),AnsiString(filename.c_str()),"application/octet-stream");
        Http->Post(AnsiString(url.c_str()),form,output);
    }
    catch(EIdException &E) { ex=E.Message; }
    catch(...) { ex="Unknown Indy multipart exception"; }
    MAX_HTTP_RESPONSE r=ReadResponse(output,ex);
    delete output;
    delete form;
    Http->Disconnect();
    return r;
}