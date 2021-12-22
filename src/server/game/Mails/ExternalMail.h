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

#ifndef _EXTERNAL_MAIL_H_
#define _EXTERNAL_MAIL_H_

#include "DatabaseEnvFwd.h"
#include "Common.h"
#include "ObjectGuid.h"
#include <unordered_map>

struct ExMail
{
    uint32 ID;
    ObjectGuid PlayerGuid;
    std::string Subject;
    std::string Body;
    uint32 Money;
    uint32 CreatureEntry;
    std::list<std::pair<uint32, uint32>> Items;
    std::list<std::list<std::pair<uint32, uint32>>> OverCountItems;

    bool AddItems(uint32 itemID, uint32 itemCount);
};

class ExternalMail
{
public:
    static ExternalMail* instance();

    void Update(uint32 diff);
    void LoadSystem();

    void AddMail(std::string charName, std::string const thanksSubject, std::string const thanksText, uint32 itemID, uint32 itemCount, uint32 creatureEntry);

private:
    void SendMails();
    void GetMailsFromDB();

    // Async
    void SendMailsAsync(PreparedQueryResult result);

    std::unordered_map<uint32, ExMail> _store;

    QueryCallbackProcessor _queryProcessor;
};

#define sExternalMail ExternalMail::instance()

#endif /* _EXTERNALMAIL_H_ */
