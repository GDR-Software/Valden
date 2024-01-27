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

    void SetCurrent( mapData_t *data );

    bool m_bShow;
    bool m_bHasSpawnWindow;
    bool m_bHasCheckpointWindow;
    bool m_bHasLightWindow;
    bool m_bMapModified;
    bool m_bTilesetModified;
    bool m_bMapNameUpdated;

    mapspawn_t *m_pSpawnEdit;
    mapcheckpoint_t *m_pCheckpointEdit;
    maplight_t *m_pLightEdit;
private:
    std::vector<MapSelection> m_MapList;
    std::vector<MapSelection>::iterator m_pCurrentMap;
    char m_szTempMapName[MAX_NPATH+2];
    char m_szCurrentShader[MAX_NPATH];
    
    uint32_t m_nSelectedSpawnEntityType;
    uint32_t m_nSelectedSpawnEntityId;

    void CreateCheckpoint( void );
    void CreateSpawn( void );
};

#endif