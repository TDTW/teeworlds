/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include "../gamecontroller.h"
#include "../player.h"

#include <game/server/gamecontext.h>
#include "flag.h"

CFlag::CFlag(CGameWorld *pGameWorld, int Team)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_FLAG)
{
	m_Team = Team;
	m_ProximityRadius = ms_PhysSize;
	m_pCarryingCharacter = NULL;
	m_GrabTick = 0;
	m_Index = -1;

	Reset();
}

void CFlag::Reset()
{
	m_pCarryingCharacter = NULL;
	m_AtStand = 1;
	m_Pos = m_StandPos;
	m_Vel = vec2(0,0);
	m_GrabTick = 0;
}

void CFlag::TickPaused()
{
	++m_DropTick;
	if(m_GrabTick)
		++m_GrabTick;
}

void CFlag::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	if (!g_Config.m_SvGameFlag)
		return;

	if (m_Team >= 2)
	{
		if (GameServer()->m_pController->IsCatcher(SnappingClient) > -1 &&
			m_Team == GameServer()->m_pController->IsCatcher(SnappingClient) + 2)
			return;
	}
	if (m_Team < 2)
	{
		if (GameServer()->m_apPlayers[SnappingClient]->GetTeam() == TEAM_SPECTATORS)
			return;
		if (GameServer()->m_pController->IsCatcher(SnappingClient) == -1)
			return;
		if (m_Index != SnappingClient)
			return;
		if (m_Team == 0)
		{
			if (GameServer()->m_pController->IsFlagCharacter(SnappingClient) >= 0)
				return;
		}
		if (m_Team == 1)
		{
			if (!m_pCarryingCharacter)
				return;
			if (GameServer()->m_pController->IsFlagCharacter(SnappingClient) < 0 ||
				GameServer()->m_pController->IsFlagCharacter(SnappingClient) > 4)
				return;
			if (m_pCarryingCharacter->GetPlayer() && m_pCarryingCharacter->GetPlayer()->GetCID() != SnappingClient)
				return;
		}
	}

	if (Server()->Tick() % Server()->TickSpeed() == 0)
	{
		char aBu2f[250];
		str_format(aBu2f, sizeof(aBu2f), "FLAG [p-%d] TEAM [%d]", SnappingClient, m_Team);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBu2f);
	}

	CNetObj_Flag *pFlag = (CNetObj_Flag *)Server()->SnapNewItem(NETOBJTYPE_FLAG, m_Team, sizeof(CNetObj_Flag));
	if(!pFlag)
		return;

	pFlag->m_X = (int)m_Pos.x;
	pFlag->m_Y = (int)m_Pos.y;
	pFlag->m_Team = m_Team;
}
