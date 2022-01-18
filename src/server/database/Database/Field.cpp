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

#include "Field.h"
#include "Errors.h"
#include "Log.h"
#include "MySQLHacks.h"
#include "StringConvert.h"
#include "Types.h"

Field::Field()
{
    data.value = nullptr;
    data.length = 0;
    data.raw = false;
    meta = nullptr;
}

namespace
{
    template<typename T>
    constexpr T GetDefaultValue()
    {
        if constexpr (std::is_same_v<T, bool>)
            return false;
        else if constexpr (std::is_integral_v<T>)
            return 0;
        else if constexpr (std::is_floating_point_v<T>)
            return 1.0f;
        else if constexpr (std::is_same_v<T, std::vector<uint8>> || std::is_same_v<std::string_view, T>)
            return {};
        else
            return "";
    }

    template<typename T>
    inline constexpr bool IsCorrectFieldType(DatabaseFieldTypes type)
    {
        // Int8
        if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int8> || std::is_same_v<T, uint8>)
        {
            if (type == DatabaseFieldTypes::Int8)
                return true;
        }

        // In16
        if constexpr (std::is_same_v<T, uint16> || std::is_same_v<T, int16>)
        {
            if (type == DatabaseFieldTypes::Int16)
                return true;
        }

        // Int32
        if constexpr (std::is_same_v<T, uint32> || std::is_same_v<T, int32>)
        {
            if (type == DatabaseFieldTypes::Int32)
                return true;
        }

        // Int64
        if constexpr (std::is_same_v<T, uint64> || std::is_same_v<T, int64>)
        {
            if (type == DatabaseFieldTypes::Int64)
                return true;
        }

        // float
        if constexpr (std::is_same_v<T, float>)
        {
            if (type == DatabaseFieldTypes::Float)
                return true;
        }

        // dobule
        if constexpr (std::is_same_v<T, double>)
        {
            if (type == DatabaseFieldTypes::Double || type == DatabaseFieldTypes::Decimal)
                return true;
        }

        // Binary
        if constexpr (std::is_same_v<T, Binary>)
        {
            if (type == DatabaseFieldTypes::Binary)
                return true;
        }

        return false;
    }
}

void Field::GetBinarySizeChecked(uint8* buf, size_t length) const
{
    ASSERT(data.value && (data.length == length), "Expected {}-byte binary blob, got {}data ({} bytes) instead", length, data.value ? "" : "no ", data.length);
    memcpy(buf, data.value, length);
}

void Field::SetByteValue(char const* newValue, uint32 length)
{
    // This value stores raw bytes that have to be explicitly cast later
    data.value = newValue;
    data.length = length;
    data.raw = true;
}

void Field::SetStructuredValue(char const* newValue, uint32 length)
{
    // This value stores somewhat structured data that needs function style casting
    data.value = newValue;
    data.length = length;
    data.raw = false;
}

bool Field::IsType(DatabaseFieldTypes type) const
{
    return meta->Type == type;
}

bool Field::IsNumeric() const
{
    return (meta->Type == DatabaseFieldTypes::Int8 ||
        meta->Type == DatabaseFieldTypes::Int16 ||
        meta->Type == DatabaseFieldTypes::Int32 ||
        meta->Type == DatabaseFieldTypes::Int64 ||
        meta->Type == DatabaseFieldTypes::Float ||
        meta->Type == DatabaseFieldTypes::Double);
}

void Field::LogWrongType(std::string_view getter, std::string_view typeName) const
{
    LOG_WARN("sql.sql", "Warning: {}<{}> on {} field {}.{} ({}.{}) at index {}.",
        getter, typeName, meta->TypeName, meta->TableAlias, meta->Alias, meta->TableName, meta->Name, meta->Index);
}

void Field::SetMetadata(QueryResultFieldMetadata const* fieldMeta)
{
    meta = fieldMeta;
}

template<typename T>
T Field::GetData() const
{
    static_assert(std::is_arithmetic_v<T>, "Unsurropt type for Field::GetData()");

    if (!data.value)
        return GetDefaultValue<T>();

#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
    if (!IsCorrectFieldType<T>(meta->Type))
    {
        LogWrongType(__FUNCTION__, typeid(T).name());
        //return GetDefaultValue<T>();
    }
#endif

    Optional<T> result = {};

    if (data.raw)
        result = *reinterpret_cast<T const*>(data.value);
    else
        result = Warhead::StringTo<T>(data.value);

    // Check -1 for *_dbc db tables
    if constexpr (std::is_same_v<T, uint32>)
    {
        std::string_view tableName{ meta->TableName };

        if (!tableName.empty() && tableName.size() > 4)
        {
            auto signedResult = Warhead::StringTo<int32>(data.value);

            if (signedResult && !result && tableName.substr(tableName.length() - 4) == "_dbc")
            {
                LOG_DEBUG("sql.sql", "> Found incorrect value '{}' for type '{}' in _dbc table.", data.value, typeid(T).name());
                LOG_DEBUG("sql.sql", "> Table name '{}'. Field name '{}'. Try return int32 value", meta->TableName, meta->Name);
                return GetData<int32>();
            }
        }
    }

    if (!result)
    {
        LOG_FATAL("sql.sql", "> Incorrect value '{}' for type '{}'. Value is raw ? '{}'", data.value, typeid(T).name(), data.raw);
        LOG_FATAL("sql.sql", "> Table name '{}'. Field name '{}'", meta->TableName, meta->Name);
        return GetDefaultValue<T>();
    }

    return *result;
}

template bool Field::GetData() const;
template uint8 Field::GetData() const;
template uint16 Field::GetData() const;
template uint32 Field::GetData() const;
template uint64 Field::GetData() const;
template int8 Field::GetData() const;
template int16 Field::GetData() const;
template int32 Field::GetData() const;
template int64 Field::GetData() const;
template float Field::GetData() const;
template double Field::GetData() const;

std::string Field::GetDataString() const
{
    if (!data.value)
        return "";

#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
    if (IsNumeric() && data.raw)
    {
        LogWrongType(__FUNCTION__, "std::string");
        return "";
    }
#endif

    return { data.value, data.length };
}

std::string_view Field::GetDataStringView() const
{
    if (!data.value)
        return {};

#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
    if (IsNumeric() && data.raw)
    {
        LogWrongType(__FUNCTION__, "std::string_view");
        return {};
    }
#endif

    return { data.value, data.length };
}

Binary Field::GetDataBinary() const
{
    Binary result = {};
    if (!data.value || !data.length)
        return result;

#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
    if (!IsCorrectFieldType<Binary>(meta->Type))
    {
        LogWrongType(__FUNCTION__, "Binary");
        return {};
    }
#endif

    result.resize(data.length);
    memcpy(result.data(), data.value, data.length);
    return result;
}
