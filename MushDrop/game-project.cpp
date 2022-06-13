// MultiSprite.cpp - multiple movable, selectable, layered sprites

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "GLXtras.h"
#include "Misc.h"
#include "Sprite.h"

int winWidth = 800, winHeight = 800;
Sprite background, sprite1, sprite2, *selected = NULL;

string dir = "C:/Users/jacob/Graphics/Assests/";
string sprite1Tex = dir+"heart-full.png", sprite1Mat = dir+"heart-mat.png";
string sprite2Tex = dir+"start.png", sprite2Mat = dir+"score-start-mat.png";
string combined32 = dir+"heart-full.png"; // png, tga ok; bmp, jpg do not support 32
string backgroundTex = dir + "background.png";

// Display

void Display() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	background.Display();
	sprite1.Display();
	sprite2.Display();
	glFlush();
}

// Mouse

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
	double x, y;
	glfwGetCursorPos(w, &x, &y);
	y = winHeight-y; // invert y for upward-increasing screen space
	if (action == GLFW_PRESS) {
		int ix = (int) x, iy = (int) y;
		selected = NULL;
		if (sprite1.Hit(ix, iy)) selected = &sprite1;
		if (sprite2.Hit(ix, iy)) selected = &sprite2;
		if (selected)
			selected->MouseDown(vec2((float) x, (float) y));
	}
}

void MouseMove(GLFWwindow *w, double x, double y) {
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && selected)
		selected->MouseDrag(vec2((float) x, (float) (winHeight-y)));
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
	if (selected)
		selected->MouseWheel(spin);
}

// Application

void Resize(GLFWwindow *w, int width, int height) {
	glViewport(0, 0, winWidth = width, winHeight = height);
}

int main(int ac, char **av) {
	// init app window and GL context
	glfwInit();
	GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "MushDrop", NULL, NULL);
	glfwSetWindowPos(w, 100, 100);
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	// read background, foreground, and mat textures
	background.Initialize(backgroundTex, "", 0, .7f);
	sprite1.Initialize(combined32, 1, .2f);
//	sprite1.Initialize(sprite1Tex, sprite1Mat, 1, .2f);
	sprite2.Initialize(sprite2Tex, sprite2Mat, 2, .1f);
	// callbacks
	glfwSetMouseButtonCallback(w, MouseButton);
	glfwSetCursorPosCallback(w, MouseMove);
	glfwSetScrollCallback(w, MouseWheel);
	glfwSetWindowSizeCallback(w, Resize);
	// event loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// terminate
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	background.Release();
	sprite1.Release();
	sprite2.Release();
	glfwDestroyWindow(w);
	glfwTerminate();
}
