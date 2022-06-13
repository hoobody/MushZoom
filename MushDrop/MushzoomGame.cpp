// MushzoomGame.cpp - Ali, Jacob, and Ian

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <time.h>
#include "GLXtras.h"
#include "Misc.h"
#include "Sprite.h"
#include "Widgets.h"
#include "Draw.h"
#include "Text.h"
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#pragma region Variables

Sprite startBackground, startButton, startMush, mushTitle, gameBackground,
	parachuteMush, collectable, winScreen, loseScreen;
Sprite leftBranch[3], rightBranch[3];

string dir = "C:/Users/jacob/Graphics/Assests/"; //"C:/Users/Ali/Graphics/Images/"; 
string start = dir+"start.png";
string startMushroom = dir+"titleMush.png";
string backgroundTex = dir+"background.png";
string title = dir+"title.png";
string gamebackgroundTex = dir+"canvabackground.png";
string parachute = dir+"float.png";
string leftM = dir+"left.png";
string rightM = dir+"right.png";
string branchRTxt = dir+"branch.png";
string branchLTxt = dir + "branch.png";
string injuredTxt = dir+"injured.png";
string collectableTxt = dir + "collectable.png";
string life0Txt = dir + "life-0.png";
string life1Txt = dir + "life-1.png";
string life2Txt = dir + "life-2.png";
string life3Txt = dir + "life-3.png";
string life4Txt = dir + "life-4.png";
string life5Txt = dir + "life-5.png";
string life6Txt = dir + "life-6.png";
string winScreenTxt = dir + "winner.png";
string loseScreenTxt = dir + "loser.png";

// Custom Sprite
enum Costume { Floating = 0, Left, Right, Injured };

// Lives Sprite
enum Lives {life0 = 0, life1, life2, life3, life4, life5, life6};
Lives lives = life6;

// Movement
enum MushMove {mm_left, mm_right, mm_none};
MushMove mushMove = mm_none;

// Program variables
int winWidth = 1000, winHeight = 1000;
bool programStarted = false; // true on start sprite click

// Timing 
time_t startTime = 0;
time_t keyReleaseTime = 0;
time_t prevBranchTime = clock();
time_t prevCollectableTime = clock();

// Animate Background Variables
float loopDuration = 2, topDuration = 1.5f; // in secs
float vScale = .4f, topV = .6f, loopLowV = .2f, loopHighV = .4f;
bool fallToGround = false;
int nloops = 0;

// Animate Variables
bool keyRecentlyReleased = false;

// Animate Mushroom Variables
MushMove prevCostume;

// Animate Branches variables
float branchRate = .5f;

// Animate Collectables variables
float collectableRate = .4f;
bool displayCollectables = true;
int collected = 0;
const int MAX_COLLECTED = 5;
int winningTime;

// Player-Branch Collisions Variables
vec2 probesLeft[] = { {.28, 0.38},{ 0.48 , 0.38}, {0.62, 0.18}, {0.26 , 0.14}, {0.04, 0.08} };
vec2 probesRight[] = { {-.28, 0.38},{ -0.48 , 0.38}, {-0.62, 0.18}, {-0.26 , 0.14}, {-0.04, 0.08} };
const int numProbes = sizeof(probesLeft) / sizeof(vec2);
float probeDepthsLeft[numProbes], probeDepthsRight[numProbes];
vec4 probeColorsLeft[numProbes], probeColorsRight[numProbes];

// Life System Variables
bool gameOver = false;
bool winGame = false;
int currentLivesUsed = 0;
time_t prevDamageTime;

#pragma endregion

#pragma region Classes

