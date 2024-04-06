#include "editor.h"

using json = nlohmann::json;

CPrefsDlg::CPrefsDlg( void )
{
}

CPrefsDlg::~CPrefsDlg()
{
    Save();
}

static inline ImVec4 json_to_vec4( const json& data, const std::string& name ) {
    const std::array<double, 4>& color = data.at( name );
    return ImVec4( color[0], color[1], color[2], color[3] ) ;
}

static inline const std::array<double, 4>& vec4_to_json( const ImVec4& data ) {
    static std::array<double, 4> color;
    color[0] = data.x;
    color[1] = data.y;
    color[2] = data.z;
    color[3] = data.w;
    return color;
}

void CPrefsDlg::Reset( void )
{
    m_bAutoSave = true;
    m_bAutoLoadOnStartup = false;
    m_bLoadLastMap = false;
    m_bLoadLastProject = false;
    m_bForceLog = false;
    m_bLogToFile = true;

    m_nFontScale = 1.0f;
    m_nOldFontScale = 1.0f;
    m_nAutoSaveTime = 5;
    m_nSelected = -1;
    m_nCameraMoveSpeed = 0.5f;
    m_nCameraRotationSpeed = 0.2f;

    m_LastProject = " ";
    m_LastMap = " ";
    m_PrefsDlgEngine = "None";

    if ( ImGui::GetCurrentContext() ) {
        ImGui::StyleColorsDark( &m_EditorStyle );
        m_bUseEditorStyleCustom = false;
        m_bUseEditorStyleLight = false;
        m_bUseEditorStyleDark = true;
        ImGui::GetStyle() = m_EditorStyle;
    }
}

void CPrefsDlg::Load( void )
{
    std::ifstream file;
    json data;
    const char *ospath = va( "%spreferences.json", g_pEditor->m_CurrentPath.c_str() );

    file.open( ospath, std::ios::in );

    if ( file.fail() ) {
        Log_Printf( "[CPrefsDlg::CPrefsDlg] Failed to load preferences file '%s'", ospath );
        Log_Printf( "Setting all preferences to default...\n" );
        Reset();
        return;
    }

    try {
        data = json::parse( file );
    } catch ( const json::exception& e ) {
        Error( "[CPrefsDlg::Load] Failed to load preferences file, nlohmann::json error ->\n    id: %i\n    what: %s", e.id, e.what() );
    }

    m_bAutoSave = data["autosave"];
    m_bLoadLastMap = data["loadlastmap"];
    m_bLoadLastProject = data["loadlastproject"];
    m_bForceLog = data["forcelog"];
    m_bLogToFile = data["logfile"];

    m_ProjectDataPath = data["projectdatapath"];
    m_LastProject = data["lastproject"];
    m_LastMap = data["lastmap"];
    m_Engine = data["engine"];
    if ( data.contains( "enginepath" ) ) {
        m_PrefsDlgEngine = data["enginepath"];
    } else {
        m_PrefsDlgEngine = g_pEditor->m_CurrentPath;
    }

    m_nCameraMoveSpeed = data["camera_move_speed"];
    m_nCameraRotationSpeed = data["camera_rotation_speed"];
    
    m_nAutoSaveTime = data["autosavetime"];
    m_nFontScale = data["fontscale"];

    if ( data.contains( "editorstyleShort" ) ) {
        const std::string& editorStyle = data["editorstyleShort"];

        if ( editorStyle == "dark" ) {
            ImGui::StyleColorsDark( &m_EditorStyle );
            m_bUseEditorStyleDark = true;
        } else if ( editorStyle == "light" ) {
            ImGui::StyleColorsLight( &m_EditorStyle );
            m_bUseEditorStyleLight = true;
        } else {
            Error( "[CPrefsDlg::Load] Failed to load editor style, invalid style type '%s'", editorStyle.c_str() );
        }
    } else {
        const json& style = data.at( "editorstyle" );
        ImGuiStyle *imStyle = &m_EditorStyle;

        m_bUseEditorStyleCustom = true;

        memset( imStyle, 0, sizeof(*imStyle) );

        imStyle->Colors[ImGuiCol_Button] = json_to_vec4( style, "button" );
        imStyle->Colors[ImGuiCol_ButtonActive] = json_to_vec4( style, "buttonActive" );
        imStyle->Colors[ImGuiCol_ButtonHovered] = json_to_vec4( style, "buttonHovered" );

        imStyle->Colors[ImGuiCol_FrameBg] = json_to_vec4( style, "frameBg" );
        imStyle->Colors[ImGuiCol_FrameBgActive] = json_to_vec4( style, "frameBgActive" );
        imStyle->Colors[ImGuiCol_FrameBgHovered] = json_to_vec4( style, "frameBgHovered" );
        imStyle->Colors[ImGuiCol_PopupBg] = json_to_vec4( style, "popupBg" );
        imStyle->Colors[ImGuiCol_MenuBarBg] = json_to_vec4( style, "menuBarBg" );
    }

    m_bActive = false;
    m_nSelected = -1;

    m_Flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
}

