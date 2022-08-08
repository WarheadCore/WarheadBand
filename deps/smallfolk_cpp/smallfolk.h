#ifndef SMALLFOLK_H
#define SMALLFOLK_H

#include <string>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <array>
#include <forward_list>
#include <unordered_map>
#include <map>
#include <memory> // std::unique_ptr
#include <stdexcept> // std::logic_error
#include <cstddef> // size_t
#include <utility> // std::move

class smallfolk_exception : public std::logic_error
{
public:
    static size_t const size = 2048;

    smallfolk_exception(const char * format, ...);
    virtual const char* what() const throw();

    std::string errmsg;
};

enum LuaTypeTag
{
    TNIL,
    TSTRING,
    TNUMBER,
    TTABLE,
    TBOOL,
};

class LuaVal;
size_t LuaValHash(LuaVal const & v);

namespace std {
    template <>
    struct hash<LuaVal> {
    public:
        size_t operator()(LuaVal const & v) const
        {
            return LuaValHash(v);
        };
    };
}

class LuaVal
{
public:

    // static nil value, same as LuaVal();
    // You can use it as for example as default const reference
    static LuaVal const nil;

    // returns the string representation of the value info similar to lua tostring
    std::string tostring() const;

    // use as the hasher for containers, for example std::unordered_map<LuaVal, int, LuaVal::LuaValHasher>
    struct LuaValHasher
    {
        size_t operator()(LuaVal const & v) const;
    };

    typedef std::unordered_map<LuaVal, LuaVal> LuaTable;
    typedef std::unique_ptr<LuaTable> TblPtr; // circular reference memleak if insert self to self

    LuaVal(const LuaTypeTag tag) : tag(tag), tbl_ptr(tag == TTABLE ? new LuaTable() : nullptr), d(0), b(false) {}
    LuaVal() : tag(TNIL), tbl_ptr(nullptr), d(0), b(false) {}
    LuaVal(const int d) : tag(TNUMBER), tbl_ptr(nullptr), d(d), b(false) {}
    LuaVal(const unsigned int d) : tag(TNUMBER), tbl_ptr(nullptr), d(d), b(false) {}
    LuaVal(const double d) : tag(TNUMBER), tbl_ptr(nullptr), d(d), b(false) {}
    LuaVal(const std::string & s) : tag(TSTRING), tbl_ptr(nullptr), s(s), d(0), b(false) {}
    LuaVal(const char * s) : tag(TSTRING), tbl_ptr(nullptr), s(s), d(0), b(false) {}
    LuaVal(const bool b) : tag(TBOOL), tbl_ptr(nullptr), d(0), b(b) {}
    LuaVal(LuaVal const & val) : tag(val.tag), tbl_ptr(val.tag == TTABLE ? val.tbl_ptr ? new LuaTable(*val.tbl_ptr) : new LuaTable() : nullptr), s(val.s), d(val.d), b(val.b) {}
    LuaVal(LuaVal && val) noexcept : tag(std::move(val.tag)), tbl_ptr(std::move(val.tbl_ptr)), s(std::move(val.s)), d(std::move(val.d)), b(std::move(val.b)) {}
    LuaVal(std::initializer_list<LuaVal> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeSequence(l);
    }
    template<typename T> LuaVal(std::initializer_list<T> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeSequence(l);
    }
    LuaVal(std::vector<LuaVal> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeSequence(l);
    }
    template<typename T> LuaVal(std::vector<T> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeSequence(l);
    }
    LuaVal(std::list<LuaVal> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeSequence(l);
    }
    template<typename T> LuaVal(std::list<T> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeSequence(l);
    }
    template<size_t C> LuaVal(std::array<LuaVal, C> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeSequence(l);
    }
    template<typename T, size_t C> LuaVal(std::array<T, C> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeSequence(l);
    }
    LuaVal(std::deque<LuaVal> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeSequence(l);
    }
    template<typename T> LuaVal(std::deque<T> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeSequence(l);
    }
    LuaVal(std::forward_list<LuaVal> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeSequence(l);
    }
    template<typename T> LuaVal(std::forward_list<T> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeSequence(l);
    }
    LuaVal(std::map<LuaVal, LuaVal> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeMap(l);
    }
    template<typename K, typename V> LuaVal(std::map<K, V> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeMap(l);
    }
    LuaVal(std::unordered_map<LuaVal, LuaVal> const & l) : tag(TTABLE), tbl_ptr(new LuaTable(l)), d(0), b(false)
    {
    }
    template<typename K, typename V> LuaVal(std::unordered_map<K, V> const & l) : tag(TTABLE), tbl_ptr(new LuaTable()), d(0), b(false)
    {
        InitializeMap(l);
    }
    static LuaVal table() { return LuaVal(TTABLE); }
    static LuaVal mrg(LuaVal const & l, LuaVal const & r)
    {
        LuaVal t = l;
        for (auto const & v : r.tbl())
            t[v.first] = v.second;
        return t;
    }
    static LuaVal mrg(LuaVal&& l, LuaVal&& r) { return mrg(l, std::move(r)); }
    static LuaVal mrg(LuaVal&& l, LuaVal const & r)
    {
        for (auto const & v : r.tbl())
            l[v.first] = v.second;
        return std::move(l);
    }
    static LuaVal mrg(LuaVal const & l, LuaVal&& r)
    {
        for (auto const & v : l.tbl())
            r.setignore(v.first, v.second);
        return std::move(r);
    }