class PlayerSprite : public Sprite {
public:
	GLuint costumeTextureNames[4] = { 0, 0, 0, 0 };
	int costume = 0;
	time_t injuryTime = 0;
	void Initialize(string floatingCostume, string leftCostume, string rightCostume, string injuredCostume, float z = 0) {
		this->z = z;
		costumeTextureNames[0] = LoadTexture(floatingCostume.c_str(), true, &nTexChannels);
		costumeTextureNames[1] = LoadTexture(leftCostume.c_str(), true, &nTexChannels);
		costumeTextureNames[2] = LoadTexture(rightCostume.c_str(), true, &nTexChannels);
		costumeTextureNames[3] = LoadTexture(injuredCostume.c_str(), true, &nTexChannels);
	}
	void SetCostume(Costume c) {
		costume = c;
		if (c == Injured) {
			injuryTime = clock();
		}
	}
	void Display(int textureUnit = 0) {
		time_t currentTime = clock();
		float dt = (float)(currentTime - injuryTime) / CLOCKS_PER_SEC;
		if (dt > .2f && costume == Injured) {
			SetCostume(Floating); 
		}
		int spriteShader = GetSpriteShader();
		glUseProgram(spriteShader);
		glActiveTexture(GL_TEXTURE0 + textureUnit + costume);
		glBindTexture(GL_TEXTURE_2D, costumeTextureNames[costume]);
		SetUniform(spriteShader, "textureImage", (int)textureUnit + costume);
		SetUniform(spriteShader, "useMat", false);
		SetUniform(spriteShader, "nTexChannels", nTexChannels);
		SetUniform(spriteShader, "z", z);
		SetUniform(spriteShader, "view", ptTransform);
		SetUniform(spriteShader, "uvTransform", uvTransform);
#ifdef GL_QUADS
		glDrawArrays(GL_QUADS, 0, 4);
#else
		glDrawArrays(GL_TRIANGLES, 0, 6);
#endif
	}
};
PlayerSprite mushroomPlayer;

class HealthSprite : public Sprite {
public:
	GLuint costumeTextureNames[7] = { 0, 0, 0, 0, 0, 0, 0 };
	int costume = 0;
	void Initialize(string life0, string life1, string life2, string life3, string life4, string life5, string life6, float z = 0.1f) {
		this->z = z;
		costumeTextureNames[0] = LoadTexture(life0.c_str(), true, &nTexChannels);
		costumeTextureNames[1] = LoadTexture(life1.c_str(), true, &nTexChannels);
		costumeTextureNames[2] = LoadTexture(life2.c_str(), true, &nTexChannels);
		costumeTextureNames[3] = LoadTexture(life3.c_str(), true, &nTexChannels);
		costumeTextureNames[4] = LoadTexture(life4.c_str(), true, &nTexChannels);
		costumeTextureNames[5] = LoadTexture(life5.c_str(), true, &nTexChannels);
		costumeTextureNames[6] = LoadTexture(life6.c_str(), true, &nTexChannels);
	}
	void SetCostume(Lives c) { costume = c; }
	void Display(int textureUnit = 0) {
		int spriteShader = GetSpriteShader();
		glUseProgram(spriteShader);
		glActiveTexture(GL_TEXTURE0 + textureUnit + costume);
		glBindTexture(GL_TEXTURE_2D, costumeTextureNames[costume]);
		SetUniform(spriteShader, "textureImage", (int)textureUnit + costume);
		SetUniform(spriteShader, "useMat", false);
		SetUniform(spriteShader, "nTexChannels", nTexChannels);
		SetUniform(spriteShader, "z", z);
		SetUniform(spriteShader, "view", ptTransform);
		SetUniform(spriteShader, "uvTransform", uvTransform);
#ifdef GL_QUADS
		glDrawArrays(GL_QUADS, 0, 4);
#else
		glDrawArrays(GL_TRIANGLES, 0, 6);
#endif
	}
};
HealthSprite healthSprite;

#pragma endregion

#pragma region Probes

// Used for checking to see if the probes are correct
vec2 PtTransform(vec2 p, mat4 m) {
	vec4 xP = m * vec4(p, 0, 1);
	return vec2(xP.x, xP.y);
}

// Used for checking if the probes are correct
float Depth(vec2 p) {
	float z = 0;
	int x = (int)((p.x + 1) * winWidth / 2.f);
	int y = (int)((p.y + 1) * winHeight / 2.f);
	DepthXY(x, y, z);
	return z;
}

// used for checking if the probes are correct
vec4 getPixel(vec2 p)
{
	vec4 rgba;
	int x = (int)((p.x + 1) * winWidth / 2.f);
	int y = (int)((p.y + 1) * winHeight / 2.f);
	glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, &rgba);

	return rgba;
}

// Returns if a value is within the range inputed
bool Within(float f, float min, float max) {
	return f >= min && f <= max;
}

#pragma endregion

#pragma region HelperFunctions

// Returns a random float between 0 and 1
float Random()
{
	float f = (float)(rand() % 100 + 1);
	f = f / 100;
	return f;
}

