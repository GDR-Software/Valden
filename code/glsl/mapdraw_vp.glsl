#version 450 core

layout( location = 0 ) in vec3 a_Position;
layout( location = 1 ) in vec3 a_WorldPos;
layout( location = 2 ) in vec2 a_TexCoords;
layout( location = 3 ) in vec4 a_Color;

layout( location = 0 ) out vec3 v_Position;
layout( location = 1 ) out vec3 v_WorldPos;
layout( location = 2 ) out vec2 v_TexCoords;
layout( location = 3 ) out vec4 v_Color;

layout( binding = 0 ) uniform ViewProjection
{
    mat4 u_ModelViewProjection;
} u_Matrices;

void main() {
   v_Position = a_Position;
   v_WorldPos = a_WorldPos;
   v_TexCoords = a_TexCoords;
   v_Color = a_Color;
   gl_Position = vec4( u_Matrices.u_ModelViewProjection * vec4( a_Position, 1.0 ) );
}
