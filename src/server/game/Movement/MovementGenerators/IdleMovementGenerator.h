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

#ifndef WARHEAD_IDLEMOVEMENTGENERATOR_H
#define WARHEAD_IDLEMOVEMENTGENERATOR_H

#include "MovementGenerator.h"

class WH_GAME_API IdleMovementGenerator : public MovementGenerator
{
public:
    void Initialize(Unit*) override;
    void Finalize(Unit*) override {  }
    void Reset(Unit*) override;
    bool Update(Unit*, uint32) override { return true; }
    MovementGeneratorType GetMovementGeneratorType() override { return IDLE_MOTION_TYPE; }
};

WH_GAME_API extern IdleMovementGenerator si_idleMovement;

class WH_GAME_API RotateMovementGenerator : public MovementGenerator
{
public:
    explicit RotateMovementGenerator(uint32 time, RotateDirection direction) : m_duration(time), m_maxDuration(time), m_direction(direction) {}

    void Initialize(Unit*) override;
    void Finalize(Unit*) override;
    void Reset(Unit* owner) override { Initialize(owner); }
    bool Update(Unit*, uint32) override;
    MovementGeneratorType GetMovementGeneratorType() override { return ROTATE_MOTION_TYPE; }

private:
    uint32 m_duration, m_maxDuration;
    RotateDirection m_direction;
};

class WH_GAME_API DistractMovementGenerator : public MovementGenerator
{
public:
    explicit DistractMovementGenerator(uint32 timer) : m_timer(timer) {}

    void Initialize(Unit*) override;
    void Finalize(Unit*) override;
    void Reset(Unit* owner) override { Initialize(owner); }
    bool Update(Unit*, uint32) override;
    MovementGeneratorType GetMovementGeneratorType() override { return DISTRACT_MOTION_TYPE; }

private:
    uint32 m_timer;
};

class WH_GAME_API AssistanceDistractMovementGenerator : public DistractMovementGenerator
{
public:
    AssistanceDistractMovementGenerator(uint32 timer) :
        DistractMovementGenerator(timer) {}

    MovementGeneratorType GetMovementGeneratorType() override { return ASSISTANCE_DISTRACT_MOTION_TYPE; }
    void Finalize(Unit*) override;
};

#endif
