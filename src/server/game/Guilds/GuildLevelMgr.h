#ifndef __GUILD_LEVEL_SYSTEL_H
#define __GUILD_LEVEL_SYSTEL_H

#include "Define.h"
#include "Player.h"
#include "DatabaseEnv.h"
#include "WorldSession.h"
#include "ScriptedGossip.h"
#include "ScriptObject.h"
#include "ScriptedCreature.h"
#include "ObjectMgr.h"

struct GuildLevelExpLevel
{
    uint32 level;
    uint32 exp;
};

struct GuildLevelExpLog
{
    uint32 guildId;
    uint64 playerGuid;
    std::string playerName;
    uint32 exp;
    std::string datetime;
};

struct GuildLevelSpellStruct
{
    uint32 level;
    uint32 spellId;
    std::string title;
};

typedef std::vector<GuildLevelExpLevel*> GuildLevelExpLevelContainer;
typedef std::vector<GuildLevelExpLog*> GuildLevelExpLogContainer;
typedef std::vector<GuildLevelSpellStruct*> GuildLevelSpellContainer;
class GuildLevelMgr
{
    private:
        GuildLevelMgr();
        ~GuildLevelMgr();
        uint32 _GuildMaxLevel = 0;

    public:
        static GuildLevelMgr* instance();

        GuildLevelExpLevelContainer _GuildLevelExpLevelContainer;
        GuildLevelExpLogContainer _GuildLevelExpLogContainer;
        GuildLevelSpellContainer _GuildLevelSpellContainer;

        // Getter for maxlevel guild
        uint32 getGuildMaxLevel() { return _GuildMaxLevel; };

        // Setter for maxlevel guild
        void setGuildMaxLevel(uint32 lvl) { _GuildMaxLevel = lvl; };

        // Loading log table -> "SELECT playerName, exp FROM guild_level_invest_log WHERE guildId = ? ORDER BY exp DESC LIMIT 10"
        void GuildLogLoadFromDB();

        // Loading guild level table -> "SELECT exp, level FROM guild"
        void GuildLevelLoadFromDB();

        // Loading guild spell level table -> "SELECT spellId, level FROM guild_level_spell"
        void GuildSpellLevelLoadFromDB();

        // Mais window for guild level
        void GuildLevelWindow(Player* player);

        // Update guild level - check if guild level up and update guild level
        void UpdateGuildLevel(Guild* /*guild*/, Player* /*player*/, uint32 /*exp*/);

        /*
         * Функция обновляет очки опыта гильдии в базе данных и объекте 
         * гильдии и записывает изменение в базу данных с новой записью журнала. 
         * Функция использует подготовленные выражения для эффективного взаимодействия с 
         * базой данных и принимает необходимые параметры для выполнения задачи.        
        */
        void SetNewExp(uint32 /*exp*/, Player* /*player*/, Guild* /*guild*/);

        /*
         * Функция обновляет уровень гильдии в базе данных и объекте 
         * гильдии и записывает изменение в базу данных с новой записью журнала. 
         * Функция использует подготовленные выражения для эффективного взаимодействия с 
         * базой данных и принимает необходимые параметры для выполнения задачи.        
        */
        void SetNewLevel(uint32 /*level*/, Player* /*player*/, Guild* /*guild*/);

        /*
         * Функция записывает новую запись в журнал. 
         * Функция использует подготовленные выражения для эффективного взаимодействия с 
         * базой данных и принимает необходимые параметры для выполнения задачи.        
        */
        void SetNewLog(uint32 /*guildId*/, Player* /*player*/, uint32 /*exp*/);

        /*
         * Функция сообщяет всем игрокам гильдии о новом уровне гильдии.      
        */
        void GuildMessage(std::string /*message*/, Guild* /*guild*/);

        void ChangeNameGuild(Guild* /*guild*/, uint32 level);

        void RewardGuild(Guild* guild, uint32 level, Player* player);

        void WelcomeGuildControl(Player* /*player*/);

        std::string MenuHeader(Player* /*player*/, uint8 /*MenuId*/);

        /*
         * Функция возвращает количество опыта необходимое для достижения следующего уровня гильдии.
        */
        uint32 GetExpToNextLevel(uint32 /*level*/);

        /*
         * Функция открывает окно информации о вкладе в гильдию.
        */
        void GuildLevelInvestment(Player* /*player*/);

        /*
         * Функция открывает окно истории покупок гильдии.
        */
        void GuildLevelInvestmentHistory(Player* /*player*/, bool /*showAll*/);

        /*
         * Функция открывает окно покупки дома гильдии.
        */
        void GuildLevelBuyHouse(Player* /*player*/);

        /*
         * Открывает окно для просмотра списка заклинаний гильдии.
        */
        void GuildLevelSpell(Player* /*player*/);

        /*
         * Генерирует текст для заклинания гильдии.
        */
        std::string GenerateSpellText(Player* /*player*/, uint32 /*level*/, std::string /*title*/, bool /*know*/);

        /*
         * Считает сколько игрок внес в гильдию.
        */
        uint32 CalculateExpForPlayerInGuild(Player* /*player*/);

        /*
         * Показывает последние 10 вложений в гильдию.
        */
        std::string ShowLastLog(Player* /*player*/, bool /*showAll*/);

        /*
         * Обучаем или разучаем у игрока заклинания
        */
        void LearnOrRemoveSpell(Player* /*player*/);

        /*
         * Ищет уровень гильдии по опыту.
        */  
        uint32 FindLevelByExp(uint32 /*gExp*/);

        /*
         * Награда за убийство босса.
        */
        void RewardOnKillBoss(Player* /*player*/);
};

#define sGuildLevelMgr GuildLevelMgr::instance()
#endif