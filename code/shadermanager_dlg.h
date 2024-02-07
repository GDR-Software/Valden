#ifndef __ASSETMANAGER_DLG__
#define __ASSETMANAGER_DLG__

#pragma once

#include "Walnut/Layer.h"

class CAssetManagerDlg : public Walnut::Layer
{
public:
    CAssetManagerDlg( void );
    virtual ~CAssetManagerDlg() override;

    virtual void OnAttach( void ) override;
    virtual void OnDetach( void ) override;
    virtual void OnUIRender( void ) override;
private:
    std::string m_CurrentDirectory;
    std::string m_BaseDirectory;

    std::vector<std::filesystem::path> m_ShaderList;

    Walnut::Image *m_pDirectoryIcon;
    Walnut::Image *m_pFileIcon;

    char m_szNewShaderName[MAX_NPATH];
    std::string m_newShaderPath;
};

#endif