#include "editor.h"

CAssetManagerDlg::CAssetManagerDlg( void )
{
	memset( m_szNewShaderName, 0, sizeof(m_szNewShaderName) );
}

CAssetManagerDlg::~CAssetManagerDlg()
{
}

static void FindShaderFiles( const std::filesystem::path& currentPath, std::vector<std::filesystem::path>& shaderList )
{
	for ( const auto& it : std::filesystem::directory_iterator{ currentPath } ) {
		if ( it.is_directory() ) {
			FindShaderFiles( it, shaderList );
		} else if ( !N_stricmp( COM_GetExtension( it.path().string().c_str() ), "shader" ) && !it.is_directory() ) {
			shaderList.emplace_back( it.path().string().c_str() );
			Log_Printf( "added shader file '%s' to cache\n", it.path().string().c_str() );
		}
	}
}

static void FindTextureFiles( const std::filesystem::path& currentPath, std::vector<std::filesystem::path>& textureList )
{
	for ( const auto& it : std::filesystem::directory_iterator{ currentPath } ) {
		if ( it.is_directory() ) {
			FindTextureFiles( it, textureList );
		} else {
			const std::string path = it.path().filename().string();
			const char *ext = COM_GetExtension( path.c_str() );
			if ( !N_stricmp( ext, "png" ) || !N_stricmp( ext, "jpeg" ) || !N_stricmp( ext, "jpg" ) || !N_stricmp( ext, "bmp" )
				|| !N_stricmp( ext, "tga" ) )
			{
				textureList.emplace_back( path );
			}
		}
	}
}

void CAssetManagerDlg::OnAttach( void )
{
	m_CurrentDirectory = g_pProjectManager->GetProject()->m_AssetDirectory;
	m_BaseDirectory = m_CurrentDirectory;

	m_pDirectoryIcon = new Walnut::Image( va( "%sbitmaps/DirectoryIcon.png", g_pEditor->m_CurrentPath.c_str() ) );
    m_pFileIcon = new Walnut::Image( va( "%sbitmaps/FileIcon.png", g_pEditor->m_CurrentPath.c_str() ) );

	FindShaderFiles( m_BaseDirectory, m_ShaderList );
	FindTextureFiles( m_BaseDirectory, m_TextureList );
}

void CAssetManagerDlg::OnDetach( void )
{
	delete m_pDirectoryIcon;
	delete m_pFileIcon;
}

static void CreateShader( const char *name )
{
	FileStream file;
	const char *path;

	Log_Printf( "Creating new shader file '%s'...\n", name );

	path = va( "%s%cshaders%c%s.shader", g_pProjectManager->GetProject()->m_AssetDirectory, PATH_SEP, path, PATH_SEP );
	if ( !file.Open( path, "w" ) ) {
		Error( "CreateShader: failed to create shader file '%s'", path );
	}

	Log_Printf( "Finished.\n" );
}

void CAssetManagerDlg::AddShaderFile( const std::string& shaderFile )
{
	const char *path;

	for ( const auto& it : m_ShaderList ) {
		if ( shaderFile == it ) {
			Log_Printf( "[CAssetManagerDlg::AddShaderFile] shader file '%s' already in cache.\n", shaderFile.c_str() );
			return;
		}
	}

	path = va( "%s%cshaders%c%s", g_pProjectManager->GetProject()->m_AssetDirectory.string().c_str(), PATH_SEP, PATH_SEP,
		strrchr( shaderFile.c_str(), PATH_SEP ) + 1 );

	Log_Printf( "[CAssetManagerDlg::AddShaderFile] creating symlink '%s'\n", path );
	
	try {
		std::filesystem::create_symlink( shaderFile, path );
	} catch ( const std::exception& e ) {
		Log_Printf( "WARNING: exception thrown -- %s\n", e.what() );
	}

	Log_Printf( "added shader file '%s'\n", shaderFile.c_str() );
	m_ShaderList.emplace_back( shaderFile );
}


void CAssetManagerDlg::AddTextureFile( const std::string& textureFile )
{
	const char *path;

	for ( const auto& it : m_TextureList ) {
		if ( textureFile == it ) {
			Log_Printf( "[CAssetManagerDlg::AddTextureFile] texture file '%s' already in cache.\n", textureFile.c_str() );
			return;
		}
	}

	path = va( "%s%ctextures%c%s", g_pProjectManager->GetProject()->m_AssetDirectory.string().c_str(), PATH_SEP, PATH_SEP,
		strrchr( textureFile.c_str(), PATH_SEP ) + 1 );

	Log_Printf( "[CAssetManagerDlg::AddTextureFile] creating symlink '%s'\n", path );
	
	try {
		std::filesystem::create_symlink( textureFile, path );
	} catch ( const std::exception& e ) {
		Log_Printf( "WARNING: exception thrown -- %s\n", e.what() );
	}

	m_TextureList.emplace_back( textureFile );
}

