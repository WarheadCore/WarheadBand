/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Field.h"
#include "Errors.h"

Field::Field()
{
    data.value = nullptr;
    data.type = MYSQL_TYPE_NULL;
    data.length = 0;
    data.raw = false;
}

Field::~Field()
{
    CleanUp();
}

void Field::SetByteValue(const void* newValue, const size_t newSize, enum_field_types newType, uint32 length)
{
    if (data.value)
        CleanUp();

    // This value stores raw bytes that have to be explicitly cast later
    if (newValue)
    {
        data.value = new char[newSize];
        memcpy(data.value, newValue, newSize);
        data.length = length;
    }
    data.type = newType;
    data.raw = true;
}

void Field::SetStructuredValue(char* newValue, enum_field_types newType, uint32 length)
{
    if (data.value)
        CleanUp();

    // This value stores somewhat structured data that needs function style casting
    if (newValue)
    {
        data.value = new char[length + 1];
        memcpy(data.value, newValue, length);
        *(reinterpret_cast<char*>(data.value) + length) = '\0';
        data.length = length;
    }

    data.type = newType;
    data.raw = false;
}

std::vector<uint8> Field::GetBinary() const
{
    std::vector<uint8> result;
    if (!data.value || !data.length)
        return result;

    result.resize(data.length);
    memcpy(result.data(), data.value, data.length);
    return result;
}

void Field::GetBinarySizeChecked(uint8* buf, size_t length) const
{
    ASSERT(data.value && (data.length == length));
    memcpy(buf, data.value, length);
}
