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

#ifndef __GITREVISION_H__
#define __GITREVISION_H__

#include "Define.h"

namespace GitRevision
{
    WH_COMMON_API char const* GetHash();
    WH_COMMON_API char const* GetDate();
    WH_COMMON_API char const* GetBranch();
    WH_COMMON_API char const* GetCMakeCommand();
    WH_COMMON_API char const* GetCMakeVersion();
    WH_COMMON_API char const* GetHostOSVersion();
    WH_COMMON_API char const* GetBuildDirectory();
    WH_COMMON_API char const* GetSourceDirectory();
    WH_COMMON_API char const* GetMySQLExecutable();
    WH_COMMON_API char const* GetFullDatabase();
    WH_COMMON_API char const* GetFullVersion();
    WH_COMMON_API char const* GetCompanyNameStr();
    WH_COMMON_API char const* GetLegalCopyrightStr();
    WH_COMMON_API char const* GetFileVersionStr();
    WH_COMMON_API char const* GetProductVersionStr();
}

#endif
