#include "posix_http_transport.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <fstream>

struct ParsedUrl
{
    std::string host;
    std::string port;
    std::string path;
};

static bool ParseHttpUrl(const std::string & url, ParsedUrl & out, std::string & error)
{
    const std::string prefix="http://";
    if(url.compare(0,prefix.size(),prefix)!=0) {
        error="E2E POSIX transport supports only http:// URLs";
        return false;
    }
    std::string rest=url.substr(prefix.size());
    std::string::size_type slash=rest.find('/');
    std::string authority=slash==std::string::npos?rest:rest.substr(0,slash);
    out.path=slash==std::string::npos?"/":rest.substr(slash);
    std::string::size_type colon=authority.rfind(':');
    if(colon==std::string::npos) { out.host=authority; out.port="80"; }
    else { out.host=authority.substr(0,colon); out.port=authority.substr(colon+1); }
    if(out.host.empty()) { error="empty host"; return false; }
    return true;
}

static bool SendAll(int fd, const std::string & data)
{
    size_t sent=0;
    while(sent<data.size()) {
        ssize_t n=send(fd,data.data()+sent,data.size()-sent,0);
        if(n<=0) return false;
        sent+=(size_t)n;
    }
    return true;
}

static std::string DecodeChunked(const std::string & body)
{
    std::string out;
    size_t pos=0;
    while(pos<body.size()) {
        size_t eol=body.find("\r\n",pos);
        if(eol==std::string::npos) return body;
        std::string hex=body.substr(pos,eol-pos);
        unsigned long n=strtoul(hex.c_str(),0,16);
        if(n==0) break;
        pos=eol+2;
        if(pos+n>body.size()) return body;
        out.append(body,pos,n);
        pos+=n+2;
    }
    return out;
}

MAX_HTTP_RESPONSE TPosixHttpTransport::Get(const std::string & url,
    const std::map<std::string,std::string> & headers)
{
    return Request("GET",url,headers,"");
}

MAX_HTTP_RESPONSE TPosixHttpTransport::Post(const std::string & url,
    const std::map<std::string,std::string> & headers,const std::string & body)
{
    return Request("POST",url,headers,body);
}

MAX_HTTP_RESPONSE TPosixHttpTransport::PostMultipartFile(const std::string & url,
    const std::map<std::string,std::string> & headers,const std::string & fieldName,
    const std::string & filename)
{
    std::ifstream in(filename.c_str(),std::ios::in|std::ios::binary);
    MAX_HTTP_RESPONSE r;
    if(!in) { r.Error=std::string("cannot open upload file: ")+filename; return r; }
    std::ostringstream data; data << in.rdbuf();
    const std::string boundary="----LanMonMaxE2EBoundary7MA4YWxk";
    std::ostringstream body;
    body << "--" << boundary << "\r\n";
    body << "Content-Disposition: form-data; name=\"" << fieldName << "\"; filename=\"map.png\"\r\n";
    body << "Content-Type: application/octet-stream\r\n\r\n";
    body << data.str() << "\r\n--" << boundary << "--\r\n";
    std::map<std::string,std::string> h=headers;
    h["Content-Type"]=std::string("multipart/form-data; boundary=")+boundary;
    return Request("POST",url,h,body.str());
}

MAX_HTTP_RESPONSE TPosixHttpTransport::Request(const std::string & method,
    const std::string & url,const std::map<std::string,std::string> & headers,
    const std::string & body)
{
    MAX_HTTP_RESPONSE result;
    ParsedUrl u; std::string error;
    if(!ParseHttpUrl(url,u,error)) { result.Error=error; return result; }

    struct addrinfo hints; std::memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_UNSPEC; hints.ai_socktype=SOCK_STREAM;
    struct addrinfo *ai=0;
    int gr=getaddrinfo(u.host.c_str(),u.port.c_str(),&hints,&ai);
    if(gr!=0) { result.Error=std::string("getaddrinfo: ")+gai_strerror(gr); return result; }

    int fd=-1;
    for(struct addrinfo *p=ai;p;p=p->ai_next) {
        fd=socket(p->ai_family,p->ai_socktype,p->ai_protocol);
        if(fd<0) continue;
        if(connect(fd,p->ai_addr,p->ai_addrlen)==0) break;
        close(fd); fd=-1;
    }
    freeaddrinfo(ai);
    if(fd<0) { result.Error=std::string("connect failed: ")+std::strerror(errno); return result; }

    std::ostringstream req;
    req << method << " " << u.path << " HTTP/1.1\r\n";
    req << "Host: " << u.host << ":" << u.port << "\r\n";
    req << "Connection: close\r\n";
    for(std::map<std::string,std::string>::const_iterator it=headers.begin();it!=headers.end();++it)
        req << it->first << ": " << it->second << "\r\n";
    if(method=="POST") req << "Content-Length: " << body.size() << "\r\n";
    req << "\r\n" << body;
    if(!SendAll(fd,req.str())) { close(fd); result.Error="send failed"; return result; }

    std::string raw; char buf[4096];
    for(;;) {
        ssize_t n=recv(fd,buf,sizeof(buf),0);
        if(n<0) { close(fd); result.Error="recv failed"; return result; }
        if(n==0) break;
        raw.append(buf,(size_t)n);
    }
    close(fd);

    size_t sep=raw.find("\r\n\r\n");
    if(sep==std::string::npos) { result.Error="malformed HTTP response"; return result; }
    std::string head=raw.substr(0,sep);
    result.Body=raw.substr(sep+4);
    std::istringstream first(head.substr(0,head.find("\r\n")));
    std::string http; first >> http >> result.StatusCode;
    if(result.StatusCode==0) { result.Error="missing HTTP status"; return result; }
    if(head.find("Transfer-Encoding: chunked")!=std::string::npos || head.find("transfer-encoding: chunked")!=std::string::npos)
        result.Body=DecodeChunked(result.Body);
    return result;
}
