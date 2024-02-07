#ifndef __MAPINFO_DLG__
#define __MAPINFO_DLG__

#pragma once

#include "shader.h"
#include "editor.h"

struct MapSelection
{
    mapData_t *m_pMapData;
    bool m_bUsed;

    MapSelection( mapData_t *data )
        : m_pMapData{ data }, m_bUsed{ true }
    {
    }
};

class CMapInfoDlg : public CEditorDialog
{
public:
    CMapInfoDlg( void );

    virtual void Draw( void ) override;

    void ProjectSettingsDlg( void );
    void SetCurrent( mapData_t *data );
    inline void SetModified( bool bMap, bool bTileset ) {
        m_bMapModified = bMap;
        m_bTilesetModified = bTileset;
    }

    bool m_bShow;
    
    bool m_bHasSpawnWindow;
    bool m_bHasCheckpointWindow;
    bool m_bHasLightWindow;
    bool m_bHasTileWindow;

    bool m_bMapModified;
    bool m_bTilesetModified;
    bool m_bMapNameUpdated;

    bool m_bHasProjectWindow;

    mapspawn_t *m_pSpawnEdit;
    mapcheckpoint_t *m_pCheckpointEdit;
    maplight_t *m_pLightEdit;

    char m_szCurrentShader[MAX_NPATH];
private:
    std::vector<entityInfo_t>::iterator m_pCurrentMob;
    std::vector<entityInfo_t>::iterator m_pCurrentItem;
    std::vector<entityInfo_t>::iterator m_pCurrentWeapon;
    std::vector<entityInfo_t>::iterator m_pCurrentBot;

    void AddMob( void );
    void AddItem( void );
    void AddBot( void );

    void ModifyMob( void );
    void ModifyItem( void );
    void ModifyBot( void );

    bool m_bHasEditItemWindow;
    bool m_bHasEditMobWindow;
    bool m_bHasEditBotWindow;

    bool m_bHasAddItemWindow;
    bool m_bHasAddMobWindow;
    bool m_bHasAddBotWindow;

    char m_szEditItemName[MAX_NPATH];
    int32_t m_nEditItemId;

    char m_szEditMobName[MAX_NPATH];
    int32_t m_nEditMobId;

    char m_szEditBotName[MAX_NPATH];
    int32_t m_nEditBotId;

    char m_szAddItemName[MAX_NPATH];
    int32_t m_nAddItemId;

    char m_szAddMobName[MAX_NPATH];
    int32_t m_nAddMobId;

    char m_szAddBotName[MAX_NPATH];
    int32_t m_nAddBotId;

    std::vector<MapSelection> m_MapList;
    std::vector<MapSelection>::iterator m_pCurrentMap;
    char m_szTempMapName[MAX_NPATH+2];
    
    uint32_t m_nSelectedSpawnEntityType;
    uint32_t m_nSelectedSpawnEntityId;

    void CreateCheckpoint( void );
    void CreateSpawn( void );
};

#endif