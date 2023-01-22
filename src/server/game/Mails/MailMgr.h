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

#ifndef WARHEAD_MAIL_MGR_H_
#define WARHEAD_MAIL_MGR_H_

#include "MailFwd.h"
#include "DatabaseEnvFwd.h"
#include "Duration.h"
#include "ObjectGuid.h"
#include <memory>
#include <unordered_map>

class Item;
class Object;
class Player;

struct AuctionEntry;
struct CalendarEvent;

struct WH_GAME_API GameMail
{
    uint32 MessageID{};
    MailMessageType MessageType{};
    MailStationery Stationery{ MAIL_STATIONERY_DEFAULT };
    uint16 MailTemplateId{};
    uint32 Sender{};
    ObjectGuid::LowType Receiver{};
    std::string Subject;
    std::string Body;
    Seconds ExpireTime;
    Seconds DeliverTime;
    Copper Money;
    Copper COD;
    MailCheckMask Checked{};
    MailState State{ MAIL_STATE_UNCHANGED };

    //
    bool IsCanPrepareItems{ true };

    inline void BuildHeaders(std::string_view subject, std::string_view body)
    {
        Subject = subject;
        Body = body;
    }

    void BuildSender(MailMessageType messageType, uint32 guidLowOrEntry, MailStationery stationery = MAIL_STATIONERY_DEFAULT);
    void BuildSender(Object* sender, MailStationery stationery = MAIL_STATIONERY_DEFAULT);
    void BuildSender(CalendarEvent* sender);
    void BuildSender(AuctionEntry* sender);
    void BuildSender(Player* sender);
    void BuildSender(uint32 senderEntry);

    void BuildExpireTime(Seconds delay = 0s, bool withItems = false);

    void SaveMailToDB(CharacterDatabaseTransaction trans);

    void AddMailItem(Item* item);
    bool DeleteMailItem(ObjectGuid::LowType lowGuid, bool includeDB = false);
    inline auto& GetItems() const { return _items; }

private:
    std::unordered_map<ObjectGuid::LowType/*item low guid*/, Item*> _items;
};

using MailItems = std::unordered_map<ObjectGuid::LowType, uint32/*item id*/>;

class WH_GAME_API MailMgr
{
public:
    static MailMgr* instance();

    void Update(Milliseconds diff);

    static std::shared_ptr<GameMail> CreateGameMail();
    void SendMail(CharacterDatabaseTransaction trans, std::shared_ptr<GameMail> mail);

    void AddMailItem(uint32 mailID, ObjectGuid::LowType lowGuid, uint32 itemID);
    void AddMailItems(std::shared_ptr<GameMail> mail);
    void DeleteMailItem(uint32 mailID, ObjectGuid::LowType lowGuid);

private:
    void PrepareItemsForPlayer(Player* player, std::shared_ptr<GameMail> mail, CharacterDatabaseTransaction trans, uint32 existItems = 0);
    void SaveMailToDB(uint32 mailID, CharacterDatabaseTransaction trans, std::shared_ptr<GameMail> mail);

    std::unordered_map<ObjectGuid/*mail owner*/, std::vector<std::shared_ptr<GameMail>>> _mails;
    std::unordered_map<uint32/*mail id*/, MailItems> _mailItems;
};

#define sMailMgr MailMgr::instance()

#endif
