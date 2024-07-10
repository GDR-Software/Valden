#include "editor.h"
#include "gui.h"
#include "stb_image.h"
#include "mapinfo_dlg.h"
#include "events.h"
#include "misc/cpp/imgui_stdlib.h"
#include <mutex>

std::mutex g_ImGuiLock;
Walnut::Application *g_pApplication;

std::shared_ptr<CEditorLayer> g_pEditor;
std::shared_ptr<CFileDialogLayer> g_pFileDlg;
std::shared_ptr<ContentBrowserPanel> g_pContentBrowserDlg;
std::shared_ptr<CTextEditDlg> g_pShaderEditDlg;
std::shared_ptr<CTextEditDlg> g_pScriptEditDlg;
std::shared_ptr<CAssetManagerDlg> g_pAssetManagerDlg;
CMapInfoDlg *g_pMapInfoDlg;
CPopupDlg *g_pConfirmModifiedDlg;
CPrefsDlg *g_pPrefsDlg;

bool g_ApplicationRunning;

static void LoadRecentFiles( void )
{
	std::string line;
	std::ifstream file( va( "%svalden-recent.txt", g_pEditor->m_CurrentPath.c_str() ), std::ios::in );

	if ( !file.is_open() ) {
		return;
	}

	file.seekg( std::ios::end );
	g_pEditor->m_RecentFiles.reserve( file.tellg() );
	file.seekg( std::ios::beg );

	while ( std::getline( file, line ) ) {
		g_pEditor->m_RecentFiles.emplace_back( line );

		if ( g_pEditor->m_RecentFiles.size() > 15 ) {
			break;
		}
	}
}

static Walnut::Image *LoadIcon( const char *name ) {
	return new Walnut::Image( va( "%sbitmaps/%s.png", g_pEditor->m_CurrentPath.c_str(), name ) );
}

static void InitDialogs( void )
{
	CDlgWidget *widget;

	//
	// Confirmation Popup
	//

	widget = new CUIText( "The current map has changed since it was last saved.\nWould your like to save before continuing?" );

	g_pConfirmModifiedDlg = new CPopupDlg;
    g_pConfirmModifiedDlg->SetLabel( "WARNING!" );
    g_pConfirmModifiedDlg->m_PopupFlags = MB_YESNOCANCEL;

    g_pConfirmModifiedDlg->AddWidget( widget );

	g_pMapInfoDlg = new CMapInfoDlg;

	g_pEditor->m_Dialogs.emplace_back( g_pMapInfoDlg );
	g_pEditor->m_Dialogs.emplace_back( g_pConfirmModifiedDlg );
	g_pEditor->m_Dialogs.emplace_back( g_pPrefsDlg );

	g_pProjectManager = new CProjectManager;
	if ( g_pPrefsDlg->m_bLoadLastProject ) {
		g_pProjectManager->SetCurrent( g_pPrefsDlg->m_LastProject, true );
	} else {
		g_pProjectManager->New();
	}

	g_pContentBrowserDlg = std::make_shared<ContentBrowserPanel>();
	g_pApplication->PushLayer( g_pContentBrowserDlg );

	g_pAssetManagerDlg = std::make_shared<CAssetManagerDlg>();
	g_pApplication->PushLayer( g_pAssetManagerDlg );

	g_pPrefsDlg->LoadImGuiData();

	parm_compression = CheckParm( "-compression" );

	if ( parm_compression != -1 ) {
		if ( !N_stricmp( myargv[parm_compression+1], "zlib" ) ) {
			parm_compression = COMPRESS_ZLIB;
			Log_Printf( "Manual compression override for zlib.\n" );
		} else if ( !N_stricmp( myargv[parm_compression+1], "bzip2" ) ) {
			parm_compression = COMPRESS_BZIP2;
			Log_Printf( "Manual compression override for bzip2.\n" );
		} else {
			Log_FPrintf( SYS_WRN, "WARNING: invalid compression format provided '%s', defaulting to zlib\n", myargv[parm_compression+1] );
			parm_compression = COMPRESS_ZLIB;
		}
	}
	else {
		parm_compression = COMPRESS_ZLIB;
		Log_Printf( "Using ZLib compression.\n" );
	}

	Walnut::InitShaders();
	Walnut::InitTextures();
	Map_Init();

	g_pEditor->m_RecentDirectory = g_pPrefsDlg->m_PrefsDlgEngine;
}

