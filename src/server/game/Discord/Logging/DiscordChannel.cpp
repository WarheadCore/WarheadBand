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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "DiscordChannel.h"
#include "DiscordMgr.h"
#include "Exception.h"
#include "Log.h"
#include "LogMessage.h"
#include "StringConvert.h"

Warhead::DiscordChannel::DiscordChannel(std::string_view name, LogLevel level, std::string_view pattern, std::vector<std::string_view> const& /*options*/) :
    LogChannel(ThisChannelType, name, level, pattern) { }

void Warhead::DiscordChannel::Write(LogMessage const& msg)
{
    // Disable logging if bot disable
    if (!sDiscordMgr->IsEnable())
        return;

    if (msg.GetText().empty() || msg.GetText() == " ")
        return;

    std::lock_guard<std::mutex> guard(_mutex);
    std::string text{ msg.GetText() };

    // Replace text with pattern
    Format(msg, text);

    auto color{ DiscordMessageColor::Teal };

    switch (msg.GetLevel())
    {
        case LogLevel::Critical:
        case LogLevel::Fatal:
        case LogLevel::Error:
            color = DiscordMessageColor::Red;
            break;
        case LogLevel::Warning:
            color = DiscordMessageColor::Orange;
            break;
        case LogLevel::Info:
            color = DiscordMessageColor::Cyan;
            break;
        case LogLevel::Trace:
            color = DiscordMessageColor::Indigo;
            break;
    }

    auto embedMsg = std::make_unique<DiscordEmbedMsg>();
    embedMsg->SetTitle("Core logs");
    embedMsg->SetDescription(text);
    embedMsg->SetColor(color);

    sDiscordMgr->SendLogMessage(std::move(embedMsg), DiscordChannelType::CoreLogs);
}
