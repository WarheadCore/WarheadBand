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

#include "PetLoadQueryHolder.h"
#include "DatabaseEnv.h"

PetLoadQueryHolder::PetLoadQueryHolder(ObjectGuid::LowType ownerGuid, uint32 petNumber)
{
    CharacterDatabasePreparedStatement stmt = nullptr;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_DECLINED_NAME);
    stmt->SetData(0, ownerGuid);
    stmt->SetData(1, petNumber);
    AddPreparedQuery(DECLINED_NAMES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_AURA);
    stmt->SetData(0, petNumber);
    AddPreparedQuery(AURAS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_SPELL);
    stmt->SetData(0, petNumber);
    AddPreparedQuery(SPELLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_SPELL_COOLDOWN);
    stmt->SetData(0, petNumber);
    AddPreparedQuery(COOLDOWNS, stmt);
}
