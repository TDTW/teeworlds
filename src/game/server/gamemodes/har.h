/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_HAR_H
#define GAME_SERVER_GAMEMODES_HAR_H
#include <game/server/gamecontroller.h>
#include <game/server/entity.h>

class CGameControllerHAR : public IGameController
{
public:
	class CFlag *m_Flag[5];
	class CFlag *Red_Flag[16];
	class CFlag *Blue_Flag[16];

	// TDTW: Add change name check logic with rcon
	char realCatcherName[MAX_NAME_LENGTH];

	CGameControllerHAR(class CGameContext *pGameServer);
	virtual bool OnEntity(int Index, vec2 Pos);
	virtual void OnCharacterSpawn(CCharacter *pChr);
	virtual void ChangeCatcher(int Index_Old, int Index_New);
	virtual void OnPlayerNameChange(class CPlayer *pP);

	void ChangeNameCatcher(int Index, bool Catch);
	void SetWeaponCatcher(int Index, bool Catch);
	void ChatCatcherChat(int Index_Old, int Index_New);
	void ClearCatchers();

	virtual void Tick();
	void FlagTick();
	int CountFlag();
	virtual int IsFlagCharacter(int Index);
	virtual void Snap(int SnappingClient);
	virtual void CreateFlags(int m_Index);
	virtual void DeleteFlags(int m_Index);
	virtual void DeleteFlag(int Index);
	void SendBroadcastTick();
};

#endif

