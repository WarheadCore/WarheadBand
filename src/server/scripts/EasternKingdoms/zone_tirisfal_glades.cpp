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

/* ScriptData
SDName: Tirisfal_Glades
SD%Complete: 100
SDComment: Quest support: 590, 1819
SDCategory: Tirisfal Glades
EndScriptData */

/* ContentData
npc_calvin_montague
go_mausoleum_door
go_mausoleum_trigger
EndContentData */

#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"

/*######
## npc_calvin_montague
######*/

enum Calvin
{
    SAY_COMPLETE        = 0,
    SPELL_DRINK         = 2639,                             // possibly not correct spell (but iconId is correct)
    QUEST_590           = 590,
    FACTION_HOSTILE     = 168
};

class npc_calvin_montague : public CreatureScript
{
public:
    npc_calvin_montague() : CreatureScript("npc_calvin_montague") { }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_590)
        {
            creature->setFaction(FACTION_HOSTILE);
            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            CAST_AI(npc_calvin_montague::npc_calvin_montagueAI, creature->AI())->AttackStart(player);
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_calvin_montagueAI(creature);
    }

    struct npc_calvin_montagueAI : public ScriptedAI
    {
        npc_calvin_montagueAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 m_uiPhase;
        uint32 m_uiPhaseTimer;
        uint64 m_uiPlayerGUID;

        void Reset() override
        {
            m_uiPhase = 0;
            m_uiPhaseTimer = 5000;
            m_uiPlayerGUID = 0;

            me->RestoreFaction();

            if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC))
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
        }

        void EnterCombat(Unit* /*who*/) override { }

        void AttackedBy(Unit* pAttacker) override
        {
            if (me->GetVictim() || me->IsFriendlyTo(pAttacker))
                return;

            AttackStart(pAttacker);
        }

        void DamageTaken(Unit* pDoneBy, uint32& uiDamage, DamageEffectType, SpellSchoolMask) override
        {
            if (!pDoneBy)
                return;

            if (uiDamage >= me->GetHealth() || me->HealthBelowPctDamaged(15, uiDamage))
            {
                uiDamage = 0;

                me->RestoreFaction();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                me->CombatStop(true);

                m_uiPhase = 1;

                if (pDoneBy->GetTypeId() == TYPEID_PLAYER)
                    m_uiPlayerGUID = pDoneBy->GetGUID();
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (m_uiPhase)
            {
                if (m_uiPhaseTimer <= diff)
                    m_uiPhaseTimer = 7500;
                else
                {
                    m_uiPhaseTimer -= diff;
                    return;
                }

                switch (m_uiPhase)
                {
                    case 1:
                        Talk(SAY_COMPLETE);
                        ++m_uiPhase;
                        break;
                    case 2:
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_uiPlayerGUID))
                            player->AreaExploredOrEventHappens(QUEST_590);

                        DoCast(me, SPELL_DRINK, true);
                        ++m_uiPhase;
                        break;
                    case 3:
                        EnterEvadeMode();
                        break;
                }

                return;
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_tirisfal_glades()
{
    new npc_calvin_montague();
}
