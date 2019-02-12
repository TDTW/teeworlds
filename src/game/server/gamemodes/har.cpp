/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/flag.h>

#include "har.h"

CGameControllerHAR::CGameControllerHAR(CGameContext *pGameServer)
        : IGameController(pGameServer)
{
    m_pGameType = "H&R";

    for (int i = 0; i < 5; i++)
    {
        if (m_Flag[i])
        {
            m_Flag[i] = NULL;
        }
    }
    for (int i = 0; i < 16; i++)
    {
        if (Red_Flag[i])
        {
            Red_Flag[i] = NULL;
        }
        if (Blue_Flag[i])
        {
            Blue_Flag[i] = NULL;
        }
    }
}

bool CGameControllerHAR::OnEntity(int Index, vec2 Pos)
{
    if (Index != ENTITY_SPAWN &&
        Index != ENTITY_SPAWN_BLUE &&
        Index != ENTITY_SPAWN_RED)
        return false;

    if (IGameController::OnEntity(Index, Pos))
        return true;
}

void CGameControllerHAR::OnCharacterSpawn(CCharacter *pChr)
{
    // give start equipment

    pChr->IncreaseHealth(10);
    pChr->SetWeapon(WEAPON_HAMMER);
    pChr->GiveWeapon(WEAPON_HAMMER, -1);

    int Catch = IGameController::IsCatcher(pChr->GetPlayer()->GetCID());
    int WeaponType = WEAPON_HAMMER;

    if (Catch > -1)
    {
        if (GameServer()->m_GameMode)
            WeaponType = WEAPON_HAMMER;
        else
            WeaponType = GameServer()->m_GameWeapon;
    }
    else
    {
        if (GameServer()->m_GameMode)
            WeaponType = GameServer()->m_GameWeapon;
        else
            WeaponType = WEAPON_HAMMER;
    }

    if (WeaponType == WEAPON_NINJA)
        pChr->GiveNinja();
    else if (WeaponType == WEAPON_HAMMER)
        pChr->GiveWeapon(WeaponType, -1);
    else
        pChr->GiveWeapon(WeaponType, 10);

    pChr->SetWeapon(WeaponType);
}

void CGameControllerHAR::Tick()
{
    IGameController::Tick();

    if (GameServer()->m_World.m_ResetRequested || GameServer()->m_World.m_Paused)
        return;

    FlagTick();
    SendBroadcastTick();
    TuneTick();

    // Change nickname (if catcher)
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (IGameController::IsCatcher(i) > -1)
        {
            if (str_comp_num(Server()->ClientName(i), "> ", 2))
            {
                char aBuf[20];
                str_format(aBuf, sizeof(aBuf), "> %s", Server()->ClientName(i));
                Server()->SetClientName(i, aBuf);
            }
        }
        else if (IGameController::IsCatcher(i) == -1)
        {
            if (!str_comp_num(Server()->ClientName(i), "> ", 2))
            {
                char aBuf[20];
                int length = str_length(Server()->ClientName(i));
                str_copy(aBuf, Server()->ClientName(i), length);
                Server()->SetClientName(i, &aBuf[2]);
            }
        }
    }

    // Check catchers if enough
    if (Server()->Tick() % Server()->TickSpeed() == 0)
    {
        while (IGameController::MinCatcher() > IGameController::GetCatcher())
        {
            int Random = rand() % MAX_CLIENTS;
            if ((IGameController::IsCatcher(Random) == -1) && (GameServer()->m_apPlayers[Random]->GetTeam() != TEAM_SPECTATORS))
            {
                ChangeCatcher(-1, Random);
            }
        }

        while (IGameController::MinCatcher() < IGameController::GetCatcher())
        {
            int Random = rand() % MAX_CLIENTS;
            if ((IGameController::IsCatcher(Random) > -1) && (GameServer()->m_apPlayers[Random]->GetTeam() != TEAM_SPECTATORS))
            {
                ChangeCatcher(Random, -1);
            }
        }
    }

    if (Server()->Tick() % Server()->TickSpeed() == 0)
    {
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (GameServer()->m_GameMode)
            {
                if (IGameController::IsCatcher(i) > -1 && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
                    if (IsFlagCharacter(i) != -1)
                        GameServer()->m_apPlayers[i]->m_Score++;
            }
            else
            {
                if (IGameController::IsCatcher(i) == -1 && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
                    GameServer()->m_apPlayers[i]->m_Score++;
            }
        }
    }
}