// Changes the life costume depending on how many lives have been used
void lifeDecrement(int numLivesUsed)
{
	float dt = (float)(prevDamageTime - clock()) / CLOCKS_PER_SEC;
	if (dt < 2)
	{
		if (numLivesUsed == 6)
		{
			healthSprite.SetCostume(life6);

		}
		if (numLivesUsed == 5)
		{
			healthSprite.SetCostume(life5);
		}
		if (numLivesUsed == 4)
		{
			healthSprite.SetCostume(life4);
		}
		if (numLivesUsed == 3)
		{
			healthSprite.SetCostume(life3);
		}
		if (numLivesUsed == 2)
		{
			healthSprite.SetCostume(life2);
		}
		if (numLivesUsed == 1)
		{
			healthSprite.SetCostume(life1);
		}
		prevDamageTime = clock();
		time_t currentTime = clock();
		float dt = (float)(currentTime - mushroomPlayer.injuryTime) / CLOCKS_PER_SEC;
		if (dt > .2f) {
			mushroomPlayer.SetCostume(Floating);
			//lives -= 1; 
		}
	}
}

// display branches and checks for hit detection
void displayBranches()
{
	// Testing branch probes before branch display 
	for (int i = 0; i < numProbes; i++) {
		probeDepthsLeft[i] = Depth(PtTransform(probesLeft[i], leftBranch[0].ptTransform));
		probeColorsLeft[i] = getPixel(probesLeft[i]);
		probeDepthsRight[i] = Depth(PtTransform(probesRight[i], rightBranch[0].ptTransform));
		probeColorsRight[i] = getPixel(probesRight[i]);
	}
	for (int i = 0; i < 1; i++)
	{
		leftBranch[i].Display();
		rightBranch[i].Display();
	}

	glDisable(GL_DEPTH_TEST);
	UseDrawShader();
	float zMush = mushroomPlayer.z;
	for (int i = 0; i < numProbes; i++) {
		bool hitLeft = Within(probeDepthsLeft[i], zMush - 0.05f, zMush + 0.05f);
		bool hitRight = Within(probeDepthsRight[i], zMush - 0.05f, zMush + 0.05f);
		if (hitLeft || hitRight)
		{
			time_t now = clock();
			float dt = (float)(now - mushroomPlayer.injuryTime) / CLOCKS_PER_SEC;
			if (dt < .5f) {
				break;
			}
			mushMove = hitLeft ? mm_right : mm_left;
			// Player hit branch
			mushroomPlayer.SetCostume(Injured);
			currentLivesUsed++;
			lifeDecrement(currentLivesUsed);
			if (currentLivesUsed == 6)
			{
				gameOver = true;
			}
			vec4 c = probeColorsLeft[i];
			//printf("c = %2.1f, %2.1f, %2.1f\n", c.x, c.y, c.z);
			break;
		}
	}
}

// Counts the collectables collected as well as displaying the clock
void countingCollectables()
{
	// in game clock 
	float elapsedTime = (float)(clock() - startTime) / CLOCKS_PER_SEC; // in secs

	if (elapsedTime > topDuration) {
		float gameTime = elapsedTime - topDuration;
		if (collectable.Intersect(mushroomPlayer))
		{
			if (displayCollectables) { collected++; }
			displayCollectables = false;

			if (collected >= MAX_COLLECTED)
			{
				winGame = true;
				winningTime = gameTime;
				winningTime += (currentLivesUsed * 2); // score is winning time plus 2 * the lives you've used
				printf("You win! Your score was %i\n", winningTime);
			}
		}
		Text(winWidth - 275, winHeight - 75, vec3(0, 0, 0), 75, "%i", (int)gameTime);
		Text(winWidth - 400, winHeight - 75, vec3(0, 0, 0), 75, "%i", collected);
	}
}

#pragma endregion

#pragma region Animation

// Loops the background
void AnimateBackground() {
	time_t now = clock();
	float elapsedTime = (float)(now - startTime) / CLOCKS_PER_SEC; // in secs
	float v = topV;

	if (elapsedTime > topDuration) {
		float midTime = elapsedTime - topDuration; // time while looping
		float f = midTime / loopDuration; // #loops
		if (f < 1) {
			// top portion
			nloops = 0;
			v = topV + f * (loopHighV - topV);
		}
		else {
			if (!fallToGround)
				// looping
				nloops = (int)f;
			v = loopHighV + (f - nloops) * (loopLowV - loopHighV);

			if (fallToGround)
			{
				// landing
				v = v < 0 ? 0 : v;
			}
		}
	}
	gameBackground.uvTransform = Translate(0, v, 0) * Scale(vec3(1, vScale, 1));
}

