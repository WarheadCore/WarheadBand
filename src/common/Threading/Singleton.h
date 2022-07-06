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

#ifndef _WARHEAD_SINGLETON_H_
#define _WARHEAD_SINGLETON_H_

#include "Define.h"

namespace Warhead
{
    template<class T>
    class WH_COMMON_API Singleton
    {
    public:
        static inline T* instance()
        {
            static T instance{};
            return &instance;
        }

    protected:
        constexpr Singleton() = default;
        ~Singleton() = default;

        Singleton(Singleton const&) = delete;
        Singleton(Singleton&&) = delete;
        Singleton& operator=(Singleton const&) = delete;
        Singleton& operator=(Singleton&&) = delete;
    };
}

#endif
