#ifndef __INFODATA_DLG__
#define __INFODATA_DLG__

#pragma once

#include "editor.h"

typedef struct {

} entityData_t;

typedef enum {
    RANK_S,
    RANK_A,
    RANK_B,
    RANK_C,
    RANK_D,
    RANK_F,
    RANK_U,

    NUMRANKS
} rank_t;

typedef struct {
    rank_t rank;
    uint32_t minKills;
    uint32_t minTime;
    uint32_t minStyle;
    uint32_t maxDeaths;
    uint32_t maxCollateral;
    bool requiresClean;
} rankData_t;

typedef struct {
    char name[MAX_NPATH];
    mapData_t *mapData;
    int difficulty;
    rankData_t ranks[NUMRANKS];
} levelData_t;

struct LevelSelection {
    levelData_t *pData;
    bool bUsed;
};

class CInfoDataDlg : public CEditorDialog
{
public:
    CInfoDataDlg( void );

    virtual void Draw( void ) override;

    bool m_bShow;
private:
    std::vector<LevelSelection> m_LevelDatas;
    std::vector<LevelSelection>::iterator m_pCurrent;

    void CreateLevelData( void );
};

#endif