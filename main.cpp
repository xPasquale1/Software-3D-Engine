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
*/

#define PIXELSIZE 2
// #define NEWTRIANGLEDRAWINGALGORITHM
#include "graphics/window.h"

bool _running = true;
Camera cam = {1., {-293.917, -197.536, -18.5511}, {-1.493, 0.411999}};

LRESULT mainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void update(float dt)noexcept;

enum RENDERMODE{
	SHADED_MODE,
	WIREFRAME_MODE,
	DEPTH_MODE,
	NORMAL_MODE,
	SSAO_MODE,
	SSR_MODE
};

RENDERMODE renderMode;

Menu settingsMenu;
// Menü Funktionen
ErrCode setRenderMode(void* mode)noexcept{
	renderMode = *(RENDERMODE*)mode;
	return SUCCESS;
}

//Sliders
FloatSlider debugSlider[2];
WORD sliderCount = 0;

#define SPEED 0.25

Window window;
Font font;

Colorbuffer colorBuffers[3];

DWORD frameCounter = 0;

fvec3 vertexShader(fvec3& point)noexcept{
	return point;
}

//TODO Geht davon aus, dass Framebuffer und Depthbuffer die gleiche Größe haben
void textureShader(Colorbuffer& colorBuffer, fvec4** attributeBuffers, BYTE* fragmentFlags, Image& image)noexcept{
	for(DWORD i=0; i < colorBuffer.width*colorBuffer.height; ++i){
		if(fragmentFlags[i] == 0) continue;
		fragmentFlags[i] = 0;
		float uvx = attributeBuffers[0][i].x;
		float uvy = attributeBuffers[0][i].y;
		DWORD color = textureRepeated(image, uvx, uvy);
		if(A(color) < 120){
			window.depth.data[i] = 0xFF'FF'FF'FF;
			continue;
		}
		colorBuffer.data[i] = color;
	}
}

#define RANDOMNORMALSCOUNT 256
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

uivec3 worldCoordinatesToScreenspace(fvec3& worldPixelPosition, float rotMat[3][3], float aspectRatio, WORD bufferWidth, WORD bufferHeight)noexcept{
	fvec3 cameraPosition = {worldPixelPosition.x-cam.pos.x, worldPixelPosition.y-cam.pos.y, worldPixelPosition.z-cam.pos.z};
	fvec3 screenPos = mulVec3Mat3x3(cameraPosition, rotMat);
	uivec3 screenCoords;
    screenCoords.x = (((screenPos.x*(cam.focal_length/screenPos.z)*aspectRatio)*0.5)+0.5)*bufferWidth;
    screenCoords.y = (((screenPos.y*(cam.focal_length/screenPos.z))*0.5)+0.5)*bufferHeight;
	screenCoords.z = screenPos.z;
	return screenCoords;
}

