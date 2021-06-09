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

#ifndef _DATA_MAP_H_
#define _DATA_MAP_H_

#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

class DataMap
{
public:
    /**
     * Base class that you should inherit in your script.
     * Inheriting classes can be stored to DataMap
     */
    class Base
    {
    public:
        virtual ~Base() = default;
    };

    /**
     * Returns a pointer to object of requested type stored with given key or nullptr
     */
    template<class T> T* Get(std::string const& k) const
    {
        static_assert(std::is_base_of<Base, T>::value, "T must derive from Base");
        if (Container.empty())
            return nullptr;

        auto it = Container.find(k);
        if (it != Container.end())
            return dynamic_cast<T*>(it->second.get());
        return nullptr;
    }

    /**
     * Returns a pointer to object of requested type stored with given key
     * or default constructs one and returns that one
     */
    template<class T, typename std::enable_if<std::is_default_constructible<T>::value, int>::type = 0>
    T * GetDefault(std::string const& k)
    {
        static_assert(std::is_base_of<Base, T>::value, "T must derive from Base");
        if (T* v = Get<T>(k))
            return v;
        T* v = new T();
        Container.emplace(k, std::unique_ptr<T>(v));
        return v;
    }

    /**
     * Stores a new object that inherits the Base class with the given key
     */
    void Set(std::string const& k, Base* v) { Container[k] = std::unique_ptr<Base>(v); }

    /**
     * Removes objects with given key and returns true if one was removed, false otherwise
     */
    bool Erase(std::string const& k) { return Container.erase(k) != 0; }

private:
    std::unordered_map<std::string, std::unique_ptr<Base>> Container;
};

#endif
