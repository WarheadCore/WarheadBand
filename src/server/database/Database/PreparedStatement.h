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

#ifndef _PREPAREDSTATEMENT_H
#define _PREPAREDSTATEMENT_H

#include "Define.h"
#include "Optional.h"
#include "SQLOperation.h"
#include <future>
#include <tuple>
#include <variant>
#include <vector>

struct PreparedStatementData
{
    std::variant<
        bool,
        uint8,
        uint16,
        uint32,
        uint64,
        int8,
        int16,
        int32,
        int64,
        float,
        double,
        std::string,
        std::vector<uint8>,
        std::nullptr_t
    > data;

    template<typename T>
    static std::string ToString(T value);

    static std::string ToString(std::nullptr_t /*value*/);
};

//- Upper-level class that is used in code
class WH_DATABASE_API PreparedStatementBase
{
friend class PreparedStatementTask;

public:
    explicit PreparedStatementBase(uint32 index, uint8 capacity);
    virtual ~PreparedStatementBase();

    // Set null
    void SetData(const uint8 index);

    // Set numerlic and default binary
    template<typename T>
    std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<std::vector<uint8>, T>> SetData(const uint8 index, T value)
    {
        SetValidData(index, value);
    }

    // Set enums
    template<typename T>
    std::enable_if_t<std::is_enum_v<T>> SetData(const uint8 index, T value)
    {
        SetValidData(index, std::underlying_type_t<T>(value));
    }

    // Set string
    template<typename T>
    std::enable_if_t<std::is_base_of_v<std::string, T> || std::is_base_of_v<std::string_view, T> || std::is_same_v<const char*, T>> SetData(const uint8 index, T value)
    {
        SetValidData(index, std::string_view{ value });
    }

    // Set non default binary
    template<std::size_t Size>
    void SetData(const uint8 index, std::array<uint8, Size> const& value)
    {
        std::vector<uint8> vec(value.begin(), value.end());
        SetValidData(index, vec);
    }

    // Set all
    template <typename... Args>
    void SetArguments(Args&&... args)
    {
        SetDataTuple(std::make_tuple(std::forward<Args>(args)...));
    }

    uint32 GetIndex() const { return m_index; }
    std::vector<PreparedStatementData> const& GetParameters() const { return statement_data; }

protected:
    template<typename T>
    void SetValidData(const uint8 index, T value);

    template<typename... Ts>
    void SetDataTuple(std::tuple<Ts...> const& argsList)
    {
        std::apply
        (
            [this](Ts const&... arguments)
            {
                uint8 index{ 0 };
                ((SetData(index, arguments), index++), ...);
            }, argsList
        );
    }

    uint32 m_index;

    //- Buffer of parameters, not tied to MySQL in any way yet
    std::vector<PreparedStatementData> statement_data;

    PreparedStatementBase(PreparedStatementBase const& right) = delete;
    PreparedStatementBase& operator=(PreparedStatementBase const& right) = delete;
};

template<typename T>
class PreparedStatement : public PreparedStatementBase
{
public:
    explicit PreparedStatement(uint32 index, uint8 capacity) : PreparedStatementBase(index, capacity)
    {
    }

private:
    PreparedStatement(PreparedStatement const& right) = delete;
    PreparedStatement& operator=(PreparedStatement const& right) = delete;
};

//- Lower-level class, enqueuable operation
class WH_DATABASE_API PreparedStatementTask : public SQLOperation
{
public:
    PreparedStatementTask(PreparedStatementBase* stmt, bool async = false);
    ~PreparedStatementTask();

    bool Execute() override;
    PreparedQueryResultFuture GetFuture() { return m_result->get_future(); }

protected:
    PreparedStatementBase* m_stmt;
    bool m_has_result;
    PreparedQueryResultPromise* m_result;
};

#endif