//TODO geht davon aus, dass der Framebuffer, Depthbuffer und die Colorbuffer die gleiche Größe haben
void ssao(Colorbuffer& colorBuffer, Depthbuffer& depthBuffer, fvec4** attributeBuffers)noexcept{
	float rotm[3][3];
	float aspect_ratio = (float)colorBuffer.height/colorBuffer.width;
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	float sin_roty = sin(cam.rot.y);
	float cos_roty = cos(cam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	DWORD idx = 0;
	for(WORD y=0; y < colorBuffer.width; ++y){
		for(WORD x=0; x < colorBuffer.height; ++x, ++idx){
			fvec3 worldNormal;
			worldNormal.x = attributeBuffers[1][idx].x;
			worldNormal.y = attributeBuffers[1][idx].y;
			worldNormal.z = attributeBuffers[1][idx].z;

			fvec3 worldPixelPosition;
			worldPixelPosition.x = attributeBuffers[2][idx].x;
			worldPixelPosition.y = attributeBuffers[2][idx].y;
			worldPixelPosition.z = attributeBuffers[2][idx].z;

			const int ssaoSamples = 16;
			const float ssaoMaxLength = 10;
			const SDWORD minDepth = 2*DEPTH_DIVISOR;
			const SDWORD maxDepth = 12*DEPTH_DIVISOR;
			bool outside = false;

			float strength = 0;
			for(int i=0; i < ssaoSamples; ++i){
				fvec3 dir = generateRandomNormalInHemisphere(worldNormal);
				float length = nextrand()/4294967294.f;
				length = -lerp(0.1f, 1.f, length*length);
				dir.x *= length*ssaoMaxLength;
				dir.y *= length*ssaoMaxLength;
				dir.z *= length*ssaoMaxLength;
				fvec3 samplePoint = {worldPixelPosition.x+dir.x, worldPixelPosition.y+dir.y, worldPixelPosition.z+dir.z};

				uivec3 screenCoords = worldCoordinatesToScreenspace(samplePoint, rotm, aspect_ratio, colorBuffer.width, colorBuffer.height);

				if(screenCoords.x >= colorBuffer.width || screenCoords.y >= colorBuffer.height){
					strength++;
					continue;
				}

				SDWORD depthDiff = screenCoords.z*DEPTH_DIVISOR-depthBuffer.data[screenCoords.y*depthBuffer.width+screenCoords.x];
				if(depthDiff <= minDepth || depthDiff >= maxDepth) strength++;
			}
			strength /= ssaoSamples;
			colorBuffers[0].data[idx] = R(colorBuffers[0].data[idx], 255*strength);
		}
	}
}

//TODO geht davon aus, dass der Framebuffer, Depthbuffer und die Colorbuffer die gleiche Größe haben
void ssr(Colorbuffer& colorBuffer, Depthbuffer& depthBuffer, fvec4** attributeBuffers, Camera& cam)noexcept{
	float rotm[3][3];
	float aspect_ratio = (float)colorBuffer.height/colorBuffer.width;
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	float sin_roty = sin(cam.rot.y);
	float cos_roty = cos(cam.rot.y);
    rotm[0][0] = cos_rotx; 				rotm[0][1] = 0; 		rotm[0][2] = sin_rotx;
    rotm[1][0] = sin_rotx*sin_roty;		rotm[1][1] = cos_roty; 	rotm[1][2] = -sin_roty*cos_rotx;
    rotm[2][0] = -sin_rotx*cos_roty;	rotm[2][1] = sin_roty; 	rotm[2][2] = cos_rotx*cos_roty;
	DWORD idx = 0;
	for(WORD y=0; y < colorBuffer.height; ++y){
		for(WORD x=0; x < colorBuffer.width; ++x, ++idx){
			fvec3 worldNormal;
			worldNormal.x = attributeBuffers[1][idx].x;
			worldNormal.y = attributeBuffers[1][idx].y;
			worldNormal.z = attributeBuffers[1][idx].z;
			if(worldNormal.y < 0.99) continue;

			fvec3 worldPixelPosition;
			worldPixelPosition.x = attributeBuffers[2][idx].x;
			worldPixelPosition.y = attributeBuffers[2][idx].y;
			worldPixelPosition.z = attributeBuffers[2][idx].z;

			fvec3 viewDir = {worldPixelPosition.x-cam.pos.x, worldPixelPosition.y-cam.pos.y, worldPixelPosition.z-cam.pos.z};
			viewDir = normalize(viewDir);

			fvec3 reflDir = reflect(viewDir, worldNormal);

            bool hit = false;
            const float stepSize = debugSlider[0].value;
            const int maxSteps = 120;
			const float maxDepth = debugSlider[1].value*DEPTH_DIVISOR;
			const float reflectionAmount = 0.4f;
			const float albedoAmount = 1.f-reflectionAmount;

            for(int step=0; step < maxSteps; ++step){
                worldPixelPosition.x += reflDir.x * stepSize;
                worldPixelPosition.y += reflDir.y * stepSize;
                worldPixelPosition.z += reflDir.z * stepSize;

				uivec3 screenCoords = worldCoordinatesToScreenspace(worldPixelPosition, rotm, aspect_ratio, colorBuffer.width, colorBuffer.height);

                if(screenCoords.x >= colorBuffer.width || screenCoords.y >= colorBuffer.height) break;

				SDWORD depthDiff = screenCoords.z*DEPTH_DIVISOR-depthBuffer.data[screenCoords.y*depthBuffer.width+screenCoords.x];
                if(depthDiff >= maxDepth) break;
				if(depthDiff > 0){
                    hit = true;
					DWORD currentColor = colorBuffer.data[idx];
					DWORD reflectionColor = colorBuffer.data[screenCoords.y*colorBuffer.width+screenCoords.x];
                    colorBuffer.data[idx] = RGBA(	R(currentColor)*albedoAmount+R(reflectionColor)*reflectionAmount,
												G(currentColor)*albedoAmount+G(reflectionColor)*reflectionAmount,
												B(currentColor)*albedoAmount+B(reflectionColor)*reflectionAmount);
                    break;
                }
            }

            if(!hit) colorBuffer.data[idx] = RGBA(0, 191, 255);
		}
	}
}

void pointLightShader(Colorbuffer& colorBuffer, fvec4** attributeBuffers)noexcept{
	fvec3 lightPos[] = {{0, -40, 0},{200, -160, 100}};
	for(DWORD i=0; i < colorBuffer.width*colorBuffer.height; ++i){
		fvec3 worldPixelPosition;
		worldPixelPosition.x = attributeBuffers[2][i].x;
		worldPixelPosition.y = attributeBuffers[2][i].y;
		worldPixelPosition.z = attributeBuffers[2][i].z;
		fvec3 worldNormal;
		worldNormal.x = attributeBuffers[1][i].x;
		worldNormal.y = attributeBuffers[1][i].y;
		worldNormal.z = attributeBuffers[1][i].z;
		float totalStrength = 0;
		for(int i=0; i < sizeof(lightPos)/sizeof(fvec3); ++i){
			fvec3 posDiff = {worldPixelPosition.x-lightPos[i].x, worldPixelPosition.y-lightPos[i].y, worldPixelPosition.z-lightPos[i].z};
			float distance = length(posDiff);
			float strength = clamp(1-(distance/240.f), 0.f, 1.f);
			strength *= clamp(dot(normalize(posDiff), worldNormal), 0.f, 1.f);
			totalStrength += strength;
		}
		totalStrength = clamp(totalStrength, 0.f, 1.f);
		DWORD color = colorBuffer.data[i];
		colorBuffer.data[i] = RGBA(R(color)*totalStrength, G(color)*totalStrength, B(color)*totalStrength);
	}
}

//TODO Geht davon aus, dass der Framebuffer und der Depthbuffer die gleiche Größe haben
void depthBufferShader(Colorbuffer& colorBuffer, Depthbuffer& depthBuffer)noexcept{
	for(DWORD i=0; i < colorBuffer.width*colorBuffer.height; ++i){
		QWORD depth = depthBuffer.data[i]*255;
		BYTE color = depth/0xFFFFFF;
		colorBuffer.data[i] = RGBA(color, color, color);
	}
}

void normalBufferShader(Colorbuffer& colorBuffer, fvec4** attributeBuffers)noexcept{
	for(DWORD i=0; i < colorBuffer.width*colorBuffer.height; ++i){
		fvec3 n;
		n.x = attributeBuffers[1][i].x;
		n.y = attributeBuffers[1][i].y;
		n.z = attributeBuffers[1][i].z;
		colorBuffer.data[i] = RGBA((n.x+1)*127.5, (n.y+1)*127.5, (n.z+1)*127.5);
	}
}

void drawTriangleModel(Colorbuffer& colorBuffer, Depthbuffer& depthBuffer, fvec4** attributeBuffers, BYTE attributeBuffersCount, BYTE* fragmentFlags, TriangleModel& model, Image& defaultTexture)noexcept{
	rasterize(colorBuffer, depthBuffer, attributeBuffers, attributeBuffersCount, fragmentFlags, model.triangles, 0, model.triangleCount, cam, vertexShader);
	if(model.material == nullptr) textureShader(colorBuffer, attributeBuffers, fragmentFlags, defaultTexture);
	else textureShader(colorBuffer, attributeBuffers, fragmentFlags, model.material->textures[0]);
}

void drawTriangleModelOutline(Colorbuffer& colorBuffer, Depthbuffer& depthBuffer, fvec4** attributeBuffers, BYTE attributeBuffersCount, BYTE* fragmentFlags, TriangleModel& model)noexcept{
	rasterizeOutline(colorBuffer, depthBuffer, attributeBuffers, attributeBuffersCount, fragmentFlags, model.triangles, 0, model.triangleCount, cam);
}

void drawDepthBuffer(Colorbuffer& colorBuffer, Depthbuffer& depthBuffer, fvec4** attributeBuffers, BYTE attributeBuffersCount, BYTE* fragmentFlags, TriangleModel& model, Image& defaultTexture)noexcept{
	rasterize(colorBuffer, depthBuffer, attributeBuffers, attributeBuffersCount, fragmentFlags, model.triangles, 0, model.triangleCount, cam, vertexShader);
	depthBufferShader(colorBuffer, depthBuffer);
}

void drawNormalBuffer(Colorbuffer& colorBuffer, Depthbuffer& depthBuffer, fvec4** attributeBuffers, BYTE attributeBuffersCount, BYTE* fragmentFlags, TriangleModel& model, Image& defaultTexture)noexcept{
	rasterize(colorBuffer, depthBuffer, attributeBuffers, attributeBuffersCount, fragmentFlags, model.triangles, 0, model.triangleCount, cam, vertexShader);
	normalBufferShader(colorBuffer, attributeBuffers);
}

INT WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow){
	if(ErrCheck(initApp(), "App initialisieren") != SUCCESS) return -1;

	if(ErrCheck(createWindow(hInstance, 1000, 1000, 250, 0, PIXELSIZE, window, "3D!!!", mainWindowProc), "Fenster erstellen") != SUCCESS) return -1;
	if(ErrCheck(assignAttributeBuffers(window, 3), "AttributeBuffer hinzufügen") != SUCCESS) return -1;
	for(int i=0; i < sizeof(colorBuffers)/sizeof(Colorbuffer); ++i){
		if(ErrCheck(createColorbuffer(colorBuffers[i], window.windowWidth, window.windowHeight), "Colorbuffer erstellen") != SUCCESS) return -1;
	}

	if(ErrCheck(loadFont("fonts/asciiOutlined.tex", font, {82, 83}), "Font laden") != SUCCESS) return -1;
	font.font_size = 40/window.pixelSize;

	//TODO dynamisch
	TriangleModel* models = new(std::nothrow) TriangleModel[40];
	DWORD modelCount = 0;
	Material* materials = new(std::nothrow) Material[40];
	DWORD materialCount = 0;
	if(!models || !materials){
		ErrCheck(BAD_ALLOC, "Konnte keinen Speicher für Modelle/Materials allokieren!");
		return -1;
	}

	Image defaultTexture;
	if(ErrCheck(loadImage("textures/basic.tex", defaultTexture), "Default Texture laden") != SUCCESS) return -1;

	if(ErrCheck(loadObj("objects/sponza.obj", models, modelCount, materials, materialCount, 3, 0, 0, 0, -4.5), "Modell laden") != SUCCESS) return -1;
	// if(ErrCheck(loadObj("objects/terrain1_optimized.obj", models, modelCount, materials, materialCount, 2, 0, 0, 0, 10), "Modell laden") != SUCCESS) return -1;
	for(DWORD i=0; i < modelCount; ++i){
		TriangleModel& model = models[i];
		for(DWORD j=0; j < model.triangleCount; ++j){
			for(BYTE k=0; k < 3; ++k){
				model.triangles[j].attribute[2][k].x = model.triangles[j].points[k].x;
				model.triangles[j].attribute[2][k].y = model.triangles[j].points[k].y;
				model.triangles[j].attribute[2][k].z = model.triangles[j].points[k].z;
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
	const RENDERMODE wireframeMode = WIREFRAME_MODE;
	settingsMenu.buttons[0].data = (void*)&wireframeMode;
	settingsMenu.buttons[0].textsize = 36/window.pixelSize;

	buttonPos.y += buttonSize.y+10/window.pixelSize;
	settingsMenu.buttons[1].size = buttonSize;
	settingsMenu.buttons[1].pos = buttonPos;
	settingsMenu.buttons[1].event = setRenderMode;
	settingsMenu.buttons[1].text = "SHADED";
	const RENDERMODE shadedMode = SHADED_MODE;
	settingsMenu.buttons[1].data = (void*)&shadedMode;
	settingsMenu.buttons[1].textsize = 36/window.pixelSize;

	buttonPos.y += buttonSize.y+10/window.pixelSize;
	settingsMenu.buttons[2].size = buttonSize;
	settingsMenu.buttons[2].pos = buttonPos;
	settingsMenu.buttons[2].event = setRenderMode;
	settingsMenu.buttons[2].text = "DEPTH";
	const RENDERMODE depthMode = DEPTH_MODE;
	settingsMenu.buttons[2].data = (void*)&depthMode;
	settingsMenu.buttons[2].textsize = 36/window.pixelSize;

	buttonPos.y += buttonSize.y+10/window.pixelSize;
	settingsMenu.buttons[3].size = buttonSize;
	settingsMenu.buttons[3].pos = buttonPos;
	settingsMenu.buttons[3].event = setRenderMode;
	settingsMenu.buttons[3].text = "NORMAL";
	const RENDERMODE normalMode = NORMAL_MODE;
	settingsMenu.buttons[3].data = (void*)&normalMode;
	settingsMenu.buttons[3].textsize = 36/window.pixelSize;

	buttonPos.y += buttonSize.y+10/window.pixelSize;
	settingsMenu.buttons[4].size = buttonSize;
	settingsMenu.buttons[4].pos = buttonPos;
	settingsMenu.buttons[4].event = setRenderMode;
	settingsMenu.buttons[4].text = "SSAO";
	const RENDERMODE button4Mode = SSAO_MODE;
	settingsMenu.buttons[4].data = (void*)&button4Mode;
	settingsMenu.buttons[4].textsize = 36/window.pixelSize;
	settingsMenu.buttonCount = 5;

	buttonPos.y += buttonSize.y+10/window.pixelSize;
	settingsMenu.buttons[5].size = buttonSize;
	settingsMenu.buttons[5].pos = buttonPos;
	settingsMenu.buttons[5].event = setRenderMode;
	settingsMenu.buttons[5].text = "SSR";
	const RENDERMODE button5Mode = SSR_MODE;
	settingsMenu.buttons[5].data = (void*)&button5Mode;
	settingsMenu.buttons[5].textsize = 36/window.pixelSize;
	settingsMenu.buttonCount = 6;

	debugSlider[0].pos = {buttonPos.x, buttonPos.y+buttonSize.y+debugSlider[0].sliderRadius+10/window.pixelSize};
	debugSlider[0].sliderPos = 10;
	debugSlider[0].value = 10;
	debugSlider[1].pos = {buttonPos.x, buttonPos.y+buttonSize.y+debugSlider[1].sliderRadius+10/window.pixelSize*2+debugSlider[0].sliderRadius*2};
	debugSlider[1].sliderPos = 20;
	debugSlider[1].value = 20;
	sliderCount = 2;

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

	while(_running){
		getMessages(window);
		resetData(_perfAnalyzer);
		resetTimer(_perfAnalyzer.timer[0]);
		resetTimer(_perfAnalyzer.timer[1]);
		float performancePreFilter;
		clearWindow(window, RGBA(0, 0, 0));

#ifdef THREADING

#else
		switch(renderMode){
			case WIREFRAME_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawTriangleModelOutline(window.framebuffer, window.depth, window.attributeBuffers, window.attributeBuffersCount, window.fragmentFlag, models[i]);
				break;
			}
			case SHADED_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawTriangleModel(window.framebuffer, window.depth, window.attributeBuffers, window.attributeBuffersCount, window.fragmentFlag, models[i], defaultTexture);
				pointLightShader(window.framebuffer, window.attributeBuffers);
				break;
			}
			case DEPTH_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawDepthBuffer(window.framebuffer, window.depth, window.attributeBuffers, window.attributeBuffersCount, window.fragmentFlag, models[i], defaultTexture);
				break;
			}
			case NORMAL_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawNormalBuffer(window.framebuffer, window.depth, window.attributeBuffers, window.attributeBuffersCount, window.fragmentFlag, models[i], defaultTexture);
				break;
			}
			// #define SSAOFILTER
			case SSAO_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawTriangleModel(window.framebuffer, window.depth, window.attributeBuffers, window.attributeBuffersCount, window.fragmentFlag, models[i], defaultTexture);
				ssao(window.framebuffer, window.depth, window.attributeBuffers);
				performancePreFilter = getTimerMicros(_perfAnalyzer.timer[1])/1000.f;
				#ifdef SSAOFILTER
				for(DWORD y=0; y < bufferHeight; ++y){
					for(DWORD x=0; x < bufferWidth; ++x){
						DWORD idx = y*bufferWidth+x;
						window.buffers[1][idx] = 0;
						for(int dy=-2; dy < 2; ++dy){
							for(int dx=-2; dx < 2; ++dx){
								DWORD sampleIdx = ((y+dy)*bufferWidth+dx+x)%(bufferWidth*bufferHeight);
								window.buffers[1][idx] += R(window.buffers[0][sampleIdx]);
							}
						}
						window.buffers[0][idx] = G(window.buffers[0][idx], window.buffers[1][idx]/16);
					}
				}
				for(DWORD i=0; i < bufferWidth*bufferHeight; ++i){
					DWORD color = window.pixels[i];
					float factor = G(window.buffers[0][i])/255.f;
					window.pixels[i] = RGBA(R(color)*factor, G(color)*factor, B(color)*factor);
				}
				#else
				for(DWORD i=0; i < window.framebuffer.width*window.framebuffer.height; ++i){
					DWORD color = window.framebuffer.data[i];
					float factor = R(colorBuffers[0].data[i])/255.f;
					window.framebuffer.data[i] = RGBA(R(color)*factor, G(color)*factor, B(color)*factor);
				}
				#endif
				break;
			}
			case SSR_MODE:{
				for(DWORD i=0; i < modelCount; ++i) drawTriangleModel(window.framebuffer, window.depth, window.attributeBuffers, window.attributeBuffersCount, window.fragmentFlag, models[i], defaultTexture);
				ssr(window.framebuffer, window.depth, window.attributeBuffers, cam);
				break;
			}
		}
