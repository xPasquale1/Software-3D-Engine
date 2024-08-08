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
#define CULLBACKFACESSHADOW
// #define CULLFRONTFACESSHADOW
#include "graphics/window.h"

bool _running = true;
Camera cam = {1., {-293.917, -197.536, -18.5511}, {-1.493, 0.411999}};
Camera shadowCamera = {1., {-687.447, -543.786, -676.166}, {-0.794004, 0.469997}};

LRESULT mainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void update(float dt)noexcept;

enum RENDERMODES{
	RENDERMODE_SHADED_MODE,
	RENDERMODE_WIREFRAME_MODE,
	RENDERMODE_DEPTH_MODE,
	RENDERMODE_NORMAL_MODE,
	RENDERMODE_SSAO_MODE,
	RENDERMODE_SSR_MODE,
	RENDERMODE_SSGI_MODE
};

RENDERMODES renderMode;

Menu settingsMenu;
// Menü Funktionen
ErrCode setRenderMode(void* mode)noexcept{
	renderMode = *(RENDERMODES*)mode;
	return ERR_SUCCESS;
}

//Sliders
FloatSlider debugSlider[8];
WORD sliderCount = 0;

#define SPEED 0.25

Window window;
Font font;

RenderBuffers renderBuffers;
Colorbuffer colorBuffers[3];
float resolutionScale = 0.5;

DWORD frameCounter = 0;

fvec3 waterVertexShader(fvec3& point, float* attributes)noexcept{
	point.x += cos(attributes[5]*5+frameCounter*0.2)*20;
	point.y += sin(attributes[5]*5+frameCounter*0.2)*20;
	return point;
}

fvec3 vertexShader(fvec3& point, float* attributes)noexcept{
	return point;
}

void textureShader(RenderBuffers& renderBuffers, Image& image)noexcept{
	DWORD totalBufferSize = renderBuffers.width*renderBuffers.height;
	for(DWORD i=0; i < totalBufferSize; ++i){
		if(renderBuffers.fragmentFlags[i] == 0) continue;
		renderBuffers.fragmentFlags[i] = 0;
		float uvx = renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 0)];
		float uvy = renderBuffers.attributeBuffers[i+getAttrLoc(totalBufferSize, 1)];
		DWORD color = textureRepeated(image, uvx, uvy);
		if(A(color) < 120){
			renderBuffers.depthBuffer[i] = std::numeric_limits<float>::max();
			continue;
		}
		renderBuffers.frameBuffer[i] = color;
	}
}

#define RANDOMNORMALSCOUNT 211
fvec3 randomNormalVectorBuffer[RANDOMNORMALSCOUNT];

fvec3 generateRandomNormalInHemisphere(fvec3& normal)noexcept{
	fvec3 random = randomNormalVectorBuffer[nextrand()%RANDOMNORMALSCOUNT];
    if(dot(normal, random) < 0){
		random.x = -random.x;
		random.y = -random.y;
		random.z = -random.z;
	}
    return random;
}

fvec3 worldCoordinatesToScreenspace(fvec3& worldPixelPosition, float rotMat[3][3], float aspectRatio, WORD bufferWidth, WORD bufferHeight)noexcept{
	fvec3 cameraPosition = {worldPixelPosition.x-cam.pos.x, worldPixelPosition.y-cam.pos.y, worldPixelPosition.z-cam.pos.z};
	fvec3 screenPos = mulVec3Mat3x3(cameraPosition, rotMat);
    screenPos.x = (((screenPos.x*(cam.focal_length/screenPos.z)*aspectRatio)*0.5)+0.5)*bufferWidth;
    screenPos.y = (((screenPos.y*(cam.focal_length/screenPos.z))*0.5)+0.5)*bufferHeight;
	return screenPos;
}

fvec3 worldPosToViewSpaceOrtho(fvec3& worldPos, float rotMat[3][3], Camera& cam)noexcept{
	fvec3 cameraPosition = {worldPos.x-cam.pos.x, worldPos.y-cam.pos.y, worldPos.z-cam.pos.z};
	return mulVec3Mat3x3(cameraPosition, rotMat);
}

