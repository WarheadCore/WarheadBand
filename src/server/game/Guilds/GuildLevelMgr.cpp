#include "Chat.h"
#include "DatabaseEnv.h"
#include "GuildLevelMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "WorldSession.h"
#include "GameConfig.h"

// конструктор
GuildLevelMgr::GuildLevelMgr() { }

// деструктор
GuildLevelMgr::~GuildLevelMgr() { }

// синглтон
GuildLevelMgr* GuildLevelMgr::instance() {
    static GuildLevelMgr instance;
    return &instance;
}

// прогрузка логов гильдии из БД 
void GuildLevelMgr::GuildLogLoadFromDB() { 
    for (auto itr = _GuildLevelExpLogContainer.begin(); itr != _GuildLevelExpLogContainer.end(); ++itr)
        delete *itr;

    _GuildLevelExpLogContainer.clear();

    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    QueryResult result = CharacterDatabase.Query("SELECT guildId, playerGuid, playerName, exp, datetime FROM guild_level_invest_log ORDER BY datetime");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 guild log. DB table `guild_level_invest_log` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field *fields = result->Fetch();
        GuildLevelExpLog* log = new GuildLevelExpLog;
        log->guildId    = fields[0].Get<uint32>();
        log->playerGuid = fields[1].Get<uint64>();
        log->playerName = fields[2].Get<std::string>();
        log->exp        = fields[3].Get<uint32>();
        log->datetime   = fields[4].Get<std::string>();

        _GuildLevelExpLogContainer.push_back(log);
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} guild log in {}ms.", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

// прогрузка опыта гильдии из БД
void GuildLevelMgr::GuildLevelLoadFromDB() { 
    for (auto itr = _GuildLevelExpLevelContainer.begin(); itr != _GuildLevelExpLevelContainer.end(); ++itr)
        delete *itr;

    _GuildLevelExpLevelContainer.clear();

    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    QueryResult result = CharacterDatabase.Query("SELECT level, exp FROM guild_level_exp ORDER BY level ASC");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 guild level. DB table `guild_level_exp` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field *fields = result->Fetch();
        GuildLevelExpLevel* GLLC = new GuildLevelExpLevel;
        GLLC->level = fields[0].Get<uint32>();
        GLLC->exp   = fields[1].Get<uint32>();

        _GuildLevelExpLevelContainer.push_back(GLLC);
        ++count;
    } while (result->NextRow());

    // максимальный уровень гильдий
    sGuildLevelMgr->setGuildMaxLevel(count);

    LOG_INFO("server.loading", ">> Loaded {} guild level in {}ms.", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

// прогрузка заклинаний гильдии из БД
void GuildLevelMgr::GuildSpellLevelLoadFromDB() { 
    for (auto itr = _GuildLevelSpellContainer.begin(); itr != _GuildLevelSpellContainer.end(); ++itr)
        delete *itr;

    _GuildLevelSpellContainer.clear();

    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    QueryResult result = CharacterDatabase.Query("SELECT level, spellId, title FROM guild_level_spell");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 guild spell. DB table `guild_level_spell` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field *fields = result->Fetch();
        GuildLevelSpellStruct* GLLC = new GuildLevelSpellStruct;
        GLLC->level       = fields[0].Get<uint32>();
        GLLC->spellId     = fields[1].Get<uint32>();
        GLLC->title       = fields[2].Get<std::string>();

        _GuildLevelSpellContainer.push_back(GLLC);
        ++count;
    } while (result->NextRow());
    LOG_INFO("server.loading", ">> Loaded {} guild spell in {}ms.", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}


// get main window for guild
void GuildLevelMgr::GuildLevelWindow(Player* player) {

    if (!player || !player->GetSession() || !player->GetGuild())
        return;


    Guild* guild = sGuildMgr->GetGuildById(player->GetGuild()->GetId());
    if (!guild) {
        ChatHandler(player->GetSession()).PSendSysMessage("Система не может найти запрашиваемую гильдию");
        return;
    }

    // открываем главное окно гильдии для игрока
    sGuildLevelMgr->WelcomeGuildControl(player);
}

void GuildLevelMgr::WelcomeGuildControl(Player* player) 
{    
    if (!player || !player->GetSession() || !player->GetGuild())
        return;

    ClearGossipMenuFor(player);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Вложение в гильдию\n|cff473B32Инвестирование в будущее команды.|r", GOSSIP_SENDER_MAIN, 1);
    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "История вложения\n|cff473B32Узнайте, как ваш опыт растет.|r", GOSSIP_SENDER_MAIN, 2);
    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Покупка дома для гильдии\n|cff473B32Приобретение дома для вашей команды.|r", GOSSIP_SENDER_MAIN, 3);
    AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Гильдийские заклинания\n|cff473B32Заклинания которые доступны членам гильдии.|r", GOSSIP_SENDER_MAIN, 4);
    player->PlayerTalkClass->GetGossipMenu().SetMenuId(99);
    player->PlayerTalkClass->SendGossipMenu(sGuildLevelMgr->MenuHeader(player, 0), player->GetGUID());
}

