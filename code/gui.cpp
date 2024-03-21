#include "editor.h"
#include "gui.h"
#include "events.h"
#include "stb_image.h"
#include <thread>
#include <mutex>
#include "ImGuizmo/ImGuizmo.h"

std::shared_ptr<CMapRenderer> g_pMapDrawer;

static const char *mapdraw_vert_shader =
"#version 330 core\n"
"\n"
"layout( location = 0 ) in vec4 a_Color;\n"
"layout( location = 1 ) in vec3 a_Position;\n"
"layout( location = 2 ) in vec3 a_WorldPos;\n"
"layout( location = 3 ) in vec2 a_TexCoords;\n"
"\n"
"out vec3 v_Position;\n"
"out vec3 v_WorldPos;\n"
"out vec2 v_TexCoords;\n"
"out vec4 v_Color;\n"
"\n"
"uniform mat4 u_ModelViewProjection;\n"
"\n"
"void main() {\n"
"   v_Position = a_Position;\n"
"   v_WorldPos = a_WorldPos;\n"
"   v_TexCoords = a_TexCoords;\n"
"   v_Color = a_Color;\n"
"   gl_Position = vec4( u_ModelViewProjection * vec4( a_Position, 1.0 ) );\n"
"}\n";

static const char *mapdraw_frag_shader =
"#version 330 core\n"
"\n"
"layout( location = 0 ) out vec4 a_Color;\n"
"\n"
"in vec3 v_Position;\n"
"in vec3 v_WorldPos;\n"
"in vec2 v_TexCoords;\n"
"in vec4 v_Color;\n"
"\n"
"uniform sampler2D u_DiffuseMap;\n"
"uniform sampler2D u_SpecularMap;\n"
"uniform sampler2D u_NormalMap;\n"
"uniform sampler2D u_AmbientOcclusionMap;\n"
"\n"
"struct Light {\n"
"   vec3 position;\n"
"   vec3 ambient;\n"
"   float range;\n"
"};\n"
"#define MAX_LIGHTS 256\n"
"uniform Light u_LightData[MAX_LIGHTS];\n"
"uniform int u_NumLights;\n"
"uniform vec3 u_AmbientLightColor;\n"
"\n"
"uniform bool u_FramebufferActive;\n"
"\n"
"uniform bool u_UseDiffuseMapping;\n"
"uniform bool u_UseSpecularMapping;\n"
"uniform bool u_UseNormalMapping;\n"
"uniform bool u_UseAmbientOcclusionMapping;\n"
"uniform bool u_TileSelected;\n"
"uniform int u_TileSelectionX;\n"
"uniform int u_TileSelectionY;\n"
"\n"
"uniform vec4 u_TexUsage;\n"
"\n"
"void CalcNormal() {\n"
"   vec3 normal = texture( u_NormalMap, v_TexCoords ).rgb;\n"
"   normal = normalize( normal * 2.0 - 1.0 );\n"
"   a_Color.rgb *= normal * 0.5 + 0.5;\n"
"}\n"
"\n"
"void CalcLighting() {\n"
"   a_Color = vec4( 0.0, 0.0, 0.0, 1.0 );\n"
"   if ( u_UseNormalMapping ) {\n"
"       CalcNormal();\n"
"   }\n"
"   if ( u_UseSpecularMapping ) {\n"
"       a_Color.rgb += texture( u_SpecularMap, v_TexCoords ).rgb;\n"
"   }\n"
"   if ( u_NumLights == 0 && u_UseDiffuseMapping ) {\n"
"       a_Color.rgb += texture( u_DiffuseMap, v_TexCoords ).rgb;\n"
"   }\n"
"   for ( int i = 0; i < u_NumLights; i++ ) {\n"
"       a_Color.rgb += u_LightData[i].ambient;\n"
"\n"
"       vec3 diffuse = a_Color.rgb;\n"
"       float dist = distance( v_WorldPos, u_LightData[i].position );\n"
"       float diff = 0.0;\n"
"       if ( dist <= u_LightData[i].range ) {\n"
"           diff = 1.0 - abs( dist / u_LightData[i].range );\n"
"       }\n"
"       diffuse = max( diff * diffuse, diffuse );\n"
"       a_Color.rgb += diffuse;\n"
"   }\n"
"}\n"
"\n"
"void main() {\n"
"   if ( u_FramebufferActive ) {\n"
"       a_Color = texture( u_DiffuseMap, v_TexCoords );\n"
"   }\n"
"   else {\n"
"       if ( !u_UseDiffuseMapping && !u_UseSpecularMapping && !u_UseNormalMapping && !u_UseAmbientOcclusionMapping ) {\n"
"           a_Color = vec4( 1.0 );\n"
"       }\n"
"       else {\n"
"           CalcLighting();\n"
"       }\n"
"       a_Color.rgb *= u_AmbientLightColor;\n"
"   }\n"
"   if ( u_TileSelected && u_TileSelectionX == v_WorldPos.x && u_TileSelectionY == v_WorldPos.y ) {\n"
"       a_Color.rgb = v_Color.rgb;\n"
"   }\n"
"}\n";

