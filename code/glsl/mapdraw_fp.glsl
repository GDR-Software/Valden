#version 450 core

layout( location = 0 ) out vec4 a_Color;

layout( location = 0 ) in vec3 v_Position;
layout( location = 1 ) in vec3 v_WorldPos;
layout( location = 2 ) in vec2 v_TexCoords;
layout( location = 3 ) in vec4 v_Color;

layout( binding = 0 ) uniform sampler2D u_DiffuseMap;

void main() {
    a_Color = texture( u_DiffuseMap, v_TexCoords );
}
