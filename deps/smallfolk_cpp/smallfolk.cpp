#include "smallfolk.h"
#include <map>
#include <iomanip> // std::setprecision
#include <sstream> // std::stringstream
#include <cmath> // std::floor
#include <stdarg.h> // va_start
#include <functional> // std::hash

namespace Serializer
{
    typedef std::vector<LuaVal> TABLES;
    typedef std::unordered_map<LuaVal, unsigned int, LuaVal::LuaValHasher> MEMO;
    typedef std::stringstream ACC;

    // sprintf is ~50% faster than other solutions
    inline std::string tostring(const double d)
    {
        char arr[128];
        sprintf(arr, "%.17g", d);
        return arr;
    }
    inline std::string tostring(LuaVal::TblPtr const & ptr)
    {
        char arr[128];
        sprintf(arr, "table: %p", static_cast<void*>(ptr.get()));
        return arr;
    }

    unsigned int dump_type_table(LuaVal const & object, unsigned int nmemo, MEMO& memo, ACC& acc);
    unsigned int dump_object(LuaVal const & object, unsigned int nmemo, MEMO& memo, ACC& acc);
    std::string escape_quotes(const std::string &before, char quote);
    std::string unescape_quotes(const std::string &before, char quote);
    bool nonzero_digit(char c);
    bool is_digit(char c);
    char strat(std::string const & string, std::string::size_type i);
    LuaVal expect_number(std::string const & string, size_t& start);
    LuaVal expect_object(std::string const & string, size_t& i, TABLES& tables);
}

LuaVal const LuaVal::nil;

std::string LuaVal::tostring() const
{
    switch (tag)
    {
    case TBOOL:
        if (b)
            return "true";
        else
            return "false";
    case TNIL:
        return "nil";
    case TSTRING:
        return s;
    case TNUMBER:
        return Serializer::tostring(d);
    case TTABLE:
        return Serializer::tostring(tbl_ptr);
    }
    throw smallfolk_exception("tostring invalid or unhandled tag %i", tag);
}

size_t LuaVal::LuaValHasher::operator()(LuaVal const & v) const
{
    return LuaValHash(v);
}

size_t LuaValHash(LuaVal const & v)
{
    switch (v.tag)
    {
    case TBOOL:
        return std::hash<bool>()(v.b);
    case TNIL:
        return std::hash<int>()(0);
    case TSTRING:
        return std::hash<std::string>()(v.s);
    case TNUMBER:
        return std::hash<double>()(v.d);
    case TTABLE:
        return std::hash<LuaVal::TblPtr>()(v.tbl_ptr);
    }
    return std::hash<std::string>()(v.tostring());
}

LuaVal & LuaVal::operator[](LuaVal const & k)
{
    if (!istable())
        throw smallfolk_exception("using [] on non table object");
    if (k.isnil())
        throw smallfolk_exception("using [] with nil key");
    LuaTable & tbl = (*tbl_ptr);
    return tbl[k];
}

LuaVal const & LuaVal::operator[](LuaVal const & k) const
{
    return get(k);
}

LuaVal const & LuaVal::get(LuaVal const & k) const
{
    if (!istable())
        throw smallfolk_exception("using get on non table object");
    if (k.isnil())
        throw smallfolk_exception("using get with nil key");
    LuaTable & tbl = (*tbl_ptr);
    auto it = tbl.find(k);
    if (it != tbl.end())
        return it->second;
    return nil;
}

bool LuaVal::has(LuaVal const & k) const
{
    if (!istable())
        throw smallfolk_exception("using has on non table object");
    if (k.isnil())
        throw smallfolk_exception("using has with nil key");
    LuaTable & tbl = (*tbl_ptr);
    auto it = tbl.find(k);
    return it != tbl.end();
}

LuaVal & LuaVal::set(LuaVal const & k, LuaVal const & v)
{
    if (!istable())
        throw smallfolk_exception("using set on non table object");
    if (k.isnil())
        throw smallfolk_exception("using set with nil key");
    LuaTable & tbl = (*tbl_ptr);
    if (v.isnil()) // on nil value erase key
        tbl.erase(k);
    else
        tbl[k] = v; // normally set pair
    return *this;
}