void CMapRenderer::OnUpdate( float timestep ) {
    if ( g_pEditor->m_InputFocus != EditorInputFocus::MapFocus ) {
        return;
    }

    if ( m_bTileSelectOn ) {
        return;
    }

    if ( ImGui::IsKeyDown( ImGuiKey_W ) ) {
        m_CameraPos.y += g_pPrefsDlg->m_nCameraMoveSpeed;
    }
    if ( ImGui::IsKeyDown( ImGuiKey_A ) ) {
        m_CameraPos.x -= g_pPrefsDlg->m_nCameraMoveSpeed;
    }
    if ( ImGui::IsKeyDown( ImGuiKey_S ) ) {
        m_CameraPos.y -= g_pPrefsDlg->m_nCameraMoveSpeed;
    }
    if ( ImGui::IsKeyDown( ImGuiKey_D ) ) {
        m_CameraPos.x += g_pPrefsDlg->m_nCameraMoveSpeed;
    }
    if ( Key_IsDown( KEY_WHEEL_DOWN ) ) {
        m_nCameraZoom += 0.2f;
    }
    if ( Key_IsDown( KEY_WHEEL_UP ) ) {
        m_nCameraZoom -= 0.2f;
    }

    /*

    ImGuizmo::BeginFrame();
    
    ImGui::Begin( "##MapMove", NULL, ImGuiWindowFlags_NoTitleBar );
    
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const ImVec2 pos = { windowSize.x / 2, windowSize.y / 2 };

    ImGuizmo::Enable( mapData->textures[Walnut::TB_DIFFUSEMAP] );
    ImGuizmo::SetOrthographic( true );
    ImGuizmo::SetDrawlist( ImGui::GetWindowDrawList() );
    ImGuizmo::SetRect( pos.x, pos.y, windowSize.x, windowSize.y );
    ImGuizmo::DrawGrid( glm::value_ptr( m_ViewMatrix ), glm::value_ptr( m_Projection ), glm::value_ptr( m_ViewProjection ),
                1024 );
    ImGuizmo::Manipulate( glm::value_ptr( m_ViewMatrix ), glm::value_ptr( m_Projection ), ImGuizmo::OPERATION::TRANSLATE_X, ImGuizmo::MODE::WORLD,
        glm::value_ptr( m_ViewMatrix ) );
    ImGuizmo::Manipulate( glm::value_ptr( m_ViewMatrix ), glm::value_ptr( m_Projection ), ImGuizmo::OPERATION::TRANSLATE_Y, ImGuizmo::MODE::WORLD,
        glm::value_ptr( m_ViewMatrix ) );
    if ( ImGuizmo::IsUsing() ) {
        ImGuizmo::DecomposeMatrixToComponents( glm::value_ptr( m_ViewMatrix ), glm::value_ptr( m_CameraPos ), &m_nCameraRotation, &m_nCameraZoom );
    }

    ImGui::End();
    */
//    m_RenderThread = std::thread( &CMapRenderer::DrawMap, this );
}

void CMapRenderer::OnUIRender( void ) {
    DrawMap();
//    m_RenderThread.join();
}

static void WorldToGL( const glm::vec2& pos, Vertex *vertices )
{
    const glm::mat4 model = glm::translate( glm::mat4( 1.0f ), glm::vec3( pos, 0.0f ) );
    const glm::mat4 mvp = g_pMapDrawer->m_ViewProjection * model;

    const glm::vec4 positions[] = {
        {  0.5f,  0.5f, 0.0f, 1.0f },
        {  0.5f, -0.5f, 0.0f, 1.0f },
        { -0.5f, -0.5f, 0.0f, 1.0f },
        { -0.5f,  0.5f, 0.0f, 1.0f },
    };

    for ( uint32_t i = 0; i < 4; i++ ) {
        vertices[i].xyz = glm::vec3( mvp * positions[i] );
    }
}

static GLint GetUniform( const std::string& name )
{
    GLint location;

    auto it = g_pMapDrawer->m_UniformCache.find( name );
    if ( it != g_pMapDrawer->m_UniformCache.end() ) {
        if ( it->second == -1 ) { // try again
            location = glGetUniformLocation( g_pMapDrawer->m_Shader, name.c_str() );
            if ( location == -1 ) {
                return -1;
            }
            it->second = location;
            return location;
        } else {
            return it->second;
        }
    }

    location = glGetUniformLocation( g_pMapDrawer->m_Shader, name.c_str() );
    if ( location == -1 ) {
        Log_Printf( "WARNING: Failed to get uniform location of '%s'\n", name.c_str() );
    }

    g_pMapDrawer->m_UniformCache[name] = location;
    return location;
}

