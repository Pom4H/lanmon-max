#ifndef maxusersH
#define maxusersH

#include "maxcore.h"
#include <string>
#include <vector>

struct MAX_USER
{
    max_int64 Id;
    MAX_PEER_TYPE PeerType;
    std::string Name;
    std::string Alias;
    std::string Comment;
    bool IsBot;
    unsigned long InCount;
    unsigned long OutCount;
    int Tag;

    MAX_USER();
    bool Valid() const { return Id!=0; }
    bool HasValidAlias(const std::string & aliasMask) const;
    MAX_PEER Peer() const { return MAX_PEER(PeerType,Id); }
};

class MAX_USER_LIST
{
    std::vector<MAX_USER> Users;
public:
    size_t Count() const { return Users.size(); }
    void Clear() { Users.clear(); }
    MAX_USER * Get(size_t index);
    const MAX_USER * Get(size_t index) const;
    MAX_USER * Find(max_int64 id);
    const MAX_USER * Find(max_int64 id) const;
    int IndexOfId(max_int64 id) const;
    MAX_USER * FindAlias(const std::string & alias);
    const MAX_USER * FindAlias(const std::string & alias) const;
    MAX_USER & Add();
    MAX_USER & Add(const MAX_USER & user);
    bool RemoveById(max_int64 id);
    std::string GetFreeAlias() const;
    void GetUsersByAlias(std::vector<MAX_USER*> & out, const std::string & aliasMask);
    void GetUsersByAlias(std::vector<const MAX_USER*> & out, const std::string & aliasMask) const;
};

#endif