void CGameControllerHAR::FlagTick()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (GameServer()->m_apPlayers[i])
        {
            int Index = GameServer()->m_apPlayers[i]->GetCID();
            if (Red_Flag[Index])
            {
                int i2 = IGameController::IsCatcher(Index);
                if (i2 > -1)
                {
                    if (!m_Flag[i2])
                        continue;
                    Red_Flag[Index]->m_Pos = m_Flag[i2]->m_Pos;
                }
            }
        }
    }

    for (int fi = 0; fi < 5; fi++)
    {
        CFlag *F = m_Flag[fi];

        if (!F)
            continue;

        // flag hits death-tile or left the game layer, reset it
        if (GameServer()->Collision()->GetCollisionAt(F->m_Pos.x, F->m_Pos.y)&CCollision::COLFLAG_DEATH || F->GameLayerClipped(F->m_Pos))
        {
            GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "flag_return");
            GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
            F->Reset();
            continue;
        }

        //
        if (F->m_pCarryingCharacter)
        {
            // update flag position
            F->m_Pos = F->m_pCarryingCharacter->m_Pos;
            if (F->m_pCarryingCharacter->GetPlayer())
            {
                int Index = F->m_pCarryingCharacter->GetPlayer()->GetCID();
                if (Blue_Flag[Index])
                {
                    Blue_Flag[Index]->m_Pos = F->m_Pos;
                }
            }
        }
        else
        {
            CCharacter *apCloseCCharacters[MAX_CLIENTS];
            int Num = GameServer()->m_World.FindEntities(F->m_Pos, CFlag::ms_PhysSize, (CEntity**)apCloseCCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
            for (int i = 0; i < Num; i++)
            {
                if (!apCloseCCharacters[i]->IsAlive() || apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_SPECTATORS || GameServer()->Collision()->IntersectLine(F->m_Pos, apCloseCCharacters[i]->m_Pos, NULL, NULL))
                    continue;

                int Index = apCloseCCharacters[i]->GetPlayer()->GetCID();
                if (IGameController::IsCatcher(Index) == -1 || IsFlagCharacter(Index) != -1)
                    continue;

                if (IGameController::IsCatcher(Index) != fi)
                    continue;

                F->m_pCarryingCharacter = apCloseCCharacters[i];

                if (GameServer()->m_GameMode)
                {
                    GameServer()->m_World.m_Core.m_apCharacters[Index]->m_DelayTime = Server()->TickSpeed() * 3;
                }

                if (Blue_Flag[Index])
                    Blue_Flag[Index]->m_pCarryingCharacter = apCloseCCharacters[i];

                GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_EN, -1);

                break;
            }

            if (!F->m_pCarryingCharacter)
            {
                F->m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
                GameServer()->Collision()->MoveBox(&F->m_Pos, &F->m_Vel, vec2(F->ms_PhysSize, F->ms_PhysSize), 0.5f);
            }
        }
    }
}

int CGameControllerHAR::CountFlag()
{
    int FlagCount = 0;

    for (int i = 0; i < 5; i++)
    {
        if (m_Flag[i])
            FlagCount++;
    }

    return FlagCount;
}

int CGameControllerHAR::IsFlagCharacter(int Index)
{
    if (!g_Config.m_SvGameFlag)
        return -2;

    for (int i = 0; i < 5; i++)
    {
        if (!m_Flag[i])
            continue;

        CCharacter * p = m_Flag[i]->m_pCarryingCharacter;
        if (!p)
            continue;

        if (p->GetPlayer() && p->GetPlayer()->GetCID() == Index)
            return i;
    }
    return -1;
}

