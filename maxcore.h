#ifndef maxcoreH
#define maxcoreH

#include <string>
#include <vector>

#ifdef __BORLANDC__
typedef __int64 max_int64;
#else
typedef long long max_int64;
#endif

enum MAX_PEER_TYPE
{
    maxPeerUser = 0,
    maxPeerChat = 1
};

struct MAX_PEER
{
    MAX_PEER_TYPE Type;
    max_int64 Id;
    MAX_PEER() : Type(maxPeerUser), Id(0) {}
    MAX_PEER(MAX_PEER_TYPE type, max_int64 id) : Type(type), Id(id) {}
};

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

struct MAX_MESSAGE
{
    std::string UpdateType;
    max_int64 UpdateTimestamp;
    max_int64 MessageTimestamp;
    max_int64 ChatId;
    max_int64 UserId;
    std::string ChatType;
    std::string MessageId;
    std::string Text;
    std::string FirstName;
    std::string LastName;
    std::string UserName;
    bool SenderIsBot;

    MAX_MESSAGE();
};

struct MAX_UPDATES
{
    bool HasMarker;
    max_int64 Marker;
    std::vector<MAX_MESSAGE> Messages;
    MAX_UPDATES() : HasMarker(false), Marker(0) {}
    void Clear();
};

std::string MaxJsonEscape(const std::string & value);
std::string MaxUtf8FromCp1251(const std::string & value);
std::string MaxBuildUpdatesUrl(bool hasMarker, max_int64 marker, int timeoutSeconds, int limit);
std::string MaxBuildSendMessageUrl(const MAX_PEER & peer);
std::string MaxBuildSendMessageBody(const std::string & text);
std::string MaxBuildImageMessageBody(const std::string & text, const std::string & token);
std::string MaxInt64ToString(max_int64 value);
bool MaxParseUploadUrl(const std::string & json, std::string & url, std::string & error);
bool MaxParseUploadToken(const std::string & json, std::string & token, std::string & error);

bool MaxParseBotInfo(const std::string & json, MAX_BOT_INFO & info, std::string & error);
bool MaxParseUpdates(const std::string & json, MAX_UPDATES & updates, std::string & error);

#endif
