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

#ifndef _DBC_DATABASE_H_
#define _DBC_DATABASE_H_

#include "DatabaseWorkerPool.h"

enum DBCDatabaseStatements : uint32
{
    /*  Naming standard for defines:
        {DB}_{SEL/INS/UPD/DEL/REP}_{Summary of data changed}
        When updating more than one field, consider looking at the calling function
        name for a suiting suffix.
    */

//    DBC_SEL_TABLE,

    MAX_DBC_DATABASE_STATEMENTS
};

class WH_DATABASE_API DBCDatabasePool : public DatabaseWorkerPool
{
public:
    DBCDatabasePool() { SetType(DatabaseType::Dbc); }
    ~DBCDatabasePool() = default;

    //- Loads database type specific prepared statements
    void DoPrepareStatements() override;
};

/// Accessor to the dbc database
WH_DATABASE_API extern DBCDatabasePool DBCDatabase;

#endif // _DBC_DATABASE_H_
