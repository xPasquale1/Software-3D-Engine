#include <iostream>
#include <chrono>
#include <math.h>
#include <thread>
#include <vector>

/*	TODO programm crashed falls die clipping region gleich/größer wie der Bildschirm ist, wahrscheinlich schreibt
	der rasterizer ausserhalb des pixel arrays
	TODO aktuell gibt es kein far clipping plane, daher wird nur ein teil der depth buffer Auflösung genutzt
	vllt kann man kein clipping machen, aber eine max. weite und daher auch Auflösung festlegen (clippe einfach alle verticies die zu groß sind? aufwand größer?)
	TODO Shadow mapping oder ähnliches
	TODO Multithreading muss noch korrekt implementiert werden mit locks auf die buffers, "faire" aufteilung,... und die Threads vllt wieder verwenden lol
	TODO eigene alloc Funktion machen, die ErrCode zurück gibt
*/

// #define NEWTRIANGLEDRAWINGALGORITHM
#define CULLBACKFACES
// #define CULLBACKFACESSHADOW
#define CULLFRONTFACESSHADOW
#include "graphics/window.h"

bool _running = true;
Camera cam = {1., {70.3199, -215.036, 163.83}, {-1.482, 0.144999}};
Camera shadowCamera = {1., {-109.7, -1122.79, 1147.08}, {-3.04176, 0.697247}};

LRESULT mainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void update(float dt)noexcept;

enum RENDERMODES{
	RENDERMODE_SHADED_MODE,
	RENDERMODE_WIREFRAME_MODE,
	RENDERMODE_SHADOW_DEPTH,
	RENDERMODE_DEPTH_MODE,
	RENDERMODE_NORMAL_MODE
};

RENDERMODES renderMode;

//Sliders
FloatSlider shadowSlider[1];
WORD shadowSliderCount = 0;
FloatSlider ssaoSlider[4];
WORD ssaoSliderCount = 0;
FloatSlider ssgiSlider[4];
WORD ssgiSliderCount = 0;
FloatSlider ssrSlider[2];
WORD ssrSliderCount = 0;
FloatSlider ssrcSlider[7];
WORD ssrcSliderCount = 0;
FloatSlider resolutionScaleSlider;

//Checkboxen
Checkbox checkboxes[5];		//Schatten, SSR, SSAO, SSGI, SSRC
WORD checkboxCount = 0;

Menu settingsMenu;
// Menü Funktionen
ErrCode setRenderMode(void* mode)noexcept{
	renderMode = *(RENDERMODES*)mode;
	switch(renderMode){
		case RENDERMODE_WIREFRAME_MODE:
		case RENDERMODE_DEPTH_MODE:
		case RENDERMODE_SHADOW_DEPTH:
		case RENDERMODE_NORMAL_MODE:{
			ssaoSliderCount = 0;
			ssgiSliderCount = 0;
			ssrSliderCount = 0;
			checkboxCount = 0;
			break;
		}
		case RENDERMODE_SHADED_MODE:{
			//Schatten
			ivec2 position = {10, settingsMenu.size.y};
			position.y += 10;
			checkboxes[0].pos = position;
			checkboxes[0].size = {30, 30};
			checkboxes[0].label = "Shadows";
			position.y += checkboxes[0].size.y;
			if(getCheckBoxFlag(checkboxes[0], CHECKBOXFLAG_CHECKED)){
				position.y += 22;
				shadowSliderCount = sizeof(shadowSlider)/sizeof(FloatSlider);
				for(int i=0; i < shadowSliderCount; ++i){
					shadowSlider[i].pos = {position.x, position.y};
					if(i != shadowSliderCount-1) position.y += shadowSlider[i].sliderRadius*2+10;
				}
				position.y += 12;
			}else shadowSliderCount = 0;
			position.y += 10;
			checkboxes[1].pos = position;
			checkboxes[1].size = {30, 30};
			checkboxes[1].label = "SSR";
			position.y += checkboxes[1].size.y;
			if(getCheckBoxFlag(checkboxes[1], CHECKBOXFLAG_CHECKED)){
				position.y += 22;
				ssrSliderCount = sizeof(ssrSlider)/sizeof(FloatSlider);
				for(int i=0; i < ssrSliderCount; ++i){
					ssrSlider[i].pos = {position.x, position.y};
					if(i != ssrSliderCount-1) position.y += ssrSlider[i].sliderRadius*2+10;
				}
				position.y += 12;
			}else ssrSliderCount = 0;
			position.y += 10;
			checkboxes[2].pos = position;
			checkboxes[2].size = {30, 30};
			checkboxes[2].label = "SSAO";
			position.y += checkboxes[2].size.y;
			if(getCheckBoxFlag(checkboxes[2], CHECKBOXFLAG_CHECKED)){
				position.y += 22;
				ssaoSliderCount = sizeof(ssaoSlider)/sizeof(FloatSlider);
				for(int i=0; i < ssaoSliderCount; ++i){
					ssaoSlider[i].pos = {position.x, position.y};
					if(i != ssaoSliderCount-1) position.y += ssaoSlider[i].sliderRadius*2+10;
				}
				position.y += 12;
			}else ssaoSliderCount = 0;
			position.y += 10;
			checkboxes[3].pos = position;
			checkboxes[3].size = {30, 30};
			checkboxes[3].label = "SSGI";
			position.y += checkboxes[3].size.y;
			if(getCheckBoxFlag(checkboxes[3], CHECKBOXFLAG_CHECKED)){
				position.y += 22;
				ssgiSliderCount = sizeof(ssgiSlider)/sizeof(FloatSlider);
				for(int i=0; i < ssgiSliderCount; ++i){
					ssgiSlider[i].pos = {position.x, position.y};
					if(i != ssgiSliderCount-1) position.y += ssgiSlider[i].sliderRadius*2+10;
				}
				position.y += 12;
			}else ssgiSliderCount = 0;
			position.y += 10;
			checkboxes[4].pos = position;
			checkboxes[4].size = {30, 30};
			checkboxes[4].label = "SSRC";
			position.y += checkboxes[4].size.y;
			if(getCheckBoxFlag(checkboxes[4], CHECKBOXFLAG_CHECKED)){
				position.y += 22;
				ssrcSliderCount = sizeof(ssrcSlider)/sizeof(FloatSlider);
				for(int i=0; i < ssrcSliderCount; ++i){
					ssrcSlider[i].pos = {position.x, position.y};
					if(i != ssrcSliderCount-1) position.y += ssrcSlider[i].sliderRadius*2+10;
				}
				position.y += 12;
			}else ssrcSliderCount = 0;
			break;
		}
	}
	return ERR_SUCCESS;
}

#define SPEED 0.25

Window window;
Font font;

RenderBuffers renderBuffers;
Colorbuffer colorBuffers[2];	//0 = SSGI
Floatbuffer dataBuffers[2];		//0 = Schatten, 1 Indirekte Beleuchtung
float resolutionScale = 0.5;

DWORD frameCounter = 0;

ColorPoint* globalPoints = nullptr;
DWORD globalPointCount = 0;

fvec3 waterVertexShader(fvec3& point, float* attributes)noexcept{
	point.x += cos(attributes[5]*5+frameCounter*0.2)*20;
	point.y += sin(attributes[5]*5+frameCounter*0.2)*20;
	return point;
}

fvec3 vertexShader(fvec3& point, float* attributes)noexcept{return point;}

void textureShader(RenderBuffers& renderBuffers, Image& image)noexcept{
	DWORD totalBufferSize = renderBuffers.width*renderBuffers.height;
	for(DWORD i=0; i < totalBufferSize; ++i){
		if(renderBuffers.fragmentFlags[i] == 0) continue;
		renderBuffers.fragmentFlags[i] = 0;
		float uvx = renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 0)];
		float uvy = renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 1)];
		DWORD color = textureRepeated(image, uvx, uvy);
		if(A(color) < 120){
			renderBuffers.depthBuffer[i] = FLOAT_MAX;
			continue;
		}
		renderBuffers.frameBuffer[i] = color;
	}
}

#define RANDOMNORMALSCOUNT 211
fvec3 randomNormalVectorBuffer[RANDOMNORMALSCOUNT];

fvec3 generateRandomNormalInHemisphere(const fvec3& normal)noexcept{
	fvec3 random = randomNormalVectorBuffer[nextrand()%RANDOMNORMALSCOUNT];
    if(dot(normal, random) < 0){
		random.x = -random.x;
		random.y = -random.y;
		random.z = -random.z;
	}
    return random;
}

