#ifndef maxclientH
#define maxclientH

#include "maxcore.h"
#include <string>
#include <map>

//Результат одного HTTP-запроса MAX API
struct MAX_HTTP_RESPONSE
{
    int StatusCode;       //HTTP status code
    std::string Body;     //Тело ответа сервера
    std::string Error;    //Ошибка транспорта до/вместо HTTP ответа
    MAX_HTTP_RESPONSE() : StatusCode(0) {}
};

//Абстракция транспорта: production использует Indy, тесты — локальный mock/posix transport
class IMaxHttpTransport
{
public:
    virtual ~IMaxHttpTransport() {}
    //HTTP GET
    virtual MAX_HTTP_RESPONSE Get(const std::string & url,
                                  const std::map<std::string,std::string> & headers)=0;
    //HTTP POST
    virtual MAX_HTTP_RESPONSE Post(const std::string & url,
                                   const std::map<std::string,std::string> & headers,
                                   const std::string & body)=0;
    //Multipart upload файла в URL, который вернул MAX /uploads
    virtual MAX_HTTP_RESPONSE PostMultipartFile(const std::string & url,
                                   const std::map<std::string,std::string> & headers,
                                   const std::string & fieldName,
                                   const std::string & filename);
    //Пауза между повторными запросами. Production Indy реально ждёт,
    //тестовые transport могут только записывать требуемые интервалы.
    virtual void SleepMilliseconds(unsigned int milliseconds);
};

//Клиент MAX Bot API, независимый от конкретной HTTP-библиотеки
class MAX_API_CLIENT
{
    //Bot token из платформы MAX
    std::string Token;
    //HTTP transport принадлежит вызывающему коду
    IMaxHttpTransport * Transport;
    //Long Polling marker
    bool HasMarker;
    max_int64 Marker;
    //Базовый URL; в тестах заменяется локальным mock-сервером
    std::string BaseUrl;
    //Диагностика последнего HTTP действия для старого UI LanMon
    int LastStatusCode;
    std::string LastResponseBody;
public:
    MAX_API_CLIENT(IMaxHttpTransport * transport, const std::string & token,
                   const std::string & baseUrl="https://platform-api2.max.ru");
    //Изменить bot token без пересоздания transport/thread
    void SetToken(const std::string & token) { Token=token; }
    //Получить информацию о боте
    bool GetMe(MAX_BOT_INFO & info, std::string & error);
    //Получить обновления через Long Polling и сохранить новый marker
    bool Poll(MAX_UPDATES & updates, std::string & error, int timeoutSeconds=30, int limit=100);
    //Послать текстовое сообщение
    bool SendMessage(const MAX_PEER & peer, const std::string & utf8Text, std::string & error);
    //Загрузить и послать изображение
    bool SendImage(const MAX_PEER & peer, const std::string & filename, const std::string & utf8Caption, std::string & error);
    //Загрузить и послать произвольный файл
    bool SendFile(const MAX_PEER & peer, const std::string & filename, const std::string & utf8Caption, std::string & error);
    //Состояние Long Polling marker
    bool MarkerValid() const { return HasMarker; }
    max_int64 CurrentMarker() const { return Marker; }
    void ResetMarker() { HasMarker=false; Marker=0; }
    //Диагностика/тестирование
    const std::string & GetBaseUrl() const { return BaseUrl; }
    int GetLastStatusCode() const { return LastStatusCode; }
    const std::string & GetLastResponseBody() const { return LastResponseBody; }
private:
    //Подменить production host на BaseUrl в тестах, не изменяя upload URL
    std::string WithBaseUrl(const std::string & url) const;
    //Сформировать обязательные HTTP-заголовки MAX
    std::map<std::string,std::string> Headers(bool json) const;
    //Проверить transport/HTTP результат и сохранить diagnostic response
    bool CheckResponse(const MAX_HTTP_RESPONSE & r, std::string & error);
    //Общий MAX upload flow: получить URL -> multipart upload -> token -> attachment message.
    //Если MAX отвечает attachment.not.ready, повторяется только финальный POST с тем же token.
    bool SendUploadedAttachment(const MAX_PEER & peer, const std::string & filename, const std::string & utf8Caption,
                                const std::string & uploadType, const std::string & attachmentType, std::string & error);
};

#endif