void CMapRenderer::DrawMap( void )
{
    uint32_t y, x;
    uint32_t i;
    float width;
    glm::vec2 pos;
    Vertex *vtx;
    const ImGuiViewport *view;

    if ( !mapData ) {
        return;
    }
//    if ( !mapData->textures[Walnut::TB_DIFFUSEMAP] || !mapData->texcoords ) {
//        return;
//    }

    m_Projection = glm::ortho( -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f );
    const glm::mat4 transpose = glm::translate( glm::mat4( 1.0f ), m_CameraPos )
                                * glm::scale( glm::mat4( 1.0f ), glm::vec3( m_nCameraZoom ) )
                                * glm::rotate( glm::mat4( 1.0f ), glm::radians( m_nCameraRotation ), glm::vec3( 0, 0, 1 ) );
    m_ViewMatrix = glm::inverse( transpose );
    m_ViewProjection = m_Projection * m_ViewMatrix;

    vtx = m_pVertices;

    view = ImGui::GetMainViewport();
    glViewport( g_pApplication->m_DockspaceWidth, 24, view->WorkSize.x - g_pApplication->m_DockspaceWidth, view->WorkSize.y );

    glBindVertexArray( m_VertexArray );
    glBindBuffer( GL_ARRAY_BUFFER, m_VertexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer );
    glUseProgram( m_Shader );

    glEnable( GL_BLEND );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_STENCIL_TEST );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

//    glBindFramebuffer( GL_FRAMEBUFFER, m_FrameBuffer );
//    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUniform1i( GetUniform( "u_UseDiffuseMapping" ), mapData->textures[Walnut::TB_DIFFUSEMAP] ? 1 : 0 );
    glUniform1i( GetUniform( "u_UseSpecularMapping" ), mapData->textures[Walnut::TB_SPECULARMAP] ? 1 : 0 );
    glUniform1i( GetUniform( "u_UseNormalMapping" ), mapData->textures[Walnut::TB_NORMALMAP] ? 1 : 0 );
    glUniform1i( GetUniform( "u_UseAmbientOcclusionMapping" ), mapData->textures[Walnut::TB_SHADOWMAP] ? 1 : 0 );

    glUniform1i( GetUniform( "u_NumLights" ), 0 );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, mapData->textures[Walnut::TB_DIFFUSEMAP] ? mapData->textures[Walnut::TB_DIFFUSEMAP]->GetID() : 0 );
    glUniform1i( GetUniform( "u_DiffuseMap" ), 0 );

    if ( mapData->textures[Walnut::TB_NORMALMAP] ) {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_2D, mapData->textures[Walnut::TB_NORMALMAP]->GetID() );
        glUniform1i( GetUniform( "u_NormalMap" ), 1 );
    }
    if ( mapData->textures[Walnut::TB_SPECULARMAP] ) {
        glActiveTexture( GL_TEXTURE2 );
        glBindTexture( GL_TEXTURE_2D, mapData->textures[Walnut::TB_SPECULARMAP]->GetID() );
        glUniform1i( GetUniform( "u_SpecularMap" ), 2 );
    }

    glUniform1i( GetUniform( "u_FramebufferActive" ), 0 );

    glUniform3fv( GetUniform( "u_AmbientLightColor" ), 1, mapData->ambientColor );

    glUniform1i( GetUniform( "u_TileSelected" ), m_bTileSelectOn );
    glUniform1i( GetUniform( "u_TileSelectionX" ), m_nTileSelectX );
    glUniform1i( GetUniform( "u_TileSelectionY" ), m_nTileSelectY );
    glUniformMatrix4fv( GetUniform( "u_ModelViewProjection" ), 1, GL_FALSE, glm::value_ptr( m_ViewProjection ) );

    for ( y = 0; y < mapData->height; y++ ) {
        for ( x = 0; x < mapData->width; x++ ) {
            pos.x = x - ( mapData->width * 0.5f );
            pos.y = mapData->height - y;

            WorldToGL( pos, vtx );

            for ( i = 0; i < 4; i++ ) {
                vtx[i].uv[0] = mapData->texcoords[mapData->tiles[y * mapData->width + x].index][i][0];
                vtx[i].uv[1] = mapData->texcoords[mapData->tiles[y * mapData->width + x].index][i][1];

                vtx[i].worldPos[0] = x;
                vtx[i].worldPos[1] = y;

                if ( m_bTileSelectOn && m_nTileSelectX == x && m_nTileSelectY == y ) {
                    vtx[i].color = { 0.0f, 1.0f, 0.0f, 1.0f };
                }
            }

            vtx += 4;
        }
    }
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(Vertex) * mapData->width * mapData->height * 4, m_pVertices );
    glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(uint32_t) * mapData->width * mapData->height * 6, m_pIndices );
    glDrawElements( GL_TRIANGLES, mapData->width * mapData->height * 6, GL_UNSIGNED_INT, NULL );

    if ( mapData->textures[Walnut::TB_NORMALMAP] ) {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }
    if ( mapData->textures[Walnut::TB_SPECULARMAP] ) {
        glActiveTexture( GL_TEXTURE2 );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, 0 );

