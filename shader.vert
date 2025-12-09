#version 450

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 color;
//normals checks the direction of a surface. If the surface it pointing towards light it is lit. The greater the angle the darker. 
layout(location = 2) in vec3 normal; 
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragTexCoord;

struct PointLight{
    vec4 position; //ignore w
    vec4 color; //w is intensity
};

//binding = 0 means first binding in the set. binding means the same thing as in vulkan descriptor set layout
layout(set = 0, binding = 0) uniform GlobalUbo{ 
    mat4 projection; //combined projection and view matrix
    mat4 view; //combined projection and view matrix
    mat4 invView; //to convert from view space to world space
    vec4 ambientLightColor; //w is intensity
    PointLight pointLights[10]; //specialization constants is a method of passing constant values to shaders at pipeline creation time
    int numLights;
} ubo;
//the purpose of writing "ubo" is to create an instance of the uniform block defined by the GlobalUbo structure so that its members can be accessed in the shader code

layout(push_constant) uniform Push{ //push constant is for vulkan only
    mat4 modelMatrix;
    mat4 normalMatrix;
} push; //this can be lowercase and uniform is upper

void main() {
    //model matrix transforms from model space to world space
    vec4 positionWorld = push.modelMatrix * vec4(positions, 1.0); 

    //the first component of gl_VertexIndex is used to index into the positions array, 
    gl_Position = ubo.projection * ubo.view * positionWorld;

    //vec3 normalWorldSpace = normalize(mat3(push.modelMatrix) * normal); //temporary: this is only correct in certain situations! Only correct if scaling is uniform (sx = sy = sz)
    
    //calculating the inverse in a shader can be expensive and should be avoided
    //can use only uniform scaling (most efficient) or pass in pre-computed normal matrix to shaders
    //mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)));
    //vec3 normalWorldSpace = normalize(normalMatrix * normal);

    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz; //world space position of the fragment
    fragColor = color;
    fragTexCoord = uv;

}