#version 450

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

struct PointLight{
	vec4 position; //ignore w
	vec4 color; //w is intensity
};

//GlobalUbo's role is to share data between multiple shaders
layout(set = 0, binding = 0) uniform GlobalUbo{ //binding = 0 means first binding in the set
    mat4 projection; //combined projection and view matrix
    mat4 view; //combined projection and view matrix
    mat4 invView;
    vec4 ambientLightColor; //w is intensity
    PointLight pointLights[10]; //specialization constants is a method of passing constant values to shaders at pipeline creation time
    int numLights;
} ubo;

layout(push_constant) uniform Push{ //push constant is for vulkan only
    vec4 position;
    vec4 color;
    float radius;
} push; //this can be lowercase and uniform is upper

const float M_PI = 3.1415926538;

void main(){
    float dis = sqrt(dot(fragOffset, fragOffset));
    if(dis > 1.0){
        discard; //discard throws away the fragment 
    }
    
    float cosDis = 0.5 * (cos(dis * M_PI) + 1.0); //ranges from 1 -> 0
    outColor = vec4(push.color.xyz + 0.5 * cosDis, cosDis); 
}