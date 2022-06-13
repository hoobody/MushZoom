// Demo-5-Mesh.cpp - cube with textured image

#include <glad.h>										// OpenGL access 
#include <glfw3.h>										// application framework
#include "CameraArcball.h"								// view transforms on mouse input
#include "GLXtras.h"									// SetUniform
#include "Mesh.h"										// cube.Read
#include "Misc.h"										// Shift

int winWidth = 700, winHeight = 700;
CameraAB camera(0, 0, winWidth, winHeight, vec3(0,0,0), vec3(0,0,-10), 30, .001f, 500, false);

Mesh cube;
const char *meshFile = "C:/Users/jules/Code/GG-Projects/Cube.obj";
const char *textureFile = "C:/Users/jules/Code/Book/Parrots.jpg";

void Display(GLFWwindow *w) {
	glClearColor(.5f, .5f, .5f, 1);						// set background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// clear background and z-buffer
	glEnable(GL_DEPTH_TEST);							// see only nearest surface
	SetUniform(UseMeshShader(), "useLight", false);		// disable shading
	cube.Display(camera);								// draw mesh with camera transform
	glFlush();											// finish
}

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

void Resize(GLFWwindow *window, int width, int height) {
	camera.Resize(winWidth = width, winHeight = height);
	glViewport(0, 0, width, height);
}

int main(int ac, char **av) {
	// init app window and GL context, read mesh
	glfwInit();
	GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "3D-Mesh", NULL, NULL);
	glfwSetWindowPos(w, 100, 100);
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	cube.Read(string(meshFile), string(textureFile), 1);
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