fvec3 worldCoordinatesToScreenspace(const fvec3& worldPixelPosition, const float rotMat[3][3], const fvec3& cameraPos, float aspectRatio, WORD bufferWidth, WORD bufferHeight)noexcept{
	fvec3 cameraPosition = {worldPixelPosition.x-cameraPos.x, worldPixelPosition.y-cameraPos.y, worldPixelPosition.z-cameraPos.z};
	fvec3 screenPos = mulVec3Mat3x3(cameraPosition, rotMat);
    screenPos.x = (((screenPos.x*(cam.focal_length/screenPos.z)*aspectRatio)*0.5)+0.5)*bufferWidth;
    screenPos.y = (((screenPos.y*(cam.focal_length/screenPos.z))*0.5)+0.5)*bufferHeight;
	return screenPos;
}

fvec3 worldPosToViewSpaceOrtho(const fvec3& worldPos, const float rotMat[3][3], const fvec3& cameraPos, WORD bufferWidth, WORD bufferHeight)noexcept{
	fvec3 cameraPosition = {worldPos.x-cameraPos.x, worldPos.y-cameraPos.y, worldPos.z-cameraPos.z};
	fvec3 screenPos = mulVec3Mat3x3(cameraPosition, rotMat);
	screenPos.x = ((screenPos.x*INVCLIPPINGREGIONSIZE*0.5)+0.5)*bufferWidth;
	screenPos.y = ((screenPos.y*INVCLIPPINGREGIONSIZE*0.5)+0.5)*bufferHeight;
	return screenPos;
}

void shadowMappingShader(RenderBuffers& renderBuffers, Depthbuffer& shadowDepthBuffer, Camera& sceneCam, Camera& shadowCam)noexcept{
	float rotm[3][3];
	float sceneCamRotm[3][3];
	float sin_rotx = sin(shadowCam.rot.x);
	float cos_rotx = cos(shadowCam.rot.x);
	float sin_roty = sin(shadowCam.rot.y);
	float cos_roty = cos(shadowCam.rot.y);
	float sinRotxSceneCam = sin(sceneCam.rot.x);
	float cosRotxSceneCam = cos(sceneCam.rot.x);
	float sinRotySceneCam = sin(sceneCam.rot.y);
	float cosRotySceneCam = cos(sceneCam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	sceneCamRotm[0][0] = cosRotxSceneCam; 					sceneCamRotm[0][1] = 0; 				sceneCamRotm[0][2] = sinRotxSceneCam;
    sceneCamRotm[1][0] = sinRotxSceneCam*sinRotySceneCam;	sceneCamRotm[1][1] = cosRotySceneCam; 	sceneCamRotm[1][2] = -sinRotySceneCam*cosRotxSceneCam;
    sceneCamRotm[2][0] = -sinRotxSceneCam*cosRotySceneCam;	sceneCamRotm[2][1] = sinRotySceneCam; 	sceneCamRotm[2][2] = cosRotxSceneCam*cosRotySceneCam;

	const float aspectRatio = (float)renderBuffers.height/renderBuffers.width;
	DWORD idx = 0;
	DWORD totalBufferSize = renderBuffers.width*renderBuffers.height;
	const fvec3 sunDir = mulVec3InvMat3x3({0, 0, -1}, rotm);
	const fvec3 invSunDir = {-sunDir.x, -sunDir.y, -sunDir.z};
	const DWORD totalShadowSize = shadowDepthBuffer.width*shadowDepthBuffer.height;
	for(WORD y=0; y < renderBuffers.height; ++y){
		for(WORD x=0; x < renderBuffers.width; ++x, ++idx){
			fvec3 worldPixelPosition;
			worldPixelPosition.x = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 5)];
			worldPixelPosition.y = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 6)];
			worldPixelPosition.z = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 7)];

			fvec3 worldNormal;
			worldNormal.x = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 2)];
			worldNormal.y = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 3)];
			worldNormal.z = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 4)];
			float lightStrength = 1.f;
			// clamp(dot(worldNormal, invSunDir), 0.f, 1.f);

			fvec3 viewPos = worldPosToViewSpaceOrtho(worldPixelPosition, rotm, shadowCam.pos, shadowDepthBuffer.width, shadowDepthBuffer.height);
			DWORD uvx = (DWORD)viewPos.x;
			DWORD uvy = (DWORD)viewPos.y;

			if(uvx >= shadowDepthBuffer.width || uvy >= shadowDepthBuffer.height){
				dataBuffers[0].data[idx] = 0.f;
				continue;
			}

			float shadowColor = 0;
			bool valid = true;
			for(SBYTE dy=-1; dy <= 1; ++dy){
				if(!valid) break;
				WORD yIdx = uvy+dy;
				if(yIdx >= shadowDepthBuffer.height){
					valid = false;
					break;
				}
				for(SBYTE dx=-1; dx <= 1; ++dx){
					WORD xIdx = uvx+dx;
					if(xIdx >= shadowDepthBuffer.width){
						valid = false;
						break;
					}
					float shadowDepth = shadowDepthBuffer.data[yIdx*shadowDepthBuffer.width+xIdx];
					if(viewPos.z*DEPTH_DIVISOR-shadowSlider[0].value*DEPTH_DIVISOR <= shadowDepth) shadowColor += 1;
				}
			}
			if(!valid){
				dataBuffers[0].data[idx] = 0.f;
				continue;
			}
			shadowColor /= 9;
			
			// const float stepSize = 1;
			// const float maxSteps = 8;
			// for(int step=0; step < maxSteps; ++step){
			// 	worldPixelPosition.x += sunDir.x * stepSize;
			// 	worldPixelPosition.y += sunDir.y * stepSize;
			// 	worldPixelPosition.z += sunDir.z * stepSize;

			// 	fvec3 viewPos = worldCoordinatesToScreenspace(worldPixelPosition, sceneCamRotm, cam.pos, aspectRatio, renderBuffers.width, renderBuffers.height);
			// 	WORD uvx = (WORD)viewPos.x;
			// 	WORD uvy = (WORD)viewPos.y;

			// 	if(uvx >= renderBuffers.width || uvy >= renderBuffers.height) break;

			// 	float depthDiff = viewPos.z*DEPTH_DIVISOR-renderBuffers.depthBuffer[uvy*renderBuffers.width+uvx];
			// 	if(depthDiff > 2*DEPTH_DIVISOR && depthDiff < 4*DEPTH_DIVISOR){
			// 		shadowColor = 0;
			// 		break;
			// 	}
			// }
			
			lightStrength *= shadowColor;
			lightStrength = clamp(lightStrength, 0.f, 1.f);
			dataBuffers[0].data[idx] = lightStrength;
		}
	}
}

float raytrace(fvec3 position, const fvec3 normal, float rotm[3][3], float aspectRatio, WORD samples, BYTE bounces, const int maxSteps, const float stepSize, DWORD totalBufferSize)noexcept{
	if(bounces == 0) return 0.f;
	float lightStrength = 0.f;
	BYTE hits = 0;
	for(WORD i=0; i < samples; ++i){
		fvec3 samplePos = position;
		fvec3 direction = generateRandomNormalInHemisphere(normal);
		for(int step=0; step < maxSteps; ++step){
			samplePos.x -= direction.x * stepSize;
			samplePos.y -= direction.y * stepSize;
			samplePos.z -= direction.z * stepSize;

			fvec3 screenCoords = worldCoordinatesToScreenspace(samplePos, rotm, cam.pos, aspectRatio, renderBuffers.width, renderBuffers.height);
			WORD screenX = (WORD)screenCoords.x;
			WORD screenY = (WORD)screenCoords.y;

			if(screenX >= renderBuffers.width || screenY >= renderBuffers.height) break;

			DWORD sampleIdx = screenY*renderBuffers.width+screenX;
			float depthDiff = screenCoords.z*DEPTH_DIVISOR-renderBuffers.depthBuffer[sampleIdx];
			if(depthDiff >= 20.f*DEPTH_DIVISOR) break;
			if(depthDiff > 2.f*DEPTH_DIVISOR){
				fvec3 pos;
				pos.x = renderBuffers.attributeBuffers[sampleIdx+getAttrLoc(totalBufferSize, 5)];
				pos.y = renderBuffers.attributeBuffers[sampleIdx+getAttrLoc(totalBufferSize, 6)];
				pos.z = renderBuffers.attributeBuffers[sampleIdx+getAttrLoc(totalBufferSize, 7)];

				fvec3 n;
				n.x = renderBuffers.attributeBuffers[sampleIdx+getAttrLoc(totalBufferSize, 2)];
				n.y = renderBuffers.attributeBuffers[sampleIdx+getAttrLoc(totalBufferSize, 3)];
				n.z = renderBuffers.attributeBuffers[sampleIdx+getAttrLoc(totalBufferSize, 4)];
				float ndotl = dot(direction, normal)*2;
				lightStrength += raytrace(pos, n, rotm, aspectRatio, samples/2, bounces-1, maxSteps, stepSize, totalBufferSize)*ndotl;
				lightStrength += dataBuffers[0].data[sampleIdx]*ndotl;
				hits++;
				break;
			}
		}
	}
	if(hits == 0) return 0.f;
	return lightStrength/hits;
}

