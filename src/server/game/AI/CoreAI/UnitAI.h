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

#ifndef WARHEAD_UNITAI_H
#define WARHEAD_UNITAI_H

#include "Containers.h"
#include "Define.h"
#include "Unit.h"
#include <list>

class Player;
class Quest;
class Unit;
struct AISpellInfoType;

//Selection method used by SelectTarget
enum SelectAggroTarget
{
    SELECT_TARGET_RANDOM = 0,                               //Just selects a random target
    SELECT_TARGET_TOPAGGRO,                                 //Selects targes from top aggro to bottom
    SELECT_TARGET_BOTTOMAGGRO,                              //Selects targets from bottom aggro to top
    SELECT_TARGET_NEAREST,
    SELECT_TARGET_FARTHEST,
};

// default predicate function to select target based on distance, player and/or aura criteria
struct WH_GAME_API DefaultTargetSelector
{
public:
    // unit: the reference unit
    // dist: if 0: ignored, if > 0: maximum distance to the reference unit, if < 0: minimum distance to the reference unit
    // playerOnly: self explaining
    // aura: if 0: ignored, if > 0: the target shall have the aura, if < 0, the target shall NOT have the aura
    DefaultTargetSelector(Unit const* unit, float dist, bool playerOnly, int32 aura);
    bool operator()(Unit const* target) const;

private:
    const Unit* me;
    float m_dist;
    bool m_playerOnly;
    int32 m_aura;
};

// Target selector for spell casts checking range, auras and attributes
// TODO: Add more checks from Spell::CheckCast
struct WH_GAME_API SpellTargetSelector
{
public:
    SpellTargetSelector(Unit* caster, uint32 spellId);
    bool operator()(Unit const* target) const;

private:
    Unit const* _caster;
    SpellInfo const* _spellInfo;
};

// Very simple target selector, will just skip main target
// NOTE: When passing to UnitAI::SelectTarget remember to use 0 as position for random selection
//       because tank will not be in the temporary list
struct WH_GAME_API NonTankTargetSelector
{
public:
    NonTankTargetSelector(Creature* source, bool playerOnly = true);
    bool operator()(Unit const* target) const;

private:
    Creature const* _source;
    bool _playerOnly;
};

// Simple selector for units using mana
struct WH_GAME_API PowerUsersSelector
{
public:
    PowerUsersSelector(Unit const* unit, Powers power, float dist, bool playerOnly);
    bool operator()(Unit const* target) const;

private:
    Unit const* _me;
    Powers const _power;
    float const _dist;
    bool const _playerOnly;
};

struct WH_GAME_API FarthestTargetSelector
{
public:
    FarthestTargetSelector(Unit const* unit, float dist, bool playerOnly, bool inLos);
    bool operator()(Unit const* target) const;

private:
    const Unit* _me;
    float _dist;
    bool _playerOnly;
    bool _inLos;
};

class WH_GAME_API UnitAI
{
protected:
    Unit* const me;
public:
    explicit UnitAI(Unit* unit) : me(unit) {}
    virtual ~UnitAI() {}

    virtual bool CanAIAttack(Unit const* /*target*/) const { return true; }
    virtual void AttackStart(Unit* /*target*/);
    virtual void UpdateAI(uint32 diff) = 0;

    virtual void InitializeAI() { if (!me->isDead()) Reset(); }

    virtual void Reset() {};

    // Called when unit is charmed
    virtual void OnCharmed(bool apply) = 0;

    // Pass parameters between AI
    virtual void DoAction(int32 /*param*/) {}
    virtual uint32 GetData(uint32 /*id = 0*/) const { return 0; }
    virtual void SetData(uint32 /*id*/, uint32 /*value*/) {}
    virtual void SetGUID(ObjectGuid /*guid*/, int32 /*id*/ = 0) {}
    virtual ObjectGuid GetGUID(int32 /*id*/ = 0) const { return ObjectGuid::Empty; }

    Unit* SelectTarget(SelectAggroTarget targetType, uint32 position = 0, float dist = 0.0f, bool playerOnly = false, int32 aura = 0);
    // Select the targets satifying the predicate.
    // predicate shall extend Warhead::unary_function<Unit*, bool>
    template <class PREDICATE> Unit* SelectTarget(SelectAggroTarget targetType, uint32 position, PREDICATE const& predicate)
    {
        ThreatContainer::StorageType const& threatlist = me->getThreatMgr().getThreatList();
        if (position >= threatlist.size())
            return nullptr;

        std::list<Unit*> targetList;
        for (ThreatContainer::StorageType::const_iterator itr = threatlist.begin(); itr != threatlist.end(); ++itr)
            if (predicate((*itr)->getTarget()))
                targetList.push_back((*itr)->getTarget());

        if (position >= targetList.size())
            return nullptr;

        if (targetType == SELECT_TARGET_NEAREST || targetType == SELECT_TARGET_FARTHEST)
            targetList.sort(Warhead::ObjectDistanceOrderPred(me));

        switch (targetType)
        {
            case SELECT_TARGET_NEAREST:
            case SELECT_TARGET_TOPAGGRO:
                {
                    std::list<Unit*>::iterator itr = targetList.begin();
                    std::advance(itr, position);
                    return *itr;
                }
            case SELECT_TARGET_FARTHEST:
            case SELECT_TARGET_BOTTOMAGGRO:
                {
                    std::list<Unit*>::reverse_iterator ritr = targetList.rbegin();
                    std::advance(ritr, position);
                    return *ritr;
                }
            case SELECT_TARGET_RANDOM:
                {
                    std::list<Unit*>::iterator itr = targetList.begin();
                    std::advance(itr, urand(position, targetList.size() - 1));
                    return *itr;
                }
            default:
                break;
        }

        return nullptr;
    }