void CGameControllerHAR::ChangeCatcher(int Index_Old, int Index_New)
{
    //ChangeCatcher(Random, -1);
    // �������� ����� ���� ����
    if (Index_New == -1)
    {
        int fi = IGameController::IsCatcher(Index_Old);
        if (m_Flag[fi])
        {
            GameServer()->m_World.RemoveEntity(m_Flag[fi]);
            delete m_Flag[fi];
            m_Flag[fi] = 0;
        }
    }

    if (Index_Old != -1)
    {
        if (IsFlagCharacter(Index_Old) == -1 && Index_New != -1)
            return;
        if (GameServer()->m_World.m_Core.m_apCharacters[Index_Old] && GameServer()->m_World.m_Core.m_apCharacters[Index_Old]->m_DelayTime > 0)
            return;

        int IndexFlag = IGameController::IsCatcher(Index_Old);
        if (IndexFlag > -1)
        {
            if (m_Flag[IndexFlag] && m_Flag[IndexFlag]->m_pCarryingCharacter)
            {
                m_Flag[IndexFlag]->m_pCarryingCharacter = 0;
            }

            GameServer()->CreateSoundGlobal(SOUND_CTF_DROP);
        }

        IGameController::ChangeDetailCatcher(Index_Old, false);
    }

    IGameController::ChatCatcherChat(Index_Old, Index_New);

    if (Index_New != -1)
    {
        IGameController::ChangeDetailCatcher(Index_New, true);
    }

    //ChangeCatcher(-1, Random);
    // �������� �����
    if (Index_Old == -1)
    {
        int fi = IGameController::IsCatcher(Index_New);
        if (!m_Flag[fi])
        {
            CFlag* F = new CFlag(&GameServer()->m_World, fi + 2);

            vec2 SpawnPos;
            if (IGameController::CanSpawn(0, &SpawnPos))
            {
                F->m_StandPos = SpawnPos;
                F->m_Pos = SpawnPos;
            }
            else
            {
                F->m_StandPos = vec2(0, 0);
                F->m_Pos = vec2(0, 0);
            }
            m_Flag[fi] = F;
            GameServer()->m_World.InsertEntity(F);
        }
    }
}

void CGameControllerHAR::Snap(int SnappingClient)
{
    IGameController::Snap(SnappingClient);

    if (!g_Config.m_SvGameFlag)
        return;

    CNetObj_GameData *pGameDataObj = (CNetObj_GameData *)Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData));
    if (!pGameDataObj)
        return;

    pGameDataObj->m_TeamscoreRed = 0;
    pGameDataObj->m_TeamscoreBlue = 0;

    /*pGameDataObj->m_FlagDropTickRed = 0;
    pGameDataObj->m_FlagDropTickBlue = Server()->Tick() + Server()->TickSpeed() * 30;*/

    pGameDataObj->m_FlagCarrierRed = FLAG_MISSING;

    if (Blue_Flag[SnappingClient])
    {
        if (Blue_Flag[SnappingClient]->m_pCarryingCharacter && Blue_Flag[SnappingClient]->m_pCarryingCharacter->GetPlayer())
        {
            pGameDataObj->m_FlagCarrierBlue = Blue_Flag[SnappingClient]->m_pCarryingCharacter->GetPlayer()->GetCID();
        }
        else
        {
            pGameDataObj->m_FlagCarrierBlue = FLAG_MISSING;
        }
    }
    else
    {
        pGameDataObj->m_FlagCarrierBlue = FLAG_MISSING;
    }
}

void CGameControllerHAR::CreateFlags(int m_Index)
{
    if (!Red_Flag[m_Index])
    {
        CFlag* F1 = new CFlag(&GameServer()->m_World, 0);
        F1->m_StandPos = vec2(0, 0);
        F1->m_Pos = vec2(0, 0);
        Red_Flag[m_Index] = F1;
        Red_Flag[m_Index]->m_Index = m_Index;
        GameServer()->m_World.InsertEntity(F1);
    }

    if (!Blue_Flag[m_Index])
    {
        CFlag* F2 = new CFlag(&GameServer()->m_World, 1);
        F2->m_StandPos = vec2(0, 0);
        F2->m_Pos = vec2(0, 0);
        Blue_Flag[m_Index] = F2;
        Blue_Flag[m_Index]->m_Index = m_Index;
        GameServer()->m_World.InsertEntity(F2);
    }
}