struct RadianceProbe{
	fvec3 pos;
	float lightStrength = 0;
	DWORD idx;
};

void ssrc(RenderBuffers& renderBuffers, Camera& camera, SDF& sceneSdf, fvec3 lightPos)noexcept{
	float rotm[3][3];
	float aspect_ratio = (float)renderBuffers.height/renderBuffers.width;
	float sin_rotx = sin(camera.rot.x);
	float cos_rotx = cos(camera.rot.x);
	float sin_roty = sin(camera.rot.y);
	float cos_roty = cos(camera.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	
	DWORD totalBufferSize = renderBuffers.width*renderBuffers.height;
	const int probesCountSlider = ssrcSlider[0].value;
	const int yInc = std::ceil((float)renderBuffers.height/probesCountSlider);
	const int xInc = std::ceil((float)renderBuffers.width/probesCountSlider);
	const BYTE bounces = ssrcSlider[1].value;
	const float stepSize = ssrcSlider[2].value;
	const int maxSteps = ssrcSlider[3].value;
	const float minDepth = ssrcSlider[5].value*DEPTH_DIVISOR;
	const float maxDepth = ssrcSlider[6].value*DEPTH_DIVISOR;

	RadianceProbe probes[probesCountSlider*probesCountSlider];
	DWORD probesCount = 0;
	#define HASHGRIDSIZEX 64
	#define HASHGRIDSIZEY 64
	#define RADIUS 2
	DWORD hashGrid[probesCountSlider/4*HASHGRIDSIZEX*HASHGRIDSIZEY];		//TODO Sollte noch richtig berechnet werden
	WORD hashGridProbeCount[HASHGRIDSIZEX*HASHGRIDSIZEY];
	for(DWORD i=0; i < HASHGRIDSIZEX*HASHGRIDSIZEY; ++i) hashGridProbeCount[i] = 0;
	for(DWORD y=yInc/2; y < renderBuffers.height; y+=yInc){
		for(DWORD x=xInc/2; x < renderBuffers.width; x+=xInc){
			DWORD idx = y*renderBuffers.width+x;
			probes[probesCount].idx = idx;
			probes[probesCount].pos.x = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 5)];
			probes[probesCount].pos.y = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 6)];
			probes[probesCount].pos.z = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 7)];
			WORD i = (y*HASHGRIDSIZEY)/renderBuffers.height;
			WORD j = (x*HASHGRIDSIZEX)/renderBuffers.width;
			DWORD hashIdx = i*HASHGRIDSIZEX+j;
			WORD count = hashGridProbeCount[hashIdx];
			hashGrid[hashIdx*probesCountSlider/4+count] = probesCount;
			hashGridProbeCount[hashIdx] += 1;
			probesCount++;
		}
	}

	for(BYTE b=0; b < bounces; ++b){
		for(DWORD p=0; p < probesCount; ++p){
			const DWORD idx = probes[p].idx;
			fvec3 worldNormal;
			worldNormal.x = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 2)];
			worldNormal.y = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 3)];
			worldNormal.z = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 4)];

			fvec3 worldPixelPosition = probes[p].pos;

			const WORD samples = ssrcSlider[4].value;

			float lightStrength = 0.f;
			for(WORD i=0; i < samples; ++i){
				fvec3 samplePos = worldPixelPosition;
				fvec3 preSamplePos = samplePos;
				fvec3 direction = generateRandomNormalInHemisphere(worldNormal);
				for(int step=0; step < maxSteps; ++step){
					samplePos.x -= direction.x*stepSize;
					samplePos.y -= direction.y*stepSize;
					samplePos.z -= direction.z*stepSize;
					// globalPoints[globalPointCount].pos = samplePos;
					// globalPoints[globalPointCount].color = RGBA(0, 255, 0);
					// globalPointCount++;

					fvec3 screenCoords = worldCoordinatesToScreenspace(samplePos, rotm, camera.pos, aspect_ratio, renderBuffers.width, renderBuffers.height);
					WORD screenX = (WORD)screenCoords.x;
					WORD screenY = (WORD)screenCoords.y;

					//Worldspace tracing
					if(screenX >= renderBuffers.width || screenY >= renderBuffers.height){
						for(int i=step; i < maxSteps; ++i){
							// globalPoints[globalPointCount].pos = samplePos;
							// globalPoints[globalPointCount].color = RGBA(255, 0, 0);
							// globalPointCount++;
							float dist = getSDFDistanceFromPosition(sceneSdf, samplePos);
							if(dist <= 8){
								if(dist < 0) break;
								//Schattenray
								samplePos = preSamplePos;
								fvec3 reflDir = direction;
								direction = normalize({lightPos.x-samplePos.x, lightPos.y-samplePos.y, lightPos.z-samplePos.z});
								for(int j=i; j < maxSteps; ++j){
									// globalPoints[globalPointCount].pos = samplePos;
									// globalPoints[globalPointCount].color = RGBA(0, 0, 255);
									// globalPointCount++;
									float dist = getSDFDistanceFromPosition(sceneSdf, samplePos);
									if(dist <= 8){
										if(dist < 0){
											float ndotl = dot(reflDir, worldNormal)*2;
											lightStrength += ndotl;
										}
										break;
									}
									samplePos.x += direction.x*dist;
									samplePos.y += direction.y*dist;
									samplePos.z += direction.z*dist;
								}
								break;
							}
							preSamplePos = samplePos;
							samplePos.x -= direction.x*dist;
							samplePos.y -= direction.y*dist;
							samplePos.z -= direction.z*dist;
						}
						break;
					}

					DWORD sampleIdx = screenY*renderBuffers.width+screenX;
					float depthDiff = screenCoords.z*DEPTH_DIVISOR-renderBuffers.depthBuffer[sampleIdx];
					if(depthDiff >= maxDepth) break;	//TODO sollte auch gegen das Distance Field tracen
					if(depthDiff > minDepth){
						fvec3 pos;
						pos.x = renderBuffers.attributeBuffers[sampleIdx+getAttrLoc(totalBufferSize, 5)];
						pos.y = renderBuffers.attributeBuffers[sampleIdx+getAttrLoc(totalBufferSize, 6)];
						pos.z = renderBuffers.attributeBuffers[sampleIdx+getAttrLoc(totalBufferSize, 7)];

						fvec3 n;
						n.x = renderBuffers.attributeBuffers[sampleIdx+getAttrLoc(totalBufferSize, 2)];
						n.y = renderBuffers.attributeBuffers[sampleIdx+getAttrLoc(totalBufferSize, 3)];
						n.z = renderBuffers.attributeBuffers[sampleIdx+getAttrLoc(totalBufferSize, 4)];
						float ndotl = dot(direction, worldNormal)*2;
						lightStrength += dataBuffers[0].data[sampleIdx]*ndotl;
						lightStrength += dataBuffers[1].data[sampleIdx]*ndotl;
						break;
					}
				}
			}
			probes[p].lightStrength = lightStrength/samples;
		}
		for(WORD y=0; y < renderBuffers.height; ++y){
			for(WORD x=0; x < renderBuffers.width; ++x){
				DWORD idx = y*renderBuffers.width+x;
				fvec3 worldPosition;
				worldPosition.x = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 5)];
				worldPosition.y = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 6)];
				worldPosition.z = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 7)];
				float totalWeight = 0;
				float lightStrength = 0;
				for(int i=-RADIUS; i <= RADIUS; ++i){
					for(int j=-RADIUS; j <= RADIUS; ++j){
						WORD dy = (y*HASHGRIDSIZEY)/renderBuffers.height;
						WORD dx = (x*HASHGRIDSIZEX)/renderBuffers.width;
						DWORD hashIdx = (dy-i)*HASHGRIDSIZEX+dx+j;
						if(hashIdx >= HASHGRIDSIZEX*HASHGRIDSIZEY) continue;
						WORD count = hashGridProbeCount[hashIdx];
						for(WORD l=0; l < count; ++l){
							DWORD probeIdx = hashGrid[hashIdx*probesCountSlider/4+l];
							float dist = length({probes[probeIdx].pos.x - worldPosition.x, probes[probeIdx].pos.y - worldPosition.y, probes[probeIdx].pos.z - worldPosition.z});
							dist = max(10.f, dist);
							float weigth = 1/(dist*dist);
							lightStrength += probes[probeIdx].lightStrength*weigth;
							totalWeight += weigth;
						}
					}
				}
				dataBuffers[1].data[idx] = lightStrength/totalWeight;
			}
		}
	}
}

