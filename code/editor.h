#ifndef __EDITOR__
#define __EDITOR__

#pragma once

#include "Application.h"
#include "Walnut/Layer.h"
#include "gln.h"
#include "dialog.h"
#include "ImGuiFileDialog.h"
#include "preferences.h"
#include "idatastream.h"
#include "stream.h"
#include "Walnut/Image.h"
#include "shader.h"
#include "map.h"
#include "ImGuiTextEditor.h"
#include "command.h"
#include "ContentBrowserPanel.h"
#include "project.h"

//
// editor dialogs
//
#include "shadermanager_dlg.h"
#include "textedit_dlg.h"
#include "mapinfo_dlg.h"

struct TextEditor
{
    std::filesystem::path m_Filename;
    ImGui::TextEditor m_Editor;
    bool m_bOpen;

    TextEditor( const std::filesystem::path& filename, const ImGui::TextEditor& editor )
        : m_Filename{ filename }, m_Editor{ editor }, m_bOpen{ true }
    {
    }
    TextEditor( const TextEditor& ) = default;

    void New( const std::filesystem::path& filename, const ImGui::TextEditor& editor )
    {
        m_Filename = filename;
        m_Editor = editor;
        m_bOpen = true;
    }

    TextEditor& operator=( TextEditor&& other )
    {
        m_Filename.clear();

        m_Filename = other.m_Filename;
        m_Editor = other.m_Editor;
        m_bOpen = other.m_bOpen;

        return *this;
    }
};

inline constexpr ImVec2 FileDlgWindowSize = ImVec2( 1012, 641 );

#define FileDialogUIRender( key, fn ) \
    if ( ImGuiFileDialog::Instance()->IsOpened( key ) ) { \
        if ( ImGuiFileDialog::Instance()->Display( key, ImGuiWindowFlags_NoResize, FileDlgWindowSize, FileDlgWindowSize ) ) { \
            if ( ImGuiFileDialog::Instance()->IsOk() ) { \
                g_pEditor->m_RecentDirectory = ImGuiFileDialog::Instance()->GetFilePathName(); \
                while ( g_pEditor->m_RecentDirectory.back() != PATH_SEP ) { g_pEditor->m_RecentDirectory.pop_back(); } \
                fn( ImGuiFileDialog::Instance()->GetFilePathName() ); \
            } \
            ImGuiFileDialog::Instance()->Close(); \
        } \
    }

enum class EditorInputFocus : uint16_t
{
    MapFocus = 0,
    ToolFocus,
};

class CEditorLayer : public Walnut::Layer
{
public:
    CEditorLayer( void );
    ~CEditorLayer();

    void OnTextEdit( const std::filesystem::path& filename );
    void OpenFileDialog( const char *id, const char *name, const char *filters, const std::string& filepath );

    void OnFileOpen( void );
    void OnFileNew( void );
    void OnFileSave( void );
    void OnFileSaveAll( void );
    void OnFileSaveAs( void );
    void OnFileImport( void );
    void OnFileProjectNew( void );
    void OnFileProjectLoad( void );
    void OnFileProjectSettings( void );

    void LoadProjectFileDlg( void );

    void OnEditUndo( void );
    void OnEditRedo( void );
    void OnEditPreferences( void );

    void OnHelpDlg( void );

    int ConfirmModified( void );

    virtual void OnUIRender( void ) override;
    virtual void OnAttach( void ) override;
    virtual void OnDetach( void ) override;
	virtual void OnUpdate( float timestep ) override;

    std::function<void( void )> m_FuncOnDoneConfirm;

    bool m_bOpenMapPopup;
    bool m_bImportMapPopup;
    bool m_bConfirmMapSave;
    bool m_bProjectModified;
    
    bool m_bShowConsole;
    bool m_bShowImGuiMetricsWindow;
    bool m_bShowTilesetData;
    bool m_bShowShaders;
    bool m_bShowInUseTextures;
    bool m_bShowAllTextures;

    bool m_bPreferencesOpen;
    bool m_bFilterShowTerrain;
    bool m_bFilterShowEntities;
    bool m_bFilterShowCheckpoints;
    bool m_bFilterShowSpawns;

    int m_nOldMapWidth;
    int m_nOldMapHeight;

    std::string m_CurrentPath;

    std::vector<std::string> m_RecentFiles;

    std::vector<TextEditor>::iterator m_pCurrentEditor;
    std::vector<TextEditor> m_TextEditors;
    std::vector<CEditorDialog *> m_Dialogs;

    bool m_bPopupMenuFile;
    bool m_bPopupMenuEdit;
    bool m_bPopupMenuView;

    std::string m_RecentDirectory;

    void *m_pCopyPasteData;

    bool m_bWindowFocused;
    EditorInputFocus m_InputFocus;
};

extern Walnut::Application *g_pApplication;;
extern std::shared_ptr<CEditorLayer> g_pEditor;
extern std::shared_ptr<CFileDialogLayer> g_pFileDlg;
extern CPopupDlg *g_pConfirmModifiedDlg;
extern CPrefsDlg *g_pPrefsDlg;
extern CMapInfoDlg *g_pMapInfoDlg;

extern std::string g_strBitmapsDir;
extern std::string g_pidFile;
extern std::mutex g_ImGuiLock;

inline bool MenuItemWithTooltip( const char *title, const char *shortcut, const char *tooltip, ... )
{
    va_list argptr;
    bool active;

    active = ImGui::MenuItem( title, shortcut );

    if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) ) {
        va_start( argptr, tooltip );
        ImGui::SetItemTooltipV( tooltip, argptr );
        va_end( argptr );
    }

    return active;
}

inline bool MenuItemWithTooltip( const char *title, const char *tooltip, ... )
{
    va_list argptr;
    bool active;

    active = ImGui::MenuItem( title );

    if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) ) {
        va_start( argptr, tooltip );
        ImGui::SetItemTooltipV( tooltip, argptr );
        va_end( argptr );
    }

    return active;
}

inline bool ButtonWithTooltip( const char *title, const char *tooltip, ... )
{
    va_list argptr;
    bool active;

    active = ImGui::Button( title );

    if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) ) {
        va_start( argptr, tooltip );
        ImGui::SetItemTooltipV( tooltip, argptr );
        va_end( argptr );
    }

    return active;
}

inline bool TreeNodeWithTooltip( const char *title, const ImGuiTreeNodeFlags flags, const char *tooltip, ... )
{
    va_list argptr;
    bool active;

    // shut up compiler
    active = ImGui::TreeNodeEx( (void *)(uintptr_t)title, flags, "%s", title );

    if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) ) {
        va_start( argptr, tooltip );
        ImGui::SetItemTooltipV( tooltip, argptr );
        va_end( argptr );
    }

    return active;
}

#endif