    void SelectTargetList(std::list<Unit*>& targetList, uint32 num, SelectAggroTarget targetType, float dist = 0.0f, bool playerOnly = false, int32 aura = 0);

    // Select the targets satifying the predicate.
    // predicate shall extend Warhead::unary_function<Unit*, bool>
    template <class PREDICATE> void SelectTargetList(std::list<Unit*>& targetList, PREDICATE const& predicate, uint32 maxTargets, SelectAggroTarget targetType)
    {
        ThreatContainer::StorageType const& threatlist = me->getThreatMgr().getThreatList();
        if (threatlist.empty())
            return;

        for (ThreatContainer::StorageType::const_iterator itr = threatlist.begin(); itr != threatlist.end(); ++itr)
            if (predicate((*itr)->getTarget()))
                targetList.push_back((*itr)->getTarget());

        if (targetList.size() < maxTargets)
            return;

        if (targetType == SELECT_TARGET_NEAREST || targetType == SELECT_TARGET_FARTHEST)
            targetList.sort(Warhead::ObjectDistanceOrderPred(me));

        if (targetType == SELECT_TARGET_FARTHEST || targetType == SELECT_TARGET_BOTTOMAGGRO)
            targetList.reverse();

        if (targetType == SELECT_TARGET_RANDOM)
            Warhead::Containers::RandomResize(targetList, maxTargets);
        else
            targetList.resize(maxTargets);
    }

    // Called at any Damage to any victim (before damage apply)
    virtual void DamageDealt(Unit* /*victim*/, uint32& /*damage*/, DamageEffectType /*damageType*/) { }

    // Called at any Damage from any attacker (before damage apply)
    // Note: it for recalculation damage or special reaction at damage
    // for attack reaction use AttackedBy called for not DOT damage in Unit::DealDamage also
    virtual void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damagetype*/, SpellSchoolMask /*damageSchoolMask*/ ) {}

    // Called when the creature receives heal
    virtual void HealReceived(Unit* /*done_by*/, uint32& /*addhealth*/) {}

    // Called when the unit heals
    virtual void HealDone(Unit* /*done_to*/, uint32& /*addhealth*/) {}

    void AttackStartCaster(Unit* victim, float dist);

    void DoAddAuraToAllHostilePlayers(uint32 spellid);
    void DoCast(uint32 spellId);
    void DoCast(Unit* victim, uint32 spellId, bool triggered = false);
    inline void DoCastSelf(uint32 spellId, bool triggered = false) { DoCast(me, spellId, triggered); }
    void DoCastToAllHostilePlayers(uint32 spellid, bool triggered = false);
    void DoCastVictim(uint32 spellId, bool triggered = false);
    void DoCastAOE(uint32 spellId, bool triggered = false);

    float DoGetSpellMaxRange(uint32 spellId, bool positive = false);

    void DoMeleeAttackIfReady();
    bool DoSpellAttackIfReady(uint32 spell);

    static AISpellInfoType* AISpellInfo;
    static void FillAISpellInfo();

    virtual void sGossipHello(Player* /*player*/) {}
    virtual void sGossipSelect(Player* /*player*/, uint32 /*sender*/, uint32 /*action*/) {}
    virtual void sGossipSelectCode(Player* /*player*/, uint32 /*sender*/, uint32 /*action*/, char const* /*code*/) {}
    virtual void sQuestAccept(Player* /*player*/, Quest const* /*quest*/) {}
    virtual void sQuestSelect(Player* /*player*/, Quest const* /*quest*/) {}
    virtual void sQuestComplete(Player* /*player*/, Quest const* /*quest*/) {}
    virtual void sQuestReward(Player* /*player*/, Quest const* /*quest*/, uint32 /*opt*/) {}
    virtual void sOnGameEvent(bool /*start*/, uint16 /*eventId*/) {}
};

class WH_GAME_API PlayerAI : public UnitAI
{
protected:
    Player* const me;
public:
    explicit PlayerAI(Player* player) : UnitAI((Unit*)player), me(player) {}

    void OnCharmed(bool apply) override;
};

class WH_GAME_API SimpleCharmedAI : public PlayerAI
{
public:
    void UpdateAI(uint32 diff) override;
    SimpleCharmedAI(Player* player): PlayerAI(player) {}
};

#endif