void shadowMappingShader(RenderBuffers& renderBuffers, RenderBuffers& shadowBuffers, Camera& sceneCam, Camera& shadowCam)noexcept{
	float rotm[3][3];
	float sin_rotx = sin(shadowCam.rot.x);
	float cos_rotx = cos(shadowCam.rot.x);
	float sin_roty = sin(shadowCam.rot.y);
	float cos_roty = cos(shadowCam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	const float aspectRatioShadow = (float)shadowBuffers.height/shadowBuffers.width;
	DWORD idx = 0;
	DWORD totalBufferSize = renderBuffers.width*renderBuffers.height;
	const fvec3 sunDir = mulVec3Mat3x3({0, -1, 0}, rotm);
	for(WORD y=0; y < renderBuffers.height; ++y){
		for(WORD x=0; x < renderBuffers.width; ++x, ++idx){
			fvec3 worldPixelPosition;
			worldPixelPosition.x = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 5)];
			worldPixelPosition.y = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 6)];
			worldPixelPosition.z = renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 7)];

			fvec3 worldNormal;
			worldNormal.x = -renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 2)];
			worldNormal.y = -renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 3)];
			worldNormal.z = -renderBuffers.attributeBuffers[idx+getAttrLoc(totalBufferSize, 4)];
			float lightStrength = clamp(dot(worldNormal, sunDir), 0.f, 1.f);
			lightStrength = 1;

			fvec3 viewPos = worldPosToViewSpaceOrtho(worldPixelPosition, rotm, shadowCam);
			DWORD uvx = ((viewPos.x*INVCLIPPINGREGIONSIZE*aspectRatioShadow*0.5)+0.5)*shadowBuffers.width;
			DWORD uvy = ((viewPos.y*INVCLIPPINGREGIONSIZE*0.5)+0.5)*shadowBuffers.height;
			
			if(uvx >= shadowBuffers.width || uvy >= shadowBuffers.height){
				renderBuffers.frameBuffer[idx] = RGBA(255, 0, 0);
				continue;
			}
			float shadowDepth = shadowBuffers.depthBuffer[uvy*shadowBuffers.width+uvx];

			if(viewPos.z*DEPTH_DIVISOR-debugSlider[7].value*DEPTH_DIVISOR > shadowDepth) renderBuffers.frameBuffer[idx] = RGBA(0, 0, 0);
			else renderBuffers.frameBuffer[idx] = RGBA(255*lightStrength, 255*lightStrength, 255*lightStrength);
		}
	}
}

//TODO Setzt vorraus, dass colorBuffers und renderBuffers die gleiche Auflösung haben
void ssgi(RenderBuffers& renderBuffers, Camera& cam)noexcept{
	float rotm[3][3];
	float aspect_ratio = (float)renderBuffers.height/renderBuffers.width;
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	float sin_roty = sin(cam.rot.y);
	float cos_roty = cos(cam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	
	const int ssgiSamples = 8;
	
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

		DWORD red = 0;
		DWORD green = 0;
		DWORD blue = 0;
		for(int i=0; i < ssgiSamples; ++i){
			fvec3 dir = generateRandomNormalInHemisphere(worldNormal);
			const float stepSize = 10;
			const int maxSteps = 60;
			const float maxDepth = 20*DEPTH_DIVISOR;
			fvec3 samplePos = worldPixelPosition;

			for(int step=0; step < maxSteps; ++step){
				samplePos.x -= dir.x * stepSize;
				samplePos.y -= dir.y * stepSize;
				samplePos.z -= dir.z * stepSize;

				fvec3 screenCoords = worldCoordinatesToScreenspace(samplePos, rotm, aspect_ratio, renderBuffers.width, renderBuffers.height);
				WORD screenX = (WORD)screenCoords.x;
				WORD screenY = (WORD)screenCoords.y;

				if(screenX >= renderBuffers.width || screenY >= renderBuffers.height) break;

				float depthDiff = screenCoords.z*DEPTH_DIVISOR-renderBuffers.depthBuffer[screenY*renderBuffers.width+screenX];
				if(depthDiff >= maxDepth) break;
				if(depthDiff > 0){
					DWORD reflectionColor = renderBuffers.frameBuffer[screenY*renderBuffers.width+screenX];
					red += R(reflectionColor);
					green += G(reflectionColor);
					blue += B(reflectionColor);
					break;
				}
			}
		}
		red /= ssgiSamples;
		green /= ssgiSamples;
		blue /= ssgiSamples;
		colorBuffers[0].data[idx] = RGBA(red, green, blue);
	}
}