LuaVal & LuaVal::setignore(LuaVal const & k, LuaVal const & v)
{
    if (!istable())
        throw smallfolk_exception("using setignore on non table object");
    if (k.isnil())
        throw smallfolk_exception("using setignore with nil key");
    if (v.isnil())
        return *this;
    LuaTable & tbl = (*tbl_ptr);
    tbl.emplace(k, v);
    return *this;
}

LuaVal & LuaVal::rem(LuaVal const & k)
{
    if (!istable())
        throw smallfolk_exception("using rem on non table object");
    if (k.isnil())
        throw smallfolk_exception("using set with nil key");
    LuaTable & tbl = (*tbl_ptr);
    tbl.erase(k);
    return *this;
}

unsigned int LuaVal::len() const
{
    if (!istable())
        throw smallfolk_exception("using len on non table object");
    LuaTable & tbl = (*tbl_ptr);
    unsigned int i = 0;
    while (++i)
    {
        auto it = tbl.find(i);
        if (it == tbl.end() || it->second.isnil())
            break;
    }
    return i - 1;
}

LuaVal & LuaVal::insert(LuaVal const & v, LuaVal const & pos)
{
    if (!istable())
        throw smallfolk_exception("using insert on non table object");
    LuaTable & tbl = (*tbl_ptr);
    if (pos.isnil())
    {
        if (!v.isnil())
            tbl[len() + 1] = v;
        return *this;
    }
    if (!pos.isnumber())
        throw smallfolk_exception("using insert with non number pos");
    if (std::floor(pos.num()) != pos.num())
        throw smallfolk_exception("using insert with invalid number key");
    unsigned int max = len() + 1;
    unsigned int val = static_cast<unsigned int>(pos.num());
    if (val <= 0 || val > max)
        throw smallfolk_exception("using insert with out of bounds key");
    for (unsigned int i = max; i > val; --i)
        tbl[i] = tbl[i - 1];
    if (v.isnil())
        tbl.erase(val);
    else
        tbl[val] = v;
    return *this;
}

LuaVal & LuaVal::remove(LuaVal const & pos)
{
    if (!istable())
        throw smallfolk_exception("using remove on non table object");
    LuaTable & tbl = (*tbl_ptr);
    if (pos.isnil())
    {
        if (unsigned int i = len())
            tbl.erase(i);
        return *this;
    }
    if (!pos.isnumber())
        throw smallfolk_exception("using remove with non number key");
    if (std::floor(pos.num()) != pos.num())
        throw smallfolk_exception("using remove with invalid number key");
    unsigned int max = len();
    unsigned int val = static_cast<unsigned int>(pos.num());
    if (val <= 0 || val > max + 1)
        throw smallfolk_exception("using remove with out of bounds key");
    for (unsigned int i = val; i < max; ++i)
        tbl[i] = tbl[i + 1];
    tbl.erase(max);
    return *this;
}

std::string LuaVal::type(LuaTypeTag tag)
{
    switch (tag)
    {
        case TBOOL:
            return "boolean";
        case TNIL:
            return "nil";
        case TSTRING:
            return "string";
        case TNUMBER:
            return "number";
        case TTABLE:
            return "table";
    }
    throw smallfolk_exception("tostring invalid or unhandled tag %i", tag);
}

std::string LuaVal::dumps(std::string * errmsg) const
{
    try
    {
        Serializer::ACC acc;
        acc << std::setprecision(17); // min lua percision
        unsigned int nmemo = 0;
        Serializer::MEMO memo;
        Serializer::dump_object(*this, nmemo, memo, acc);
        return acc.str();
    }
    catch (smallfolk_exception const & e)
    {
        if (errmsg)
            *errmsg += e.what();
    }
    return std::string();
}

LuaVal LuaVal::loads(std::string const & string, std::string * errmsg)
{
    try
    {
        Serializer::TABLES tables;
        size_t i = 0;
        return Serializer::expect_object(string, i, tables);
    }
    catch (smallfolk_exception const & e)
    {
        if (errmsg)
            *errmsg += e.what();
    }
    return LuaVal();
}

bool LuaVal::operator==(LuaVal const& rhs) const
{
    if (tag != rhs.tag)
        return false;
    switch (tag)
    {
    case TBOOL:
        return b == rhs.b;
    case TNIL:
        return true;
    case TSTRING:
        return s == rhs.s;
    case TNUMBER:
        return d == rhs.d;
    case TTABLE:
        return tbl_ptr == rhs.tbl_ptr;
    }
    throw smallfolk_exception("operator== invalid or unhandled tag %i", tag);
}

