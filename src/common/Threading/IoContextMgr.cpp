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

#include "IoContextMgr.h"
#include "IoContext.h"

Warhead::IoContextMgr::IoContextMgr() :
    _ioContext(std::make_shared<Asio::IoContext>()) { }

/*static*/ Warhead::IoContextMgr* Warhead::IoContextMgr::instance()
{
    static IoContextMgr instance;
    return &instance;
}

void Warhead::IoContextMgr::Run()
{
    _ioContext->run();
}

void Warhead::IoContextMgr::Stop()
{
    _ioContext->stop();
}

Warhead::Asio::IoContext& Warhead::IoContextMgr::GetIoContext()
{
    return *_ioContext;
}

void Warhead::IoContextMgr::Post(std::function<void()>&& work)
{
    Warhead::Asio::post(*_ioContext, std::move(work));
}