//TODO Setzt vorraus, dass colorBuffers und renderBuffers die gleiche Auflösung haben
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
	
	const int ssaoSamples = debugSlider[3].value;
	const float ssaoMaxLength = debugSlider[4].value;
	const float minDepth = debugSlider[5].value*DEPTH_DIVISOR;
	const float maxDepth = debugSlider[6].value*DEPTH_DIVISOR;
	
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

			fvec3 screenCoords = worldCoordinatesToScreenspace(samplePoint, rotm, aspect_ratio, renderBuffers.width, renderBuffers.height);
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
		colorBuffers[0].data[idx] = R(colorBuffers[0].data[idx], 255*strength);
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
        const float stepSize = debugSlider[1].value;
        const int maxSteps = 120;
		const float maxDepth = debugSlider[2].value*DEPTH_DIVISOR;
		const float reflectionAmount = 0.4f;
		const float albedoAmount = 1.f-reflectionAmount;

        for(int step=0; step < maxSteps; ++step){
            worldPixelPosition.x += reflDir.x * stepSize;
            worldPixelPosition.y += reflDir.y * stepSize;
            worldPixelPosition.z += reflDir.z * stepSize;

			fvec3 screenCoords = worldCoordinatesToScreenspace(worldPixelPosition, rotm, aspect_ratio, renderBuffers.width, renderBuffers.height);
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
		BYTE color = depth/std::numeric_limits<float>::max()*255;
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

INT WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow){
	if(ErrCheck(initApp(), "App initialisieren") != ERR_SUCCESS) return -1;

	if(ErrCheck(createWindow(hInstance, 1000, 1000, 250, 0, 1, window, "3D!!!", mainWindowProc), "Fenster erstellen") != ERR_SUCCESS) return -1;
	if(ErrCheck(createRenderBuffers(renderBuffers, window.windowWidth*resolutionScale, window.windowHeight*resolutionScale, 8), "Renderbuffers erstellen") != ERR_SUCCESS) return -1;
	for(int i=0; i < sizeof(colorBuffers)/sizeof(Colorbuffer); ++i){
		if(ErrCheck(createColorbuffer(colorBuffers[i], renderBuffers.width, renderBuffers.height), "Colorbuffer erstellen") != ERR_SUCCESS) return -1;
	}

	if(ErrCheck(loadFont("fonts/asciiOutlined.tex", font, {82, 83}), "Font laden") != ERR_SUCCESS) return -1;
	font.font_size = 40/window.pixelSize;

	//TODO dynamisch
	#define MODELSTORAGECOUNT 1000
	TriangleModel* models = new(std::nothrow) TriangleModel[MODELSTORAGECOUNT];
	for(int i=0; i < MODELSTORAGECOUNT; ++i){
		models[i].attributesCount = 8;
	}
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

	if(ErrCheck(loadObj("objects/SSAO_test.obj", models, modelCount, materials, materialCount, 3, 0, 0, 0, 5, -5, 5), "Modell laden") != ERR_SUCCESS) return -1;
	// if(ErrCheck(loadObj("objects/classroom_low_poly.obj", models, modelCount, materials, materialCount, 3, 0, 0, 0, -100, -100, 100), "Modell laden") != ERR_SUCCESS) return -1;
	#define POSITIONATTRIBUTEOFFSET 5
	for(DWORD i=0; i < modelCount; ++i){
		TriangleModel& model = models[i];
		DWORD idx = 0;
		for(DWORD j=0; j < model.triangleCount; ++j, ++idx){
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

	ivec2 buttonPos = {20/window.pixelSize, 80/window.pixelSize};
	ivec2 buttonSize = {280/window.pixelSize, 40/window.pixelSize};

	settingsMenu.buttons[0].size = buttonSize;
	settingsMenu.buttons[0].pos = buttonPos;
	settingsMenu.buttons[0].event = setRenderMode;
	settingsMenu.buttons[0].text = "WIREFRAME";
	const RENDERMODES wireframeMode = RENDERMODE_WIREFRAME_MODE;
	settingsMenu.buttons[0].data = (void*)&wireframeMode;
	settingsMenu.buttons[0].textsize = 36/window.pixelSize;

	buttonPos.y += buttonSize.y+10/window.pixelSize;
	settingsMenu.buttons[1].size = buttonSize;
	settingsMenu.buttons[1].pos = buttonPos;
	settingsMenu.buttons[1].event = setRenderMode;
	settingsMenu.buttons[1].text = "SHADED";
	const RENDERMODES shadedMode = RENDERMODE_SHADED_MODE;
	settingsMenu.buttons[1].data = (void*)&shadedMode;
	settingsMenu.buttons[1].textsize = 36/window.pixelSize;

	buttonPos.y += buttonSize.y+10/window.pixelSize;
	settingsMenu.buttons[2].size = buttonSize;
	settingsMenu.buttons[2].pos = buttonPos;
	settingsMenu.buttons[2].event = setRenderMode;
	settingsMenu.buttons[2].text = "DEPTH";
	const RENDERMODES depthMode = RENDERMODE_DEPTH_MODE;
	settingsMenu.buttons[2].data = (void*)&depthMode;
	settingsMenu.buttons[2].textsize = 36/window.pixelSize;

	buttonPos.y += buttonSize.y+10/window.pixelSize;
	settingsMenu.buttons[3].size = buttonSize;
	settingsMenu.buttons[3].pos = buttonPos;
	settingsMenu.buttons[3].event = setRenderMode;
	settingsMenu.buttons[3].text = "NORMAL";
	const RENDERMODES normalMode = RENDERMODE_NORMAL_MODE;
	settingsMenu.buttons[3].data = (void*)&normalMode;
	settingsMenu.buttons[3].textsize = 36/window.pixelSize;

	buttonPos.y += buttonSize.y+10/window.pixelSize;
	settingsMenu.buttons[4].size = buttonSize;
	settingsMenu.buttons[4].pos = buttonPos;
	settingsMenu.buttons[4].event = setRenderMode;
	settingsMenu.buttons[4].text = "SSAO";
	const RENDERMODES button4Mode = RENDERMODE_SSAO_MODE;
	settingsMenu.buttons[4].data = (void*)&button4Mode;
	settingsMenu.buttons[4].textsize = 36/window.pixelSize;

	buttonPos.y += buttonSize.y+10/window.pixelSize;
	settingsMenu.buttons[5].size = buttonSize;
	settingsMenu.buttons[5].pos = buttonPos;
	settingsMenu.buttons[5].event = setRenderMode;
	settingsMenu.buttons[5].text = "SSR";
	const RENDERMODES button5Mode = RENDERMODE_SSR_MODE;
	settingsMenu.buttons[5].data = (void*)&button5Mode;
	settingsMenu.buttons[5].textsize = 36/window.pixelSize;

	buttonPos.y += buttonSize.y+10/window.pixelSize;
	settingsMenu.buttons[6].size = buttonSize;
	settingsMenu.buttons[6].pos = buttonPos;
	settingsMenu.buttons[6].event = setRenderMode;
	settingsMenu.buttons[6].text = "SSGI";
	const RENDERMODES button6Mode = RENDERMODE_SSGI_MODE;
	settingsMenu.buttons[6].data = (void*)&button6Mode;
	settingsMenu.buttons[6].textsize = 36/window.pixelSize;
	settingsMenu.buttonCount = 7;

	buttonPos.y += buttonSize.y+34;
	debugSlider[0].pos = {buttonPos.x, buttonPos.y};
	debugSlider[0].size = {200, 6};
	debugSlider[0].sliderRadius = 12;
	debugSlider[0].minValue = 0.1;
	debugSlider[0].maxValue = 2.0;
	debugSlider[0].value = 0.5;
	debugSlider[0].sliderPos = getFloatSliderPosFromValue(debugSlider[0]);

	buttonPos.y += debugSlider[0].sliderRadius*2+10;
	buttonPos.y += debugSlider[0].sliderRadius*2+10;
	debugSlider[1].pos = {buttonPos.x, buttonPos.y};
	debugSlider[1].size = {200, 6};
	debugSlider[1].sliderRadius = 12;
	debugSlider[1].value = 10;
	debugSlider[1].sliderPos = getFloatSliderPosFromValue(debugSlider[1]);
	buttonPos.y += debugSlider[0].sliderRadius*2+10;
	debugSlider[2].pos = {buttonPos.x, buttonPos.y};
	debugSlider[2].size = {200, 6};
	debugSlider[2].sliderRadius = 12;
	debugSlider[2].value = 20;
	debugSlider[2].sliderPos = getFloatSliderPosFromValue(debugSlider[2]);

	buttonPos.y += debugSlider[0].sliderRadius*2+10;
	buttonPos.y += debugSlider[0].sliderRadius*2+10;
	debugSlider[3].pos = {buttonPos.x, buttonPos.y};
	debugSlider[3].size = {200, 6};
	debugSlider[3].sliderRadius = 12;
	debugSlider[3].minValue = 1;
	debugSlider[3].maxValue = 64;
	debugSlider[3].value = 8;
	debugSlider[3].sliderPos = getFloatSliderPosFromValue(debugSlider[3]);
	buttonPos.y += debugSlider[0].sliderRadius*2+10;
	debugSlider[4].pos = {buttonPos.x, buttonPos.y};
	debugSlider[4].size = {200, 6};
	debugSlider[4].sliderRadius = 12;
	debugSlider[4].minValue = 2;
	debugSlider[4].maxValue = 20;
	debugSlider[4].value = 8;
	debugSlider[4].sliderPos = getFloatSliderPosFromValue(debugSlider[4]);
	buttonPos.y += debugSlider[0].sliderRadius*2+10;
	debugSlider[5].pos = {buttonPos.x, buttonPos.y};
	debugSlider[5].size = {200, 6};
	debugSlider[5].sliderRadius = 12;
	debugSlider[5].minValue = 0;
	debugSlider[5].maxValue = 10;
	debugSlider[5].value = 2;
	debugSlider[5].sliderPos = getFloatSliderPosFromValue(debugSlider[5]);
	buttonPos.y += debugSlider[0].sliderRadius*2+10;
	debugSlider[6].pos = {buttonPos.x, buttonPos.y};
	debugSlider[6].size = {200, 6};
	debugSlider[6].sliderRadius = 12;
	debugSlider[6].minValue = 1;
	debugSlider[6].maxValue = 30;
	debugSlider[6].value = 13;
	debugSlider[6].sliderPos = getFloatSliderPosFromValue(debugSlider[6]);

	buttonPos.y += debugSlider[0].sliderRadius*2+10;
	buttonPos.y += debugSlider[0].sliderRadius*2+10;
	debugSlider[7].pos = {buttonPos.x, buttonPos.y};
	debugSlider[7].size = {200, 6};
	debugSlider[7].sliderRadius = 12;
	debugSlider[7].minValue = -1;
	debugSlider[7].maxValue = 99;
	debugSlider[7].value = 0;
	debugSlider[7].sliderPos = getFloatSliderPosFromValue(debugSlider[7]);
	sliderCount = 8;

	for(WORD i=0; i < RANDOMNORMALSCOUNT; ++i){
		while(1){
			float a = nextrand()/2147483648.f-1;
			float b = nextrand()/2147483648.f-1;
			float c = nextrand()/2147483648.f-1;
			if(a*a+b*b+c*c <= 1){
				randomNormalVectorBuffer[i] = {a, b, c};
				break;
			}
		}
	}

	RenderBuffers shadowBuffers;
	if(ErrCheck(createRenderBuffers(shadowBuffers, 1024, 1024, 0), "Shadowmap erstellen") != ERR_SUCCESS) return -1;

	while(_running){
		getMessages(window);
		resetData(_perfAnalyzer);
		resetTimer(_perfAnalyzer.timer[0]);
		resetTimer(_perfAnalyzer.timer[1]);
		float performancePreFilter;
		clearRenderBuffers(renderBuffers);
		clearRenderBuffers(shadowBuffers);

		switch(renderMode){
			case RENDERMODE_WIREFRAME_MODE:{
				// for(DWORD i=0; i < modelCount; ++i) drawTriangleModelOutline(renderBuffers, models[i]);

				for(DWORD i=0; i < modelCount; ++i){
					rasterizeShadowMap(shadowBuffers, models[i].triangles, 0, models[i].triangleCount, shadowCamera);
				}
				for(WORD y=0; y < renderBuffers.height; ++y){
					for(WORD x=0; x < renderBuffers.width; ++x){
						WORD uvx = (x*(shadowBuffers.width-1))/renderBuffers.width;
						WORD uvy = (y*(shadowBuffers.height-1))/renderBuffers.height;
						float depth = shadowBuffers.depthBuffer[uvy*shadowBuffers.width+uvx];
						BYTE color = depth/std::numeric_limits<float>::max()*255;
						renderBuffers.frameBuffer[y*renderBuffers.width+x] = RGBA(color, color, color);
					}
				}
				// std::cout << shadowCamera.pos.x << ", " << shadowCamera.pos.y << ", " << shadowCamera.pos.z << std::endl;
				// std::cout << shadowCamera.rot.x << ", " << shadowCamera.rot.y << std::endl;
				break;
			}
			case RENDERMODE_SHADED_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawTriangleModel(renderBuffers, models[i], defaultTexture);
				for(DWORD i=0; i < modelCount; ++i){
					rasterizeShadowMap(shadowBuffers, models[i].triangles, 0, models[i].triangleCount, shadowCamera);
				}
				shadowMappingShader(renderBuffers, shadowBuffers, cam, shadowCamera);
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
			// #define SSAOFILTER
			case RENDERMODE_SSAO_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawTriangleModel(renderBuffers, models[i], defaultTexture);
				ssao(renderBuffers);
				performancePreFilter = getTimerMicros(_perfAnalyzer.timer[1])/1000.f;
				#ifdef SSAOFILTER
				for(DWORD y=0; y < renderBuffers.height; ++y){
					for(DWORD x=0; x < renderBuffers.width; ++x){
						DWORD idx = y*renderBuffers.width+x;
						colorBuffers[1].data[idx] = 0;
						for(int dy=-2; dy < 2; ++dy){
							for(int dx=-2; dx < 2; ++dx){
								DWORD sampleIdx = ((y+dy)*renderBuffers.width+dx+x)%(renderBuffers.width*renderBuffers.height);
								colorBuffers[1].data[idx] += R(colorBuffers[0].data[sampleIdx]);
							}
						}
						colorBuffers[0].data[idx] = R(colorBuffers[0].data[idx], colorBuffers[1].data[idx]/16);
					}
				}
				for(DWORD i=0; i < renderBuffers.width*renderBuffers.height; ++i){
					DWORD color = renderBuffers.frameBuffer[i];
					float factor = R(colorBuffers[0].data[i])/255.f;
					renderBuffers.frameBuffer[i] = RGBA(R(color)*factor, G(color)*factor, B(color)*factor);
				}
				#else
				for(DWORD i=0; i < renderBuffers.width*renderBuffers.height; ++i){
					DWORD color = renderBuffers.frameBuffer[i];
					float factor = (R(colorBuffers[0].data[i])+R(colorBuffers[1].data[i]))/510.f;
					colorBuffers[1].data[i] = colorBuffers[0].data[i];
					renderBuffers.frameBuffer[i] = RGBA(R(color)*factor, G(color)*factor, B(color)*factor);
				}
				#endif
				break;
			}
			case RENDERMODE_SSR_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawTriangleModel(renderBuffers, models[i], defaultTexture);
				ssr(renderBuffers, cam);
				break;
			}
			case RENDERMODE_SSGI_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawTriangleModel(renderBuffers, models[i], defaultTexture);
				// pointLightShader(renderBuffers);
				ssgi(renderBuffers, cam);
				for(DWORD i=0; i < renderBuffers.width*renderBuffers.height; ++i){
					renderBuffers.frameBuffer[i] = colorBuffers[0].data[i];
				}
				break;
			}
		}

		Image dummyImage;
		dummyImage.width = renderBuffers.width;
		dummyImage.height = renderBuffers.height;
		dummyImage.data = renderBuffers.frameBuffer;
		copyImageToColorbuffer(window.framebuffer, dummyImage, 0, 0, window.windowWidth, window.windowHeight);

		std::string val = floatToString(getTimerMicros(_perfAnalyzer.timer[0])*0.001f) + "ms";
		drawFontString(window.framebuffer, font, val.c_str(), 5, 2);
		val = floatToString(performancePreFilter) + "ms";
		drawFontString(window.framebuffer, font, val.c_str(), 5, font.font_size+4);
		val = longToString(_perfAnalyzer.totalTriangles);
		drawFontString(window.framebuffer, font, val.c_str(), 5, font.font_size*2+6);
		val = longToString(_perfAnalyzer.drawnTriangles);
		drawFontString(window.framebuffer, font, val.c_str(), 5, font.font_size*3+8);
		val = longToString(_perfAnalyzer.pixelsDrawn);
		drawFontString(window.framebuffer, font, val.c_str(), 5, font.font_size*4+10);
		val = longToString(_perfAnalyzer.pixelsCulled);
		drawFontString(window.framebuffer, font, val.c_str(), 5, font.font_size*5+12);
		val = longToString(_perfAnalyzer.pointlessTriangles);
		drawFontString(window.framebuffer, font, val.c_str(), 5, font.font_size*6+14);

		update(getTimerMillis(_perfAnalyzer.timer[0])+1);

		drawWindow(window);
		++frameCounter;
	}

	for(DWORD i=0; i < modelCount; ++i) destroyTriangleModel(models[i]);
	for(DWORD i=0; i < materialCount; ++i) destroyMaterial(materials[i]);
	for(int i=0; i < sizeof(colorBuffers)/sizeof(Colorbuffer); ++i) destroyColorbuffer(colorBuffers[i]);
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

		if(renderMode != RENDERMODE_WIREFRAME_MODE){
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
		updateFloatSliders(window.framebuffer, font, debugSlider, sliderCount);
		resolutionScale = debugSlider[0].value;
		if(resolutionScale != preResolutionScale){
			resizeRenderBuffers(renderBuffers, window.windowWidth*resolutionScale, window.windowHeight*resolutionScale);
			for(int i=0; i < sizeof(colorBuffers)/sizeof(Colorbuffer); ++i){
				destroyColorbuffer(colorBuffers[i]);
				createColorbuffer(colorBuffers[i], window.windowWidth*resolutionScale, window.windowHeight*resolutionScale);
			}
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
