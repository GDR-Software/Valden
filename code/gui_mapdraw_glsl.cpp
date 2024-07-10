
const char *fallbackShader_mapdraw_fp =
"#version 450 core\n"
"\n"
"#define POINT_LIGHT 0\n"
"#define DIRECTION_LIGHT 1\n"
"#define MAX_LIGHTS 1024\n"
"\n"
"layout( location = 0 ) out vec4 a_Color;\n"
"\n"
"in vec3 v_Position;\n"
"in vec3 v_WorldPos;\n"
"in vec2 v_TexCoords;\n"
"in vec4 v_Color;\n"
"in vec3 v_FragPos;\n"
"\n"
"uniform sampler2D u_DiffuseMap;\n"
"uniform sampler2D u_SpecularMap;\n"
"uniform sampler2D u_NormalMap;\n"
"uniform sampler2D u_AmbientOcclusionMap;\n"
"uniform sampler2D u_PointLightTexture;\n"
"uniform vec2 u_PointLightTexCoords;\n"
"\n"
"struct Light {\n"
"    vec4 color;\n"
"    uvec2 origin;\n"
"    float brightness;\n"
"    float range;\n"
"    float linear;\n"
"    float quadratic;\n"
"    float constant;\n"
"    int type;\n"
"};\n"
"layout( std140 ) uniform LightData {\n"
"    Light lights[MAX_LIGHTS];\n"
"};\n"
"uniform int u_NumLights;\n"
"uniform vec3 u_AmbientLightColor;\n"
"\n"
"uniform bool u_FramebufferActive;\n"
"uniform float u_CameraZoom;\n"
"uniform vec3 u_CameraPos;\n"
"\n"
"uniform bool u_UseDiffuseMapping;\n"
"uniform bool u_UseSpecularMapping;\n"
"uniform bool u_UseNormalMapping;\n"
"uniform bool u_UseAmbientOcclusionMapping;\n"
"uniform bool u_TileSelected;\n"
"uniform int u_TileSelectionX;\n"
"uniform int u_TileSelectionY;\n"
"uniform bool u_FilterSpawns;\n"
"uniform bool u_FilterCheckpoints;\n"
"\n"
"uniform vec4 u_TexUsage;\n"
"\n"
"vec3 CalcPointLight( Light light ) {\n"
"    vec3 diffuse = a_Color.rgb;\n"
"    float dist = distance( v_WorldPos, vec3( light.origin, v_WorldPos.z ) );\n"
"    float diff = 0.0;\n"
"    float range = light.range;\n"
"    if ( dist <= light.range ) {\n"
"        diff = 1.0 - abs( dist / range );\n"
"    }\n"
"    diff += light.brightness;\n"
"    diffuse = min( diff * ( diffuse + vec3( light.color ) ), diffuse );\n"
"\n"
"    vec3 lightDir = vec3( 0.0 );\n"
"//    vec3 lightDir = normalize( vec3( light.origin, 0.0 ) );\n"
"//    vec3 lightDir = normalize( vec3( light.origin, 0.0 ) - v_FragPos );\n"
"    vec3 viewDir = normalize( v_WorldPos - vec3( light.origin, 0.0 ) );\n"
"    vec3 halfwayDir = normalize( lightDir + viewDir );\n"
"\n"
"    vec3 reflectDir = reflect( -lightDir, v_WorldPos );\n"
"    float spec = pow( max( dot( v_WorldPos, reflectDir ), 0.0 ), 1.0 );\n"
"//    float spec = pow( max( dot( v_WorldPos, halfwayDir ), 0.0 ), 1.0 );\n"
"\n"
"    vec3 specular = vec3( 0.0 );\n"
"    if ( u_UseSpecularMapping ) {\n"
"        specular = spec * texture( u_SpecularMap, v_TexCoords ).rgb;\n"
"    }\n"
"\n"
"    range = light.range + light.brightness;\n"
"    float attenuation = ( light.constant + light.linear * range\n"
"        + light.quadratic * ( range * range ) );\n"
"\n"
"    diffuse *= attenuation;\n"
"    specular *= attenuation;\n"
"\n"
"    return diffuse + specular;\n"
"#if 0\n"
"    vec3 lightDir = normalize( vec3( light.origin, 0.0 ) - v_FragPos );\n"
"//    vec3 lightDir = normalize( vec3( light.origin, 0.0 ) - v_FragPos );\n"
"    vec3 viewDir = normalize( u_CameraPos - vec3( light.origin, 0.0 ) );\n"
"    vec3 halfwayDir = normalize( lightDir + viewDir );\n"
"\n"
"    vec3 reflectDir = reflect( -lightDir, v_WorldPos );\n"
"//    float spec = pow( max( dot( u_CameraPos, reflectDir ), 0.0 ), 1.0 );\n"
"    float spec = pow( max( dot( v_WorldPos, halfwayDir ), 0.0 ), 1.0 );\n"
"\n"
"    range = light.range + light.brightness;\n"
"    float attenuation = ( light.constant + light.linear * range\n"
"        + light.quadratic * ( range * range ) );\n"
"\n"
"    vec3 specular = light.color.rgb * spec;\n"
"    vec3 ambient = u_AmbientLightColor * texture( u_DiffuseMap, v_TexCoords ).rgb;\n"
"    if ( u_UseSpecularMapping ) {\n"
"        specular *= spec * texture( u_SpecularMap, v_TexCoords ).rgb;\n"
"    }\n"
"\n"
"    ambient *= attenuation;\n"
"    specular *= attenuation;\n"
"    diffuse *= attenuation;\n"
"\n"
"    return ( ambient + diffuse + specular );\n"
"#endif\n"
"}\n"
"\n"
"vec3 CalcDirLight( Light light ) {\n"
"    vec3 lightDir = normalize( -vec3( 0.0 ) );\n"
"    float diff = max( dot( v_WorldPos, lightDir ), 0.0 );\n"
"\n"
"    vec3 reflectDir = reflect( -lightDir, v_WorldPos );\n"
"    float spec = pow( max( dot( u_CameraPos, reflectDir ), 0.0 ), 1.0 );\n"
"\n"
"    vec3 specular = vec3( 0.0 );\n"
"    vec3 ambient = u_AmbientLightColor * vec3( texture( u_DiffuseMap, v_TexCoords ) );\n"
"    if ( u_UseSpecularMapping ) {\n"
"        specular = spec * vec3( texture( u_SpecularMap, v_TexCoords ) );\n"
"    }\n"
"    vec3 diffuse = diff * vec3( texture( u_DiffuseMap, v_TexCoords ) );\n"
"\n"
"    return ( ambient + diffuse + specular );\n"
"}\n"
"\n"
"void CalcNormal() {\n"
"    vec3 normal = texture( u_NormalMap, v_TexCoords ).rgb;\n"
"    normal = normalize( normal * 2.0 - 1.0 );\n"
"    a_Color.rgb *= normal * 0.5 + 0.5;\n"
"}\n"
"\n"
"void CalcLighting() {\n"
"    a_Color = texture( u_DiffuseMap, v_TexCoords );\n"
"    if ( u_UseNormalMapping ) {\n"
"        CalcNormal();\n"
"    }\n"
"    if ( u_UseSpecularMapping && u_NumLights == 0 ) {\n"
"        a_Color.rgb += texture( u_SpecularMap, v_TexCoords ).rgb;\n"
"    }\n"
"    for ( int i = 0; i < u_NumLights; i++ ) {\n"
"        switch ( lights[i].type ) {\n"
"        case POINT_LIGHT:\n"
"            a_Color.rgb += CalcPointLight( lights[i] );\n"
"            break;\n"
"        case DIRECTION_LIGHT:\n"
"            a_Color.rgb += CalcDirLight( lights[i] );\n"
"            break;\n"
"        };\n"
"    }\n"
"    a_Color.rgb += texture( u_DiffuseMap, v_TexCoords ).rgb;\n"
"}\n"
"\n"
"void main() {\n"
"    if ( u_FramebufferActive ) {\n"
"        a_Color = texture( u_DiffuseMap, v_TexCoords );\n"
"    }\n"
"    else {\n"
"        if ( !u_UseDiffuseMapping && !u_UseSpecularMapping && !u_UseNormalMapping && !u_UseAmbientOcclusionMapping ) {\n"
"            a_Color = vec4( 1.0 );\n"
"        } else {\n"
"            CalcLighting();\n"
"        }\n"
"        a_Color.rgb *= u_AmbientLightColor;\n"
"    }\n"
"    if ( u_TileSelected && u_TileSelectionX == v_WorldPos.x && u_TileSelectionY == v_WorldPos.y\n"
"        || ( u_FilterCheckpoints && v_Color.a == 0.04546 ) || ( u_FilterSpawns && v_Color.a == 0.07274 ) )\n"
"    {\n"
"        if ( v_WorldPos.xy != uvec2( 0, 0 ) ) {\n"
"            a_Color.rgb *= v_Color.rgb;\n"
"        }\n"
"    }\n"
"}\n"
;

const char *fallbackShader_mapdraw_vp =
"#version 450 core\n"
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
"out vec3 v_FragPos;\n"
"\n"
"uniform mat4 u_ModelViewProjection;\n"
"\n"
"void main() {\n"
"   v_Position = a_Position;\n"
"   v_WorldPos = a_WorldPos;\n"
"   v_TexCoords = a_TexCoords;\n"
"   v_Color = a_Color;\n"
"   v_FragPos = vec4( u_ModelViewProjection * vec4( a_Position, 1.0 ) ).xyz;\n"
"   gl_Position = vec4( u_ModelViewProjection * vec4( a_Position, 1.0 ) );\n"
"}\n"
;