void CAssetManagerDlg::OnUIRender( void )
{
    static float padding = 16.0f;
	static float thumbnailSize = 128.0f;
	const float cellSize = thumbnailSize + padding;
	const float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount;
	const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
                                                | ImGuiTreeNodeFlags_FramePadding;

    if ( ImGui::Begin( "Asset Manager" ) ) {
		if ( ImGui::Button( "Reload Shader List") ) {
			m_ShaderList.clear();
			Log_Printf( "[CAssetManagerDlg::OnUIRender] Reloading shader file list...\n" );
			FindShaderFiles( m_BaseDirectory, m_ShaderList );
		}
		if ( ImGui::TreeNodeEx( (void *)(uintptr_t)"NewShader", treeNodeFlags, "New Shader" ) ) {
			ImGui::TextUnformatted( "Name: " );
			ImGui::InputText( "##NewShaderName", m_szNewShaderName, sizeof(m_szNewShaderName) - ( sizeof(".shader") - 1 ),
				ImGuiInputTextFlags_EnterReturnsTrue );
			
			if ( !m_szNewShaderName[0] ) {
				ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.75, 0.75, 0.75, 1.0f ) );
				ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.75, 0.75, 0.75, 1.0f ) );
				ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.75, 0.75, 0.75, 1.0f ) );
			}
			if ( ImGui::Button( "Create" ) && m_szNewShaderName[0] ) {
				CreateShader( m_szNewShaderName );
			}
			if ( !m_szNewShaderName[0] ) {
				ImGui::PopStyleColor( 3 );
			}
			ImGui::TreePop();
		}
		if ( ImGui::Button( "Add Shader File" ) ) {
			ImGuiFileDialog::Instance()->OpenDialog( "AddShaderFileDlg", "Open Shader File", "Shader Files (*.shader){.shader}",
				g_pEditor->m_CurrentPath );
		}
		ImGui::SeparatorText( "Shader Files" );
		for ( auto& it : m_ShaderList ) {
			if ( ImGui::MenuItem( strrchr( it.c_str(), PATH_SEP ) ? strrchr( it.c_str(), PATH_SEP ) + 1 : it.c_str() ) ) {
				g_pEditor->OnTextEdit( it );
			}
		}

		ImGui::SeparatorText( "Texture Files" );
		// show all textures in Assets/textures
		if ( g_pEditor->m_bShowAllTextures ) {
			for ( auto& it : m_TextureList ) {
				ImGui::MenuItem( strrchr( it.c_str(), PATH_SEP ) ? strrchr( it.c_str(), PATH_SEP ) + 1 : it.c_str() );
			}
		}
		// only show active textures
		else {
			if ( mapData->textures[Walnut::TB_DIFFUSEMAP] ) {
				const char *name = mapData->textures[Walnut::TB_DIFFUSEMAP]->GetName().c_str();
				ImGui::MenuItem( strrchr( name, PATH_SEP ) ? strrchr( name, PATH_SEP ) + 1 : name );
			}
			if ( mapData->textures[Walnut::TB_SPECULARMAP] ) {
				const char *name = mapData->textures[Walnut::TB_SPECULARMAP]->GetName().c_str();
				ImGui::MenuItem( strrchr( name, PATH_SEP ) ? strrchr( name, PATH_SEP ) + 1 : name );
			}
			if ( mapData->textures[Walnut::TB_NORMALMAP] ) {
				const char *name = mapData->textures[Walnut::TB_NORMALMAP]->GetName().c_str();
				ImGui::MenuItem( strrchr( name, PATH_SEP ) ? strrchr( name, PATH_SEP ) + 1 : name );
			}
			if ( mapData->textures[Walnut::TB_SHADOWMAP] ) {
				const char *name = mapData->textures[Walnut::TB_SHADOWMAP]->GetName().c_str();
				ImGui::MenuItem( strrchr( name, PATH_SEP ) ? strrchr( name, PATH_SEP ) + 1 : name );
			}
			if ( mapData->textures[Walnut::TB_LIGHTMAP] ) {
				const char *name = mapData->textures[Walnut::TB_LIGHTMAP]->GetName().c_str();
				ImGui::MenuItem( strrchr( name, PATH_SEP ) ? strrchr( name, PATH_SEP ) + 1 : name );
			}
		}

		if ( ImGui::Button( "Add Texture File" ) ) {
			ImGuiFileDialog::Instance()->OpenDialog( "AddTextureFileDlg", "Open Texture File",
				".jpg,.jpeg,.png,.bmp,.tga,.webp,"
				"Jpeg Files (*.jpeg *.jpg){.jpeg,.jpg},"
				"Image Files (*.jpg *.jpeg *.png *.bmp *.tga *.webp){.jpg,.jpeg,.png,.bmp,.tga,.webp}",
				g_pEditor->m_CurrentPath );
		}

        ImGui::End();
    }
}
