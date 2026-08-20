#ifndef maxclientH
#define maxclientH

#include "maxcore.h"

#ifdef __BORLANDC__
//HTTP-заголовки на TStringList: production BCB2007 не использует std::map.
class MAX_HTTP_HEADERS
{
    TStringList * Items;
public:
    MAX_HTTP_HEADERS();
    MAX_HTTP_HEADERS(const MAX_HTTP_HEADERS & source);
    ~MAX_HTTP_HEADERS();
    MAX_HTTP_HEADERS & operator=(const MAX_HTTP_HEADERS & source);
    void Clear();
    void Set(const MAX_TEXT & name,const MAX_TEXT & value);
    bool Has(const MAX_TEXT & name) const;
    MAX_TEXT Get(const MAX_TEXT & name) const;
    int Count() const;
    MAX_TEXT Name(int index) const;
    MAX_TEXT Value(int index) const;
};
#else
#include <map>
typedef std::map<MAX_TEXT,MAX_TEXT> MAX_HTTP_HEADERS;
#endif

//Результат одного HTTP-запроса MAX API
struct MAX_HTTP_RESPONSE
{
    int StatusCode;   //HTTP status code
    MAX_TEXT Body;    //Тело ответа сервера
    MAX_TEXT Error;   //Ошибка транспорта до/вместо HTTP ответа
    MAX_HTTP_RESPONSE() : StatusCode(0) {}
};

//Абстракция транспорта: production использует VCL Indy, тесты — локальный mock/posix transport
class IMaxHttpTransport
{
public:
    virtual ~IMaxHttpTransport() {}
    //HTTP GET
    virtual MAX_HTTP_RESPONSE Get(const MAX_TEXT & url,
                                  const MAX_HTTP_HEADERS & headers)=0;
    //HTTP POST
    virtual MAX_HTTP_RESPONSE Post(const MAX_TEXT & url,
                                   const MAX_HTTP_HEADERS & headers,
                                   const MAX_TEXT & body)=0;
    //Multipart upload файла в URL, который вернул MAX /uploads
    virtual MAX_HTTP_RESPONSE PostMultipartFile(const MAX_TEXT & url,
                                   const MAX_HTTP_HEADERS & headers,
                                   const MAX_TEXT & fieldName,
                                   const MAX_TEXT & filename);
    //Пауза между повторными запросами. Production transport реально ждёт,
    //тестовые transport могут только записывать требуемые интервалы.
    virtual void SleepMilliseconds(unsigned int milliseconds);
};

//Клиент MAX Bot API, независимый от конкретной HTTP-библиотеки
class MAX_API_CLIENT
{
    //Bot token из платформы MAX
    MAX_TEXT Token;
    //HTTP transport принадлежит вызывающему коду
    IMaxHttpTransport * Transport;
    //Long Polling marker
    bool HasMarker;
    max_int64 Marker;
    //Базовый URL; в тестах заменяется локальным mock-сервером
    MAX_TEXT BaseUrl;
    //Диагностика последнего HTTP действия для старого UI LanMon
    int LastStatusCode;
    MAX_TEXT LastResponseBody;
public:
    MAX_API_CLIENT(IMaxHttpTransport * transport,const MAX_TEXT & token,
                   const MAX_TEXT & baseUrl="https://platform-api2.max.ru");
    //Изменить bot token без пересоздания transport/thread
    void SetToken(const MAX_TEXT & token){Token=token;}
    //Получить информацию о боте
    bool GetMe(MAX_BOT_INFO & info,MAX_TEXT & error);
    //Получить обновления через Long Polling и сохранить новый marker
    bool Poll(MAX_UPDATES & updates,MAX_TEXT & error,int timeoutSeconds=30,int limit=100);
    //Послать текстовое сообщение
    bool SendMessage(const MAX_PEER & peer,const MAX_TEXT & utf8Text,MAX_TEXT & error);
    //Загрузить и послать изображение
    bool SendImage(const MAX_PEER & peer,const MAX_TEXT & filename,const MAX_TEXT & utf8Caption,MAX_TEXT & error);
    //Загрузить и послать произвольный файл
    bool SendFile(const MAX_PEER & peer,const MAX_TEXT & filename,const MAX_TEXT & utf8Caption,MAX_TEXT & error);
    //Состояние Long Polling marker
    bool MarkerValid() const{return HasMarker;}
    max_int64 CurrentMarker() const{return Marker;}
    void ResetMarker(){HasMarker=false;Marker=0;}
    //Диагностика/тестирование
    const MAX_TEXT & GetBaseUrl() const{return BaseUrl;}
    int GetLastStatusCode() const{return LastStatusCode;}
    const MAX_TEXT & GetLastResponseBody() const{return LastResponseBody;}
private:
    //Подменить production host на BaseUrl в тестах, не изменяя upload URL
    MAX_TEXT WithBaseUrl(const MAX_TEXT & url) const;
    //Сформировать обязательные HTTP-заголовки MAX
    MAX_HTTP_HEADERS Headers(bool json) const;
    //Проверить transport/HTTP результат и сохранить diagnostic response
    bool CheckResponse(const MAX_HTTP_RESPONSE & r,MAX_TEXT & error);
    //Общий MAX upload flow: получить URL -> multipart upload -> token -> attachment message.
    //Если MAX отвечает attachment.not.ready, повторяется только финальный POST с тем же token.
    bool SendUploadedAttachment(const MAX_PEER & peer,const MAX_TEXT & filename,const MAX_TEXT & utf8Caption,
                                const MAX_TEXT & uploadType,const MAX_TEXT & attachmentType,MAX_TEXT & error);
};

#endif
