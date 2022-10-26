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

#ifndef _DISCORD_EMBED_MSG_H_
#define _DISCORD_EMBED_MSG_H_

#include "DisccordCommon.h"
#include <memory>
#include <string_view>

namespace dpp
{
    struct embed;
}

class WH_GAME_API DiscordEmbedMsg
{
public:
    DiscordEmbedMsg();
    ~DiscordEmbedMsg() = default;

    inline dpp::embed const* GetMessage() const { return _message.get(); }

    void SetColor(DiscordMessageColor color);
    void SetTitle(std::string_view title);
    void SetDescription(std::string_view description);
    void AddDescription(std::string_view description);
    void AddEmbedField(std::string_view name, std::string_view value, bool isInline = false);
    void SetFooterText(std::string_view text);

private:
    std::unique_ptr<dpp::embed> _message;
    std::size_t _fieldsCount{};
};

#endif