void CPrefsDlg::LoadImGuiData( void )
{
    if ( ImGui::GetCurrentContext() == NULL ) {
        return;
    }

    m_nOldFontScale = ImGui::GetIO().FontDefault->Scale;
    ImGui::GetIO().FontDefault->Scale *= m_nFontScale;

    memcpy( &ImGui::GetStyle(), &m_EditorStyle, sizeof(ImGuiStyle) );
}

void CPrefsDlg::Save( void ) const
{
    json data;
    uint32_t i;

    Log_Printf( "CPrefsDlg::Save: saving editor preferences file...\n" );

    if ( m_bUseEditorStyleDark ) {
        data["editorstyleShort"] = "dark";
    } else if ( m_bUseEditorStyleLight ) {
        data["editorstyleShort"] = "light";
    } else {
        data["editorstyle"]["button"] = vec4_to_json( m_EditorStyle.Colors[ImGuiCol_Button] );
        data["editorstyle"]["buttonActive"] = vec4_to_json( m_EditorStyle.Colors[ImGuiCol_ButtonActive] );
        data["editorstyle"]["buttonHovered"] = vec4_to_json( m_EditorStyle.Colors[ImGuiCol_ButtonHovered] );

        data["editorstyle"]["frameBg"] = vec4_to_json( m_EditorStyle.Colors[ImGuiCol_FrameBg] );
        data["editorstyle"]["frameBgActive"] = vec4_to_json( m_EditorStyle.Colors[ImGuiCol_FrameBgActive] );
        data["editorstyle"]["frameBgHovered"] = vec4_to_json( m_EditorStyle.Colors[ImGuiCol_FrameBgHovered] );
        data["editorstyle"]["popupBg"] = vec4_to_json( m_EditorStyle.Colors[ImGuiCol_PopupBg] );
        data["editorstyle"]["menuBarBg"] = vec4_to_json( m_EditorStyle.Colors[ImGuiCol_MenuBarBg] );
    }

    if ( !m_bLogToFile ) {
        Sys_LogFile();
        ImGui::LogFinish();
    }

    data["lastproject"] = g_pProjectManager->GetProject()->m_Name;
    data["lastmap"] = mapData->name;
    data["engine"] = m_Engine;
    data["projectdatapath"] = m_ProjectDataPath;
    data["enginepath"] = m_PrefsDlgEngine;

    data["autosave"] = m_bAutoSave;
    data["loadlastmap"] = m_bLoadLastMap;
    data["loadlastproject"] = m_bLoadLastProject;
    data["forcelog"] = m_bForceLog;
    data["logfile"] = m_bLogToFile;
    data["lastproject"] = m_LastProject;
    data["lastmap"] = m_LastMap;

    data["camera_move_speed"] = m_nCameraMoveSpeed;
    data["camera_rotation_speed"] = m_nCameraRotationSpeed;

    data["autosavetime"] = m_nAutoSaveTime;
    data["fontscale"] = m_nFontScale;

    std::ofstream file( va( "%spreferences.json", g_pEditor->m_CurrentPath.c_str() ), std::ios::out );

    if ( !file.is_open() ) {
        Error( "[CPrefsDlg::Save] Failed to save preferences file" );
    }

    file.width( 4 );
    file << data;
}

void CPrefsDlg::OnActivate( void )
{
    m_bActive = true;
    ImGui::SetNextWindowPos( ImVec2( 1920 / 2, 1080 / 2 ) );
}

static void SetEnginePath( const std::string& filepath ) {
    g_pPrefsDlg->m_PrefsDlgEngine = filepath;
}