void CEditorLayer::OnTextEdit( const std::filesystem::path& filename )
{
	const char *ext;
	const char *defs;
	ImGui::TextEditor editor;
	const ImGui::TextEditor::LanguageDefinition *lang;
	
	ext = COM_GetExtension( filename.filename().c_str() );

	if ( !N_stricmp( ext, "shader" ) ) {
		lang = std::addressof( ImGui::TextEditor::LanguageDefinition::Q3Shader() );
	} else if ( !N_stricmp( ext, "glsl" ) ) {

	} else if ( !N_stricmp( ext, "json" ) ) {
		lang = std::addressof( ImGui::TextEditor::LanguageDefinition::CPlusPlus() );
	} else if ( !N_stricmp( ext, "c" ) ) {
		lang = std::addressof( ImGui::TextEditor::LanguageDefinition::C() );
	} else {
		lang = std::addressof( ImGui::TextEditor::LanguageDefinition::C() ); // just assume c-style syntax
	}

	editor.SetLanguageDefinition( *lang );

	std::ifstream file( filename, std::ios::in );
	if ( file.good() ) {
		std::string str( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );
		editor.SetText( str );
	}

	for ( auto it = m_TextEditors.begin(); it != m_TextEditors.end(); it++ ) {
		if ( !it->m_bOpen ) {
			break;
		}
	}

	m_TextEditors.emplace_back( filename, editor );
	m_pCurrentEditor = m_TextEditors.end() - 1;
}

void CEditorLayer::OnUpdate( float timestep ) {
}

bool IsMouseInCurrentWindow( void )
{
	const ImVec2& mousePos = ImGui::GetIO().MousePos;
	const ImVec2 windowPos = ImGui::GetWindowPos();
	const ImVec2 windowSize = ImGui::GetWindowSize();

	if ( mousePos.x >= windowPos.x
	&& mousePos.y >= windowPos.y
	&& mousePos.x <= ( windowPos.x + windowSize.x )
	&& mousePos.y <= ( windowPos.y + windowSize.y ) )
	{
		return true;
	}
	return false;
}

void DrawFileDialogs( void )
{
	FileDialogUIRender( "SelectDiffuseMapDlg", []( const std::string& path ){ g_pAssetManagerDlg->AddTextureFile( path );
			mapData->textures[Walnut::TB_DIFFUSEMAP] = Walnut::FindImage( path.c_str() ); } );
	FileDialogUIRender( "SelectSpecularMapDlg", []( const std::string& path ){ g_pAssetManagerDlg->AddTextureFile( path );
			mapData->textures[Walnut::TB_SPECULARMAP] = Walnut::FindImage( path.c_str() ); } );
	FileDialogUIRender( "SelectAmbientOccMapDlg", []( const std::string& path ){ g_pAssetManagerDlg->AddTextureFile( path );
			mapData->textures[Walnut::TB_LIGHTMAP] = Walnut::FindImage( path.c_str() ); } );
	FileDialogUIRender( "SelectNormalMapDlg", []( const std::string& path ){ g_pAssetManagerDlg->AddTextureFile( path );
		mapData->textures[Walnut::TB_NORMALMAP] = Walnut::FindImage( path.c_str() ); } );
	FileDialogUIRender( "AddTextureFileDlg", []( const std::string& path ){ g_pAssetManagerDlg->AddTextureFile( path ); } );
	FileDialogUIRender( "LoadProjectFileDlg", []( const std::string& path ){ g_pProjectManager->SetCurrent( path, false ); } );
	FileDialogUIRender( "ImportMapFileDlg", []( const std::string& path ){ if ( IsMap( path.c_str() ) ) { Map_LoadFile( path.c_str() ); } } );
	FileDialogUIRender( "OpenMapFileDlg", []( const std::string& path ){ Map_LoadFile( path.c_str() ); } );
	FileDialogUIRender( "AddMapToProjectDlg", []( const std::string& path ){ Map_LoadFile( path.c_str() ); } );
	FileDialogUIRender( "AddShaderFileDlg", []( const std::string& path ){ g_pAssetManagerDlg->AddShaderFile( path ); } );
	FileDialogUIRender( "CompileMapFileDlg", []( const std::string& path ){ g_pMapInfoDlg->CompileMap( path ); } );
}

