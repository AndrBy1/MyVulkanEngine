#version 450

const vec2 OFFSETS[6] = vec2[](
	vec2(-1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, -1.0),
	vec2(1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, 1.0)
);


//fragOffset is the offset from the center of the light sphere to the current vertex
layout (location = 0) out vec2 fragOffset;

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

//a push constant is a small piece of data that can be passed directly to a shader stage without the need for a buffer
//push constant is glsl for vulkan only
layout(push_constant) uniform Push{ //push constant is glsl for vulkan only
    vec4 position;
    vec4 color;
    float radius;
} push; //this can be lowercase and uniform is upper

void main() {
	//gl_VertexIndex is a built-in variable that contains the index of the vertex currently being processed.
	//may complain that gl_VertexIndex is not supported in opengl. gl_VertexID is opengl replacement.
	fragOffset = OFFSETS[gl_VertexIndex];
    
	  //rightworld is x axis, upworld is y axis
    vec3 cameraRightWorld = {ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]};
    vec3 cameraUpWorld = {ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]};
    vec3 positionWorld = push.position.xyz + (push.radius * fragOffset.x * cameraRightWorld) + (push.radius * fragOffset.y * cameraUpWorld);
    gl_Position = ubo.projection * ubo.view * vec4(positionWorld, 1.0);

	//alternative method is to first transform light position to camera space then apply offset in camera space.
    //vec4 lightInCameraSpace = ubo.view * vec4(ubo.lightPosition, 1.0);
    //vec4 positionInCameraSpace = lightInCameraSpace + LIGHT_RADIUS * vec4(fragOffset, 0.0, 0.0);
    //gl_Position = ubo.projection * positionInCameraSpace;
}