//    glBindFramebuffer( GL_READ_FRAMEBUFFER, m_FrameBuffer );
//    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
//    glBlitFramebuffer( 0, 0, view->WorkSize.x - g_pApplication->m_DockspaceWidth, view->WorkSize.y,
//                        0, 0, view->WorkSize.x - g_pApplication->m_DockspaceWidth, view->WorkSize.y,
//                        GL_COLOR_BUFFER_BIT,
//                        GL_NEAREST );
//    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
//    glClear( GL_COLOR_BUFFER_BIT );
//    glBindTexture( GL_TEXTURE_2D, m_ColorBuffer );
//
//    m_pVertices[0].xyz = {  1.0f,  1.0f, 0.0f };
//    m_pVertices[1].xyz = {  1.0f, -1.0f, 0.0f };
//    m_pVertices[2].xyz = { -1.0f, -1.0f, 0.0f };
//    m_pVertices[3].xyz = { -1.0f,  1.0f, 0.0f };
//
//    m_pVertices[0].uv = { 0.0f, 0.0f };
//    m_pVertices[1].uv = { 1.0f, 0.0f };
//    m_pVertices[2].uv = { 1.0f, 1.0f };
//    m_pVertices[3].uv = { 0.0f, 1.0f };
//
//    glUniform1i( GetUniform( "u_FramebufferActive" ), 1 );
//    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(Vertex) * 4, m_pVertices );
//    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL );

    glUseProgram( 0 );

    glBindVertexArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    glViewport( 0, 0, view->WorkSize.x, view->WorkSize.y );
}

static void CheckShader( GLuint id, GLenum type )
{
    int success;
    char str[1024];

    glGetShaderiv( id, GL_COMPILE_STATUS, &success );
    if ( success == GL_FALSE ) {
        memset( str, 0, sizeof(str) );
        glGetShaderInfoLog( id, sizeof(str), NULL, str );
    
        Error( "CheckShader: failed to compile shader of type %s, glslang error message: %s",
            (type == GL_VERTEX_SHADER ? "vertex" : type == GL_FRAGMENT_SHADER ? "fragment" : "unknown shader type"), str );
    }
}

static GLuint GenShader( const char **sources, uint32_t numSources, GLenum type )
{
    GLuint id;

    id = glCreateShader( type );

    glShaderSource( id, numSources, sources, NULL );
    glCompileShader( id );

    return id;
}

static void ReloadShaders_f( void ) {
    Log_Printf( "Reloading shader cache...\n" );
    Walnut::InitTextures();
    Walnut::InitShaders();
}

static void CenterCamera_f( void ) {
    g_pMapDrawer->m_CameraPos = glm::vec3( 0.0f );
}

static void CheckFramebuffer( void )
{
    GLenum err = glCheckFramebufferStatus( GL_FRAMEBUFFER );

    switch ( err ) {
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        Error( "CheckFramebuffer: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT reported" );
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        Error( "CheckFramebuffer: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT reported" );
        break;
    default:
        Log_Printf( "Created framebuffer OK.\n" );
        break;
    };
}