// Вложение в гильдию
void GuildLevelMgr::GuildLevelInvestment(Player* player) 
{    
    if (!player || !player->GetSession() || !player->GetGuild())
        return;

    ClearGossipMenuFor(player);
    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Благодарю вас за помощь.", GOSSIP_SENDER_MAIN, 0);
    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Покажи мне мои вложения", GOSSIP_SENDER_MAIN, 5);
    player->PlayerTalkClass->GetGossipMenu().SetMenuId(99);
    player->PlayerTalkClass->SendGossipMenu(sGuildLevelMgr->MenuHeader(player, 2), player->GetGUID());
}

// История вложения
void GuildLevelMgr::GuildLevelInvestmentHistory(Player* player, bool showAll) 
{
    // Если игрок не найден или не в гильдии
    if (!player || !player->GetSession() || !player->GetGuild())
        return;

    ClearGossipMenuFor(player);
    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Мои последние вложения", GOSSIP_SENDER_MAIN, 5);
    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Последние вложения в гильдию", GOSSIP_SENDER_MAIN, 2);
    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Назад", GOSSIP_SENDER_MAIN, 0);
    player->PlayerTalkClass->GetGossipMenu().SetMenuId(99);
    player->PlayerTalkClass->SendGossipMenu(sGuildLevelMgr->MenuHeader(player, showAll ? 3 : 5), player->GetGUID());
}

// Покупка дома
void GuildLevelMgr::GuildLevelBuyHouse(Player* player) 
{
    // Если игрок не найден или не в гильдии
    if (!player || !player->GetSession() || !player->GetGuild())
        return;

    // Если игрок является лидером гильдии
    if (player->GetGuild()->GetLeaderGUID() == player->GetGUID()) {
        ChatHandler(player->GetSession()).PSendSysMessage("Данный функционал еще не реализован.");
    }
    else {
        ChatHandler(player->GetSession()).PSendSysMessage("Вы не являетесь лидером гильдии");
    }
    sGuildLevelMgr->GuildLevelWindow(player);
}

// Доступные заклинания по уровню
void GuildLevelMgr::GuildLevelSpell(Player* player) 
{
    // Если игрок не найден или не в гильдии    
    if (!player || !player->GetSession() || !player->GetGuild())
        return;

    ClearGossipMenuFor(player);

    int32 min = player->GetGuild()->GetGuildLevel() - 5;
    int32 max = player->GetGuild()->GetGuildLevel() + 5;

    for (auto itr = _GuildLevelSpellContainer.begin(); itr != _GuildLevelSpellContainer.end(); ++itr) {
        // если между уровнем гильдии и уровнем заклинания разница не более 5
        if (int32((*itr)->level) >= min && int32((*itr)->level) <= max) {
            // генерируем текст для отображения
            std::string text = sGuildLevelMgr->GenerateSpellText(
                player, 
                (*itr)->level,
                (*itr)->title,
                (*itr)->level <= player->GetGuild()->GetGuildLevel() ? true : false);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, text, GOSSIP_SENDER_MAIN, 4);
        }
    }
    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Назад", GOSSIP_SENDER_MAIN, 0);
    player->PlayerTalkClass->GetGossipMenu().SetMenuId(99);
    player->PlayerTalkClass->SendGossipMenu(sGuildLevelMgr->MenuHeader(player, 1), player->GetGUID());
}