#endif

		std::string val = floatToString(getTimerMicros(_perfAnalyzer.timer[0])/1000.f) + "ms";
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

void update(float dt)noexcept{
	float sin_rotx = sin(cam.rot.x);
	float cos_rotx = cos(cam.rot.x);
	if(!getMenuFlag(settingsMenu, MENU_OPEN)){
		cam.pos.x -= getButton(keyboard, KEY_W)*sin_rotx*SPEED*dt;
		cam.pos.z += getButton(keyboard, KEY_W)*cos_rotx*SPEED*dt;
		cam.pos.x += getButton(keyboard, KEY_S)*sin_rotx*SPEED*dt;
		cam.pos.z -= getButton(keyboard, KEY_S)*cos_rotx*SPEED*dt;
		cam.pos.x += getButton(keyboard, KEY_D)*cos_rotx*SPEED*dt;
		cam.pos.z += getButton(keyboard, KEY_D)*sin_rotx*SPEED*dt;
		cam.pos.x -= getButton(keyboard, KEY_A)*cos_rotx*SPEED*dt;
		cam.pos.z -= getButton(keyboard, KEY_A)*sin_rotx*SPEED*dt;
		cam.pos.y -= getButton(keyboard, KEY_SPACE)*SPEED*dt;
		cam.pos.y += getButton(keyboard, KEY_SHIFT)*SPEED*dt;
	}else{
		updateMenu(window.framebuffer, settingsMenu, font);
		updateFloatSliders(window.framebuffer, font, debugSlider, sliderCount);
	}
}

