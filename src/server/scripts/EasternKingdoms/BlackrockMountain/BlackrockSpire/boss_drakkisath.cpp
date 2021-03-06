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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackrock_spire.h"

enum Spells
{
    SPELL_FIRENOVA                  = 23462,
    SPELL_CLEAVE                    = 20691,
    SPELL_CONFLIGURATION            = 16805,
    SPELL_THUNDERCLAP               = 15548, //Not sure if right ID. 23931 would be a harder possibility.
};

enum Events
{
    EVENT_FIRE_NOVA                = 1,
    EVENT_CLEAVE                   = 2,
    EVENT_CONFLIGURATION           = 3,
    EVENT_THUNDERCLAP              = 4,
};

enum ChromaticEliteGuardEvents
{
    EVENT_MORTAL_STRIKE = 1,
    EVENT_KNOCKDOWN     = 2,
    EVENT_STRIKE        = 3
};

enum ChromaticEliteGuardSpells
{
    SPELL_MORTAL_STRIKE = 15708,
    SPELL_KNOCKDOWN     = 16790,
    SPELL_STRIKE        = 15580
};

int const ChromaticEliteGuardEntry  = 10814;
int const GeneralDrakkisathEntry    = 10814;

class boss_drakkisath : public CreatureScript
{
public:
    boss_drakkisath() : CreatureScript("boss_drakkisath") { }

    struct boss_drakkisathAI : public BossAI
    {
        boss_drakkisathAI(Creature* creature) : BossAI(creature, DATA_GENERAL_DRAKKISATH) { }

        void Reset() override
        {
            _Reset();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            CallForHelp();
            events.ScheduleEvent(EVENT_FIRE_NOVA, 6000);
            events.ScheduleEvent(EVENT_CLEAVE,    8000);
            events.ScheduleEvent(EVENT_CONFLIGURATION, 15000);
            events.ScheduleEvent(EVENT_THUNDERCLAP,    17000);
        }

        // Will make his two adds engage combat
        void CallForHelp()
        {
            std::list<Creature*> ChromaticEliteGuards;
            me->GetCreaturesWithEntryInRange(ChromaticEliteGuards, 15.0f, ChromaticEliteGuardEntry);
            for (std::list<Creature*>::const_iterator itr = ChromaticEliteGuards.begin(); itr != ChromaticEliteGuards.end(); ++itr)
            {
                (*itr)->ToCreature()->AI()->AttackStart(me->GetVictim());
            }
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_FIRE_NOVA:
                        DoCastVictim(SPELL_FIRENOVA);
                        events.ScheduleEvent(EVENT_FIRE_NOVA, 10000);
                        break;
                    case EVENT_CLEAVE:
                        DoCastVictim(SPELL_CLEAVE);
                        events.ScheduleEvent(EVENT_CLEAVE, 8000);
                        break;
                    case EVENT_CONFLIGURATION:
                        DoCastVictim(SPELL_CONFLIGURATION);
                        events.ScheduleEvent(EVENT_CONFLIGURATION, 18000);
                        break;
                    case EVENT_THUNDERCLAP:
                        DoCastVictim(SPELL_THUNDERCLAP);
                        events.ScheduleEvent(EVENT_THUNDERCLAP, 20000);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockSpireAI<boss_drakkisathAI>(creature);
    }
};

class chromatic_elite_guard : public CreatureScript
{
public:
    chromatic_elite_guard() : CreatureScript("chromatic_elite_guard") { }

    struct chromatic_elite_guardAI : public ScriptedAI
    {
        chromatic_elite_guardAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap _events;

        void EnterCombat(Unit* who) override
        {
            _events.ScheduleEvent(EVENT_MORTAL_STRIKE, urand(5000, 12800));
            _events.ScheduleEvent(EVENT_KNOCKDOWN, urand(5600, 15400));
            _events.ScheduleEvent(EVENT_STRIKE, urand(12000, 20800));

            std::list<Creature*> GeneralDrakkisath;
            me->GetCreaturesWithEntryInRange(GeneralDrakkisath, 15.0f, GeneralDrakkisathEntry);
            for (std::list<Creature*>::const_iterator itr = GeneralDrakkisath.begin(); itr != GeneralDrakkisath.end(); ++itr)
            {
                (*itr)->ToCreature()->AI()->AttackStart(who);
            }

            std::list<Creature*> ChromaticEliteGuards;
            me->GetCreaturesWithEntryInRange(ChromaticEliteGuards, 15.0f, ChromaticEliteGuardEntry);
            for (std::list<Creature*>::const_iterator itr = ChromaticEliteGuards.begin(); itr != ChromaticEliteGuards.end(); ++itr)
            {
                (*itr)->ToCreature()->AI()->AttackStart(who);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            switch (_events.ExecuteEvent())
            {
                case EVENT_MORTAL_STRIKE:
                    DoCastVictim(SPELL_MORTAL_STRIKE);
                    _events.ScheduleEvent(EVENT_MORTAL_STRIKE, 13000);
                    break;
                case EVENT_KNOCKDOWN:
                    DoCastVictim(SPELL_KNOCKDOWN);
                    _events.ScheduleEvent(EVENT_MORTAL_STRIKE, urand(11200, 25700));
                    break;
                case EVENT_STRIKE:
                    DoCastVictim(SPELL_STRIKE);
                    _events.ScheduleEvent(EVENT_MORTAL_STRIKE, 9000);
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new chromatic_elite_guardAI(creature);
    }
};

void AddSC_boss_drakkisath()
{
    new boss_drakkisath();
    new chromatic_elite_guard();
}
