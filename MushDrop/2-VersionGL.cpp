// 2-VersionGL.cpp: determine GL and GLSL versions

#include "glad.h"
#include <glfw3.h>
#include <stdio.h>

int AppEnd(const char *msg) {
	if (msg) printf("%s\n", msg);
	getchar();
	glfwTerminate();
	return msg == NULL? 0 : 1;
}

int main() {
	if (!glfwInit()) return AppEnd("can't init GLFW\n");
	GLFWwindow *w = glfwCreateWindow(1, 1, "", NULL, NULL);
	// need window to create GL context
	if (!w) return AppEnd("can't open window\n");
	glfwSetWindowPos(w, 0, 0);
	glfwMakeContextCurrent(w);
	// must load OpenGL runtime subroutine pointers
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	printf("GL vendor: %s\n", glGetString(GL_VENDOR));
	printf("GL renderer: %s\n", glGetString(GL_RENDERER));
	printf("GL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	return AppEnd(NULL);
}
