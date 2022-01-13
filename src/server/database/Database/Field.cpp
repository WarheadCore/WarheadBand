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
#include "Types.h"
#include "StringConvert.h"

Field::Field()
{
    data.value = nullptr;
    data.length = 0;
    data.raw = false;
    meta = nullptr;
}

//uint8 Field::GetUInt8() const
//{
//    if (!data.value)
//        return 0;
//
//#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
//    if (!IsType(DatabaseFieldTypes::Int8))
//    {
//        LogWrongType(__FUNCTION__);
//        return 0;
//    }
//#endif
//
//    if (data.raw)
//        return *reinterpret_cast<uint8 const*>(data.value);
//
//    return static_cast<uint8>(strtoul(data.value, nullptr, 10));
//}
//
//int8 Field::GetInt8() const
//{
//    if (!data.value)
//        return 0;
//
//#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
//    if (!IsType(DatabaseFieldTypes::Int8))
//    {
//        LogWrongType(__FUNCTION__);
//        return 0;
//    }
//#endif
//
//    if (data.raw)
//        return *reinterpret_cast<int8 const*>(data.value);
//
//    return static_cast<int8>(strtol(data.value, nullptr, 10));
//}
//
//uint16 Field::GetUInt16() const
//{
//    if (!data.value)
//        return 0;
//
//#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
//    if (!IsType(DatabaseFieldTypes::Int16))
//    {
//        LogWrongType(__FUNCTION__);
//        return 0;
//    }
//#endif
//
//    if (data.raw)
//        return *reinterpret_cast<uint16 const*>(data.value);
//
//    return static_cast<uint16>(strtoul(data.value, nullptr, 10));
//}
//
//int16 Field::GetInt16() const
//{
//    if (!data.value)
//        return 0;
//
//#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
//    if (!IsType(DatabaseFieldTypes::Int16))
//    {
//        LogWrongType(__FUNCTION__);
//        return 0;
//    }
//#endif
//
//    if (data.raw)
//        return *reinterpret_cast<int16 const*>(data.value);
//
//    return static_cast<int16>(strtol(data.value, nullptr, 10));
//}
//
//uint32 Field::GetUInt32() const
//{
//    if (!data.value)
//        return 0;
//
//#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
//    if (!IsType(DatabaseFieldTypes::Int32))
//    {
//        LogWrongType(__FUNCTION__);
//        return 0;
//    }
//#endif
//
//    if (data.raw)
//        return *reinterpret_cast<uint32 const*>(data.value);
//
//    return static_cast<uint32>(strtoul(data.value, nullptr, 10));
//}
//
//int32 Field::GetInt32() const
//{
//    if (!data.value)
//        return 0;
//
//#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
//    if (!IsType(DatabaseFieldTypes::Int32))
//    {
//        LogWrongType(__FUNCTION__);
//        return 0;
//    }
//#endif
//
//    if (data.raw)
//        return *reinterpret_cast<int32 const*>(data.value);
//
//    return static_cast<int32>(strtol(data.value, nullptr, 10));
//}
//
//uint64 Field::GetUInt64() const
//{
//    if (!data.value)
//        return 0;
//
//#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
//    if (!IsType(DatabaseFieldTypes::Int64))
//    {
//        LogWrongType(__FUNCTION__);
//        return 0;
//    }
//#endif
//
//    if (data.raw)
//        return *reinterpret_cast<uint64 const*>(data.value);
//
//    return static_cast<uint64>(strtoull(data.value, nullptr, 10));
//}
//
//int64 Field::GetInt64() const
//{
//    if (!data.value)
//        return 0;
//
//#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
//    if (!IsType(DatabaseFieldTypes::Int64))
//    {
//        LogWrongType(__FUNCTION__);
//        return 0;
//    }
//#endif
//
//    if (data.raw)
//        return *reinterpret_cast<int64 const*>(data.value);
//
//    return static_cast<int64>(strtoll(data.value, nullptr, 10));
//}
//
//float Field::GetFloat() const
//{
//    if (!data.value)
//        return 0.0f;
//
//#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
//    if (!IsType(DatabaseFieldTypes::Float))
//    {
//        LogWrongType(__FUNCTION__);
//        return 0.0f;
//    }
//#endif
//
//    if (data.raw)
//        return *reinterpret_cast<float const*>(data.value);
//
//    return static_cast<float>(atof(data.value));
//}
//
//double Field::GetDouble() const
//{
//    if (!data.value)
//        return 0.0f;
//
//#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
//    if (!IsType(DatabaseFieldTypes::Double) && !IsType(DatabaseFieldTypes::Decimal))
//    {
//        LogWrongType(__FUNCTION__);
//        return 0.0f;
//    }
//#endif
//
//    if (data.raw && !IsType(DatabaseFieldTypes::Decimal))
//        return *reinterpret_cast<double const*>(data.value);
//
//    return static_cast<double>(atof(data.value));
//}
//
//std::string Field::GetString() const
//{
//    return data.value;
//}
//
//std::string_view Field::GetStringView() const
//{
//    if (data.value.empty())
//        return {};
//
//    return { data.value };
//}
//
//std::vector<uint8> Field::GetBinary() const
//{
//    std::vector<uint8> result;
//    if (data.value.empty() || !data.length)
//        return result;
//
//    result.resize(data.length);
//    memcpy(result.data(), data.value.c_str(), data.length);
//    return result;
//}

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