void CEditorLayer::OnUIRender( void )
{
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
                                                | ImGuiTreeNodeFlags_FramePadding;
    uint32_t i;
    bool open, resized;

	std::unique_lock<std::mutex> lock{ g_ImGuiLock };

	for ( auto *it : m_Dialogs ) {
		it->Draw();
	}

	DrawFileDialogs();

	if ( g_pContentBrowserDlg->ItemIsSelected() ) {
	}

	g_pMapInfoDlg->ProjectSettingsDlg();

	if ( ImGui::Begin( "Text Editor", NULL, ImGuiWindowFlags_None ) ) {
		if ( ImGui::IsWindowHovered() || ImGui::IsWindowFocused() ) {
			m_bWindowFocused = true;
			m_InputFocus = EditorInputFocus::ToolFocus;
		}

		bool empty = true;
		for ( const auto& it : m_TextEditors ) {
			if ( it.m_bOpen ) {
				empty = false;
				break;
			}
		}

		if ( empty ) {
			ImGui::TextUnformatted( "No Files Open" );
		}
		else {
			if ( ImGui::BeginTabBar( "##TextEditorSelection" ) ) {
				for ( auto it = m_TextEditors.begin(); it != m_TextEditors.end(); it++ ) {
					if ( !it->m_bOpen ) {
						continue;
					}
					if ( ImGui::BeginTabItem( it->m_Filename.filename().c_str(), &it->m_bOpen ) ) {
						m_pCurrentEditor = it;
						ImGui::EndTabItem();
					}

					if ( !it->m_bOpen ) {
						it->m_Filename.clear();
						if ( m_pCurrentEditor == it ) {
							if ( m_pCurrentEditor == m_TextEditors.begin() && m_TextEditors.size() > 1 ) {
								m_pCurrentEditor++;
							} else if ( m_pCurrentEditor == m_TextEditors.end() - 1 && m_TextEditors.size() > 1 ) {
								m_pCurrentEditor--;
							}
						}
					}
				}

				ImGui::EndTabBar();
			}

			if ( m_pCurrentEditor->m_bOpen ) {
				m_pCurrentEditor->m_Editor.Render( m_pCurrentEditor->m_Filename.filename().c_str() );
				if ( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) && ImGui::IsKeyDown( ImGuiKey_S )
					&& m_pCurrentEditor->m_Editor.IsTextChanged() )
				{
					const std::string&& data = std::move( m_pCurrentEditor->m_Editor.GetText() );
					WriteFile( m_pCurrentEditor->m_Filename.string().c_str(), data.data(), data.size() );
				}
			}
		}
		ImGui::End();
	}

	if ( g_pMapInfoDlg->m_bMapModified ) {
		m_bPreferencesOpen = true;
	}
}

static bool s_ConsoleOpen;

qboolean ConfirmModified( void )
{
	if ( !g_pMapInfoDlg->m_bMapModified ) {
		return qtrue;
	}

	const int option = Sys_MessageBox( "Map Modified", "The current map has changed since it was last saved.\nWould your like to save before continuing?",
		MB_YESNOCANCEL | MB_ICONWARNING );

	switch ( option ) {
	case IDYES:
		g_pEditor->OnFileSave();
		break;
	case IDNO:
	case IDCANCEL:
	default:
		return qfalse;
	};

	return qtrue;
}