// генерация текст для отоброжения заклинаний с уровнем требования
std::string GuildLevelMgr::GenerateSpellText(Player* player, uint32 level, std::string title, bool know) {

    if (!player || !player->GetSession())
        return "";

    std::ostringstream ss;
    know ? ss << title << "\nДоступно с " << level << " уровня" :
           ss << "|cff473B32" << title << "\nТребуется " << level << " уровень гильдии|r";
    return ss.str().c_str();
}

std::string GuildLevelMgr::MenuHeader(Player* player, uint8 MenuId)
{
    // Если игрок не найден или не в гильдии
    if (!player || !player->GetSession() || !player->GetGuild())
        return "";

    std::ostringstream ss;
    switch (MenuId) {
        // Главное меню
        case 0: {
            uint32 expForNextLevel = sGuildLevelMgr->GetExpToNextLevel(player->GetGuild()->GetGuildLevel() + 1);
            uint32 expLacksForNextLevel = (expForNextLevel - player->GetGuild()->GetGuildExp()) < 0 ? 0 : (expForNextLevel - player->GetGuild()->GetGuildExp());

            ss << "Уважаемый " << player->GetName() << "!\n\n";
            ss << "Система гильдии дает возможность получать привилегии и доступ к новым возможностям по мере роста уровня.\n\n";
            ss << "Ваша гильдия: " << player->GetGuild()->GetDefaultName() << "\n";
            ss << "Уровень гильдии: " << player->GetGuild()->GetGuildLevel() << " / " << sGuildLevelMgr->getGuildMaxLevel() << "\n";
            ss << "Прогресс гильдии: " << player->GetGuild()->GetGuildExp() << " / " << expForNextLevel << "\n";
            ss << "До следующего уровня: " << expLacksForNextLevel;
         } break;

        // Гильдийские заклинания
        case 1: {
            ss << "Уважаемый " << player->GetName() << "!\n\n";
            ss << "Заклинания, соответствующие уровню вашей гильдии, автоматически добавляются в вашу книгу заклинаний при вступлении в гильдию и при ее повышении.\n\n";
            ss << "Уровень гильдии: " << player->GetGuild()->GetGuildLevel();
        } break;

        // Вложение опыта
        case 2: {
            ss << "Уважаемый " << player->GetName() << "!\n\n";
            ss << "На данный момент в гильдию вложено: " << player->GetGuild()->GetGuildExp() << " опыта, ";
            ss << "из них " << sGuildLevelMgr->CalculateExpForPlayerInGuild(player) << " опыта было вложено вами.\n\n";
            ss << "Путешествуя по увлекательному миру фэнтези, каждый член гильдии стремится к тому, чтобы повысить свой уровень и стать более влиятельным. И как же это сделать? Очень просто! Убивайте грозных боссов и активируйте жетоны гильдии - это надежный путь к успеху. К тому же, в будущем, будут доступны и другие способы повышения уровня, которые позволят вам проявить свою мощь и мастерство. Станьте лучшими из лучших и достигните вершин своих возможностей вместе со своей гильдией!";
        } break;

        // История вложения всех членов гильдии
        case 3: {
            ss << sGuildLevelMgr->ShowLastLog(player, true);
        } break;

        // Покупка дома
        case 4: {
            ss << "Уважаемый " << player->GetName() << "!\n\n";
            ss << "Покупка дома для гильдии.\n\n";
            ss << "Уровень гильдии: " << player->GetGuild()->GetGuildLevel() << "\n";
        } break;

        // История вложения одного члена гильдии
        case 5: {
            ss << sGuildLevelMgr->ShowLastLog(player, false);
        } break;

        default: break;
    }
    return ss.str().c_str();  
}

uint32 GuildLevelMgr::GetExpToNextLevel(uint32 level) {
    for (auto itr = _GuildLevelExpLevelContainer.begin(); itr != _GuildLevelExpLevelContainer.end(); ++itr)
        if ((*itr)->level == level)
            return (*itr)->exp;
    return 0;
}

void GuildLevelMgr::UpdateGuildLevel(Guild* guild, Player* player, uint32 exp) { 
    // проверить, существует ли гильдия, игрок и опыт
    if (!player || !exp || !guild)
        return;

    sGuildLevelMgr->SetNewExp(exp, player, guild);

    // обновить уровень гильдии
    for (auto itr = _GuildLevelExpLevelContainer.begin(); itr != _GuildLevelExpLevelContainer.end(); ++itr)
    {
        // если игрок набрал достаточно опыта для уровня гильдии, обновить уровень гильдии
        if (guild->GetGuildLevel() < (*itr)->level && guild->GetGuildExp() >= (*itr)->exp)
        {
            // отправить сообщение игроку о том, что уровень гильдии был обновлен
            sGuildLevelMgr->SetNewLevel((*itr)->level, player, guild);
            break;
        }
    }
}

