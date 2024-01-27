#ifndef _GUI_H_
#define _GUI_H_

#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>

#include "glad/gl.h"

#define FRAME_QUADS 256
#define FRAME_VERTICES (FRAME_QUADS*4)
#define FRAME_INDICES (FRAME_QUADS*6)

struct Vertex
{
    glm::vec4 color;
    glm::vec3 xyz;
    glm::vec3 worldPos;
    glm::vec2 uv;
};

class CMapRenderer : public Walnut::Layer
{
public:
    CMapRenderer( void ) = default;
    ~CMapRenderer() = default;

    static void Print( const char *fmt, ... );

    virtual void OnUIRender( void ) override;
    virtual void OnUpdate( float timestep ) override;
    virtual void OnAttach( void ) override;
    virtual void OnDetach( void ) override;

    void DrawMap( void );

    std::unordered_map<std::string, GLint> m_UniformCache;
    std::thread m_RenderThread;
    std::mutex m_RenderLock;

    void *m_pIconBuf;

    Vertex *m_pVertices;
    uint32_t *m_pIndices;

    int m_nTileSelectX;
    int m_nTileSelectY;
    bool m_bTileSelectOn;

    char m_szInputBuf[4096];

    uint32_t m_nWindowWidth;
    uint32_t m_nWindowHeight;

    float m_nCameraZoom;
    float m_nCameraZoomInverse;
    float m_nCameraRotation;
    glm::vec3 m_CameraPos;

    glm::mat4 m_ViewProjection;
    glm::mat4 m_Projection;
    glm::mat4 m_ViewMatrix;

    GLuint m_Shader;
    GLuint m_GridTexture;
    GLuint m_VertexArray;
    GLuint m_VertexBuffer;
    GLuint m_IndexBuffer;

    GLuint m_FrameBuffer;
    GLuint m_NormalBuffer;
    GLuint m_SpecularBuffer;
    GLuint m_PositionBuffer;
    GLuint m_ColorBuffer;
    GLuint m_DepthBuffer;

    static inline std::vector<char> g_CommandConsoleString;
};

extern std::shared_ptr<CMapRenderer> g_pMapDrawer;

inline glm::ivec4 RGBAToNormal( const glm::vec4& color )
{
    return glm::ivec4((
        ((uint32_t)(color.a)<<IM_COL32_A_SHIFT) |
        ((uint32_t)(color.b)<<IM_COL32_B_SHIFT) |
        ((uint32_t)(color.g)<<IM_COL32_G_SHIFT) |
        ((uint32_t)(color.a)<<IM_COL32_R_SHIFT))
    );
}

inline glm::vec4 NormalToRGBA( const glm::ivec4& color )
{
    const uint32_t rgba =
        color[0] | (color[1] << 8) | (color[2] << 16) | (color[3] << 24);
    return glm::vec4(
        (float)((rgba >> IM_COL32_R_SHIFT) & 0xFF) * (1.0f / 255.0f),
        (float)((rgba >> IM_COL32_G_SHIFT) & 0xFF) * (1.0f / 255.0f),
        (float)((rgba >> IM_COL32_B_SHIFT) & 0xFF) * (1.0f / 255.0f),
        (float)((rgba >> IM_COL32_A_SHIFT) & 0xFF) * (1.0f / 255.0f)
    );
}

#endif