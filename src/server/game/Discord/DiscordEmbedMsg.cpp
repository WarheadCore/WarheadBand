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

#include "DiscordEmbedMsg.h"
#include "GameTime.h"
#include "Log.h"
#include "Util.h"
#include <dpp/message.h>
#include <dpp/channel.h>

namespace
{
    constexpr std::size_t WARHEAED_DISCORD_MAX_EMBED_FIELDS = 24;
    constexpr std::size_t WARHEAED_DISCORD_MAX_EMBED_FIELDS_NAME = 256;
    constexpr std::size_t WARHEAED_DISCORD_MAX_EMBED_FIELDS_VALUE = 1024;
}

DiscordEmbedMsg::DiscordEmbedMsg()
{
    _message = std::make_unique<dpp::embed>();
    _message->set_timestamp(GameTime::GetGameTime().count());
}

DiscordEmbedMsg::~DiscordEmbedMsg() = default;

void DiscordEmbedMsg::SetColor(DiscordMessageColor color)
{
    _message->set_color(AsUnderlyingType(color));
}

void DiscordEmbedMsg::SetTitle(std::string_view title)
{
    _message->set_title(std::string{ title });
}

void DiscordEmbedMsg::SetDescription(std::string_view description)
{
    _message->set_description(std::string{ description });
}

void DiscordEmbedMsg::AddDescription(std::string_view description)
{
    _message->description.append(description);
}

void DiscordEmbedMsg::AddEmbedField(std::string_view name, std::string_view value, bool isInline /*= false*/)
{
    if (++_fieldsCount > WARHEAED_DISCORD_MAX_EMBED_FIELDS)
    {
        LOG_ERROR("discord", "Maximum number of fields has been reached. Skip this field");
        return;
    }

    if (name.length() > WARHEAED_DISCORD_MAX_EMBED_FIELDS_NAME)
    {
        LOG_ERROR("discord", "Maximum number of chars for field name has been reached. Skip this field");
        return;
    }

    if (value.length() > WARHEAED_DISCORD_MAX_EMBED_FIELDS_VALUE)
    {
        LOG_ERROR("discord", "Maximum number of chars for field value has been reached. Skip this field");
        return;
    }

    _message->add_field(std::string{ name }, std::string{ value }, isInline);
}

void DiscordEmbedMsg::SetFooterText(std::string_view text)
{
    _message->set_footer(std::string{ text }, "");
}