// Demo-Scale.cpp

#include <glad.h>                                         
#include <glfw3.h>                                        
#include "GLXtras.h"  
#include "time.h"													// for animation

bool animate = true;												// scale via time, else interactive

GLuint program = 0;													// make global
float scale = .5;													// make global
time_t startTime = clock();											// app start

const char *vertexShader = R"(
	#version 130
	uniform float scale = 1;										// from CPU
	in vec2 point;													// from GPU             
	void main() { gl_Position = vec4(scale*point, 0, 1); }			// transform
)";

const char *pixelShader = R"(
	#version 130                                           
	out vec4 pColor;                                       
	void main() { pColor = vec4(0, 1, 0, 1); }
)";

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
	scale *= spin > 0? 1.1f : .9f;
}

void Display() {
	if (animate) {
		float dt = (float)(clock()-startTime)/CLOCKS_PER_SEC;		// elapsed time
		float freq = .5f, ang = 2*3.1415f*dt*freq;					// current angle
		scale = (1+sin(ang))/2;
	}
	SetUniform(program, "scale", scale);							// set scale with sin function
	glClearColor(0, 0, 1, 1);										// set background color
	glClear(GL_COLOR_BUFFER_BIT);									// clear background
	glDrawArrays(GL_QUADS, 0, 4);									// draw object                     
	glFlush();														// finish scene
}

int main() {
	glfwInit();
	GLFWwindow *w = glfwCreateWindow(600, 600, "Scale", NULL,NULL);	// new title
	glfwSetWindowPos(w, 200, 200);
	glfwMakeContextCurrent(w);
	glfwSetScrollCallback(w, MouseWheel);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	glUseProgram(program);
	GLuint vBuffer = 0;
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	float pts[][2] = { {-1,-1}, {-1,1}, {1,1}, {1,-1} };
	glBufferData(GL_ARRAY_BUFFER, sizeof(pts), pts, GL_STATIC_DRAW);
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);
	while (!glfwWindowShouldClose(w)) {
		Display();													// modularize
		glfwSwapBuffers(w);                          
		glfwPollEvents();
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