// Loops the branches
void AnimateBranches()
{
	time_t now = clock();
	float elapsedTime = (float)(now - startTime) / CLOCKS_PER_SEC; // in secs

	if (elapsedTime > topDuration && !fallToGround)
	{
		float dt = (float)(now - prevBranchTime) / CLOCKS_PER_SEC;

		for (int i = 0; i < 3; i++)
		{
			Sprite& s = leftBranch[i];
			vec2 p = s.GetPosition();

			p.y += dt * branchRate;
			if (p.y > 1)
			{
				p.y = -1.5f + .3f * Random();
				float prevY = leftBranch[(i + 1) % 3].GetPosition().y;
				float minYDistance = .5f;

				if (prevY - p.y < minYDistance)
				{
					p.y = prevY - minYDistance;
				}
			}

			s.SetPosition(p);
		}

		for (int i = 0; i < 3; i++)
		{
			Sprite& s = rightBranch[i];
			vec2 p = s.GetPosition();

			p.y += dt * branchRate;
			if (p.y > 1.1)
			{
				p.y = -1.5f + .3f * Random();
				float prevY = rightBranch[(i + 1) % 3].GetPosition().y;
				float minYDistance = .5f;

				if (prevY - p.y < minYDistance)
				{
					p.y = prevY - minYDistance;
				}
			}

			s.SetPosition(p);
		}
	}
	prevBranchTime = now;
}

// Loops the collectables
void AnimateCollectables()
{
	time_t now = clock();
	float elapsedTime = (float)(now - startTime) / CLOCKS_PER_SEC; // in secs

	if (elapsedTime > topDuration && !fallToGround)
	{
		float dt = (float)(now - prevCollectableTime) / CLOCKS_PER_SEC;

		for (int i = 0; i < 2; i++)
		{
			Sprite& s = collectable;
			vec2 p = s.GetPosition();

			p.y += dt * collectableRate;
			if (p.y > 1)
			{
				displayCollectables = true;
				p.y = -1.5f + .3f * Random();
				p.x = .65f * (Random() * 2 - 1);
			}

			s.SetPosition(p);
		}
	}
	prevCollectableTime = now;
}

// Moves the mushroom left and right as well as checking to see if the player hits the tree trunk
void AnimateMushroom()
{
	float f = 0.01f;
	float f1 = 0.0015f;
	vec2 p = mushroomPlayer.GetPosition();
	if (mushMove == mm_left && p.x > -.7f)
	{
		p.x -= f;
	}
	else if (mushMove == mm_right && p.x < .7f)
	{
		p.x += f;
	}
	else if (mushMove == mm_none && p.x > -.7f && p.x < .7f);
	else {
		// Player hit tree trunk
		mushroomPlayer.SetCostume(Injured);
		currentLivesUsed++;
		lifeDecrement(currentLivesUsed);
		if (currentLivesUsed == 6)
		{
			gameOver = true;
		}

		if (mushMove == mm_left)
		{
			mushMove = mm_right;
			prevCostume = mm_left;
		}
		else if (mushMove == mm_right)
		{
			mushMove = mm_left;
			prevCostume = mm_right;
		}
	}

	if (fallToGround && p.y > -.38)
	{
		p.y -= f1;
	}
	mushroomPlayer.SetPosition(p);
}

// Calls all the other animate functions
void Animate() {
	if (keyRecentlyReleased && (float)(clock() - keyReleaseTime) > 0.2f) {
		keyRecentlyReleased = false;
	}
	AnimateBackground();
	AnimateBranches();
	AnimateMushroom();
	AnimateCollectables();
}
#pragma endregion

#pragma region Application+Input

// Mouse
void MouseButton(GLFWwindow* w, int butn, int action, int mods) {
	double x, y;
	glfwGetCursorPos(w, &x, &y);
	y = winHeight - y; // invert y for upward-increasing screen space
	if (action == GLFW_PRESS) {
		int ix = (int)x, iy = (int)y;
		if (startButton.Hit(ix, iy)) {
			programStarted = true;
			startTime = clock();
		}
	}
}

// Keyboard
void Keyboard(GLFWwindow* w, int key, int scancode, int action, int mods) {
	float f = 0.02f;
	if (programStarted) {
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			keyRecentlyReleased = false;
			switch (key) {
			case 263: mushMove = mm_left; mushroomPlayer.SetCostume(Left); break; // left arrow
			case 262: mushMove = mm_right; mushroomPlayer.SetCostume(Right); break; // right arrow
			//case 'F': fallToGround = true; break;
			//case 'R': Reset(); break;
			}
		}
		if (action == GLFW_RELEASE)
			keyReleaseTime = clock();
		keyRecentlyReleased = true;
	}
}

