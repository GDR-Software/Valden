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
		} else if ( !N_stricmp( COM_GetExtension( it.path().filename().c_str() ), "shader" ) && !it.is_directory() ) {
			shaderList.emplace_back( it.path() );
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

void CAssetManagerDlg::OnUIRender( void )
{
    static float padding = 16.0f;
	static float thumbnailSize = 128.0f;
	const float cellSize = thumbnailSize + padding;
	const float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount;
	const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
                                                | ImGuiTreeNodeFlags_FramePadding;

    if ( ImGui::Begin( "Shader Manager" ) ) {
		if ( ImGui::Button( "Reload Shader List") ) {
			m_ShaderList.clear();
			Log_Printf( "[CAssetManagerDlg:OnUIRender] Reloading shader file list...\n" );
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
			if ( ImGui::MenuItem( it.filename().c_str() ) ) {
				g_pEditor->OnTextEdit( it );
			}
		}

        ImGui::End();
    }
}