void Valden_HandleInput( void )
{
	const ImGuiIO& io = ImGui::GetIO();

	//
	// check for new input focus
	//
	if ( Key_IsDown( KEY_MOUSE_MIDDLE ) || Key_IsDown( KEY_MOUSE_LEFT ) ) {

		const ImVec2 mousePos = ImGui::GetMousePos();

		if ( mousePos.x > g_pApplication->m_DockspaceWidth && mousePos.y > 24 ) {
			g_pEditor->m_InputFocus = EditorInputFocus::MapFocus;
		} else {
			g_pEditor->m_InputFocus = EditorInputFocus::ToolFocus;
		}
	}

	switch ( g_pEditor->m_InputFocus ) {
	case EditorInputFocus::MapFocus: {

		if ( io.MouseWheel > 0 ) { // wheel up
			g_pMapDrawer->m_nCameraZoom -= 1.0f;
		} else if ( io.MouseWheel < 0 ) { // wheel down
			g_pMapDrawer->m_nCameraZoom += 1.0f;
		}

	/*
		if ( ImGui::IsKeyDown( ImGuiKey_W ) ) {
			g_pMapDrawer->m_CameraPos.y += g_pPrefsDlg->m_nCameraMove;
		}
		if ( ImGui::IsKeyDown( ImGuiKey_A ) ) {
			g_pMapDrawer->m_CameraPos.x -= g_pPrefsDlg->m_nCameraMove;
		}
		if ( ImGui::IsKeyDown( ImGuiKey_S ) ) {
			g_pMapDrawer->m_CameraPos.y -= g_pPrefsDlg->m_nCameraMove;
		}
		if ( ImGui::IsKeyDown( ImGuiKey_D ) ) {
			g_pMapDrawer->m_CameraPos.x += g_pPrefsDlg->m_nCameraMove;
		}
	*/
		if ( ImGui::IsKeyDown( ImGuiKey_MouseMiddle ) && ImGui::IsMouseDragging( ImGuiMouseButton_Middle ) ) {
			const ImVec2 mouseDragDelta = ImGui::GetMouseDragDelta();
			g_pMapDrawer->m_CameraPos.x -= ( mouseDragDelta.x / g_pApplication->m_Specification.Width );
			g_pMapDrawer->m_CameraPos.y += ( mouseDragDelta.y / g_pApplication->m_Specification.Height );
		}

		break; }
	case EditorInputFocus::ToolFocus: {
		break; }
	default:
		break;
	};

	if ( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) ) {
		if ( ImGui::IsKeyDown( ImGuiKey_S ) ) {
			if ( ImGui::IsKeyDown( ImGuiKey_LeftAlt ) ) {
				g_pEditor->OnFileSaveAs();
			} else if ( ImGui::IsKeyDown( ImGuiKey_LeftShift ) ) {
				g_pEditor->OnFileSaveAll();
			} else {
				g_pEditor->OnFileSave();
			}
		}
		if ( ImGui::IsKeyDown( ImGuiKey_T ) ) {
			g_pMapDrawer->m_bTileSelectOn = true;
			g_pMapInfoDlg->m_bHasTileWindow = true;
		}
		if ( ImGui::IsKeyDown( ImGuiKey_C ) ) {
			if ( g_pMapDrawer->m_bTileSelectOn ) {
				g_pEditor->m_pCopyPasteData = CopyMemory( &mapData->tiles[g_pMapDrawer->m_nTileSelectY * mapData->width + g_pMapDrawer->m_nTileSelectX],
					sizeof(maptile_t) );
				Log_Printf( "Copied tile at %ix%i to editor clipboard.\n" );
			}
		}
		if ( ImGui::IsKeyDown( ImGuiKey_V ) ) {
			if ( g_pMapDrawer->m_bTileSelectOn ) {
				memcpy( &mapData->tiles[g_pMapDrawer->m_nTileSelectY * mapData->width + g_pMapDrawer->m_nTileSelectX], g_pEditor->m_pCopyPasteData,
					sizeof(maptile_t) );
				g_pMapInfoDlg->m_bMapModified = true;
				g_pMapInfoDlg->m_bMapNameUpdated = false;
			}
			free( g_pEditor->m_pCopyPasteData );
		}
		if ( ImGui::IsKeyDown( ImGuiKey_N ) ) {
			g_pEditor->OnFileNew();
		}
		if ( ImGui::IsKeyDown( ImGuiKey_M ) ) {
			g_pEditor->m_InputFocus = EditorInputFocus::MapFocus;
		}
	}
}