// Эта функция ищет уровень гильдии по опыту.
uint32 GuildLevelMgr::FindLevelByExp(uint32 gExp) {
    if (_GuildLevelExpLevelContainer.empty() || gExp <= 0)
        return 0;

    for (auto itr = _GuildLevelExpLevelContainer.begin(); itr != _GuildLevelExpLevelContainer.end(); ++itr) {
        // если опыт гильдии между текущим и следующим уровнем, вернуть текущий уровень
        if (gExp < (*itr)->exp)
            return (*itr)->level - 1;
    }

    return sGuildLevelMgr->getGuildMaxLevel();
}

// Эта функция обновляет очки опыта заданной гильдии.
void GuildLevelMgr::SetNewExp(uint32 exp, Player* player, Guild* guild) {

    if (!exp || !player || !guild)
        return;

    // установить очки опыта гильдии
    // Обновить объект гильдии новым значением очков опыта.
    guild->SetGuildExp(guild->GetGuildExp() + exp);

    // анонсировать в чате игроку о вложении опыта
    ChatHandler(player->GetSession()).PSendSysMessage("Вы вложили {} очков опыта в гильдию.", exp);

    // установить очки опыта гильдии в БД
    // Получить подготовленное выражение для обновления очков опыта гильдии в базе данных.
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_EXP);

    // Установить новое значение очков опыта и идентификатор гильдии как параметры подготовленного выражения.
    stmt->SetData(0, exp);
    stmt->SetData(1, guild->GetId());
    CharacterDatabase.Execute(stmt);

    // добавить запись в БД о вложении в гильдию
    sGuildLevelMgr->SetNewLog(guild->GetId(), player, exp);
}

void GuildLevelMgr::SetNewLog(uint32 guildId, Player* player, uint32 exp) {
    // Получить подготовленное выражение для вставки новой записи журнала в базу данных для вложения в гильдию.
    CharacterDatabasePreparedStatement* stmtLog = CharacterDatabase.GetPreparedStatement(CHAR_INS_GUILD_LEVEL_LOG);

    // Установить идентификатор гильдии, GUID игрока, имя игрока и новое значение очков опыта как параметры подготовленного выражения.
    stmtLog->SetData(0, guildId);
    stmtLog->SetData(1, player->GetGUID().GetCounter());
    stmtLog->SetData(2, player->GetName());
    stmtLog->SetData(3, exp);
    CharacterDatabase.Execute(stmtLog);
}

// Эта функция обновляет уровень гильдии в базе данных.
void GuildLevelMgr::SetNewLevel(uint32 level, Player* player, Guild* guild) { 

    // Обновить объект гильдии новым значением уровня.
    guild->SetGuildLevel(level);

    // Разослать сообщение всем участникам гильдии.
    std::stringstream ss;
    ss << "Поздравляем ! Ваша гильдия достигла " << level << " уровня!";
    // Сообщить всем участникам гильдии о том, что уровень гильдии был обновлен.
    sGuildLevelMgr->GuildMessage(ss.str(), guild);  
    // Обновить название гильдии.
    sGuildLevelMgr->ChangeNameGuild(guild, level);
    // Выдать награду за достижение нового уровня гильдии. (заклинание)
    sGuildLevelMgr->RewardGuild(guild, level, player);
    // Обновить уровень гильдии в базе данных.
    // Получить подготовленное выражение для обновления уровня гильдии в базе данных.
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_LEVEL);
    // Установить новый уровень гильдии и идентификатор гильдии как параметры подготовленного выражения.
    stmt->SetData(0, level);
    stmt->SetData(1, guild->GetId());
    CharacterDatabase.Execute(stmt);
}

void GuildLevelMgr::GuildMessage(std::string message, Guild* guild)
{
	for (auto const& m_member : guild->GetMember())
        if (Player* player = m_member.second.FindPlayer()) {
			ChatHandler(player->GetSession()).PSendSysMessage(message.c_str());
        }
}

