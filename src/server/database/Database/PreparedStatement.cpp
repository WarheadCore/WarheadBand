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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "PreparedStatement.h"
#include "Errors.h"

PreparedStatementBase::PreparedStatementBase(uint32 index, uint8 capacity) :
    _index(index),
    _statementData(capacity) { }

//- Bind to buffer
template<typename T>
Warhead::Types::is_non_string_view_v<T> PreparedStatementBase::SetValidData(const uint8 index, T const& value)
{
    ASSERT(index < _statementData.size());
    _statementData[index].data.emplace<T>(value);
}

// Non template functions
void PreparedStatementBase::SetValidData(const uint8 index)
{
    ASSERT(index < _statementData.size());
    _statementData[index].data.emplace<std::nullptr_t>(nullptr);
}

void PreparedStatementBase::SetValidData(const uint8 index, std::string_view value)
{
    ASSERT(index < _statementData.size(), "> Incorrect index ({}). Statement data size: {}", index, _statementData.size());
    _statementData[index].data.emplace<std::string>(value);
}

template WH_DATABASE_API void PreparedStatementBase::SetValidData(const uint8 index, uint8 const& value);
template WH_DATABASE_API void PreparedStatementBase::SetValidData(const uint8 index, int8 const& value);
template WH_DATABASE_API void PreparedStatementBase::SetValidData(const uint8 index, uint16 const& value);
template WH_DATABASE_API void PreparedStatementBase::SetValidData(const uint8 index, int16 const& value);
template WH_DATABASE_API void PreparedStatementBase::SetValidData(const uint8 index, uint32 const& value);
template WH_DATABASE_API void PreparedStatementBase::SetValidData(const uint8 index, int32 const& value);
template WH_DATABASE_API void PreparedStatementBase::SetValidData(const uint8 index, uint64 const& value);
template WH_DATABASE_API void PreparedStatementBase::SetValidData(const uint8 index, int64 const& value);
template WH_DATABASE_API void PreparedStatementBase::SetValidData(const uint8 index, bool const& value);
template WH_DATABASE_API void PreparedStatementBase::SetValidData(const uint8 index, float const& value);
template WH_DATABASE_API void PreparedStatementBase::SetValidData(const uint8 index, std::string const& value);
template WH_DATABASE_API void PreparedStatementBase::SetValidData(const uint8 index, std::vector<uint8> const& value);

template<typename T>
std::string PreparedStatementData::ToString(T value)
{
    return Warhead::StringFormat("{}", value);
}

template<>
std::string PreparedStatementData::ToString(std::vector<uint8> /*value*/)
{
    return "BINARY";
}

std::string PreparedStatementData::ToString(std::nullptr_t /*value*/)
{
    return "NULL";
}

template WH_DATABASE_API std::string PreparedStatementData::ToString(uint8);
template WH_DATABASE_API std::string PreparedStatementData::ToString(uint16);
template WH_DATABASE_API std::string PreparedStatementData::ToString(uint32);
template WH_DATABASE_API std::string PreparedStatementData::ToString(uint64);
template WH_DATABASE_API std::string PreparedStatementData::ToString(int8);
template WH_DATABASE_API std::string PreparedStatementData::ToString(int16);
template WH_DATABASE_API std::string PreparedStatementData::ToString(int32);
template WH_DATABASE_API std::string PreparedStatementData::ToString(int64);
template WH_DATABASE_API std::string PreparedStatementData::ToString(std::string);
template WH_DATABASE_API std::string PreparedStatementData::ToString(float);
template WH_DATABASE_API std::string PreparedStatementData::ToString(double);
template WH_DATABASE_API std::string PreparedStatementData::ToString(bool);
