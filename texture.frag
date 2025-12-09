#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

struct PointLight{
    vec4 position; //ignore w
    vec4 color; //w is intensity
};

//binding = 0 means first binding in the set. binding means the same thing as in vulkan descriptor set layout
layout(set = 0, binding = 0) uniform GlobalUbo{ 
    mat4 projection; //combined projection and view matrix
    mat4 view; //combined projection and view matrix
    mat4 invView;
    vec4 ambientLightColor; //w is intensity
    PointLight pointLights[10]; //specialization constants is a method of passing constant values to shaders at pipeline creation time
    int numLights;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D texSampler; //texture sampler

//push constants are a small amount of data that can be passed to shaders very efficiently
layout(push_constant) uniform Push{ //push constant is glsl for vulkan only
    mat4 modelMatrix;
    mat4 normalMatrix;
} push; //this can be lowercase and uniform is upper

void main() {
    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w; //start with ambient light
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormalWorld);
    
    vec3 cameraPosWorld = ubo.invView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    for(int i = 0; i < ubo.numLights; i++){
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = light.position.xyz - fragPosWorld; //direction from the surface to the light source

        float attenuation = 1.0 / (dot(directionToLight, directionToLight)); //distance squared. attenuation is how much the light dims over distance
        //float cosAngIncidence = max(dot(surfaceNormal, normalize(directionToLight)), 0); //cosine of angle of incidence, clamped to 0
        
        directionToLight = normalize(directionToLight);

        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation; //rgb * intensity

        diffuseLight += intensity * cosAngIncidence; //add to total light
        
        // specular lighting
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = pow(blinnTerm, 512.0); // higher values -> sharper highlight
        specularLight += intensity * blinnTerm;
    }

    outColor = vec4(fragColor * diffuseLight + specularLight * fragColor, 1.0); 
    //outColor = vec4(fragTexCoord, 0.0, 1.0); // visualize UV coordinates (should be blue/cyan/magenta)
    //outColor = texture(texSampler, fragTexCoord) * vec4(diffuseLight, 1.0) + vec4(specularLight, 1.0);
}