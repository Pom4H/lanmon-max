#ifndef maxcoreH
#define maxcoreH

// C++Builder 2007 must not instantiate the old STL in production.
// Linux CI keeps std::string/std::vector only in the non-Borland branch.
#ifdef __BORLANDC__
#include <System.hpp>
#include <Classes.hpp>
typedef AnsiString MAX_TEXT;
#else
#include <string>
#include <vector>
typedef std::string MAX_TEXT;
#endif

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
    MAX_TEXT FirstName;
    MAX_TEXT LastName;
    MAX_TEXT UserName;
    bool IsBot;
    MAX_BOT_INFO() : Id(0), IsBot(false) {}
    void Clear();
};

//Нормализованное входящее сообщение MAX
//Эта структура скрывает JSON-формат API от VCL-слоя maxmsg/maxbot
struct MAX_MESSAGE
{
    MAX_TEXT UpdateType;       //Тип update, например message_created
    max_int64 UpdateTimestamp; //Timestamp самого update
    max_int64 MessageTimestamp;//Timestamp сообщения
    max_int64 ChatId;          //Идентификатор чата
    max_int64 UserId;          //Идентификатор отправителя
    MAX_TEXT ChatType;         //Тип чата MAX
    MAX_TEXT MessageId;        //mid сообщения
    MAX_TEXT Text;             //UTF-8 текст
    MAX_TEXT FirstName;        //Имя отправителя
    MAX_TEXT LastName;         //Фамилия отправителя
    MAX_TEXT UserName;         //Username отправителя
    bool SenderIsBot;          //True, если отправитель бот

    MAX_MESSAGE();
};

#ifdef __BORLANDC__
//VCL-список значений MAX_MESSAGE без std::vector.
//Снаружи сохраняет минимальный vector-подобный API, чтобы верхний слой LanMon
//не зависел от того, каким контейнером хранится результат парсинга.
class MAX_MESSAGE_ARRAY
{
    TList * List;
public:
    MAX_MESSAGE_ARRAY();
    MAX_MESSAGE_ARRAY(const MAX_MESSAGE_ARRAY & source);
    ~MAX_MESSAGE_ARRAY();
    MAX_MESSAGE_ARRAY & operator=(const MAX_MESSAGE_ARRAY & source);
    int size() const;
    bool empty() const;
    void clear();
    void push_back(const MAX_MESSAGE & message);
    MAX_MESSAGE & operator[](int index);
    const MAX_MESSAGE & operator[](int index) const;
};
#endif

//Ответ GET /updates
struct MAX_UPDATES
{
    bool HasMarker;               //Сервер вернул новый marker
    max_int64 Marker;             //Курсор следующего Long Poll чтения
#ifdef __BORLANDC__
    MAX_MESSAGE_ARRAY Messages;
#else
    std::vector<MAX_MESSAGE> Messages;
#endif
    MAX_UPDATES() : HasMarker(false), Marker(0) {}
    void Clear();
};

//Экранирование строки для JSON
MAX_TEXT MaxJsonEscape(const MAX_TEXT & value);
//Преобразование CP1251 LanMon -> UTF-8 MAX
MAX_TEXT MaxUtf8FromCp1251(const MAX_TEXT & value);
//Построить URL GET /updates с timeout/limit/marker
MAX_TEXT MaxBuildUpdatesUrl(bool hasMarker, max_int64 marker, int timeoutSeconds, int limit);
//Построить URL POST /messages с user_id или chat_id
MAX_TEXT MaxBuildSendMessageUrl(const MAX_PEER & peer);
//Построить JSON обычного текстового сообщения
MAX_TEXT MaxBuildSendMessageBody(const MAX_TEXT & text);
//Построить JSON сообщения с image attachment token
MAX_TEXT MaxBuildImageMessageBody(const MAX_TEXT & text, const MAX_TEXT & token);
//Преобразовать 64-битный идентификатор в строку без зависимости от C++11
MAX_TEXT MaxInt64ToString(max_int64 value);
//Получить upload URL из ответа POST /uploads
bool MaxParseUploadUrl(const MAX_TEXT & json, MAX_TEXT & url, MAX_TEXT & error);
//Получить attachment token из ответа upload-host
bool MaxParseUploadToken(const MAX_TEXT & json, MAX_TEXT & token, MAX_TEXT & error);

//Декодировать ответ GET /me
bool MaxParseBotInfo(const MAX_TEXT & json, MAX_BOT_INFO & info, MAX_TEXT & error);
//Декодировать ответ GET /updates
bool MaxParseUpdates(const MAX_TEXT & json, MAX_UPDATES & updates, MAX_TEXT & error);

#endif
