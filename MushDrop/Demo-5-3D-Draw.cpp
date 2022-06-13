// Demo-5-Draw.cpp - 3D line-drawing

#include <glad.h>               // access to OpenGL routines
#include <glfw3.h>				// framework for application
#include "CameraArcball.h"      // create view transformations on mouse input
#include "Draw.h"				// line-drawing
#include "GLXtras.h"            // GLSL convenience routines
#include "Misc.h"               // Shift

// window and camera
int winWidth = 700, winHeight = 700;
CameraAB camera(0, 0, winWidth, winHeight, vec3(0,0,0), vec3(0,0,-10), 30, .001f, 500, false);

// cube
float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1;					// left, right, bottom, top, near, far
vec3 points[] = {{l, b, n}, {l, b, f}, {l, t, n}, {l, t, f},
				 {r, b, n}, {r, b, f}, {r, t, n}, {r, t, f}};		// 8 cube corners
vec3 colors[] = {{1,1,1}, {0,0,0}, {1,0,0}, {0,1,0},				// white, black, red, green
				 {0,0,1}, {1,1,0}, {1,0,1}, {0,1,1}};				// blue, yellow, magenta, cyan

// Display

void DrawQuad(int i1, int i2, int i3, int i4) {
	Line(points[i1], points[i2], 4, colors[i1], colors[i2]);
	Line(points[i2], points[i3], 4, colors[i2], colors[i3]);
	Line(points[i3], points[i4], 4, colors[i3], colors[i4]);
	Line(points[i4], points[i1], 4, colors[i4], colors[i1]);
}

void Display(GLFWwindow *w) {
	glClearColor(.5f, .5f, .5f, 1);                                 // set background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);             // clear background and z-buffer
	glEnable(GL_DEPTH_TEST);										// view only nearest surface
	enum { lbn=0, lbf, ltn, ltf, rbn, rbf, rtn, rtf };
	UseDrawShader(camera.fullview);
	DrawQuad(lbf, ltf, ltn, lbn);									// left face
	DrawQuad(rtn, rtf, rbf, rbn);									// right
	DrawQuad(rbn, rbf, lbf, lbn);									// bottom
	DrawQuad(ltf, rtf, rtn, ltn);									// top
	DrawQuad(ltn, rtn, rbn, lbn);									// near
	DrawQuad(rbf, rtf, ltf, lbf);									// far
	glFlush();														// finish
}

// Mouse Callbacks

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
	camera.MouseWheel(spin);
}

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
	double x, y;
	glfwGetCursorPos(w, &x, &y);
	if (action == GLFW_PRESS)
		camera.MouseDown(x, y, Shift());
	if (action == GLFW_RELEASE)
		camera.MouseUp();
}

void MouseMove(GLFWwindow *w, double x, double y) {
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		camera.MouseDrag(x, y);
}

// Application

void Resize(GLFWwindow *window, int width, int height) {
	camera.Resize(winWidth = width, winHeight = height);
	glViewport(0, 0, width, height);
}

int main(int ac, char **av) {
	// init app window and GL context
	glfwInit();
	GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "3D-ShaderFree", NULL, NULL);
	glfwSetWindowPos(w, 100, 400);
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	// callbacks
	glfwSetCursorPosCallback(w, MouseMove);
	glfwSetMouseButtonCallback(w, MouseButton);
	glfwSetScrollCallback(w, MouseWheel);
	glfwSetWindowSizeCallback(w, Resize);
	printf("mouse-drag:  rotate\nwith shift:  translate xy\nmouse-wheel: translate z");
	// event loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(w)) {
		Display(w);
		glfwPollEvents();
		glfwSwapBuffers(w);
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