void ssgi(RenderBuffers& renderBuffers, Camera& cam, DWORD startIdx, DWORD endIdx)noexcept{
	float rotm[3][3];
	float aspect_ratio = (float)renderBuffers.height/renderBuffers.width;
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	float sin_roty = sin(cam.rot.y);
	float cos_roty = cos(cam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	
	DWORD totalBufferSize = renderBuffers.width*renderBuffers.height;
	// DWORD middelPixel = (renderBuffers.height/2)*renderBuffers.width+renderBuffers.width/2;
	for(DWORD idx=startIdx; idx < endIdx; ++idx){
	// for(DWORD idx=middelPixel; idx < middelPixel+1; ++idx){
		fvec3 worldNormal;
		worldNormal.x = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 2)];
		worldNormal.y = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 3)];
		worldNormal.z = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 4)];

		fvec3 worldPixelPosition;
		worldPixelPosition.x = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 5)];
		worldPixelPosition.y = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 6)];
		worldPixelPosition.z = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 7)];

		const float stepSize = ssgiSlider[2].value;
		const int maxSteps = ssgiSlider[3].value;
		const WORD ssgiSamples = ssgiSlider[0].value;
		const BYTE bounces = ssgiSlider[1].value;
		dataBuffers[1].data[idx] = raytrace(worldPixelPosition, worldNormal, rotm, aspect_ratio, ssgiSamples, bounces, maxSteps, stepSize, totalBufferSize);
	}
}

void ssao(RenderBuffers& renderBuffers)noexcept{
	float rotm[3][3];
	float aspect_ratio = (float)renderBuffers.height/renderBuffers.width;
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	float sin_roty = sin(cam.rot.y);
	float cos_roty = cos(cam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	
	const int ssaoSamples = ssaoSlider[0].value;
	const float ssaoMaxLength = ssaoSlider[1].value;
	const float minDepth = ssaoSlider[2].value*DEPTH_DIVISOR;
	const float maxDepth = ssaoSlider[3].value*DEPTH_DIVISOR;
	
	DWORD totalBufferSize = renderBuffers.width*renderBuffers.height;
	for(DWORD idx=0; idx < totalBufferSize; ++idx){
		fvec3 worldNormal;
		worldNormal.x = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 2)];
		worldNormal.y = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 3)];
		worldNormal.z = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 4)];

		fvec3 worldPixelPosition;
		worldPixelPosition.x = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 5)];
		worldPixelPosition.y = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 6)];
		worldPixelPosition.z = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 7)];

		float strength = 0;
		for(int i=0; i < ssaoSamples; ++i){
			fvec3 dir = generateRandomNormalInHemisphere(worldNormal);
			float length = (float)i/ssaoSamples;
			length = -lerp(0.1f, 1.f, length*length);
			dir.x *= length*ssaoMaxLength;
			dir.y *= length*ssaoMaxLength;
			dir.z *= length*ssaoMaxLength;
			fvec3 samplePoint = {worldPixelPosition.x+dir.x, worldPixelPosition.y+dir.y, worldPixelPosition.z+dir.z};

			fvec3 screenCoords = worldCoordinatesToScreenspace(samplePoint, rotm, cam.pos, aspect_ratio, renderBuffers.width, renderBuffers.height);
			WORD screenX = (WORD)screenCoords.x;
			WORD screenY = (WORD)screenCoords.y;

			if(screenX >= renderBuffers.width || screenY >= renderBuffers.height){
				strength++;
				continue;
			}

			float depthDiff = screenCoords.z*DEPTH_DIVISOR-renderBuffers.depthBuffer[screenY*renderBuffers.width+screenX];
			if(depthDiff <= minDepth || depthDiff >= maxDepth) strength++;
		}
		strength /= ssaoSamples;
		dataBuffers[1].data[idx] = strength;
	}
}

void ssr(RenderBuffers& renderBuffers, Camera& cam)noexcept{
	float rotm[3][3];
	float aspect_ratio = (float)renderBuffers.height/renderBuffers.width;
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	float sin_roty = sin(cam.rot.y);
	float cos_roty = cos(cam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	
	DWORD totalBufferSize = renderBuffers.width*renderBuffers.height;
	for(DWORD idx=0; idx < totalBufferSize; ++idx){
		fvec3 worldNormal;
		worldNormal.x = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 2)];
		worldNormal.y = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 3)];
		worldNormal.z = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 4)];
		if(worldNormal.y < 0.99) continue;

		fvec3 worldPixelPosition;
		worldPixelPosition.x = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 5)];
		worldPixelPosition.y = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 6)];
		worldPixelPosition.z = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 7)];

		fvec3 viewDir = {worldPixelPosition.x-cam.pos.x, worldPixelPosition.y-cam.pos.y, worldPixelPosition.z-cam.pos.z};
		viewDir = normalize(viewDir);

		fvec3 reflDir = reflect(viewDir, worldNormal);

        bool hit = false;
        const float stepSize = ssrSlider[0].value;
        const int maxSteps = 120;
		const float maxDepth = ssrSlider[1].value*DEPTH_DIVISOR;
		const float reflectionAmount = 0.4f;
		const float albedoAmount = 1.f-reflectionAmount;

        for(int step=0; step < maxSteps; ++step){
            worldPixelPosition.x += reflDir.x * stepSize;
            worldPixelPosition.y += reflDir.y * stepSize;
            worldPixelPosition.z += reflDir.z * stepSize;

			fvec3 screenCoords = worldCoordinatesToScreenspace(worldPixelPosition, rotm, cam.pos, aspect_ratio, renderBuffers.width, renderBuffers.height);
			WORD screenX = (WORD)screenCoords.x;
			WORD screenY = (WORD)screenCoords.y;

            if(screenX >= renderBuffers.width || screenY >= renderBuffers.height) break;

			float depthDiff = screenCoords.z*DEPTH_DIVISOR-renderBuffers.depthBuffer[screenY*renderBuffers.width+screenX];
            if(depthDiff >= maxDepth) break;
			if(depthDiff >= 0){
                hit = true;
				DWORD currentColor = renderBuffers.frameBuffer[idx];
				DWORD reflectionColor = renderBuffers.frameBuffer[screenY*renderBuffers.width+screenX];
                renderBuffers.frameBuffer[idx] = RGBA(	R(currentColor)*albedoAmount+R(reflectionColor)*reflectionAmount,
														G(currentColor)*albedoAmount+G(reflectionColor)*reflectionAmount,
														B(currentColor)*albedoAmount+B(reflectionColor)*reflectionAmount);
                break;
            }
        }

        if(!hit) renderBuffers.frameBuffer[idx] = RGBA(0, 191, 255);
	}
}

void pointLightShader(RenderBuffers& renderBuffers)noexcept{
	fvec3 lightPos[] = {{0, -40, 0},{200, -160, 100}};
	DWORD totalBufferSize = renderBuffers.width*renderBuffers.height;
	for(DWORD i=0; i < totalBufferSize; ++i){
		fvec3 worldPixelPosition;
		worldPixelPosition.x = renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 5)];
		worldPixelPosition.y = renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 6)];
		worldPixelPosition.z = renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 7)];
		fvec3 worldNormal;
		worldNormal.x = renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 2)];
		worldNormal.y = renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 3)];
		worldNormal.z = renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 4)];
		float totalStrength = 0;
		for(int i=0; i < sizeof(lightPos)/sizeof(fvec3); ++i){
			fvec3 posDiff = {worldPixelPosition.x-lightPos[i].x, worldPixelPosition.y-lightPos[i].y, worldPixelPosition.z-lightPos[i].z};
			float distance = length(posDiff);
			float strength = clamp(1-(distance/240.f), 0.f, 1.f);
			strength *= clamp(dot(normalize(posDiff), worldNormal), 0.f, 1.f);
			totalStrength += strength;
		}
		totalStrength = clamp(totalStrength, 0.f, 1.f);
		DWORD color = renderBuffers.frameBuffer[i];
		renderBuffers.frameBuffer[i] = RGBA(R(color)*totalStrength, G(color)*totalStrength, B(color)*totalStrength);
	}
}

