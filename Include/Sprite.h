// Sprite.h - 2D quad with optional texture or animation

#ifndef SPRITE_HDR
#define SPRITE_HDR

#include <glad.h>
#include <time.h>
#include <vector>
#include "VecMat.h"

using namespace std;

void BuildShader();
int GetSpriteShader();

// Sprite Class

class Sprite {
public:
	vec2 position, scale = vec2(1, 1), mouseDown, oldMouse;
	float z = 0; // in device coordinates (+/-1)
	float rotation = 0;
	int winWidth = 0, winHeight = 0;
	int nTexChannels = 0;
	// for animation:
	GLuint frame = 0, nFrames = 0;
	vector<GLuint> textureNames;
	float frameDuration = 1.5f;
	time_t change;
	GLuint textureName = 0, matName = 0;
	mat4 ptTransform, uvTransform;
	bool Intersect(Sprite &s);
	void UpdateTransform();
	void Initialize(GLuint texName, float z = 0);
	void Initialize(string imageFile, float z = 0);
	void Initialize(string imageFile, string matFile, float z = 0);
	void Initialize(vector<string> &imageFiles, string matFile, float z = 0);
	bool Hit(int x, int y);
	void SetPosition(vec2 p);
	vec2 GetPosition();
	void MouseDown(vec2 mouse);
	vec2 MouseDrag(vec2 mouse);
	void MouseWheel(double spin);
	vec2 GetScale();
	void SetScale(vec2 s);
	mat4 GetPtTransform();
	void SetPtTransform(mat4 m);
	void SetUvTransform(mat4 m);
	void Display(mat4 *view = NULL, int textureUnit = 0);
	void Release();
	void SetFrameDuration(float dt); // if animating
	Sprite(vec2 p = vec2(), float s = 1) : position(p), scale(vec2(s, s)) { UpdateTransform(); }
	Sprite(vec2 p, vec2 s) : position(p), scale(s) { UpdateTransform(); }
	~Sprite() { Release(); }
};

#endif
