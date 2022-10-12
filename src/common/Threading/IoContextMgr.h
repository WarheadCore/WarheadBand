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

#ifndef _IO_CONTEXT_MGR_H_
#define _IO_CONTEXT_MGR_H_

#include "Define.h"
#include "AsioHacksFwd.h"
#include <memory>
#include <functional>

namespace Warhead
{
    class WH_COMMON_API IoContextMgr
    {
    public:
        IoContextMgr();
        ~IoContextMgr() = default;

        static IoContextMgr* instance();

        void Run();
        void Stop();
        void Post(std::function<void()>&& work);

        inline std::shared_ptr<Asio::IoContext> GetIoContextPtr() { return _ioContext; }
        Asio::IoContext& GetIoContext();

    private:
        std::shared_ptr<Asio::IoContext> _ioContext;
    };
}

#define sIoContextMgr Warhead::IoContextMgr::instance()

#endif