void depthBufferShader(RenderBuffers& renderBuffers)noexcept{
	for(DWORD i=0; i < renderBuffers.width*renderBuffers.height; ++i){
		float depth = renderBuffers.depthBuffer[i];
		BYTE color = depth/FLOAT_MAX*255;
		renderBuffers.frameBuffer[i] = RGBA(color, color, color);
	}
}

void normalBufferShader(RenderBuffers& renderBuffers)noexcept{
	DWORD totalBufferSize = renderBuffers.width*renderBuffers.height;
	for(DWORD i=0; i < totalBufferSize; ++i){
		BYTE red = (renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 2)]+1)*127.5;
		BYTE green = (renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 3)]+1)*127.5;
		BYTE blue = (renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 4)]+1)*127.5;
		renderBuffers.frameBuffer[i] = RGBA(red, green, blue);
	}
}

void drawTriangleModel(RenderBuffers& renderBuffers, TriangleModel& model, Image& defaultTexture)noexcept{
	rasterize(renderBuffers, model.triangles, model.attributesBuffer, model.attributesCount, 0, model.triangleCount, cam, vertexShader);
	if(model.material == nullptr) textureShader(renderBuffers, defaultTexture);
	else textureShader(renderBuffers, model.material->textures[0]);
}

void drawTriangleModelOutline(RenderBuffers& renderBuffers, TriangleModel& model)noexcept{
	rasterizeOutline(renderBuffers, model.triangles, model.attributesBuffer, model.attributesCount, 0, model.triangleCount, cam, vertexShader);
}

void drawDepthBuffer(RenderBuffers& renderBuffers, TriangleModel& model, Image& defaultTexture)noexcept{
	rasterize(renderBuffers, model.triangles, model.attributesBuffer, model.attributesCount, 0, model.triangleCount, cam, vertexShader);
	depthBufferShader(renderBuffers);
}

void drawNormalBuffer(RenderBuffers& renderBuffers, TriangleModel& model, Image& defaultTexture)noexcept{
	rasterize(renderBuffers, model.triangles, model.attributesBuffer, model.attributesCount, 0, model.triangleCount, cam, vertexShader);
	normalBufferShader(renderBuffers);
}

float pointScaling(WORD radius, float depth)noexcept{return max(radius-depth*0.005+0.005, 1.f);}