static void FileMenu( void )
{
	if ( ImGui::MenuItem( "New Map", "Ctrl+N" ) ) {
        g_pEditor->OnFileNew();
    }
    ImGui::Separator();
    if ( ImGui::MenuItem( "Open...", "Ctrl+O" ) ) {
        g_pEditor->OnFileOpen();
    }
    if ( ImGui::MenuItem( "Import..." ) ) {
        g_pEditor->OnFileImport();
    }
    if ( ImGui::MenuItem( "Save", "Ctrl+S" ) ) {
        g_pEditor->OnFileSave();
    }
	if ( ImGui::MenuItem( "Save All", "Ctrl+Shift+S" ) ) {
		g_pEditor->OnFileSaveAll();
	}
    if ( ImGui::MenuItem( "Save As...", "Ctrl+Alt+S" ) ) {
        g_pEditor->OnFileSaveAs();
    }
    ImGui::Separator();
    if ( ImGui::MenuItem( "New Project...", "Ctrl+Shift+N" ) ) {
		g_pProjectManager->New();
    }
    if ( ImGui::MenuItem( "Load Project...", "Ctrl+Shitf+L" ) ) {
		g_pEditor->LoadProjectFileDlg();
    }
    if ( ImGui::MenuItem( "Project Settings..." ) ) {
		g_pMapInfoDlg->m_bHasProjectWindow = true;
    }
    ImGui::Separator();
	if ( ImGui::BeginMenu( "Recent Files", g_pEditor->m_RecentFiles.size() ) ) {
		if ( !g_pEditor->m_RecentFiles.size() || !g_pEditor->m_RecentFiles.front().c_str()[0] ) {
			ImGui::MenuItem( "None" );
		}
		else {
			for ( const auto& it : g_pEditor->m_RecentFiles ) {
				if ( ImGui::MenuItem( it.c_str() ) ) {
					Map_LoadFile( it.c_str() );
				}
			}
		}
		ImGui::EndMenu();
	}
    if ( ImGui::MenuItem( "Exit" ) ) {
		if ( ::ConfirmModified() ) {
			g_pApplication->Close();
			exit( EXIT_SUCCESS );
		}
    }
}

static void EditMenu( void )
{
	if ( ImGui::MenuItem( "Undo", "Ctrl+Z" ) ) {
    }
    if ( ImGui::MenuItem( "Redo", "Ctrl+Y" ) ) {
    }
    ImGui::Separator();
    if ( ImGui::MenuItem( "Copy", "Ctrl+C" ) ) {
		if ( g_pMapDrawer->m_bTileSelectOn ) {
			g_pEditor->m_pCopyPasteData = CopyMemory( &mapData->tiles[g_pMapDrawer->m_nTileSelectY * mapData->width + g_pMapDrawer->m_nTileSelectX],
				sizeof(maptile_t) );
			Log_Printf( "Copied tile at %ix%i to editor clipboard.\n" );
		}
    }
    if ( ImGui::MenuItem( "Paste", "Ctrl+V" ) && g_pEditor->m_pCopyPasteData ) {
		if ( g_pMapDrawer->m_bTileSelectOn ) {
			memcpy( &mapData->tiles[g_pMapDrawer->m_nTileSelectY * mapData->width + g_pMapDrawer->m_nTileSelectX], g_pEditor->m_pCopyPasteData,
				sizeof(maptile_t) );
			g_pMapInfoDlg->m_bMapModified = true;
			g_pMapInfoDlg->m_bMapNameUpdated = false;
		}
		free( g_pEditor->m_pCopyPasteData );
    }
	ImGui::Separator();
	if ( ImGui::MenuItem( "Current Tile", "Ctrl-T" ) ) {
		g_pMapDrawer->m_bTileSelectOn = true;
		g_pMapInfoDlg->m_bHasTileWindow = true;
	}
    ImGui::Separator();
    if ( ImGui::MenuItem( "Preferences...", "P" ) ) {
		g_pPrefsDlg->OnActivate();
    }
}

static void ViewMenu( void )
{
	if ( ImGui::Button( "Center Camera" ) ) {
		Cmd_ExecuteText( "/centercamera\n" );
	}
    if ( ImGui::BeginMenu( "Filter" ) ) {
        ImGui::Checkbox( "Show Checkpoints", &g_pEditor->m_bFilterShowCheckpoints );
		ImGui::Checkbox( "Show Spawns", &g_pEditor->m_bFilterShowSpawns );
        ImGui::EndMenu();
    }
	ImGui::Checkbox( "Console Window", &s_ConsoleOpen );
	ImGui::Checkbox( "ImGui Metrics", &g_pEditor->m_bShowImGuiMetricsWindow );
	ImGui::Checkbox( "Map Data", &g_pMapInfoDlg->m_bShow );
}

