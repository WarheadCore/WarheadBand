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

#ifndef _WARHEAD_SIGNAL_HANDLER_H_
#define _WARHEAD_SIGNAL_HANDLER_H_

#include "Define.h"
#include <boost/asio/signal_set.hpp>
#include <functional>
#include <memory>

namespace Warhead
{
    class WH_COMMON_API SignalHandlerMgr
    {
    public:
        SignalHandlerMgr() = default;
        ~SignalHandlerMgr();

        static SignalHandlerMgr* instance();

        void Initialize(std::function<void()>&& execute = {});
        void Stop();

    private:
        std::unique_ptr<boost::asio::signal_set> _signalSet;
    };
}

#define sSignalMgr Warhead::SignalHandlerMgr::instance()

#endif