void CPrefsDlg::Draw( void )
{
    if ( !m_bActive ) {
        m_bOpen = false;
        return;
    }

    auto editorStyleToString = [this]() -> const char * {
        if ( m_bUseEditorStyleDark ) {
            return "Dark";
        } else if ( m_bUseEditorStyleLight ) {
            return "Light";
        } else {
            return "Custom";
        }
    };
    bool open;
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
                                                | ImGuiTreeNodeFlags_FramePadding;
    
    if ( ImGui::Begin( "Valden Preferences", &m_bActive, m_Flags ) ) {
        if ( ImGui::Button( "Clear" ) ) {
            Reset();
        }
        ImGui::SameLine();
        if ( ImGui::Button( "Save" ) ) {
            Save();
            Sys_LogFile();
            Cmd_ExecuteText( "/reloadshaders" );
        }
        ImGui::SameLine();
        if ( ImGui::Button( "Cancel" ) ) {
            m_bOpen = false;
            m_bActive = false;
        }

        ImGui::Separator();

        ImGui::Columns( 2 );
        ImGui::SetColumnWidth( 0, 200 );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 4 ) );

        open = ImGui::TreeNodeEx( (void *)(uintptr_t)"Globals", treeNodeFlags, "Globals" );

        if ( open ) {
            ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImVec4( 0.0f, 1.0f, 1.0f, 1.0f ) );

            if ( ImGui::MenuItem( "Editor Settings" ) ) {
                m_nSelected = 0;
            }

            ImGui::PopStyleColor();
            ImGui::TreePop();
        }

        open = ImGui::TreeNodeEx( (void *)(uintptr_t)"Graphics", treeNodeFlags, "Graphics" );

        if ( open ) {
            if ( ImGui::MenuItem( "View" ) ) {
                m_nSelected = 1;
            }
            if ( ImGui::MenuItem( "Texture Settings" ) ) {
                m_nSelected = 2;
            }
            ImGui::TreePop();
        }

        ImGui::NextColumn();

        switch ( m_nSelected ) {
        case -1:
            break;
        case 0: {
            ImGui::Checkbox( "Auto load game on startup", &m_bAutoLoadOnStartup );
            ImGui::Checkbox( "Log the console to editor.log", &m_bLogToFile );

//            ImGui::TextUnformatted( "Editor Font Scale" );
//            ImGui::SameLine();
//            if ( ImGui::SliderFloat( " ", &m_nFontScale, 1.0f, 5.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp ) ) {
//                ImGui::GetIO().FontDefault->Scale = m_nOldFontScale * m_nFontScale;
//            }
            ImGui::TextUnformatted( "Set Editor Save Path" );
            ImGui::SameLine();
            if ( ImGui::Button( m_PrefsDlgEngine.c_str() ) ) {
                ImGuiFileDialog::Instance()->OpenDialog( "PrefsDlgEnginePath", "Select Save Path", "/", "." );
            }

            ImGui::Checkbox( "Load last project on open", &m_bLoadLastProject );
            ImGui::Checkbox( "Load last map on open", &m_bLoadLastMap );

            if ( !m_bAutoSave ) {
                ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.45f, 0.45f, 0.45f, 1.0f ) );
                ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImVec4( 0.45f, 0.45f, 0.45f, 1.0f ) );
                ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImVec4( 0.45f, 0.45f, 0.45f, 1.0f ) );
            }

            ImGui::Checkbox( "Auto save every", &m_bAutoSave );
            ImGui::SameLine();
            ImGui::InputInt( " ", &m_nAutoSaveTime );
            ImGui::SameLine();
            ImGui::TextUnformatted( "minutes" );

            if ( !m_bAutoSave ) {
                ImGui::PopStyleColor( 3 );
            }

            if ( ImGui::BeginCombo( "Editor Style", editorStyleToString() ) ) {
                
                if ( ImGui::Selectable( "Dark", m_bUseEditorStyleDark ) ) {
                    m_bUseEditorStyleDark = true;
                    m_bUseEditorStyleLight = false;
                    m_bUseEditorStyleCustom = false;

                    ImGui::StyleColorsDark( &m_EditorStyle );
                    ImGui::GetStyle() = m_EditorStyle;
                }
                if ( ImGui::Selectable( "Light", m_bUseEditorStyleLight ) ) {
                    m_bUseEditorStyleDark = false;
                    m_bUseEditorStyleLight = true;
                    m_bUseEditorStyleCustom = false;

                    ImGui::StyleColorsLight( &m_EditorStyle );
                    ImGui::GetStyle() = m_EditorStyle;
                }
                if ( ImGui::Selectable( "Custom", m_bUseEditorStyleCustom ) ) {
                    m_bUseEditorStyleDark = false;
                    m_bUseEditorStyleLight = false;
                    m_bUseEditorStyleCustom = true;
                }

                ImGui::EndCombo();
            }

            break; }
        case 1: {
            ImGui::SliderFloat( "Camera Movement Speed", &m_nCameraMoveSpeed, 0.001f, 32.0f );
            if ( ImGui::IsItemHovered() ) {
                ImGuiIO& io = ImGui::GetIO();
                if ( io.MouseWheel > 0 ) {
                    m_nCameraMoveSpeed += 2.0f;
                } else if ( io.MouseWheel < 0 ) {
                    m_nCameraMoveSpeed -= 2.0f;
                }
            }

            if ( m_nCameraMoveSpeed > 32.0f ) {
                m_nCameraMoveSpeed = 32.0f;
            } else if ( m_nCameraMoveSpeed < 0.001f ) {
                m_nCameraMoveSpeed = 0.001f;
            }

            ImGui::SliderFloat( "Camera Rotation Speed", &m_nCameraRotationSpeed, 1.0f, 20.0f );
            if ( ImGui::IsItemHovered() ) {
                ImGuiIO& io = ImGui::GetIO();
                if ( io.MouseWheel > 0 ) {
                    m_nCameraRotationSpeed += 2.0f;
                } else if ( io.MouseWheel < 0 ) {
                    m_nCameraRotationSpeed -= 2.0f;
                }
            }

            if ( m_nCameraRotationSpeed > 20.0f ) {
                m_nCameraRotationSpeed = 20.0f;
            } else if ( m_nCameraRotationSpeed < 1.0f ) {
                m_nCameraRotationSpeed = 1.0f;
            }

            break; }
        case 2:
            break;
        };

        ImGui::PopStyleVar();

        ImGui::End();
    }

    FileDialogUIRender( "PrefsDlgEnginePath", SetEnginePath );
}
