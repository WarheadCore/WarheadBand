# smallfolk_cpp

[![Build Status](https://travis-ci.org/Rochet2/smallfolk_cpp.svg?branch=master)](https://travis-ci.org/Rochet2/smallfolk_cpp)

Smallfolk_cpp is a library for representing `Lua` values in `C++` and (de)serializing them. The serialization is made to work with smallfolk serializer made for lua. Most serializer logic is borrowed from gvx/Smallfolk. https://github.com/gvx/Smallfolk

Smallfolk_cpp does not have dependencies other than `C++11` and it **does not need lua**. It simply uses same format and logic as gvx/Smallfolk for serialization.

Smallfolk_cpp has its own type `LuaVal` to represent `Lua` values in `C++`.
They allow representing bool, number, string, nil and table.

Due to implementation difficulties and security some features of gvx/Smallfolk are not supported. A version of smallfolk for lua with the unsupported features removed can be found at https://github.com/Rochet2/Smallfolk

You use, distribute and extend Smallfolk_cpp under the terms of the MIT license.

## Usage

```C++
#include smallfolk.h

// create a lua table and set some values to it
LuaVal table = LuaVal::table();
table[1] = "Hello"; // the values is automatically converted to LuaVal
table["test"] = "world";
table[67.5] = -234.5;

// serialize the table
std::string serialized = table.dumps();

// print the serialization, it should be rather human readable
// Example output: {"Hello","test":"world",67.5:-234.5}
std::cout << serialized << std::endl;

// form lua values from the string
LuaVal deserialized = LuaVal::loads(serialized);

// print the values from deserialized result table
// Example output: Hello world -234.5
std::cout << deserialized[1].str() << " ";
std::cout << deserialized["test"].str() << " ";
std::cout << deserialized[67.5].num() << std::endl;
```

## Fast

Its C++, duh!?

Some poor benchmarking shows that plain serializing takes ~0.01ms. If creating, serializing and destroying created objects ~0.025ms. Deserializing takes ~0.05ms when destroying the created objects as well.
This is of course completely different depending on what data you serialize and deserialize.
In general it would seem that deserializing is ~50% slower.

To put this into any kind of perspective, here is the print of the serialized data:
```lua
{t,"somestring",123.456,t:-678,"test":123.45600128173828,f:268435455,"subtable":{1,2,3}}
```

## Table cycles

__Note: This feature was disabled cause of difficult implementing in C++ and possibly unwanted infinite cycles. All table assigning create copies now in the C++ code and no @ notation is recognised for serializing or deserializing. Any such references are set to nil when deserializing. Any @ references are otherwise deep copies in the C++ code__

From original smallfolk
> Sometimes you have strange, non-euclidean geometries in your table
> constructions. It happens, I don't judge. Smallfolk can deal with that, where
> some other serialization libraries (or anything that produces JSON) cry "Iä!
> Iä! Cthulhu fhtagn!" and give up &mdash; or worse, silently produce incorrect
> data.
>
> ```C++
> #include smallfolk.h
>
> // Essentially {{},{},{}}
> LuaVal cthulhu(TTABLE);
> cthulhu[1] = LuaVal(TTABLE);
> cthulhu[2] = LuaVal(TTABLE);
> cthulhu[3] = LuaVal(TTABLE);
> cthulhu["fhtagn"] = cthulhu;
> cthulhu[1][cthulhu[2]] = cthulhu[3];
> cthulhu[2][cthulhu[1]] = cthulhu[2];
> cthulhu[3][cthulhu[3]] = cthulhu;
> std::cout << cthulhu.dumps() << std::endl;
> // prints:
> // {"fhtagn":@1,1:{{@2:@3}:{@4:@1}},2:@3,3:@4}
> ```

## Security

I cannot guarantee that this code is secure. All I can give is that I have attempted to make it safe and implemented exceptions best I know to handle unexpected situations.

## Tested

All tests can be seen in the main.cpp provided.
The code has been in use with a server-client C++-Lua communication system called AIO through which the API has been made more usable and critical issues have been addressed.
- https://github.com/Rochet2/AIO
- https://github.com/Rochet2/TrinityCore/tree/c_aio
- https://github.com/SaiFi0102/TrinityCore/tree/CAIO-3.3.5

## Reference

### try-catch
Most functions can throw `smallfolk_exception` and some string library errors and possibly more.
One method for try catching errors you can use is this:
```C++
try {
  // smallfolk_cpp code
}
catch (smallfolk_exception& e) {
    std::cout << e.what() << std::endl;
}
```

You need to catch exceptions mostly from incorrect handling of LuaVal. For example trying to access a number like a table will cause an exception.

### serializing
Serializing happens by calling the member function `std::string LuaVal::dumps(std::string* errmsg = nullptr)`. When an error occurs with the serialization an empty string is returned and if errmsg points to a string then it is filled with the error message.
This function does not throw.

### deserializing
Deserializing happens by calling the function `static LuaVal LuaVal::loads(std::string const & string, std::string* errmsg = nullptr)`. When an error occurs with the deserialization a LuaVal representing a nil is returned and if errmsg points to a string then it is filled with the error message.
This function does not throw.

### LuaVal
LuaVal is a type used to represent lua values in C++. LuaVal has a range of functions to access the underlying values and to construct LuaVal from different values. LuaVal is the input for serialization and output of deserialization.

### LuaVal constructors
Constructors allow implicitly constructing values.
Constructors do not throw. Watch out for quirks with initializer list constructor: http://stackoverflow.com/questions/26947704/implicit-conversion-failure-from-initializer-list
```C++
LuaVal implicit_test = -123;
LuaVal copy_test(implicit_test);
LuaVal copy_test2 = implicit_test;
LuaVal n; // nil
LuaVal b(true);
LuaVal s("a string");
LuaVal d(123.456);
LuaVal f(123.456f);
LuaVal i(-678);
LuaVal u(0xFFFFFFF);
LuaVal t1(TTABLE); // create a value through typetag creates empty or zero initialized value
LuaVal t2 = LuaVal::table(); // another way to create tables
LuaVal t3 = { 1, 2, { 1,2,3 } }; // initializer list is supported for making sequences (tables)
LuaVal t4 = LuaVal::LuaTable{ { "key", "value" }, { 2, "value2" } }; // Table can be created with map table initializer list constructor also

// watch out - depending on C++ version serializes to {2:{},3:{3},4:4}
LuaVal quirks = { {}, {{}}, { 3 }, { LuaVal(4) } };

// You can mix and match a lot of different types and containers for creating tables.
// For example vectors, lists, maps, arrays are supported for creating LuaVal.
std::vector<std::list<std::string>> vec = {{"a", "b"},{"a", "b"}};
LuaVal t5 = {1,2, "test", vec};
// Resulting table: {1,2,"test",{{"a","b"},{"a","b"}}}
```

Creating sequences is easy, but creating complex tables that contain different types of values can be difficult or take a lot of space in code. To avoid quirks and for conveience you can deserialize strings to create values in a compact way. Here two equivalent values are created with normal style and deserialization:
```c++
LuaVal val1 = { 1,2, LuaVal::mrg({3,4.5}, LuaVal::LuaTable({{"ke","test"}})) };
LuaVal val2 = LuaVal::loads("{1,2,{3,4.5,'ke':'test'}}");
```

### static nil
A static value `static const LuaVal LuaVal::nil` is a preconstructed nil object.
It can be used as a default value or return value when a const nil value reference is needed to avoid constructing unnecessary copies.

### hash
The LuaVal class contains a hasher `LuaVal::LuaValHasher`. You need to use it when you use a LuaVal in a hash container for example: `std::unordered_set<LuaVal, LuaVal::LuaValHasher> myset;` or `std::unordered_map<LuaVal, int, LuaVal::LuaValHasher> mymap;`.
Currently there are no order operators implemented to be used for sorted sets and maps however.
May throw if LuaVal is not valid for some reason (which should not be possible).

### typetag
There are definitions for typetags used to identify each value type. These can be used in the constructor of a LuaValue as well.
For example a table can be created with `LuaValue table(TTABLE)`. You can get the typetag of an object with the member function `LuaTypeTag LuaVal::typetag()`.
GetTypeTag does not throw.
```C++
enum LuaTypeTag
{
    TNIL,
    TSTRING,
    TNUMBER,
    TTABLE,
    TBOOL,
};
```

### tostring
The member function `std::string LuaVal::tostring()` returns a string representation of the object. This is similar to tostring in lua.
You can get a string representation of the typetag of a value with `value.type()`.
You can get a string representation of a typetag with `LuaVal::type(tag)`.
All of these may throw if LuaVal or tag is not valid for some reason (which should not be possible).

### operators
The LuaVal class offers a few operators.  
You can use == and != operators to compare, however different table objects are copies so they are never equal unless you actually compare with the same object.
LuaVal has the bool operator implemented so that nil and false will return false if a LuaVal is in a conditional statement. The assignment operator is also implemented and works as you would expect.
May throw if LuaVal is not valid for some reason (which should not be possible).

### isvalue
There is a collection of member functions you can use to check whether the object is really of some type.
These functions do not throw.
```C++
luaval.isstring()
luaval.isnumber()
luaval.istable()
luaval.isbool()
luaval.isnil()
```

### LuaVal values
LuaVal can represent different types of data like a string and a number. To access the underlying value you must use specific functions.
The functions will throw if you use them on the wrong type object, for example using the str function on a table will throw.
```C++
luaval.num()
luaval.str()
luaval.boolean()
luaval.tbl()
```

### table access
There are several methods for accessing and editing a table.
**Note Inserted values will be deep copies in all cases.**

The way of accessing and inserting map elements are the get and set member functions `luaval.get(key)`, `luaval.set(key, value)`.
The function `set` returns the accessed table itself, so you can chain it to set multiple values.
When a value is attempted to be set as nil, it will be erased from the table instead.
These functions do not throw unless you use them on non table objects or with nil keys. `luaval.setignore(key, value)` works like `luaval.set(key, value)`, except it will not do anything if a value already exists in the table for that key.

The get method above will provide only const reference access to the table elements. For non const access to elements you must use the `[]` operator like so `luaval[key]`. If the accessed key does not exist in the accessed table then a nil value is created to the table for that key. This means that accessing nonexisting elements will create clutter to the table. Setting a value to nil will not erase it from the table.
This operator does not throw unless you use it on non table objects or with nil keys.

`luaval.has(key)` can be used to check if a value can be found in a table.
This function do not throw unless you use it on non table objects or with nil keys.

A method for erasing data with a key is `luaval.rem(key)` which also returns the accessed table.
This function do not throw unless you use it on non table objects or with nil keys.

Example usage of the functions:
```C++
LuaVal table(TTABLE); // create an empty table
table.set(1, "test").set(2, 77.234).set(3, -324); // set multiple values
table.set("self copy", table); // attempting to set a table into itself will create a deep copy
table.set(table, "table as key?"); // table will work as a key, but it will be a deep copy so you can not access it later
std::cout << table.get("self copy").get(3).num() << std::endl; // get a value from a nested table
table["number"] = 234; // Use table access operator to assign a value
LuaVal & value = table["number"]; // Use table access operator to get a value
table.set(e, LuaVal::nil).rem("number"); // remove some values through set and rem functions
if (table.has(100) and table[100].isstring())
	std::cout << table[100].str() << std::end;
```

For conveniency tables also have the methods `luaval.insert(value[, pos])`, `luaval.remove([pos])` and `luaval.len()`.
The len function returns the number of consecutive integer key elements in the table starting at index 1. It is similar to the # operator in lua.
Insert and remove shift the values on the right side of the given position and insert or remove a value to or at the given position. If position is omitted, the value is inserted to the end of the list or the last element is removed.
Insert and remove both return the accessed table.
Each function throws if used on a non table object or pos is not valid.

### table merging
You can merge two tables with `LuaVal::mrg(tbl1, tbl2)`. This will make a new table that contains values from both tables. If they have same keys then tbl2 will overwrite tbl1 value in the new table.