LuaVal::operator bool() const
{
    return !isnil() && (!isbool() || boolean());
}

LuaVal& LuaVal::operator=(LuaVal const& val)
{
    tag = val.tag;
    if (istable())
        tbl_ptr.reset(new LuaTable(*val.tbl_ptr));
    else
        tbl_ptr = nullptr;
    s = val.s;
    d = val.d;
    b = val.b;
    return *this;
}

unsigned int Serializer::dump_type_table(LuaVal const & object, unsigned int nmemo, MEMO & memo, ACC & acc)
{
    if (!object.istable())
        throw smallfolk_exception("using dump_type_table on non table object");

    /*
    auto it = memo.find(object);
    if (it != memo.end())
    {
    acc << '@' << it->second;
    return nmemo;
    }
    memo[object] = ++nmemo;
    */
    acc << '{';
    std::map<unsigned int, const LuaVal*> arr;
    std::unordered_map<const LuaVal*, const LuaVal*> hash;
    for (auto&& v : object.tbl())
    {
        if (v.first.isnumber() && v.first.num() >= 1 && std::floor(v.first.num()) == v.first.num())
            arr[static_cast<unsigned int>(v.first.num())] = &v.second;
        else
            hash[&v.first] = &v.second;
    }
    unsigned int i = 1;
    for (auto&& v : arr)
    {
        if (v.first != i)
        {
            nmemo = dump_object(v.first, nmemo, memo, acc);
            acc << ':';
        }
        else
            ++i;
        nmemo = dump_object(*v.second, nmemo, memo, acc);
        acc << ',';
    }
    for (auto&& v : hash)
    {
        nmemo = dump_object(*v.first, nmemo, memo, acc);
        acc << ':';
        nmemo = dump_object(*v.second, nmemo, memo, acc);
        acc << ',';
    }
    std::string l = acc.str();
    char c = strat(l, l.length() - 1);
    if (c != '{')
    {
        // remove last char
        l.pop_back();
        acc.clear();
        acc.str(std::string());
        acc << l;
    }
    acc << '}';
    return nmemo;
}

unsigned int Serializer::dump_object(LuaVal const & object, unsigned int nmemo, MEMO & memo, ACC & acc)
{
    switch (object.typetag())
    {
    case TBOOL:
        acc << (object.boolean() ? 't' : 'f');
        break;
    case TNIL:
        acc << 'n';
        break;
    case TSTRING:
        acc << '"';
        acc << escape_quotes(object.str(), '"'); // change to std::quote() in c++14?
        acc << '"';
        break;
    case TNUMBER:
        if (!std::isfinite(object.num()))
        {
            // slightly ugly :(
            std::string nn = tostring(object.num());
            if (nn == "inf")
                acc << 'I';
            else if (nn == "-inf")
                acc << 'i';
            else if (nn == "-nan(ind)")
                acc << 'N';
            else if (nn == "nan")
                acc << 'Q';
            else
                acc << 'I';
        }
        else
            acc << object.num();
        break;
    case TTABLE:
        return dump_type_table(object, nmemo, memo, acc);
        break;
    default:
        throw smallfolk_exception("dump_object invalid or unhandled tag %i", object.typetag());
        break;
    }
    return nmemo;
}

std::string Serializer::escape_quotes(const std::string & before, char quote)
{
    std::string after;
    after.reserve(before.length() + 4);

    for (std::string::size_type i = 0; i < before.length(); ++i)
    {
        if (before[i] == quote)
            after += quote; // no break
        else
            after += before[i];
    }

    return after;
}

std::string Serializer::unescape_quotes(const std::string & before, char quote)
{
    std::string after;
    after.reserve(before.length());

    for (std::string::size_type i = 0; i < before.length(); ++i)
    {
        if (before[i] == quote)
        {
            if (i + 1 < before.length() && before[i + 1] == quote)
                ++i;
        }
        else
            after += before[i];
    }

    return after;
}

bool Serializer::nonzero_digit(char c)
{
    switch (c)
    {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return true;
    }
    return false;
}

bool Serializer::is_digit(char c)
{
    switch (c)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return true;
    }
    return false;
}