void GuildLevelMgr::ChangeNameGuild(Guild* guild, uint32 level) {
    // получаем имя гильдии без уровня
    std::string guildName = guild->GetDefaultName();
    // создаем новое имя гильдии с уровнем
    std::stringstream ss;
    ss << guildName << ", " << std::to_string(level) << " ур.";
    // обновляем имя гильдии
    guild->SetName(ss.str(), true);
}

void GuildLevelMgr::RewardGuild(Guild* guild, uint32 level, Player* player) {

    if (!guild || !level || !player)
        return;

    sGuildLevelMgr->LearnOrRemoveSpell(player);    
    ChatHandler(player->GetSession()).PSendSysMessage("Вы получили награду за достижение {} уровня гильдии.", level);
}

// считает количество вложеного опыта в гильдию игроком
uint32 GuildLevelMgr::CalculateExpForPlayerInGuild(Player* player) {
    uint32 exp = 0;
    for (auto itr = _GuildLevelExpLogContainer.begin(); itr != _GuildLevelExpLogContainer.end(); ++itr)
        if ((*itr)->guildId == player->GetGuild()->GetId() && (*itr)->playerGuid == player->GetGUID().GetCounter())
            exp += (*itr)->exp;
    return exp;        
}

// показывает последние 10 вложений в гильдию
std::string GuildLevelMgr::ShowLastLog(Player* player, bool showAll) {
    std::stringstream ss;
    showAll ? ss << "\nПоследние 10 вложений в гильдию:\n" : ss << "\nВаши последние 10 вложений:\n";
    uint8 count = 0;
    for (auto itr = _GuildLevelExpLogContainer.begin(); itr != _GuildLevelExpLogContainer.end(); ++itr) {
        if ((*itr)->guildId == player->GetGuild()->GetId()) {
            if (showAll || (*itr)->playerGuid == player->GetGUID().GetCounter()) 
            {
                ss << "\n     " << (*itr)->datetime << "\n";
                ss << "     |cff473B32" << (*itr)->playerName << " вложил " << (*itr)->exp << " опыта.|r";
                count++;
                if (count == 10)
                    break;
            }
        }
    }

    if (count == 0)
        ss << "\nНет вложений|r\n|cff473B32Вложения автоматически обновляются при каждом перезапуске сервера.";

    return ss.str();
}

void GuildLevelMgr::LearnOrRemoveSpell(Player* player) {
    // если игрок не найден, выходим
    if (!player)
        return;
        
    // обучение / разучение при входе в игру
    for (auto itr = _GuildLevelSpellContainer.begin(); itr != _GuildLevelSpellContainer.end(); ++itr) {
        if (player->GetGuild() && (*itr)->level <= player->GetGuild()->GetGuildLevel()) {
            if (!player->HasSpell((*itr)->spellId))
                player->learnSpell((*itr)->spellId);
        } else {
            if (player->HasSpell((*itr)->spellId))
                player->removeSpell((*itr)->spellId, SPEC_MASK_ALL, false);
        }
    }  
}

void GuildLevelMgr::RewardOnKillBoss(Player* player) {
    if (!player)
        return;

    Group* group = player->GetGroup();
    uint32 exp = CONF_GET_INT("guild.LevelRewardKillBoss");

    if (group == nullptr) {
        if (player->GetGuild()) {
            sGuildLevelMgr->UpdateGuildLevel(player->GetGuild(), player, exp);
        } else {
            ChatHandler(player->GetSession()).PSendSysMessage("Если бы вы были в гильдии, вы принесли бы ей {} очков опыта.", exp); 
        }
    } else {
        uint32 numPlayers = group->GetMembersCount();
        uint32 pointsPerPlayer = exp / numPlayers;
        for (GroupReference* itr = player->GetGroup()->GetFirstMember(); itr != NULL; itr = itr->next()) {
            if (Player* p = itr->GetSource()) {
                if (p->GetGuild()) {
                    sGuildLevelMgr->UpdateGuildLevel(p->GetGuild(), p, pointsPerPlayer);
                } else {
                ChatHandler(p->GetSession()).PSendSysMessage("Если бы вы были в гильдии, вы принесли бы ей {} очков опыта.", pointsPerPlayer); 
                }
            }
        }
    }
}