void CGameControllerHAR::DeleteFlags(int m_Index)
{
    if (Red_Flag[m_Index])
    {
        GameServer()->m_World.RemoveEntity(Red_Flag[m_Index]);
        delete Red_Flag[m_Index];
        Red_Flag[m_Index] = 0;
    }

    if (Blue_Flag[m_Index])
    {
        GameServer()->m_World.RemoveEntity(Blue_Flag[m_Index]);
        delete Blue_Flag[m_Index];
        Blue_Flag[m_Index] = 0;
    }
}

void CGameControllerHAR::DeleteFlag(int i)
{
    if (m_Flag[i])
    {
        GameServer()->m_World.RemoveEntity(m_Flag[i]);
        delete m_Flag[i];
        m_Flag[i] = 0;
    }
}

void CGameControllerHAR::TuneTick()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!GameServer()->m_apPlayers[i])
            continue;

        if (!GameServer()->m_apPlayers[i]->GetCharacter())
            continue;

        CTuningParams OldTuning = GameServer()->m_apPlayers[i]->MyTuning;
        CTuningParams NewTuning = OldTuning;

        if (GameServer()->m_World.m_Core.m_apCharacters[i]->m_DelayTime > 0 || !GameServer()->m_World.m_Core.m_apCharacters[i]->m_Visible)
        {
            if (Server()->Tick() % (Server()->TickSpeed() / 5) && GameServer()->m_World.m_Core.m_apCharacters[i]->m_DelayTime > 0)
            {
                // make sure that the damage indicators doesn't group together
                GameServer()->CreateDamageInd(GameServer()->GetPlayerChar(i)->m_Pos, Server()->Tick() * 0.35f, 1);
            }

            NewTuning.Set("player_collision", 0);
            NewTuning.Set("player_hooking", 0);
        }
        else
        {
            NewTuning.Set("player_collision", 1);
            NewTuning.Set("player_hooking", 1);
        }

        if (mem_comp(&OldTuning, &NewTuning, sizeof(CTuningParams)) != 0)
        {
            CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
            int *pParams = (int *)&NewTuning;
            for (unsigned j = 0; j < sizeof(NewTuning) / sizeof(int); j++)
                Msg.AddInt(pParams[j]);

            Server()->SendMsg(&Msg, MSGFLAG_VITAL, i);
            GameServer()->m_apPlayers[i]->MyTuning = NewTuning;
        }
    }
}

void CGameControllerHAR::SendBroadcastTick()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!GameServer()->m_apPlayers[i])
            continue;
        if (Server()->Tick() > GameServer()->m_apPlayers[i]->m_SendBroadcastTick)
        {
            if (!GameServer()->m_GameMode && IGameController::IsCatcher(i) > -1 && IsFlagCharacter(i) == -1)
            {
                GameServer()->SendBroadcast("Take the RED flag", i);
                continue;
            }
            if (!GameServer()->m_GameMode && IGameController::IsCatcher(i) > -1 && IsFlagCharacter(i) > -1)
            {
                GameServer()->SendBroadcast("Catch the players", i);
                continue;
            }
            if (!GameServer()->m_GameMode && IGameController::IsCatcher(i) == -1)
            {
                GameServer()->SendBroadcast("Run away from catchers", i);
                continue;
            }
            if (GameServer()->m_GameMode && IGameController::IsCatcher(i) == -1)
            {
                GameServer()->SendBroadcast("Catch the flagtakers", i);
                continue;
            }
            if (GameServer()->m_GameMode && IGameController::IsCatcher(i) > -1 && IsFlagCharacter(i) == -1)
            {
                GameServer()->SendBroadcast("Take the RED flag", i);
                continue;
            }

            if (GameServer()->m_GameMode && IGameController::IsCatcher(i) > -1 && IsFlagCharacter(i) > -1)
            {
                GameServer()->SendBroadcast("Run away from other players", i);
                continue;
            }

        }
    }
}
