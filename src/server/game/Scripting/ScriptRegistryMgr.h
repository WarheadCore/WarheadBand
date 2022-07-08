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

#ifndef _WARHEAD_SCRIPT_REGISTY_H_
#define _WARHEAD_SCRIPT_REGISTY_H_

#include "Singleton.h"
#include <unordered_map>
#include <vector>
#include <memory>

template<class T>
class ScriptRegistryMgr : public Warhead::Singleton<ScriptRegistryMgr<T>>
{
public:
    void AddScript(T* const script);
    void AddAfterLoadDBScripts();

    // Gets a script by its ID (assigned by ObjectMgr).
    T* GetScriptById(uint32 id);

    inline auto const& GetScriptPointerList() { return &_scriptPointerList; }
    inline auto const& GetScriptAfterLoadDBList() { return &_scriptAfterLoadDBList; }

    inline void ClearPoinerList() { _scriptPointerList.clear(); }

private:
    // See if the script is using the same memory as another script. If this happens, it means that
    // someone forgot to allocate new memory for a script.
    bool CheckSameMemory(T* const script);

    // Counter used for code-only scripts.
    std::size_t _scriptIdCounter{ 0 };

    std::unordered_map<uint32, std::unique_ptr<T>> _scriptPointerList;
    std::vector<std::unique_ptr<T>> _scriptAfterLoadDBList;
};

#define sScriptRegistryMgr(__scriptName) ScriptRegistryMgr<__scriptName>::instance()

#endif