static void DrawEditor( void )
{
	std::unique_lock<std::mutex> lock{ g_ImGuiLock };

	Valden_HandleInput();

	if ( g_pEditor->m_bPopupMenuFile ) {
		if ( ImGui::Begin( "File##FileEx", &g_pEditor->m_bPopupMenuFile, ImGuiWindowFlags_NoCollapse ) ) {
			FileMenu();
			ImGui::End();
		}
	}
	else if ( ImGui::BeginMenu( "File" ) ) {
		if ( ImGui::Button( "-------------------------" ) ) {
			g_pEditor->m_bPopupMenuFile = true;
		}
		FileMenu();
        ImGui::EndMenu();
	}

	if ( g_pEditor->m_bPopupMenuEdit ) {
		if ( ImGui::Begin( "Edit##EditEx", &g_pEditor->m_bPopupMenuEdit, ImGuiWindowFlags_NoCollapse ) ) {
			EditMenu();
			ImGui::End();
		}
	}
    else if ( ImGui::BeginMenu( "Edit" ) ) {
		if ( ImGui::Button( "-------------------------" ) ) {
			g_pEditor->m_bPopupMenuEdit = true;
		}
		EditMenu();
        ImGui::EndMenu();
    }

	if ( g_pEditor->m_bPopupMenuView ) {
		if ( ImGui::Begin( "View##ViewEx", &g_pEditor->m_bPopupMenuView, ImGuiWindowFlags_NoCollapse ) ) {
			ViewMenu();
			ImGui::End();
		}
	}
    else if ( ImGui::BeginMenu( "View" ) ) {
		if ( ImGui::Button( "-------------------------" ) ) {
			g_pEditor->m_bPopupMenuView = true;
		}
		ViewMenu();
        ImGui::EndMenu();
	}

	if ( ImGui::BeginMenu( "Build" ) ) {
		if ( ImGui::MenuItem( "Compile Map" ) ) {
			g_pMapInfoDlg->CompileMap(
				va( "%s%clevels%c%s.bmf", g_pProjectManager->GetAssetDirectory().c_str(), PATH_SEP, PATH_SEP, mapData->name )
			);
		}
		ImGui::EndMenu();
	}
	if ( ImGui::BeginMenu( "Textures" ) ) {
		ImGui::Checkbox( "Show In Use", &g_pEditor->m_bShowInUseTextures );
		ImGui::Checkbox( "Show All", &g_pEditor->m_bShowAllTextures );
		ImGui::Separator();
		ImGui::Checkbox( "Show Shaders", &g_pEditor->m_bShowShaders );
		if ( ImGui::Button( "Flush & Reload Shaders" ) ) {
			Cmd_ExecuteText( "/reloadshaders\n" );
		}
		ImGui::EndMenu();
	}
    if ( ImGui::BeginMenu( "Help" ) ) {
        ImGui::EndMenu();
    }

	if ( ImGui::IsKeyPressed( ImGuiKey_GraveAccent, false ) ) {
		s_ConsoleOpen = !s_ConsoleOpen;
	}

	if ( s_ConsoleOpen ) {
		ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.0f, 0.0f, 0.0f, 1.0f ) );
		ImGui::Begin( "Editor Debug Console", NULL, ImGuiWindowFlags_NoCollapse );
		ImGui::TextUnformatted( CMapRenderer::g_CommandConsoleString.data() );
		ImGui::End();
		ImGui::PopStyleColor();
	}
}

Walnut::Application* Walnut::CreateApplication( int argc, char **argv )
{
	Walnut::ApplicationSpecification spec;
	int channels;

//	iconImage.pixels = stbi_load( "icon.png", &iconImage.width, &iconImage.height, &channels, STBI_rgb_alpha );
//	IM_ASSERT( iconImage.pixels );

	spec.Name = "Valden";
	spec.Width = 1920;
	spec.Height = 1080;

	Cmd_Init();
	s_ConsoleOpen = false;

	// init wallnut
	g_pApplication = new Walnut::Application( spec );

	g_pMapDrawer = std::make_shared<CMapRenderer>();
	g_pMapDrawer->OnAttach();

	// initialize app
	g_pApplication->PushLayer( g_pEditor );
	InitDialogs();
	LoadRecentFiles();

	g_pApplication->SetMenubarCallback( DrawEditor );

	return g_pApplication;
}

/*
* HelpMenu: this chunky function gives a complete in-app manual, imma make it a link to web docs sometime in the future
*/
static void HelpMenu( void )
{
	const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
                                            | ImGuiTreeNodeFlags_FramePadding;

	ImGui::Begin( "Valden Documentation" );
	
	if ( ImGui::TreeNodeEx( (void *)(uintptr_t)"Texture Mapping", treeNodeFlags, "Texture Mapping" ) ) {
		ImGui::TreePop();	
	}
	if ( ImGui::TreeNodeEx( (void *)(uintptr_t)"Entities", treeNodeFlags, "Entities" ) ) {
		ImGui::TreePop();	
	}

	ImGui::End();
}
