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

#ifndef WARHEAD_TARGETEDMOVEMENTGENERATOR_H
#define WARHEAD_TARGETEDMOVEMENTGENERATOR_H

#include "FollowerReference.h"
#include "MovementGenerator.h"
#include "PathGenerator.h"
#include "Timer.h"
#include "Unit.h"
#include <optional>

class WH_GAME_API TargetedMovementGeneratorBase
{
public:
    TargetedMovementGeneratorBase(Unit* target) { i_target.link(target, this); }
    void stopFollowing() { }
protected:
    FollowerReference i_target;
};

template<class T>
class ChaseMovementGenerator : public MovementGeneratorMedium<T, ChaseMovementGenerator<T>>, public TargetedMovementGeneratorBase
{
public:
    ChaseMovementGenerator(Unit* target, std::optional<ChaseRange> range = {}, std::optional<ChaseAngle> angle = {})
        : TargetedMovementGeneratorBase(target), i_path(nullptr), i_recheckDistance(0), i_recalculateTravel(true), _range(range), _angle(angle) {}
    ~ChaseMovementGenerator() { }

    MovementGeneratorType GetMovementGeneratorType() { return CHASE_MOTION_TYPE; }

    bool DoUpdate(T*, uint32);
    void DoInitialize(T*);
    void DoFinalize(T*);
    void DoReset(T*);
    void MovementInform(T*);

    bool PositionOkay(T* owner, Unit* target, std::optional<float> maxDistance, std::optional<ChaseAngle> angle);

    void unitSpeedChanged() { _lastTargetPosition.reset(); }
    Unit* GetTarget() const { return i_target.getTarget(); }

    bool EnableWalking() const { return false; }
    bool HasLostTarget(Unit* unit) const { return unit->GetVictim() != this->GetTarget(); }

private:
    std::unique_ptr<PathGenerator> i_path;
    TimeTrackerSmall i_recheckDistance;
    bool i_recalculateTravel;

    std::optional<Position> _lastTargetPosition;
    std::optional<ChaseRange> const _range;
    std::optional<ChaseAngle> const _angle;
    bool _movingTowards = true;
    bool _mutualChase = true;
};

template<class T>
class FollowMovementGenerator : public MovementGeneratorMedium<T, FollowMovementGenerator<T>>, public TargetedMovementGeneratorBase
{
public:
    FollowMovementGenerator(Unit* target, float range, ChaseAngle angle)
        : TargetedMovementGeneratorBase(target), i_path(nullptr), i_recheckDistance(0), i_recalculateTravel(true), _range(range), _angle(angle) {}
    ~FollowMovementGenerator() { }

    MovementGeneratorType GetMovementGeneratorType() { return FOLLOW_MOTION_TYPE; }

    bool DoUpdate(T*, uint32);
    void DoInitialize(T*);
    void DoFinalize(T*);
    void DoReset(T*);
    void MovementInform(T*);

    Unit* GetTarget() const { return i_target.getTarget(); }

    void unitSpeedChanged() { _lastTargetPosition.reset(); }

    bool PositionOkay(T* owner, Unit* target, float range, std::optional<ChaseAngle> angle = {});

    static void _clearUnitStateMove(T* u) { u->ClearUnitState(UNIT_STATE_FOLLOW_MOVE); }
    static void _addUnitStateMove(T* u) { u->AddUnitState(UNIT_STATE_FOLLOW_MOVE); }

    void _updateSpeed(T* owner);

private:
    std::unique_ptr<PathGenerator> i_path;
    TimeTrackerSmall i_recheckDistance;
    bool i_recalculateTravel;

    std::optional<Position> _lastTargetPosition;
    float _range;
    ChaseAngle _angle;
};

#endif
