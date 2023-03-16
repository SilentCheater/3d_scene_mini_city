#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

in vec3 fragPos;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
uniform mat3 lightDirMatrix;

//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

//components
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;

//fog
uniform int foginit;
uniform float fogDensity;

// spotlight
uniform int spotLightInitialize;
float shininess = 10.0f;
float spotLightQuadratic = 0.2f;
float spotLightLinear = 0.22f;
float spotLightConstant = 1.0f;

vec3 spotLightAmbient = vec3(0.0f, 0.0f, 0.0f);
vec3 spotLightSpecular = vec3(1.0f, 1.0f, 1.0f);
vec3 spotLightColor = vec3(1.0,0.0,0.0);

uniform float spotlight1;
uniform float spotlight2;

uniform vec3 spotLightDirection;
uniform vec3 spotLightPosition;

float computeShadow()
{

	//perform perspective divide
	vec3 normalizedCoords= fragPosLightSpace.xyz / fragPosLightSpace.w;

	//tranform from [-1,1] range to [0,1] range
	normalizedCoords = normalizedCoords * 0.5 + 0.5;

	//get closest depth value from lights perspective
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;

	//get depth of current fragment from lights perspective
	float currentDepth = normalizedCoords.z;

	//if the current fragments depth is greater than the value in the depth map, the current fragment is in shadow 
	//else it is illuminated
	//float shadow = currentDepth > closestDepth ? 1.0 : 0.0;
	float bias = 0.005f;
	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
	if (normalizedCoords.z > 1.0f)
		return 0.0f;
	return shadow;

}


void computeDirLight()
{
	vec3 cameraPosEye = vec3(0.0f);

    //compute eye space coordinates
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));
	//vec3 lightDirN = normalize(lightDir);

    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(cameraPosEye - fPosEye.xyz);

    //compute ambient light
    ambient = ambientStrength * lightColor;

    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    //compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;
}


float computeFog()
{
 vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
 float fragmentDistance = length(fPosEye.xyz);
 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

 return clamp(fogFactor, 0.0f, 1.0f);
}



// spot light
vec3 computeLightSpotComponents() {
	vec3 cameraPosEye = vec3(0.0f);
	vec3 lightDir = normalize(spotLightPosition - fragPos);
	vec3 normalEye = normalize(normalMatrix * fNormal);
	vec3 lightDirN = normalize(lightDirMatrix * lightDir);
	vec3 viewDirN = normalize(cameraPosEye - (view * model * vec4(fPosition, 1.0f)).xyz);
	vec3 halfVector = normalize(lightDirN + viewDirN);

	float diff = max(dot(fNormal, lightDir), 0.0f);
	float spec = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
	float distance = length(spotLightPosition - fragPos);
	float attenuation = 1.0f / (spotLightConstant + spotLightLinear * distance + spotLightQuadratic * distance * distance);
	
	vec3 ambient = spotLightColor * spotLightAmbient * vec3(texture(diffuseTexture, fTexCoords));
	vec3 diffuse = spotLightColor * spotLightSpecular * diff * vec3(texture(diffuseTexture, fTexCoords));
	vec3 specular = spotLightColor * spotLightSpecular * spec * vec3(texture(specularTexture, fTexCoords));

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;
	
	return ambient + diffuse + specular;
}

void main() 
{
    computeDirLight();

    //compute final vertex color
	float shadow = computeShadow();

    vec3 color = min((ambient + (1.0f - shadow) * diffuse) * texture(diffuseTexture, fTexCoords).rgb 
	+ (1.0f - shadow) * specular * texture(specularTexture, fTexCoords).rgb, 1.0f);

	//vec3 color = min((ambient + diffuse) * texture(diffuseTexture, fTexCoords).rgb 
	//+ specular * texture(specularTexture, fTexCoords).rgb, 1.0f);

	float fogFactor = computeFog();
	vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);

	// spotlight
	if (spotLightInitialize == 1){
	color += computeLightSpotComponents();
	}


	if ( foginit == 0)
	{
		fColor = vec4(color, 1.0f);
	}
	else
	{
		fColor = mix(fogColor, min(vec4(color, 1.0f), 1.0f), fogFactor);
	}

    //fColor = vec4(color, 1.0f);
}
