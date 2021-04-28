/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 * Copyright (C) 2021+ WarheadCore <https://github.com/WarheadCore>
 */

#ifndef _SERVER_AUTO_SHUTDOWN_H_
#define _SERVER_AUTO_SHUTDOWN_H_

#include "Common.h"

class ServerAutoShutdown
{
public:
    static ServerAutoShutdown* instance();

    void Init();
    void OnUpdate(uint32 diff);

private:
    bool _isEnableModule = false;
};

#define sSAS ServerAutoShutdown::instance()

#endif /* _SERVER_AUTO_SHUTDOWN_H_ */