LRESULT CALLBACK mainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if(window == nullptr) return DefWindowProc(hwnd, uMsg, wParam, lParam);
	switch(uMsg){
		case WM_DESTROY:{
			_running = false;
			ErrCheck(setWindowFlag(*window, WINDOW_CLOSE), "setze close Fensterstatus");
			break;
		}
		case WM_SIZE:{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			if(!width || !height) break;
			// ErrCheck(setWindowFlag(window, WINDOW_RESIZE), "setzte resize Fensterstatus");
			ErrCheck(resizeWindow(*window, width, height, PIXELSIZE), "Fenster skalieren");
			for(int i=0; i < sizeof(colorBuffers)/sizeof(Colorbuffer); ++i){
				destroyColorbuffer(colorBuffers[i]);
				createColorbuffer(colorBuffers[i], width, height);
			}
			break;
		}
		case WM_LBUTTONDOWN:{
			if(!getButton(mouse, MOUSE_LMB)){

			};
			setButton(mouse, MOUSE_LMB);
			break;
		}
		case WM_LBUTTONUP:{
			resetButton(mouse, MOUSE_LMB);
			break;
		}
		case WM_RBUTTONDOWN:{
			if(!getButton(mouse, MOUSE_RMB)){

			};
			setButton(mouse, MOUSE_RMB);
			break;
		}
		case WM_RBUTTONUP:{
			resetButton(mouse, MOUSE_RMB);
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
				if(!getMenuFlag(settingsMenu, MENU_OPEN)){
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
				if(!getMenuFlag(settingsMenu, MENU_OPEN_TOGGLE)){
					setMenuFlag(settingsMenu, MENU_OPEN_TOGGLE);
					settingsMenu.flags ^= MENU_OPEN;			//Änder offen bit
					tagRECT w_pos;
					GetWindowRect(hwnd, &w_pos);
					if(!getMenuFlag(settingsMenu, MENU_OPEN)) SetCursorPos(window->windowWidth/2+w_pos.left, window->windowHeight/2+w_pos.top);
				}
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
				resetMenuFlag(settingsMenu, MENU_OPEN_TOGGLE);
				break;
			}
			return 0L;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