void CMapRenderer::OnAttach( void )
{
    uint32_t i, offset;
    GLuint vertShader;
    GLuint fragShader;

    Cmd_AddCommand( "reloadshaders", ReloadShaders_f );
    Cmd_AddCommand( "centercamera", CenterCamera_f );

    Log_Printf( "[CMapRenderer::OnAttach] allocating OpenGL buffer objects...\n" );

    m_CameraPos = { 0, 0, 0 };
    m_nCameraRotation = 0.0f;
    m_nCameraZoom = 9.5f;

    m_pVertices = (Vertex *)GetMemory( sizeof(Vertex) * MAX_MAP_WIDTH * MAX_MAP_HEIGHT * 4 );
    m_pIndices = (uint32_t *)GetMemory( sizeof(uint32_t) * MAX_MAP_WIDTH * MAX_MAP_HEIGHT * 6 );

    offset = 0;
    const uint32_t maxIndices = MAX_MAP_WIDTH * MAX_MAP_HEIGHT * 6;
    for ( i = 0; i < maxIndices; i += 6 ) {
        m_pIndices[i + 0] = offset + 0;
        m_pIndices[i + 1] = offset + 1;
        m_pIndices[i + 2] = offset + 2;

        m_pIndices[i + 3] = offset + 3;
        m_pIndices[i + 4] = offset + 2;
        m_pIndices[i + 5] = offset + 0;

        offset += 4;
    }

    glGenVertexArrays( 1, &m_VertexArray );
    glGenBuffers( 1, &m_VertexBuffer );
    glGenBuffers( 1, &m_IndexBuffer );

    glBindVertexArray( m_VertexArray );

    glBindBuffer( GL_ARRAY_BUFFER, m_VertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(Vertex) * MAX_MAP_WIDTH * MAX_MAP_HEIGHT * 4, NULL, GL_DYNAMIC_DRAW );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * MAX_MAP_WIDTH * MAX_MAP_HEIGHT * 6, NULL, GL_DYNAMIC_DRAW );

    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof( Vertex, color ) );

    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof( Vertex, xyz ) );

    glEnableVertexAttribArray( 2 );
    glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof( Vertex, worldPos ) );

    glEnableVertexAttribArray( 3 );
    glVertexAttribPointer( 3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof( Vertex, uv ) );

    glBindVertexArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    vertShader = GenShader( (const char **)&mapdraw_vert_shader, 1, GL_VERTEX_SHADER );
    fragShader = GenShader( (const char **)&mapdraw_frag_shader, 1, GL_FRAGMENT_SHADER );

    CheckShader( vertShader, GL_VERTEX_SHADER );
    CheckShader( fragShader, GL_FRAGMENT_SHADER );

    m_Shader = glCreateProgram();

    glAttachShader( m_Shader, vertShader );
    glAttachShader( m_Shader, fragShader );
    glLinkProgram( m_Shader );
    glValidateProgram( m_Shader );

    glUseProgram( m_Shader );

    glDeleteShader( vertShader );
    glDeleteShader( fragShader );
    
    glUseProgram( 0 );

    glGenFramebuffers( 1, &m_FrameBuffer );
    glBindFramebuffer( GL_FRAMEBUFFER, m_FrameBuffer );

    glGenTextures( 1, &m_ColorBuffer );
    glBindTexture( GL_TEXTURE_2D, m_ColorBuffer );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, g_pApplication->m_Specification.Width, g_pApplication->m_Specification.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
    glBindTexture( GL_TEXTURE_2D, 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorBuffer, 0 );

    glGenRenderbuffers( 1, &m_DepthBuffer );
    glBindRenderbuffer( GL_RENDERBUFFER, m_DepthBuffer );
    glRenderbufferStorageMultisample( GL_RENDERBUFFER, 8, GL_DEPTH24_STENCIL8, g_pApplication->m_Specification.Width, g_pApplication->m_Specification.Height );
    glBindRenderbuffer( GL_RENDERBUFFER, 0 );
    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer );

    CheckFramebuffer();

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void CMapRenderer::OnDetach( void )
{
    FreeMemory( m_pVertices );
    FreeMemory( m_pIndices );

    glDeleteBuffers( 1, &m_VertexBuffer );
    glDeleteBuffers( 1, &m_IndexBuffer );
    glDeleteVertexArrays( 1, &m_VertexArray );
    glDeleteProgram( m_Shader );
}

void CMapRenderer::Print( const char *fmt, ... )
{
    va_list argptr;
    char msg[1024];
    int length;

    va_start( argptr, fmt );
    length = vsnprintf( msg, sizeof(msg), fmt, argptr );
    va_end( argptr );

    CMapRenderer::g_CommandConsoleString.insert( CMapRenderer::g_CommandConsoleString.end(), msg, msg + length );
}

