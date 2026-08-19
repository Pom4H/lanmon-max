#ifndef maxcoreH
#define maxcoreH

#include <string>
#include <vector>

//Единый 64-битный тип для старого C++Builder и Linux CI
#ifdef __BORLANDC__
typedef __int64 max_int64;
#else
typedef long long max_int64;
#endif

//Тип адресата MAX: личный пользователь или чат
//От него зависит query-параметр user_id/chat_id при отправке сообщения
enum MAX_PEER_TYPE
{
    maxPeerUser = 0,
    maxPeerChat = 1
};

//Адресат исходящего сообщения MAX
struct MAX_PEER
{
    MAX_PEER_TYPE Type;
    max_int64 Id;
    MAX_PEER() : Type(maxPeerUser), Id(0) {}
    MAX_PEER(MAX_PEER_TYPE type, max_int64 id) : Type(type), Id(id) {}
};

//Информация о текущем MAX боте, полученная через GET /me
struct MAX_BOT_INFO
{
    max_int64 Id;
    std::string FirstName;
    std::string LastName;
    std::string UserName;
    bool IsBot;
    MAX_BOT_INFO() : Id(0), IsBot(false) {}
    void Clear();
};

//Нормализованное входящее сообщение MAX
//Эта структура скрывает JSON-формат API от VCL-слоя maxmsg/maxbot
struct MAX_MESSAGE
{
    std::string UpdateType;       //Тип update, например message_created
    max_int64 UpdateTimestamp;    //Timestamp самого update
    max_int64 MessageTimestamp;   //Timestamp сообщения
    max_int64 ChatId;             //Идентификатор чата
    max_int64 UserId;             //Идентификатор отправителя
    std::string ChatType;         //Тип чата MAX
    std::string MessageId;        //mid сообщения
    std::string Text;             //UTF-8 текст
    std::string FirstName;        //Имя отправителя
    std::string LastName;         //Фамилия отправителя
    std::string UserName;         //Username отправителя
    bool SenderIsBot;             //True, если отправитель бот

    MAX_MESSAGE();
};

//Ответ GET /updates
struct MAX_UPDATES
{
    bool HasMarker;               //Сервер вернул новый marker
    max_int64 Marker;             //Курсор следующего Long Poll чтения
    std::vector<MAX_MESSAGE> Messages;
    MAX_UPDATES() : HasMarker(false), Marker(0) {}
    void Clear();
};

//Экранирование строки для JSON
std::string MaxJsonEscape(const std::string & value);
//Преобразование CP1251 LanMon -> UTF-8 MAX
std::string MaxUtf8FromCp1251(const std::string & value);
//Построить URL GET /updates с timeout/limit/marker
std::string MaxBuildUpdatesUrl(bool hasMarker, max_int64 marker, int timeoutSeconds, int limit);
//Построить URL POST /messages с user_id или chat_id
std::string MaxBuildSendMessageUrl(const MAX_PEER & peer);
//Построить JSON обычного текстового сообщения
std::string MaxBuildSendMessageBody(const std::string & text);
//Построить JSON сообщения с image attachment token
std::string MaxBuildImageMessageBody(const std::string & text, const std::string & token);
//Преобразовать 64-битный идентификатор в строку без зависимости от C++11
std::string MaxInt64ToString(max_int64 value);
//Получить upload URL из ответа POST /uploads
bool MaxParseUploadUrl(const std::string & json, std::string & url, std::string & error);
//Получить attachment token из ответа upload-host
bool MaxParseUploadToken(const std::string & json, std::string & token, std::string & error);

//Декодировать ответ GET /me
bool MaxParseBotInfo(const std::string & json, MAX_BOT_INFO & info, std::string & error);
//Декодировать ответ GET /updates
bool MaxParseUpdates(const std::string & json, MAX_UPDATES & updates, std::string & error);

#endif