INT WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow){
	if(ErrCheck(initApp(), "App initialisieren") != ERR_SUCCESS) return -1;

	if(ErrCheck(createWindow(hInstance, 1000, 1000, 250, 0, 1, window, "3D!!!", mainWindowProc), "Fenster erstellen") != ERR_SUCCESS) return -1;
	if(ErrCheck(createRenderBuffers(renderBuffers, window.windowWidth*resolutionScale, window.windowHeight*resolutionScale, 8), "Renderbuffers erstellen") != ERR_SUCCESS) return -1;
	for(int i=0; i < sizeof(colorBuffers)/sizeof(Colorbuffer); ++i){
		if(ErrCheck(createColorbuffer(colorBuffers[i], renderBuffers.width, renderBuffers.height), "Colorbuffer erstellen") != ERR_SUCCESS) return -1;
	}
	for(int i=0; i < sizeof(dataBuffers)/sizeof(Floatbuffer); ++i){
		if(ErrCheck(createFloatbuffer(dataBuffers[i], renderBuffers.width, renderBuffers.height), "Floatbuffer erstellen") != ERR_SUCCESS) return -1;
	}

	if(ErrCheck(loadFont("fonts/asciiOutlined.tex", font, {82, 83}), "Font laden") != ERR_SUCCESS) return -1;
	font.font_size = 26;

	//TODO dynamisch
	#define MODELSTORAGECOUNT 50
	TriangleModel* models = new(std::nothrow) TriangleModel[MODELSTORAGECOUNT];
	for(int i=0; i < MODELSTORAGECOUNT; ++i) models[i].attributesCount = 8;
	DWORD modelCount = 0;
	Material* materials = new(std::nothrow) Material[MODELSTORAGECOUNT];
	DWORD materialCount = 0;
	if(!models || !materials){
		ErrCheck(ERR_BAD_ALLOC, "Konnte keinen Speicher für Modelle/Materials allokieren!");
		return -1;
	}

	Image defaultTexture;
	if(ErrCheck(loadImage("textures/basic.tex", defaultTexture), "Default Texture laden") != ERR_SUCCESS) return -1;

	// char filepath[MAX_PATH]{0};
	// ErrCheck(openExplorer(filepath, sizeof(filepath), "OBJ Wavefront Format .obj\0*.obj\0"), "Explorer öffnen");
	// std::cout << filepath << std::endl;

	if(ErrCheck(loadObj("objects/room.obj", models, modelCount, materials, materialCount, 3, 0, 0, 0, -60, -60, 60), "Modell laden") != ERR_SUCCESS) return -1;
	// if(ErrCheck(loadObj("objects/sponza.obj", models, modelCount, materials, materialCount, 3, 0, 0, 0, 5, -5, 5), "Modell laden") != ERR_SUCCESS) return -1;
	// if(ErrCheck(loadObj("objects/classroom_low_poly.obj", models, modelCount, materials, materialCount, 3, 0, 0, 0, -100, -100, 100), "Modell laden") != ERR_SUCCESS) return -1;
	#define POSITIONATTRIBUTEOFFSET 5
	for(DWORD i=0; i < modelCount; ++i){
		TriangleModel& model = models[i];
		for(DWORD j=0; j < model.triangleCount; ++j){
			for(BYTE k=0; k < 3; ++k){
				float* positionLocation = getAttrLoc(model, j, k, POSITIONATTRIBUTEOFFSET);
				positionLocation[0] = model.triangles[j].points[k].x;
				positionLocation[1] = model.triangles[j].points[k].y;
				positionLocation[2] = model.triangles[j].points[k].z;
			}
		}
	}

	RECT rect;
	GetWindowRect(window.handle, &rect);
	SetCursorPos(window.windowWidth/2+rect.left, window.windowHeight/2+rect.top);

	ivec2 buttonPos = {20, 20};
	ivec2 buttonSize = {160, 30};

	resolutionScaleSlider.pos = {buttonSize.x+20+20, 20};
	resolutionScaleSlider.size = {200, 6};
	resolutionScaleSlider.sliderRadius = 12;
	resolutionScaleSlider.minValue = 0.1;
	resolutionScaleSlider.maxValue = 2;
	resolutionScaleSlider.value = 0.5;
	resolutionScaleSlider.sliderPos = getFloatSliderPosFromValue(resolutionScaleSlider);

	settingsMenu.buttons[0].size = buttonSize;
	settingsMenu.buttons[0].pos = buttonPos;
	settingsMenu.buttons[0].event = setRenderMode;
	settingsMenu.buttons[0].text = "Wireframe";
	const RENDERMODES wireframeMode = RENDERMODE_WIREFRAME_MODE;
	settingsMenu.buttons[0].data = (void*)&wireframeMode;
	settingsMenu.buttons[0].textsize = 24;

	buttonPos.y += buttonSize.y+5;
	settingsMenu.buttons[1].size = buttonSize;
	settingsMenu.buttons[1].pos = buttonPos;
	settingsMenu.buttons[1].event = setRenderMode;
	settingsMenu.buttons[1].text = "Shaded";
	const RENDERMODES shadedMode = RENDERMODE_SHADED_MODE;
	settingsMenu.buttons[1].data = (void*)&shadedMode;
	settingsMenu.buttons[1].textsize = 24;

	buttonPos.y += buttonSize.y+5;
	settingsMenu.buttons[2].size = buttonSize;
	settingsMenu.buttons[2].pos = buttonPos;
	settingsMenu.buttons[2].event = setRenderMode;
	settingsMenu.buttons[2].text = "Depth";
	const RENDERMODES depthMode = RENDERMODE_DEPTH_MODE;
	settingsMenu.buttons[2].data = (void*)&depthMode;
	settingsMenu.buttons[2].textsize = 24;

	buttonPos.y += buttonSize.y+5;
	settingsMenu.buttons[3].size = buttonSize;
	settingsMenu.buttons[3].pos = buttonPos;
	settingsMenu.buttons[3].event = setRenderMode;
	settingsMenu.buttons[3].text = "Normals";
	const RENDERMODES normalMode = RENDERMODE_NORMAL_MODE;
	settingsMenu.buttons[3].data = (void*)&normalMode;
	settingsMenu.buttons[3].textsize = 24;

	buttonPos.y += buttonSize.y+5;
	settingsMenu.buttons[4].size = buttonSize;
	settingsMenu.buttons[4].pos = buttonPos;
	settingsMenu.buttons[4].event = setRenderMode;
	settingsMenu.buttons[4].text = "Shadowdepth";
	const RENDERMODES shadowDepthMode = RENDERMODE_SHADOW_DEPTH;
	settingsMenu.buttons[4].data = (void*)&shadowDepthMode;
	settingsMenu.buttons[4].textsize = 24;

	settingsMenu.buttonCount = 5;
	settingsMenu.size = {buttonSize.x, buttonPos.y+buttonSize.y};

	shadowSlider[0].size = {200, 6};
	shadowSlider[0].sliderRadius = 12;
	shadowSlider[0].minValue = -10;
	shadowSlider[0].maxValue = 10;
	shadowSlider[0].value = 3;
	shadowSlider[0].sliderPos = getFloatSliderPosFromValue(shadowSlider[0]);
	ssrSlider[0].size = {200, 6};
	ssrSlider[0].sliderRadius = 12;
	ssrSlider[0].value = 10;
	ssrSlider[0].sliderPos = getFloatSliderPosFromValue(ssrSlider[0]);
	ssrSlider[1].size = {200, 6};
	ssrSlider[1].sliderRadius = 12;
	ssrSlider[1].value = 20;
	ssrSlider[1].sliderPos = getFloatSliderPosFromValue(ssrSlider[1]);
	ssaoSlider[0].size = {200, 6};
	ssaoSlider[0].sliderRadius = 12;
	ssaoSlider[0].minValue = 1;
	ssaoSlider[0].maxValue = 64;
	ssaoSlider[0].value = 8;
	ssaoSlider[0].sliderPos = getFloatSliderPosFromValue(ssaoSlider[0]);
	ssaoSlider[1].size = {200, 6};
	ssaoSlider[1].sliderRadius = 12;
	ssaoSlider[1].minValue = 2;
	ssaoSlider[1].maxValue = 20;
	ssaoSlider[1].value = 8;
	ssaoSlider[1].sliderPos = getFloatSliderPosFromValue(ssaoSlider[1]);
	ssaoSlider[2].size = {200, 6};
	ssaoSlider[2].sliderRadius = 12;
	ssaoSlider[2].minValue = 0;
	ssaoSlider[2].maxValue = 10;
	ssaoSlider[2].value = 2;
	ssaoSlider[2].sliderPos = getFloatSliderPosFromValue(ssaoSlider[2]);
	ssaoSlider[3].size = {200, 6};
	ssaoSlider[3].sliderRadius = 12;
	ssaoSlider[3].minValue = 1;
	ssaoSlider[3].maxValue = 30;
	ssaoSlider[3].value = 13;
	ssaoSlider[3].sliderPos = getFloatSliderPosFromValue(ssaoSlider[3]);
	ssgiSlider[0].size = {200, 6};
	ssgiSlider[0].sliderRadius = 12;
	ssgiSlider[0].minValue = 4;
	ssgiSlider[0].maxValue = 64;
	ssgiSlider[0].value = 16;
	ssgiSlider[0].sliderPos = getFloatSliderPosFromValue(ssgiSlider[0]);
	ssgiSlider[1].size = {200, 6};
	ssgiSlider[1].sliderRadius = 12;
	ssgiSlider[1].minValue = 0;
	ssgiSlider[1].maxValue = 3;
	ssgiSlider[1].value = 1;
	ssgiSlider[1].sliderPos = getFloatSliderPosFromValue(ssgiSlider[1]);
	ssgiSlider[2].size = {200, 6};
	ssgiSlider[2].sliderRadius = 12;
	ssgiSlider[2].minValue = 0.5;
	ssgiSlider[2].maxValue = 12;
	ssgiSlider[2].value = 6;
	ssgiSlider[2].sliderPos = getFloatSliderPosFromValue(ssgiSlider[2]);
	ssgiSlider[3].size = {200, 6};
	ssgiSlider[3].sliderRadius = 12;
	ssgiSlider[3].minValue = 8;
	ssgiSlider[3].maxValue = 120;
	ssgiSlider[3].value = 40;
	ssgiSlider[3].sliderPos = getFloatSliderPosFromValue(ssgiSlider[3]);
	ssrcSlider[0].size = {200, 6};
	ssrcSlider[0].sliderRadius = 12;
	ssrcSlider[0].minValue = 8;
	ssrcSlider[0].maxValue = 512;
	ssrcSlider[0].value = 64;
	ssrcSlider[0].sliderPos = getFloatSliderPosFromValue(ssrcSlider[0]);
	ssrcSlider[1].size = {200, 6};
	ssrcSlider[1].sliderRadius = 12;
	ssrcSlider[1].minValue = 0;
	ssrcSlider[1].maxValue = 4;
	ssrcSlider[1].value = 1;
	ssrcSlider[1].sliderPos = getFloatSliderPosFromValue(ssrcSlider[1]);
	ssrcSlider[2].size = {200, 6};
	ssrcSlider[2].sliderRadius = 12;
	ssrcSlider[2].minValue = 0.5;
	ssrcSlider[2].maxValue = 12;
	ssrcSlider[2].value = 10;
	ssrcSlider[2].sliderPos = getFloatSliderPosFromValue(ssrcSlider[2]);
	ssrcSlider[3].size = {200, 6};
	ssrcSlider[3].sliderRadius = 12;
	ssrcSlider[3].minValue = 8;
	ssrcSlider[3].maxValue = 160;
	ssrcSlider[3].value = 80;
	ssrcSlider[3].sliderPos = getFloatSliderPosFromValue(ssrcSlider[3]);
	ssrcSlider[4].size = {200, 6};
	ssrcSlider[4].sliderRadius = 12;
	ssrcSlider[4].minValue = 4;
	ssrcSlider[4].maxValue = 512;
	ssrcSlider[4].value = 64;
	ssrcSlider[4].sliderPos = getFloatSliderPosFromValue(ssrcSlider[4]);
	ssrcSlider[5].size = {200, 6};
	ssrcSlider[5].sliderRadius = 12;
	ssrcSlider[5].minValue = 0.5;
	ssrcSlider[5].maxValue = 8;
	ssrcSlider[5].value = 2;
	ssrcSlider[5].sliderPos = getFloatSliderPosFromValue(ssrcSlider[5]);
	ssrcSlider[6].size = {200, 6};
	ssrcSlider[6].sliderRadius = 12;
	ssrcSlider[6].minValue = 1;
	ssrcSlider[6].maxValue = 80;
	ssrcSlider[6].value = 20;
	ssrcSlider[6].sliderPos = getFloatSliderPosFromValue(ssrcSlider[6]);

	for(int i=0; i < sizeof(checkboxes)/sizeof(Checkbox); ++i){
		checkboxes[i].data = (void*)&shadedMode;
		checkboxes[i].event = setRenderMode;
	}

	setRenderMode((void*)&shadedMode);

	for(WORD i=0; i < RANDOMNORMALSCOUNT; ++i){
		while(1){
			float a = nextrand()/2147483648.f-1.f;
			float b = nextrand()/2147483648.f-1.f;
			float c = nextrand()/2147483648.f-1.f;
			if(a*a+b*b+c*c <= 1){
				randomNormalVectorBuffer[i] = normalize({a, b, c});
				break;
			}
		}
	}

	Depthbuffer shadowDepthBuffer;
	if(ErrCheck(createDepthbuffer(shadowDepthBuffer, 1024, 1024), "Shadowmap erstellen") != ERR_SUCCESS) return -1;

	SDF sceneSdf;
	createSDF(sceneSdf, {-400, -500, -400}, {800, 550, 800}, 50, 50, 100);
	calculateSDFBounds(sceneSdf, models, modelCount, {20, 20, 20});
	setSDFValues(sceneSdf, 1000000.f);
	for(DWORD i=0; i < modelCount; ++i) calculateSDFFromMesh(sceneSdf, models[i]);
	float rotDir = 0;

	globalPoints = new ColorPoint[10000];

	while(_running){
		getMessages(window);
		resetData(_perfAnalyzer);
		resetTimer(_perfAnalyzer.timer[0]);
		resetTimer(_perfAnalyzer.timer[1]);
		float performancePreFilter;
		clearRenderBuffers(renderBuffers);
		clearDepthbuffer(shadowDepthBuffer);

		switch(renderMode){
			case RENDERMODE_WIREFRAME_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawTriangleModelOutline(renderBuffers, models[i]);
				break;
			}
			case RENDERMODE_SHADOW_DEPTH:{
				for(DWORD i=0; i < modelCount; ++i){
					rasterizeShadowMap(shadowDepthBuffer, models[i].triangles, 0, models[i].triangleCount, shadowCamera);
				}
				for(WORD y=0; y < renderBuffers.height; ++y){
					for(WORD x=0; x < renderBuffers.width; ++x){
						WORD uvx = (x*(shadowDepthBuffer.width-1))/renderBuffers.width;
						WORD uvy = (y*(shadowDepthBuffer.height-1))/renderBuffers.height;
						float depth = shadowDepthBuffer.data[uvy*shadowDepthBuffer.width+uvx];
						BYTE color = depth/FLOAT_MAX*255;
						renderBuffers.frameBuffer[y*renderBuffers.width+x] = RGBA(color, color, color);
					}
				}
				std::cout << shadowCamera.pos.x << ", " << shadowCamera.pos.y << ", " << shadowCamera.pos.z << std::endl;
				std::cout << shadowCamera.rot.x << ", " << shadowCamera.rot.y << std::endl;
				break;
			}
			case RENDERMODE_SHADED_MODE:{
				DWORD totalBufferSize = renderBuffers.width*renderBuffers.height;
				for(DWORD i=0; i < modelCount; ++i) drawTriangleModel(renderBuffers, models[i], defaultTexture);

				for(DWORD i=0; i < totalBufferSize; ++i) dataBuffers[1].data[i] = 0.f;
				if(getCheckBoxFlag(checkboxes[0], CHECKBOXFLAG_CHECKED)){
					for(DWORD i=0; i < modelCount; ++i) rasterizeShadowMap(shadowDepthBuffer, models[i].triangles, 0, models[i].triangleCount, shadowCamera);
					shadowMappingShader(renderBuffers, shadowDepthBuffer, cam, shadowCamera);
				}
				if(getCheckBoxFlag(checkboxes[1], CHECKBOXFLAG_CHECKED)) ssr(renderBuffers, cam);
				if(getCheckBoxFlag(checkboxes[2], CHECKBOXFLAG_CHECKED)){
					ssao(renderBuffers);
				}
				if(getCheckBoxFlag(checkboxes[3], CHECKBOXFLAG_CHECKED)){
					ssgi(renderBuffers, cam, 0, totalBufferSize);
				}
				if(getCheckBoxFlag(checkboxes[4], CHECKBOXFLAG_CHECKED)){
					ssrc(renderBuffers, cam, sceneSdf, shadowCamera.pos);
				}
				// boxBlur(dataBuffers[1].data, dataBuffers[1].width, dataBuffers[1].height, 3);
				for(DWORD i=0; i < totalBufferSize; ++i){
					float direct = dataBuffers[0].data[i];
					float indirect = dataBuffers[1].data[i];
					float strength = clamp(direct + indirect, 0.f, 1.f);
					DWORD color = renderBuffers.frameBuffer[i];
					renderBuffers.frameBuffer[i] = RGBA(R(color)*strength, G(color)*strength, B(color)*strength);
				}
				break;
			}
			case RENDERMODE_DEPTH_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawDepthBuffer(renderBuffers, models[i], defaultTexture);
				break;
			}
			case RENDERMODE_NORMAL_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawNormalBuffer(renderBuffers, models[i], defaultTexture);
				break;
			}
		}

		// drawSDF(renderBuffers, sceneSdf, 3, cam, pointScaling);
		// drawPoints(renderBuffers, globalPoints, 0, globalPointCount, 3, cam, pointScaling);

		copyImageToColorbuffer(window.framebuffer, {renderBuffers.width, renderBuffers.height, renderBuffers.frameBuffer}, 0, 0, window.windowWidth, window.windowHeight);

		std::string val = floatToString(getTimerMicros(_perfAnalyzer.timer[0])*0.001f) + "ms";
		drawFontString(window.framebuffer, font, val.c_str(), 5, 2);
		val = "Total Triangles: ";
		val += longToString(_perfAnalyzer.totalTriangles);
		drawFontString(window.framebuffer, font, val.c_str(), 5, font.font_size+4);
		val = "Drawn Triangles: ";
		val += longToString(_perfAnalyzer.drawnTriangles);
		drawFontString(window.framebuffer, font, val.c_str(), 5, font.font_size*2+6);
		val = "Pixels drawn: ";
		val += longToString(_perfAnalyzer.pixelsDrawn);
		drawFontString(window.framebuffer, font, val.c_str(), 5, font.font_size*3+8);
		val = "Pixels culled: ";
		val += longToString(_perfAnalyzer.pixelsCulled);
		drawFontString(window.framebuffer, font, val.c_str(), 5, font.font_size*4+10);
		val = "Punkteanzahl: ";
		val += longToString(globalPointCount);
		drawFontString(window.framebuffer, font, val.c_str(), 5, font.font_size*5+12);

		update(getTimerMillis(_perfAnalyzer.timer[0])+1);

		drawWindow(window);
		++frameCounter;
	}

	for(DWORD i=0; i < modelCount; ++i) destroyTriangleModel(models[i]);
	for(DWORD i=0; i < materialCount; ++i) destroyMaterial(materials[i]);
	for(int i=0; i < sizeof(colorBuffers)/sizeof(Colorbuffer); ++i) destroyColorbuffer(colorBuffers[i]);
	for(int i=0; i < sizeof(dataBuffers)/sizeof(Floatbuffer); ++i) destroyFloatbuffer(dataBuffers[i]);
	delete[] globalPoints;
	destroySDF(sceneSdf);
	destroyRenderBuffers(renderBuffers);
	destroyDepthbuffer(shadowDepthBuffer);
	destroyFont(font);
	destroyWindow(window);
	destroyApp();

	return 0;
}

