/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FIELD_H
#define _FIELD_H

#include "DatabaseEnvFwd.h"
#include "Define.h"
#include <array>
#include <string>
#include <string_view>
#include <vector>

enum class DatabaseFieldTypes : uint8
{
    Null,
    Int8,
    Int16,
    Int32,
    Int64,
    Float,
    Double,
    Decimal,
    Date,
    Binary
};

struct QueryResultFieldMetadata
{
    std::string_view TableName = {};
    std::string_view TableAlias = {};
    std::string_view Name = {};
    std::string_view Alias = {};
    std::string_view TypeName = {};
    uint32 Index = 0;
    DatabaseFieldTypes Type = DatabaseFieldTypes::Null;
};

/**
    @class Field

    @brief Class used to access individual fields of database query result

    Guideline on field type matching:

    |   MySQL type           |  method to use                         |
    |------------------------|----------------------------------------|
    | TINYINT                | GetBool, GetInt8, GetUInt8             |
    | SMALLINT               | GetInt16, GetUInt16                    |
    | MEDIUMINT, INT         | GetInt32, GetUInt32                    |
    | BIGINT                 | GetInt64, GetUInt64                    |
    | FLOAT                  | GetFloat                               |
    | DOUBLE, DECIMAL        | GetDouble                              |
    | CHAR, VARCHAR,         | GetCString, GetString                  |
    | TINYTEXT, MEDIUMTEXT,  | GetCString, GetString                  |
    | TEXT, LONGTEXT         | GetCString, GetString                  |
    | TINYBLOB, MEDIUMBLOB,  | GetBinary, GetString                   |
    | BLOB, LONGBLOB         | GetBinary, GetString                   |
    | BINARY, VARBINARY      | GetBinary                              |

    Return types of aggregate functions:

    | Function |       Type        |
    |----------|-------------------|
    | MIN, MAX | Same as the field |
    | SUM, AVG | DECIMAL           |
    | COUNT    | BIGINT            |
*/
class WH_DATABASE_API Field
{
friend class ResultSet;
friend class PreparedResultSet;

public:
    Field();
    ~Field() = default;

    template<typename T>
    inline std::enable_if_t<std::is_arithmetic_v<T>, T> Get()
    {
        return GetData<T>();
    }

    template<typename T>
    inline std::enable_if_t<std::is_same_v<std::string, T>, T> Get() const
    {
        return GetDataString();
    }

    template<typename T>
    inline std::enable_if_t<std::is_same_v<std::string_view, T>, T> Get() const
    {
        return GetDataStringView();
    }

    template<typename T>
    inline std::enable_if_t<std::is_same_v<std::vector<uint8>, T>, T> Get() const
    {
        return GetDataBinary();
    }

    template <typename T, size_t S>
    inline std::enable_if_t<std::is_same_v<std::array<uint8, S>, T>, T> Get() const
    {
        std::array<uint8, S> buf = {};
        GetBinarySizeChecked(buf.data(), S);
        return buf;
    }

    //bool GetBool() const// Wrapper, actually gets integer
    //{
    //    return GetData<uint8>() == 1 ? true : false;
    //}

    //uint8 GetUInt8() const
    //{
    //    return GetData<uint8>();
    //}

    //int8 GetInt8() const
    //{
    //    return GetData<int8>();
    //}

    //uint16 GetUInt16() const
    //{
    //    return GetData<uint16>();
    //}

    //int16 GetInt16() const
    //{
    //    return GetData<int16>();
    //}

    //uint32 GetUInt32() const
    //{
    //    return GetData<uint32>();
    //}

    //int32 GetInt32() const
    //{
    //    return GetData<int32>();
    //}

    //uint64 GetUInt64() const
    //{
    //    return GetData<uint64>();
    //}

    //int64 GetInt64() const
    //{
    //    return GetData<int64>();
    //}

    //float GetFloat() const
    //{
    //    return GetData<float>();
    //}

    //double GetDouble() const
    //{
    //    return GetData<double>();
    //}

    //std::string GetString() const
    //{
    //    return GetDataString();
    //}

    //std::string_view GetStringView() const
    //{
    //    return GetDataStringView();
    //}

    //std::vector<uint8> GetBinary() const
    //{
    //    return GetDataBinary();
    //}

    bool IsNull() const
    {
        return data.value == nullptr;
    }

    DatabaseFieldTypes GetType() { return meta->Type; }

protected:
    struct
    {
        char const* value; // Actual data in memory
        uint32 length;     // Length
        bool raw;          // Raw bytes? (Prepared statement or ad hoc)
    } data;

    void SetByteValue(char const* newValue, uint32 length);
    void SetStructuredValue(char const* newValue, uint32 length);

    bool IsType(DatabaseFieldTypes type) const;
    bool IsNumeric() const;

private:
    QueryResultFieldMetadata const* meta;
    void LogWrongType(char const* getter) const;
    void SetMetadata(QueryResultFieldMetadata const* fieldMeta);

    void GetBinarySizeChecked(uint8* buf, size_t size) const;

    template<typename T>
    T GetData() const;

    std::string GetDataString() const;
    std::string_view GetDataStringView() const;
    std::vector<uint8> GetDataBinary() const;
};

#endif
