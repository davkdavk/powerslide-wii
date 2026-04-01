#ifndef WII_SIMPLEINI_STUB_H
#define WII_SIMPLEINI_STUB_H

#include <string>
#include <map>
#include <set>

typedef int SI_Error;

class CSimpleIniA
{
public:
    struct Entry {
        std::string pItem;
        bool operator<(const Entry& o) const { return pItem < o.pItem; }
    };

    typedef std::set<Entry> TNamesDepend;

    CSimpleIniA() {}

    void Reset() { mData.clear(); }

    SI_Error LoadData(const char* data, size_t len)
    {
        (void)data;
        (void)len;
        return 0;
    }

    template<typename StringLike>
    SI_Error Save(StringLike& out) const
    {
        out.clear();
        for (std::map<std::string, std::map<std::string, std::string> >::const_iterator s = mData.begin(); s != mData.end(); ++s) {
            if (!s->first.empty()) {
                out += "[" + s->first + "]\n";
            }
            for (std::map<std::string, std::string>::const_iterator kv = s->second.begin(); kv != s->second.end(); ++kv) {
                out += kv->first + "=" + kv->second + "\n";
            }
            out += "\n";
        }
        return 0;
    }

    const char* GetValue(const char* section, const char* key, const char* def = "") const
    {
        std::string s = section ? section : "";
        std::string k = key ? key : "";
        std::map<std::string, std::map<std::string, std::string> >::const_iterator itS = mData.find(s);
        if (itS == mData.end()) {
            return def;
        }
        std::map<std::string, std::string>::const_iterator itK = itS->second.find(k);
        if (itK == itS->second.end()) {
            return def;
        }
        return itK->second.c_str();
    }

    SI_Error SetValue(const char* section, const char* key, const char* value)
    {
        std::string s = section ? section : "";
        std::string k = key ? key : "";
        std::string v = value ? value : "";
        mData[s][k] = v;
        return 0;
    }

    SI_Error GetAllSections(TNamesDepend& out) const
    {
        out.clear();
        for (std::map<std::string, std::map<std::string, std::string> >::const_iterator s = mData.begin(); s != mData.end(); ++s) {
            Entry e;
            e.pItem = s->first;
            out.insert(e);
        }
        return 0;
    }

    SI_Error GetAllKeys(const char* section, TNamesDepend& out) const
    {
        out.clear();
        std::string s = section ? section : "";
        std::map<std::string, std::map<std::string, std::string> >::const_iterator itS = mData.find(s);
        if (itS == mData.end()) {
            return 0;
        }
        for (std::map<std::string, std::string>::const_iterator k = itS->second.begin(); k != itS->second.end(); ++k) {
            Entry e;
            e.pItem = k->first;
            out.insert(e);
        }
        return 0;
    }

private:
    std::map<std::string, std::map<std::string, std::string> > mData;
};

#endif