    ~LuaVal() = default;

    bool isstring() const { return tag == TSTRING; }
    bool isnumber() const { return tag == TNUMBER; }
    bool istable() const { return tag == TTABLE; }
    bool isbool() const { return tag == TBOOL; }
    bool isnil() const { return tag == TNIL; }

    // gettable, adds key-nil pair if not existing
    // nil key throws error
    LuaVal & operator[](LuaVal const & k);
    LuaVal const & operator[](LuaVal const & k) const;
    // gettable
    LuaVal const & get(LuaVal const & k) const;
    // returns true if value was found with key
    bool has(LuaVal const & k) const;
    // settable, return self
    LuaVal & set(LuaVal const & k, LuaVal const & v);
    // settable ignore if exists, return self
    LuaVal & setignore(LuaVal const & k, LuaVal const & v);
    // erase, return self
    LuaVal & rem(LuaVal const & k);
    // table array size, not actual element count
    unsigned int len() const;
    // table.insert, return self
    LuaVal & insert(LuaVal const & v, LuaVal const & pos = nil);
    // table.remove, return self
    LuaVal & remove(LuaVal const & pos = nil);

    // get a number value
    double num() const
    {
        if (!isnumber())
            throw smallfolk_exception("using num on non number object");
        return d;
    }
    // get a boolean value
    bool boolean() const
    {
        if (!isbool())
            throw smallfolk_exception("using boolean on non bool object");
        return b;
    }
    // get a string value
    std::string const & str() const
    {
        if (!isstring())
            throw smallfolk_exception("using str on non string object");
        return s;
    }
    // get a table value
    LuaTable const & tbl() const
    {
        if (!istable())
            throw smallfolk_exception("using tbl on non table object");
        return *tbl_ptr;
    }

    // Returns a typetag, the internal identifier for each type
    LuaTypeTag typetag() const { return tag; }
    // Returns the LuaVal's type as a string
    std::string type() const { return type(typetag()); }
    // Returns the type tag's type as a string
    static std::string type(LuaTypeTag tag);

    // serializes the value into string
    // errmsg is optional value to output error message to on failure
    // returns empty string on error
    std::string dumps(std::string* errmsg = nullptr) const;

    // deserialize a string into a LuaVal
    // string param is deserialized string
    // errmsg is optional value to output error message to on failure
    static LuaVal loads(std::string const & string, std::string* errmsg = nullptr);

    bool operator==(LuaVal const& rhs) const;
    bool operator!=(LuaVal const& rhs) const { return !(*this == rhs); }

    // You can use !val to check for nil or false
    explicit operator bool() const;

    LuaVal& operator=(LuaVal const& val);
    LuaVal& operator=(LuaVal && val)
    {
        tag = std::move(val.tag);
        tbl_ptr = std::move(val.tbl_ptr);
        s = std::move(val.s);
        d = std::move(val.d);
        b = std::move(val.b);
        return *this;
    }

private:

    template<typename T> void InitializeSequence(T const & l)
    {
        LuaTable & tbl = *tbl_ptr;
        unsigned int i = 0;
        for (auto const & v : l)
        {
            LuaVal vv(v);
            if (vv.isnil())
                ++i;
            else
                tbl[++i] = std::move(vv);
        }
    }

    template<typename T> void InitializeMap(T const & l)
    {
        LuaTable & tbl = *tbl_ptr;
        for (auto const & e : l)
        {
            LuaVal k(e.first);
            LuaVal v(e.second);
            if (!k.isnil() && !v.isnil())
                tbl[std::move(k)] = std::move(v);
        }
    }
    
    friend size_t LuaValHash(LuaVal const & v);

    LuaTypeTag tag;
    TblPtr tbl_ptr;
    std::string s;
    // int64_t i; // lua 5.3 support?
    double d;
    bool b;
};

#endif
