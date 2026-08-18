#include "maxusers.h"
#include <sstream>

MAX_USER::MAX_USER()
    : Id(0), PeerType(maxPeerUser), IsBot(false), InCount(0), OutCount(0), Tag(0)
{
}

bool MAX_USER::HasValidAlias(const std::string & aliasMask) const
{
    if(!Valid()) return false;
    if(aliasMask=="*" || aliasMask.empty()) return true;
    if(Alias.size()<aliasMask.size()) return false;
    for(size_t i=0;i<aliasMask.size();++i) {
        if(aliasMask[i]!='!' && Alias[i]!=aliasMask[i]) return false;
    }
    return true;
}

MAX_USER * MAX_USER_LIST::Get(size_t index)
{
    return index<Users.size()?&Users[index]:0;
}

const MAX_USER * MAX_USER_LIST::Get(size_t index) const
{
    return index<Users.size()?&Users[index]:0;
}

MAX_USER * MAX_USER_LIST::Find(max_int64 id)
{
    for(size_t i=0;i<Users.size();++i) if(Users[i].Id==id) return &Users[i];
    return 0;
}

const MAX_USER * MAX_USER_LIST::Find(max_int64 id) const
{
    for(size_t i=0;i<Users.size();++i) if(Users[i].Id==id) return &Users[i];
    return 0;
}

int MAX_USER_LIST::IndexOfId(max_int64 id) const
{
    for(size_t i=0;i<Users.size();++i) if(Users[i].Id==id) return (int)i;
    return -1;
}

MAX_USER * MAX_USER_LIST::FindAlias(const std::string & alias)
{
    for(size_t i=0;i<Users.size();++i) if(Users[i].Alias==alias) return &Users[i];
    return 0;
}

const MAX_USER * MAX_USER_LIST::FindAlias(const std::string & alias) const
{
    for(size_t i=0;i<Users.size();++i) if(Users[i].Alias==alias) return &Users[i];
    return 0;
}

MAX_USER & MAX_USER_LIST::Add()
{
    Users.push_back(MAX_USER());
    return Users.back();
}

MAX_USER & MAX_USER_LIST::Add(const MAX_USER & user)
{
    Users.push_back(user);
    return Users.back();
}

bool MAX_USER_LIST::RemoveById(max_int64 id)
{
    for(size_t i=0;i<Users.size();++i) {
        if(Users[i].Id==id) { Users.erase(Users.begin()+i); return true; }
    }
    return false;
}

std::string MAX_USER_LIST::GetFreeAlias() const
{
    int n=1;
    for(;;++n) {
        std::ostringstream os; os << "$" << n;
        if(!FindAlias(os.str())) return os.str();
    }
}

void MAX_USER_LIST::GetUsersByAlias(std::vector<MAX_USER*> & out, const std::string & aliasMask)
{
    out.clear();
    for(size_t i=0;i<Users.size();++i) if(Users[i].HasValidAlias(aliasMask)) out.push_back(&Users[i]);
}

void MAX_USER_LIST::GetUsersByAlias(std::vector<const MAX_USER*> & out, const std::string & aliasMask) const
{
    out.clear();
    for(size_t i=0;i<Users.size();++i) if(Users[i].HasValidAlias(aliasMask)) out.push_back(&Users[i]);
}