void update(const float dt)noexcept{
	const float sin_rotx = sin(cam.rot.x);
	const float cos_rotx = cos(cam.rot.x);
	const float speedDt = SPEED*dt;
	if(!getMenuFlag(settingsMenu, MENUFLAG_OPEN)){

		if(renderMode != RENDERMODE_SHADOW_DEPTH){
			cam.pos.x -= getButton(keyboard, KEY_W)*sin_rotx*speedDt;
			cam.pos.z += getButton(keyboard, KEY_W)*cos_rotx*speedDt;
			cam.pos.x += getButton(keyboard, KEY_S)*sin_rotx*speedDt;
			cam.pos.z -= getButton(keyboard, KEY_S)*cos_rotx*speedDt;
			cam.pos.x += getButton(keyboard, KEY_D)*cos_rotx*speedDt;
			cam.pos.z += getButton(keyboard, KEY_D)*sin_rotx*speedDt;
			cam.pos.x -= getButton(keyboard, KEY_A)*cos_rotx*speedDt;
			cam.pos.z -= getButton(keyboard, KEY_A)*sin_rotx*speedDt;
			cam.pos.y -= getButton(keyboard, KEY_SPACE)*speedDt;
			cam.pos.y += getButton(keyboard, KEY_SHIFT)*speedDt;
		}else{
			shadowCamera.pos.x += getButton(keyboard, KEY_W)*speedDt;
			shadowCamera.pos.x -= getButton(keyboard, KEY_S)*speedDt;
			shadowCamera.pos.z += getButton(keyboard, KEY_A)*speedDt;
			shadowCamera.pos.z -= getButton(keyboard, KEY_D)*speedDt;
			shadowCamera.pos.y -= getButton(keyboard, KEY_SPACE)*speedDt;
			shadowCamera.pos.y += getButton(keyboard, KEY_SHIFT)*speedDt;
		}

		shadowCamera.rot.y -= getButton(keyboard, KEY_UP)*0.00025*dt;
		shadowCamera.rot.y += getButton(keyboard, KEY_DOWN)*0.00025*dt;
		shadowCamera.rot.x += getButton(keyboard, KEY_LEFT)*0.00025*dt;
		shadowCamera.rot.x -= getButton(keyboard, KEY_RIGHT)*0.00025*dt;
	}else{
		updateMenu(window.framebuffer, settingsMenu, font);
		float preResolutionScale = resolutionScale;
		updateFloatSliders(window.framebuffer, font, &resolutionScaleSlider, 1);
		resolutionScale = resolutionScaleSlider.value;
		if(resolutionScale != preResolutionScale){
			resizeRenderBuffers(renderBuffers, window.windowWidth*resolutionScale, window.windowHeight*resolutionScale);
			for(int i=0; i < sizeof(colorBuffers)/sizeof(Colorbuffer); ++i){
				destroyColorbuffer(colorBuffers[i]);
				createColorbuffer(colorBuffers[i], window.windowWidth*resolutionScale, window.windowHeight*resolutionScale);
			}
			for(int i=0; i < sizeof(dataBuffers)/sizeof(Floatbuffer); ++i){
				destroyFloatbuffer(dataBuffers[i]);
				createFloatbuffer(dataBuffers[i], window.windowWidth*resolutionScale, window.windowHeight*resolutionScale);
			}
		}
		if(renderMode == RENDERMODE_SHADED_MODE){
			updateCheckBoxs(window.framebuffer, font, checkboxes, sizeof(checkboxes)/sizeof(Checkbox));
			updateFloatSliders(window.framebuffer, font, shadowSlider, shadowSliderCount);
			updateFloatSliders(window.framebuffer, font, ssrSlider, ssrSliderCount);
			updateFloatSliders(window.framebuffer, font, ssaoSlider, ssaoSliderCount);
			updateFloatSliders(window.framebuffer, font, ssgiSlider, ssgiSliderCount);
			updateFloatSliders(window.framebuffer, font, ssrcSlider, ssrcSliderCount);
		}
	}
	if(getButton(mouse, MOUSEBUTTON_LMB)) setButton(mouse, MOUSEBUTTON_PREV_LMB);
	if(getButton(mouse, MOUSEBUTTON_RMB)) setButton(mouse, MOUSEBUTTON_PREV_RMB);
	if(!getButton(mouse, MOUSEBUTTON_LMB)) resetButton(mouse, MOUSEBUTTON_PREV_LMB);
	if(!getButton(mouse, MOUSEBUTTON_RMB)) resetButton(mouse, MOUSEBUTTON_PREV_RMB);
}