/*

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

extern VkAllocationCallbacks* g_Allocator;
extern VkInstance               g_Instance;
extern uint32_t                 g_QueueFamily;
extern VkPhysicalDevice         g_PhysicalDevice;
extern VkDevice                 g_Device;
extern VkQueue                  g_Queue;
extern VkDebugReportCallbackEXT g_DebugReport;
extern VkPipelineCache          g_PipelineCache;
extern VkDescriptorPool         g_DescriptorPool;
extern VkQueueFamilyProperties* g_pFamilyQueues;
static uint32_t                 g_Indices = -1;
extern VkSurfaceKHR			    g_Surface;

#define FRAME_COUNT 60

#include "glsl/mapdraw_frag.h"
#include "glsl/mapdraw_vert.h"

void Window::Print( const char *fmt, ... )
{
    va_list argptr;
    char msg[1024];

    va_start( argptr, fmt );
    vsnprintf( msg, sizeof(msg), fmt, argptr );
    va_end( argptr );

    g_pMapDrawer->m_CmdLine.emplace_back( msg );
}

void Window::OnUIRender( void )
{
    return;
    {
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = 0;
        info.pInheritanceInfo = NULL;

        if ( vkBeginCommandBuffer( m_CommandBuffer, &info ) != VK_SUCCESS ) {
            Error( "Failed to begin recording command buffer" );
        }
    }
    {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_RenderPass;
    }
}

void Window::OnUpdate( float timestep ) {
}

SwapChainSupportDetails& querySwapChainSupport(VkPhysicalDevice device) {
    static SwapChainSupportDetails details;
    uint32_t formatCount;
    uint32_t presentModeCount;
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( g_PhysicalDevice, g_Surface, &details.capabilities );

    vkGetPhysicalDeviceSurfaceFormatsKHR( g_PhysicalDevice, g_Surface, &formatCount, NULL );
    if ( formatCount != 0 ) {
        details.formats.resize( formatCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( g_PhysicalDevice, g_Surface, &formatCount, details.formats.data() );
    }
    
    vkGetPhysicalDeviceSurfacePresentModesKHR( g_PhysicalDevice, g_Surface, &presentModeCount, nullptr );
    if ( presentModeCount != 0 ) {
        details.presentModes.resize( presentModeCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR( g_PhysicalDevice, g_Surface, &presentModeCount, details.presentModes.data() );
    }
    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for ( const auto& availableFormat : availableFormats ) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

static uint32_t GetVkMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties )
{
    VkPhysicalDeviceMemoryProperties memProperties;

    vkGetPhysicalDeviceMemoryProperties( g_PhysicalDevice, &memProperties );

    for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ ) {
        if ( typeFilter & ( 1 << i ) && ( memProperties.memoryTypes[i].propertyFlags & properties ) == properties ) {
            return i;
        }
    }

    Error( "Failed to find suitable Vulkan memory type" );
}

static void InitBuffer( VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory )
{
    VkMemoryAllocateInfo allocInfo;
    VkBufferCreateInfo bufferInfo;
    VkMemoryRequirements memRequirements;

    memset( &bufferInfo, 0, sizeof(bufferInfo) );
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if ( vkCreateBuffer( g_Device, &bufferInfo, g_Allocator, buffer ) != VK_SUCCESS ) {
        Error( "Failed to create map draw buffer" );
    }

    memset( &memRequirements, 0, sizeof(memRequirements) );
    vkGetBufferMemoryRequirements( g_Device, *buffer, &memRequirements );

    memset( &allocInfo, 0, sizeof(allocInfo) );
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = GetVkMemoryType( memRequirements.memoryTypeBits, properties );

    if ( vkAllocateMemory( g_Device, &allocInfo, g_Allocator, bufferMemory ) != VK_SUCCESS ) {
        Error( "Failed to allocate sufficient memory to map draw buffer" );
    }

    vkBindBufferMemory( g_Device, *buffer, *bufferMemory, 0 );
}

void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *imageMemory)
{
    VkImageCreateInfo imageInfo;
    VkMemoryRequirements memRequirements;
    VkMemoryAllocateInfo allocInfo;

    memset( &imageInfo, 0, sizeof(imageInfo) );
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if ( vkCreateImage( g_Device, &imageInfo, g_Allocator, image ) != VK_SUCCESS ) {
        Error( "Failed to create vulkan image" );
    }
    
    vkGetImageMemoryRequirements( g_Device, *image, &memRequirements );
    
    memset( &allocInfo, 0, sizeof(allocInfo) );
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = GetVkMemoryType( memRequirements.memoryTypeBits, properties );
    
    if ( vkAllocateMemory( g_Device, &allocInfo, g_Allocator, imageMemory ) != VK_SUCCESS ) {
        Error( "failed to allocate image memory" );
    }

    vkBindImageMemory( g_Device, *image, *imageMemory, 0 );
}

texture_t *CreateTexture( const char *filename )
{
    texture_t *tex;
    byte *buffer;
    uint64_t bufLen;
    int width, height;
    int channels;
    void *imageData;
    VkBuffer imageBuffer;
    VkDeviceMemory imageBufferMemory;

    Log_Printf( "Loading texture file '%s'.\n" );

    if ( strlen( filename ) >= MAX_NPATH ) {
        Log_Printf( "WARNING: texture file name '%s' is too long.\n", filename );
        return NULL;
    }

    buffer = stbi_load( filename, &width, &height, &channels, STBI_rgb_alpha );
    if ( !buffer ) {
        Log_Printf( "WARNING: failed to load texture file '%s', stbi_error: %s\n", filename, stbi_failure_reason() );
        return NULL;
    }

    tex = (texture_t *)GetMemory( sizeof(*tex) );

    tex->width = width;
    tex->height = height;

    bufLen = width * height * channels;

    InitBuffer( bufLen, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &imageBuffer,  &imageBufferMemory );

    vkMapMemory( g_Device, imageBufferMemory, 0, bufLen, 0, &imageData );
    memcpy( imageData, buffer, bufLen );
    vkUnmapMemory( g_Device, imageBufferMemory );

    free( buffer );

    vkDestroyBuffer( g_Device, imageBuffer, g_Allocator );
    vkFreeMemory( g_Device, imageBufferMemory, g_Allocator );
}

static void InitUniformBuffer( VkShaderStageFlagBits shaderStage, VkDescriptorSetLayout *descriptorLayout )
{
    VkDescriptorSetLayoutBinding layoutBinding;
    VkDescriptorSetLayoutCreateInfo createInfo;
    VkPipelineLayoutCreateInfo pipelineInfo;

    memset( &layoutBinding, 0, sizeof(layoutBinding) );
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = shaderStage;
    layoutBinding.pImmutableSamplers = NULL;

    memset( &createInfo, 0, sizeof(createInfo) );
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = 1;
    createInfo.pBindings = &layoutBinding;

    if ( vkCreateDescriptorSetLayout( g_Device, &createInfo, g_Allocator, descriptorLayout ) != VK_SUCCESS ) {
        Error( "Failed to create a map draw uniform buffer (descriptor set layout)" );
    }

    memset( &pipelineInfo, 0, sizeof(pipelineInfo) );
    pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineInfo.setLayoutCount = 1;
    pipelineInfo.pSetLayouts = descriptorLayout;
}

VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for ( VkFormat format : candidates ) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties( g_PhysicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    Error( "Failed to find supported format" );
}

VkFormat findDepthFormat( void ) {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

static void CreateShader( VkShaderStageFlagBits stage, VkShaderModule *shaderModule, uint32_t *code, uint32_t size, const char *name )
{
    VkShaderModuleCreateInfo createInfo;
    VkPipelineShaderStageCreateInfo shaderStageInfo;

    memset( &createInfo, 0, sizeof(createInfo) );
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = code;

    if ( vkCreateShaderModule( g_Device, &createInfo, g_Allocator, shaderModule ) != VK_SUCCESS ) {
        Error( "Failed to create shader '%s'", name );
    }

    memset( &shaderStageInfo, 0, sizeof(shaderStageInfo) );
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageInfo.module = *shaderModule;
    shaderStageInfo.pName = name;
}

static void InitShaders( void )
{
    VkShaderModuleCreateInfo vertShaderCreateInfo, fragShaderCreateInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineShaderStageCreateInfo vertShaderStageInfo, fragShaderStageInfo;
    VkVertexInputBindingDescription bindingDescription;
    VkVertexInputAttributeDescription attributeDescriptions[4];

    memset( &vertShaderCreateInfo, 0, sizeof(vertShaderCreateInfo) );
    vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertShaderCreateInfo.codeSize = sizeof(mapdraw_vert_spv);
    vertShaderCreateInfo.pCode = mapdraw_vert_spv;

    memset( &fragShaderCreateInfo, 0, sizeof(fragShaderCreateInfo) );
    fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragShaderCreateInfo.codeSize = sizeof(mapdraw_vert_spv);
    fragShaderCreateInfo.pCode = mapdraw_vert_spv;

    if ( vkCreateShaderModule( g_Device, &vertShaderCreateInfo, g_Allocator, &g_pMapDrawer->m_VertShader ) != VK_SUCCESS ) {
        Error( "Failed to create vertex map draw shader" );
    }

    if ( vkCreateShaderModule( g_Device, &fragShaderCreateInfo, g_Allocator, &g_pMapDrawer->m_FragShader ) != VK_SUCCESS ) {
        Error( "Failed to create fragment map draw shader" );
    }

    memset( &vertShaderStageInfo, 0, sizeof(vertShaderStageInfo) );
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.module = g_pMapDrawer->m_VertShader;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    vertShaderStageInfo.pName = "map draw vertex shader";

    memset( &fragShaderStageInfo, 0, sizeof(fragShaderStageInfo) );
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.module = g_pMapDrawer->m_FragShader;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    fragShaderStageInfo.pName = "map draw fragment shader";

    const VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    memset( &bindingDescription, 0, sizeof(bindingDescription) );
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, color);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, xyz);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, worldPos);

//    attributeDescriptions[3].binding = 0;
//    attributeDescriptions[3].location = 2;
//    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
//    attributeDescriptions[3].offset = offsetof(Vertex, normal);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(Vertex, uv);

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = (sizeof(attributeDescriptions)/sizeof(*attributeDescriptions)); // FIXME: make into the arraylen() macro
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    memset( &inputAssembly, 0, sizeof(inputAssembly) ) ;
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    memset( &viewportState, 0, sizeof(viewportState) );
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    memset( &rasterizer, 0, sizeof(rasterizer) );
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

//    VkPipelineMultisampleStateCreateInfo multisampling{};
//    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//    multisampling.sampleShadingEnable = VK_FALSE;
//    multisampling.rasterizationSamples = msaaSamples;

//    VkPipelineDepthStencilStateCreateInfo depthStencil{};
//    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//    depthStencil.depthTestEnable = VK_TRUE;
//    depthStencil.depthWriteEnable = VK_TRUE;
//    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
//    depthStencil.depthBoundsTestEnable = VK_FALSE;
//    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    const std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    if ( vkCreatePipelineLayout( g_Device, &pipelineLayoutInfo, g_Allocator, &pipelineLayout ) != VK_SUCCESS ) {
        Error( "Failed to create pipeline layout" );
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
//    pipelineInfo.pMultisampleState = &multisampling;
//    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if ( vkCreateGraphicsPipelines( g_Device, VK_NULL_HANDLE, 1, &pipelineInfo, g_Allocator, &graphicsPipeline ) != VK_SUCCESS) {
        Error( "Failed to create graphics pipeline" );
    }

    vkDestroyShaderModule( g_Device, g_pMapDrawer->m_FragShader, g_Allocator );
    vkDestroyShaderModule( g_Device, g_pMapDrawer->m_VertShader, g_Allocator );
}

void Window::OnAttach( void )
{
    VkCommandPoolCreateInfo poolInfo;
    VkCommandBufferAllocateInfo allocInfo;
    VkRenderPassCreateInfo renderPassInfo;
    VkDeviceSize bufferSize;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkFormat swapChainImageFormat;
    const VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport( g_PhysicalDevice );
    swapChainImageFormat = VK_FORMAT_R8G8B8A8_SRGB;

    InitShaders();

    //
    // init uniform buffers (descriptor set layouts)
    //
    InitUniformBuffer( VK_SHADER_STAGE_VERTEX_BIT, &m_MatrixDescriptorSet );
    InitUniformBuffer( VK_SHADER_STAGE_FRAGMENT_BIT, &m_TextureUnitsDescriptorSet );

    m_MatrixUBO.resize( FRAME_COUNT );

    for ( uint32_t i = 0; i < FRAME_COUNT; i++ ) {
        InitBuffer( sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &m_MatrixUBO[i].buffer, &m_MatrixUBO[i].memory );
            
        vkMapMemory( g_Device, m_MatrixUBO[i].memory, 0, sizeof(glm::mat4), 0, &m_MatrixUBO[i].data );
    }

    //
    // init render pass
    //
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    memset( &renderPassInfo, 0, sizeof(renderPassInfo) );
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if ( vkCreateRenderPass( g_Device, &renderPassInfo, g_Allocator, &m_RenderPass ) != VK_SUCCESS ) {
        Error( "Failed to create map draw render pass" );
    }

    //
    // init command pool
    //
    memset( &poolInfo, 0, sizeof(poolInfo) );
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = g_QueueFamily;

    if ( vkCreateCommandPool( g_Device, &poolInfo, g_Allocator, &m_CommandPool ) != VK_SUCCESS ) {
        Error( "Failed to create map draw command pool" );
    }

    memset( &allocInfo, 0, sizeof(allocInfo) );
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if ( vkAllocateCommandBuffers( g_Device, &allocInfo, &m_CommandBuffer ) != VK_SUCCESS ) {
        Error( "Failed to allocate map draw command buffers" );
    }

    //
    // init vertex buffer
    //
    InitBuffer( sizeof(Vertex) * FRAME_VERTICES, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &m_VertexBuffer, &m_VertexBufferMemory );

    //
    // init index buffer
    //
    InitBuffer( sizeof(uint32_t) * FRAME_INDICES, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &m_IndexBuffer, &m_IndexBufferMemory );
    
    // map the memory to cpu space
    vkMapMemory( g_Device, m_VertexBufferMemory, 0, sizeof(*m_pVertices) * FRAME_VERTICES, 0, (void **)&m_pVertices );
    vkMapMemory( g_Device, m_IndexBufferMemory, 0, sizeof(uint32_t) * FRAME_INDICES, 0, (void **)&m_pIndices );
}

void Window::OnDetach( void )
{
    vkDestroyCommandPool( g_Device, m_CommandPool, g_Allocator );

    vkDestroyRenderPass( g_Device, m_RenderPass, g_Allocator );

    for ( uint32_t i = 0; i < FRAME_COUNT; i++ ) {
        vkDestroyBuffer( g_Device, m_MatrixUBO[i].buffer, g_Allocator );
        vkFreeMemory( g_Device, m_MatrixUBO[i].memory, g_Allocator );
    }

    vkDestroyDescriptorSetLayout( g_Device, m_MatrixDescriptorSet, g_Allocator );
    vkDestroyDescriptorSetLayout( g_Device, m_TextureUnitsDescriptorSet, g_Allocator );

    vkUnmapMemory( g_Device, m_VertexBufferMemory );
    vkUnmapMemory( g_Device, m_IndexBufferMemory );

    vkDestroyBuffer( g_Device, m_VertexBuffer, g_Allocator );
    vkDestroyBuffer( g_Device, m_IndexBuffer, g_Allocator );

    vkFreeMemory( g_Device, m_VertexBufferMemory, g_Allocator );
    vkFreeMemory( g_Device, m_IndexBufferMemory, g_Allocator );
}

*/