// Application
void Resize(GLFWwindow* w, int width, int height) {
	glViewport(0, 0, winWidth = width, winHeight = height);
}

const char* usage = R"(
	Welcome to MushZoom! 
	The goal of the game is to collect dandelions as fast as possible.
	As you collect them you need to avoid tree branches.
	If you collect all the dandelions before you run out of lives you win!
	The lower the score the better!

	Controls:
	left arrow: move left
	right arrow: move right


	Game by Ali, Jacob, and Ian
)";

// Display
void Display() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	if (!programStarted) { // title screen
		startBackground.Display();
		startButton.Display();
		startMush.Display();
		mushTitle.Display();
	}
	if (gameOver)
	{
		loseScreen.Display();
	}
	if (winGame)
	{
		winScreen.Display();
		Text(winWidth - 400, winHeight - 75, vec3(0, 0, 0), 75, "Score : %i", winningTime);
	}
	if (programStarted && !gameOver && !winGame) { // main game
		Animate();
		gameBackground.Display();
		healthSprite.Display();
		mushroomPlayer.Display();
		displayBranches();

		if(displayCollectables)
			collectable.Display();

		countingCollectables();
	}
	glFlush();
}

#pragma endregion

#pragma region Initialization
// Initialized the left branch
void initializeLeftBranch(int i)
{
	float x = -0.53f;
	float y = -.5f + i * .8f;
	leftBranch[i].Initialize(branchLTxt, .3f);
	leftBranch[i].SetPosition(vec2(x, y));
	leftBranch[i].SetScale(vec2(0.5f, 0.5f));
}

// Initializes the right branch
void initializeRightBranch(int i)
{
	float x = 0.45f;
	float y = -.3f + i * .6f;
	rightBranch[i].Initialize(branchRTxt, .3f);
	rightBranch[i].uvTransform[0][0] *= -1; // reflects image left to right
	rightBranch[i].SetPosition(vec2(x, y));
	rightBranch[i].SetScale(vec2(0.5f, 0.5f));
}

// Initalizes and randomly places the collectable
void initializeCollectables()
{
	// randomizing the x and y of the collectable
	float y = -1.5f + .3f * Random();
	float x = .65f * (Random() * 2 - 1);

	collectable.Initialize(collectableTxt, .1f);
	collectable.SetPosition(vec2(x, y));
	collectable.SetScale(vec2(0.1f, 0.1f));
}

void initializeSprites()
{
	startBackground.Initialize(backgroundTex, .7f);
	gameBackground.Initialize(gamebackgroundTex, .7f);
	startButton.Initialize(start, .2f);
	startButton.SetPosition(vec2(0.0f, -1.0f));
	startMush.Initialize(startMushroom, .1f);
	startMush.SetPosition(vec2(0.0f, -0.07f));
	mushTitle.Initialize(title, .1f);
	mushTitle.SetPosition(vec2(0.0f, 0.3f));
	winScreen.Initialize(winScreenTxt, 0);
	loseScreen.Initialize(loseScreenTxt, 0);
	for (int i = 0; i < 3; i++)
	{
		initializeLeftBranch(i);
		initializeRightBranch(i);
	}
	initializeCollectables();
	mushroomPlayer.Initialize(parachute, leftM, rightM, injuredTxt);
	mushroomPlayer.SetScale(vec2(.2f, .2f));

	healthSprite.Initialize(life0Txt, life1Txt, life2Txt, life3Txt, life4Txt, life5Txt, life6Txt);
	healthSprite.SetPosition(vec2(-0.6f, 0.75f));
	healthSprite.SetScale(vec2(0.4f, 0.4f));

}

#pragma endregion

int main(int ac, char **av) {
	// init app window and GL context
	glfwInit();
	GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "MushZoom", NULL, NULL);
	glfwSetWindowPos(w, 100, 100);
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSetKeyCallback(w, Keyboard);

	initializeSprites();
	srand(clock());

	// callbacks
	glfwSetMouseButtonCallback(w, MouseButton);
	glfwSetWindowSizeCallback(w, Resize);
	printf("Game Description: %s\n", usage);
	// event loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// terminate
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glfwDestroyWindow(w);
	glfwTerminate();
}