LRESULT CALLBACK mainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if(window == nullptr) return DefWindowProc(hwnd, uMsg, wParam, lParam);
	switch(uMsg){
		case WM_DESTROY:{
			_running = false;
			ErrCheck(setWindowFlag(*window, WINDOWFLAG_CLOSE), "setze close Fensterstatus");
			break;
		}
		case WM_SIZE:{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			if(!width || !height) break;
			// ErrCheck(setWindowFlag(window, WINDOW_RESIZE), "setzte resize Fensterstatus");
			ErrCheck(resizeWindow(*window, width, height, 1), "Fenster skalieren");
			ErrCheck(resizeRenderBuffers(renderBuffers, width*resolutionScale, height*resolutionScale), "Renderbuffers skalieren");
			for(int i=0; i < sizeof(colorBuffers)/sizeof(Colorbuffer); ++i){
				destroyColorbuffer(colorBuffers[i]);
				ErrCheck(createColorbuffer(colorBuffers[i], width*resolutionScale, height*resolutionScale), "Colorbuffer erstellen");
			}
			for(int i=0; i < sizeof(dataBuffers)/sizeof(Floatbuffer); ++i){
				destroyFloatbuffer(dataBuffers[i]);
				ErrCheck(createFloatbuffer(dataBuffers[i], width*resolutionScale, height*resolutionScale), "Floatbuffer erstellen");
			}
			break;
		}
		case WM_LBUTTONDOWN:{
			setButton(mouse, MOUSEBUTTON_LMB);
			break;
		}
		case WM_LBUTTONUP:{
			resetButton(mouse, MOUSEBUTTON_LMB);
			break;
		}
		case WM_RBUTTONDOWN:{
			setButton(mouse, MOUSEBUTTON_RMB);
			break;
		}
		case WM_RBUTTONUP:{
			resetButton(mouse, MOUSEBUTTON_RMB);
			break;
		}
		case WM_MOUSEMOVE:{
			tagPOINT m_pos;
			tagRECT w_pos;
			GetCursorPos(&m_pos);
			GetWindowRect(hwnd, &w_pos);
			POINT point = {0, 0};
			ClientToScreen(hwnd, &point);
			mouse.pos.x = (m_pos.x - w_pos.left)/window->pixelSize;
			mouse.pos.y = ((m_pos.y - w_pos.top)-(point.y-w_pos.top))/window->pixelSize;
				if(!getMenuFlag(settingsMenu, MENUFLAG_OPEN)){
					cam.rot.x -= ((float)m_pos.x-(window->windowWidth/2+w_pos.left)) * 0.001;
					cam.rot.y += ((float)m_pos.y-(window->windowHeight/2+w_pos.top)) * 0.001;
					cam.rot.y = clamp(cam.rot.y, -PI/2, PI/2);
					SetCursorPos(window->windowWidth/2+w_pos.left, window->windowHeight/2+w_pos.top);
				}
			break;
		}
		case WM_KEYDOWN:{
			switch(wParam){
			case 0x57:	//W
				setButton(keyboard, KEY_W);
				break;
			case 0x53:	//S
				setButton(keyboard, KEY_S);
				break;
			case 0x44:	//D
				setButton(keyboard, KEY_D);
				break;
			case 0x41:	//A
				setButton(keyboard, KEY_A);
				break;
			case VK_SPACE:
				setButton(keyboard, KEY_SPACE);
				break;
			case VK_SHIFT:
				setButton(keyboard, KEY_SHIFT);
				break;
			case VK_ESCAPE:
				setButton(keyboard, KEY_ESC);
				if(!getMenuFlag(settingsMenu, MENUFLAG_OPEN_TOGGLE)){
					setMenuFlag(settingsMenu, MENUFLAG_OPEN_TOGGLE);
					settingsMenu.flags ^= MENUFLAG_OPEN;			//Änder offen bit
					tagRECT w_pos;
					GetWindowRect(hwnd, &w_pos);
					if(!getMenuFlag(settingsMenu, MENUFLAG_OPEN)) SetCursorPos(window->windowWidth/2+w_pos.left, window->windowHeight/2+w_pos.top);
				}
				break;
			case VK_UP:
				setButton(keyboard, KEY_UP);
				break;
			case VK_DOWN:
				setButton(keyboard, KEY_DOWN);
				break;
			case VK_LEFT:
				setButton(keyboard, KEY_LEFT);
				break;
			case VK_RIGHT:
				setButton(keyboard, KEY_RIGHT);
				break;
			}
			return 0L;
		}
		case WM_KEYUP:{
			switch(wParam){
			case 0x57:	//W
				resetButton(keyboard, KEY_W);
				break;
			case 0x53:	//S
				resetButton(keyboard, KEY_S);
				break;
			case 0x44:	//D
				resetButton(keyboard, KEY_D);
				break;
			case 0x41:	//A
				resetButton(keyboard, KEY_A);
				break;
			case VK_SPACE:
				resetButton(keyboard, KEY_SPACE);
				break;
			case VK_SHIFT:
				resetButton(keyboard, KEY_SHIFT);
				break;
			case VK_ESCAPE:
				resetButton(keyboard, KEY_ESC);
				resetMenuFlag(settingsMenu, MENUFLAG_OPEN_TOGGLE);
				break;
			case VK_UP:
				resetButton(keyboard, KEY_UP);
				break;
			case VK_DOWN:
				resetButton(keyboard, KEY_DOWN);
				break;
			case VK_LEFT:
				resetButton(keyboard, KEY_LEFT);
				break;
			case VK_RIGHT:
				resetButton(keyboard, KEY_RIGHT);
				break;
			}
			return 0L;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