char Serializer::strat(std::string const & string, std::string::size_type i)
{
    if (i != std::string::npos &&
        i < string.length())
        return string.at(i);
    return '\0'; // bad?
}

LuaVal Serializer::expect_number(std::string const & string, size_t & start)
{
    size_t i = start;
    char head = strat(string, i);
    if (head == '-')
        head = strat(string, ++i);
    if (nonzero_digit(head))
    {
        do
        {
            head = strat(string, ++i);
        } while (is_digit(head));
    }
    else if (head == '0')
        head = strat(string, ++i);
    else
        throw smallfolk_exception("expect_number at %u unexpected character %c", i, head);
    if (head == '.')
    {
        size_t oldi = i;
        do
        {
            head = strat(string, ++i);
        } while (is_digit(head));
        if (i == oldi + 1)
            throw smallfolk_exception("expect_number at %u no numbers after decimal", i);
    }
    if (head == 'e' || head == 'E')
    {
        head = strat(string, ++i);
        if (head == '+' || head == '-')
            head = strat(string, ++i);
        if (!is_digit(head))
            throw smallfolk_exception("expect_number at %u not a digit part %c", i, head);
        do
        {
            head = strat(string, ++i);
        } while (is_digit(head));
    }
    size_t temp = start;
    start = i;
    return std::atof(string.substr(temp, i).c_str());
}

LuaVal Serializer::expect_object(std::string const & string, size_t & i, Serializer::TABLES & tables)
{
    static double _zero = 0.0;

    char cc = strat(string, i++);
    switch (cc)
    {
    case ' ':
    case '\t':
        // skip whitespace
        return expect_object(string, i, tables);
    case 't':
        return true;
    case 'f':
        return false;
    case 'n':
        return LuaVal();
    case 'Q':
        return -(0 / _zero);
    case 'N':
        return (0 / _zero);
    case 'I':
        return (1 / _zero);
    case 'i':
        return -(1 / _zero);
    case '\'':
    case '"':
    {
        size_t nexti = i - 1;
        do
        {
            nexti = string.find(cc, nexti + 1);
            if (nexti == std::string::npos)
            {
                throw smallfolk_exception("expect_object at %u was %c eof before string ends", i, cc);
            }
            ++nexti;
        } while (strat(string, nexti) == cc);
        size_t temp = i;
        i = nexti;
        return unescape_quotes(string.substr(temp, nexti - temp - 1), cc);
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
    case '.':
        return expect_number(string, --i);
    case '{':
    {
        LuaVal nt(TTABLE);
        unsigned int j = 1;
        tables.push_back(nt);
        if (strat(string, i) == '}')
        {
            ++i;
            return nt;
        }
        while (true)
        {
            LuaVal k = expect_object(string, i, tables);
            char at = strat(string, i);
            while (at == ' ')
                at = strat(string, ++i);
            if (at == ':')
            {
                nt.set(k, expect_object(string, ++i, tables));
            }
            else
            {
                nt.set(j, k);
                ++j;
            }
            char head = strat(string, i);
            while (head == ' ')
                head = strat(string, ++i);
            if (head == ',')
                ++i;
            else if (head == '}')
            {
                ++i;
                return nt;
            }
            else
            {
                throw smallfolk_exception("expect_object at %u was %c unexpected character %c", i, cc, head);
            }
        }
        break;
    }
    /*
    case '@':
    {
    std::string::size_type x = i;
    for (; x < string.length(); ++x)
    {
    if (!isdigit(string[x]))
    break;
    }
    std::string substr = string.substr(i, x - i);
    size_t index = std::stoul(substr.c_str());
    if (index >= 1 && index <= tables.size())
    {
    i += substr.length();
    return tables[index - 1];
    }

    throw smallfolk_exception("expect_object at %u was %c invalid index %u", i, cc, index);
    break;
    }
    */
    default:
    {
        throw smallfolk_exception("expect_object at %u was %c", i, cc);
        break;
    }
    }
    return LuaVal();
}

smallfolk_exception::smallfolk_exception(const char * format, ...) : std::logic_error("Smallfolk exception")
{
    char buffer[size];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, size, format, args);
    errmsg = std::string("Smallfolk: ") + buffer;
    va_end(args);
}

const char * smallfolk_exception::what() const throw()
{
    return errmsg.c_str();
}
