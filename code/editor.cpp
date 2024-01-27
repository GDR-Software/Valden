#include "editor.h"

CEditorLayer::CEditorLayer( void )
{
    m_bFilterShowCheckpoints = true;
    m_bFilterShowEntities = true;
    m_bFilterShowSpawns = true;
    m_bFilterShowTerrain = true;
    
    m_bShowTilesetData = true;
}

CEditorLayer::~CEditorLayer()
{
}

void CEditorLayer::OnDetach( void )
{
    Map_Free();
//    delete g_pProjectManager;
//    delete g_pConfirmModifiedDlg;
//    delete g_pPrefsDlg;
    Cmd_Shutdown();

    std::ofstream file( va( "%svalden-recent.txt", g_pEditor->m_CurrentPath.c_str() ), std::ios::out );

    if ( !file.is_open() ) {
        Log_FPrintf( SYS_ERR, "Failed to create valden-recent.txt file!\n" );
        return;
    }

    for ( const auto& it : m_RecentFiles ) {
        file << it << '\n';
    }
    file.flush();
}

void CEditorLayer::LoadProjectFileDlg( void )
{
    ImGuiFileDialog::Instance()->OpenDialog( "LoadProjectFileDlg", "Open File", "Project Files (*.proj *.json){.proj, .json},.json,.proj",
        m_RecentDirectory );
}

void CEditorLayer::OnFileSave( void )
{
    if ( !g_pMapInfoDlg->m_bMapModified ) {
        return;
    }
    Map_Save();
}

void CEditorLayer::OnFileSaveAll( void )
{
    if ( g_pMapInfoDlg->m_bMapModified ) {
        Map_Save();
    }
    if ( m_bProjectModified ) {
        g_pProjectManager->Save();
    }
}

void CEditorLayer::OnFileImport( void )
{
    ImGuiFileDialog::Instance()->OpenDialog( "ImportMapFileDlg", "Import Map File", MAP_FILEDLG_FILTERS,
        m_RecentDirectory );
}

void CEditorLayer::OnFileSaveAs( void )
{
    ImGuiFileDialog::Instance()->OpenDialog( "SaveMapFileAsDlg", "Select Save Folder", ".map,.bmf", m_RecentDirectory );
}

void CEditorLayer::OnFileNew( void )
{
    Map_New();
}

void CEditorLayer::OnFileOpen( void )
{
    ImGuiFileDialog::Instance()->OpenDialog( "OpenMapFileDlg", "Open Map File", MAP_FILEDLG_FILTERS,
        m_RecentDirectory );
}

void CEditorLayer::OnFileProjectNew( void )
{
    Map_New();
    g_pProjectManager->New();
}

void CEditorLayer::OnFileProjectLoad( void )
{
}

void CEditorLayer::OnAttach( void ) {
}

