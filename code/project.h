#ifndef __PROJECT__
#define __PROJECT__

#pragma once

struct Project
{
    std::string m_Name;
    std::vector<mapData_t *> m_MapList;

    std::vector<entityInfo_t> m_EntityList[NUMENTITYTYPES];

    std::string m_AssetPath;

    std::string m_FilePath;
    std::filesystem::path m_AssetDirectory;
};

class CProjectManager
{
public:
    CProjectManager( void );
    ~CProjectManager();

    const std::string& GetName( void ) const;
    const std::string& GetProjectDirectory( void ) const;
    const std::vector<mapData_t *>& GetMapList( void ) const;
    const std::filesystem::path& GetAssetDirectory( void ) const;
    std::shared_ptr<Project>& GetProject( void );

    const std::unordered_map<std::string, std::shared_ptr<Project>>& GetProjectCache( void ) const;

    void New( void );
    void Save( void ) const;
    void SetCurrent( const std::string& name, bool buildPath = false );

    friend class CProjectSettingsDlg;
private:
    void InitProjectConfig( const char *filepath ) const;
    void AddToCache( const std::string& path, bool loadJSON = false, bool buildPath = false );

    std::unordered_map<std::string, std::shared_ptr<Project>> m_ProjList;
    std::string m_ProjectsDirectory;

    std::shared_ptr<Project> m_CurrentProject;
};

extern CProjectManager *g_pProjectManager;

inline const std::string& CProjectManager::GetName( void ) const {
    return m_CurrentProject->m_Name;
}

inline const std::string& CProjectManager::GetProjectDirectory( void ) const {
    return m_CurrentProject->m_FilePath;
}

inline const std::vector<mapData_t *>& CProjectManager::GetMapList( void ) const {
    return m_CurrentProject->m_MapList;
}

inline const std::filesystem::path& CProjectManager::GetAssetDirectory( void ) const {
    return m_CurrentProject->m_AssetDirectory;
}

inline std::shared_ptr<Project>& CProjectManager::GetProject( void ) {
    return m_CurrentProject;
}

inline const std::unordered_map<std::string, std::shared_ptr<Project>>& CProjectManager::GetProjectCache( void ) const {
    return m_ProjList;
}

class CProjectSettingsDlg : public Walnut::Layer
{
public:
    virtual void OnAttach( void ) override;
    virtual void OnDetach( void ) override;
    virtual void OnUIRender( void ) override;
};

#endif