void Field::LogWrongType(char const* getter) const
{
    LOG_WARN("sql.sql", "Warning: {} on {} field {}.{} ({}.{}) at index {}.",
        getter, meta->TypeName, meta->TableAlias, meta->Alias, meta->TableName, meta->Name, meta->Index);
}

void Field::SetMetadata(QueryResultFieldMetadata const* fieldMeta)
{
    meta = fieldMeta;
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
}

template<typename T>
T Field::GetData() const
{
    static_assert(std::is_arithmetic_v<T>, "Unsurropt type for Field::GetData()");

    if (!data.value)
        return GetDefaultValue<T>();

#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
    if (!IsType(DatabaseFieldTypes::Int8))
    {
        LogWrongType(__FUNCTION__);
        return GetDefaultValue<T>();
    }
#endif

    Optional<T> result = {};

    if (data.raw)
        result = *reinterpret_cast<T const*>(data.value);
    else
        result = Warhead::StringTo<T>(data.value);

    if (!result)
    {
        LOG_FATAL("sql.sql", "> Incorrect value '{}' for type '{}'", data.value, typeid(T).name());
        return GetDefaultValue<T>();
    }

    return *result;
}

template uint8 Field::GetData() const;
template uint16 Field::GetData() const;
template uint32 Field::GetData() const;
template uint64 Field::GetData() const;
template int8 Field::GetData() const;
template int16 Field::GetData() const;
template int32 Field::GetData() const;
template int64 Field::GetData() const;
template float Field::GetData() const;

std::string Field::GetDataString() const
{
    if (!data.value)
        return "";

#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
    if (!IsType(DatabaseFieldTypes::Int8))
    {
        LogWrongType(__FUNCTION__);
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
    if (!IsType(DatabaseFieldTypes::Int8))
    {
        LogWrongType(__FUNCTION__);
        return {};
    }
#endif

    return { data.value, data.length };
}

std::vector<uint8> Field::GetDataBinary() const
{
    if (!data.value)
        return {};

#ifdef WARHEAD_STRICT_DATABASE_TYPE_CHECKS
    if (!IsType(DatabaseFieldTypes::Int8))
    {
        LogWrongType(__FUNCTION__);
        return {};
    }
#endif

    std::vector<uint8> result = {};
    if (!data.value || !data.length)
        return result;

    result.resize(data.length);
    memcpy(result.data(), data.value, data.length);
    return result;
}
