// Demo-4-Matrix.cpp: same as Demo-3-Texture.cpp, but with matrix controlled by mouse drap

#include <glad.h>                                         
#include <glfw3.h>                                        
#include "GLXtras.h"  
#include "Misc.h"                                                   // LoadTexture

mat4 m;																// transformation matrix
GLuint program = 0;
GLuint textureName = 0, textureUnit = 0;                            // OpenGL identifiers
const char *pixFile = "C:/Users/jules/Code/Book/Parrots.jpg";       // bmp, jpg, png, tga supported

const char *vertexShader = R"(
	#version 130
	uniform mat4 m;                                                 // from CPU
	in vec2 point, uv;                                              // from GPU
	out vec2 vuv;           
	void main() {
		vec4 v = vec4(point, 0, 1);
		gl_Position = m*v;                                          // transform
		vuv = uv;                                                   // send to pixel shader
	}
)";

const char *pixelShader = R"(
	#version 130
	in vec2 vuv;                                                    // from vertex shader
	uniform sampler2D textureImage;                                 // from CPU
	out vec4 pColor;                                       
	void main() { pColor = texture(textureImage, vuv); }
)";

void Display() {
	glClearColor(0, 0, 1, 1);                                       // set background color
	glClear(GL_COLOR_BUFFER_BIT);                                   // clear background
	SetUniform(program, "m", m);                                    // set vertex shader transform
	SetUniform(program, "textureImage", (int) textureUnit);         // set texture map for pixel shader
	glDrawArrays(GL_QUADS, 0, 4);                                   // draw object                     
	glFlush();                                                      // finish scene
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
	m = m*RotateZ((float)spin);										// add rotation
}

vec2 mouseRef;

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
	double x, y;
	glfwGetCursorPos(w, &x, &y);
	if (action == GLFW_PRESS)
		mouseRef = vec2(x, y);
}

void MouseMove(GLFWwindow *w, double x, double y) {
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		vec2 mDif(x-mouseRef.x, mouseRef.y-y);					// invert vertical
		mat4 tDif = Translate(mDif.x/1000, mDif.y/1000, 0);
		m = tDif*m;													// add translation
		mouseRef = vec2(x, y);
	}
}
void Resize(GLFWwindow *w, int width, int height) {
	glViewport(0, 0, width, height);
}

int main() {
	glfwInit();
	GLFWwindow *w = glfwCreateWindow(600, 600, "Scale & Texture", NULL, NULL);
	glfwSetWindowPos(w, 200, 200);
	glfwMakeContextCurrent(w);
	glfwSetScrollCallback(w, MouseWheel);
	glfwSetMouseButtonCallback(w, MouseButton);
	glfwSetCursorPosCallback(w, MouseMove);
	glfwSetWindowSizeCallback(w, Resize);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	glUseProgram(program);
	GLuint vBuffer = 0;
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	vec2 pts[] = { {-1,-1}, {-1,1}, {1,1}, {1,-1} };                // vertex geometric location
	vec2 uvs[] = { {0, 0}, {0, 1}, {1, 1}, {1, 0} };                // vertex texture location
	int spts = sizeof(pts), suvs = sizeof(uvs);                     // array sizes
	glBufferData(GL_ARRAY_BUFFER, spts+suvs, NULL, GL_STATIC_DRAW); // allocate GPU buffer
	glBufferSubData(GL_ARRAY_BUFFER, 0, spts, pts);                 // store pts
	glBufferSubData(GL_ARRAY_BUFFER, spts, suvs, uvs);              // store uvs after pts in mem
	textureName = LoadTexture(pixFile, textureUnit);                // store texture image
	glActiveTexture(GL_TEXTURE0+textureUnit);                       // make texture active
	glBindTexture(GL_TEXTURE_2D, textureName);
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);        // set feed for pts
	VertexAttribPointer(program, "uv", 2, 0, (void *) spts);        // set feed for uvs
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);                          
		glfwPollEvents();
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
