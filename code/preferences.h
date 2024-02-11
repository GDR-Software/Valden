#ifndef __PREFERENCES__
#define __PREFERENCES__

#pragma once

typedef struct entityInfo_s {
    std::string m_Name;
    uint32_t m_Id;
    std::map<std::string, std::string> m_Properties;

    entityInfo_s( const char *name, uint32_t id )
        : m_Name{ name }, m_Id{ id }, m_Properties{}
    {
    }
    ~entityInfo_s() = default;
} entityInfo_t;

class CPrefsDlg : public CEditorDialog
{
public:
    CPrefsDlg( void );
    virtual ~CPrefsDlg() override;

    void LoadImGuiData( void );
    void Save( void ) const;
    void Reset( void );
    void Load( void );

    virtual void Draw( void ) override;
    virtual void OnActivate( void ) override;

    int m_nSelected;
    
    // engine executable name
    std::string m_Engine;

    // ospath to the engine
    std::string m_PrefsDlgEngine;

    // path to the last loaded project file
    std::string m_LastProject;

    // path to the last loaded map file
    std::string m_LastMap;

    std::string m_ProjectDataPath;

    bool m_bAutoSave;
    bool m_bAutoLoadOnStartup;
    bool m_bLogToFile;
    bool m_bLoadLastMap;
    bool m_bLoadLastProject;
    bool m_bForceLog;

    float m_nFontScale;
    float m_nOldFontScale;

    float m_nCameraMoveSpeed;
    float m_nCameraRotationSpeed;

    int m_nAutoSaveTime;
    ImGuiStyle m_EditorStyle;
private:
    bool m_bUseEditorStyleDark;
    bool m_bUseEditorStyleLight;
    bool m_bUseEditorStyleCustom;
};

#endif