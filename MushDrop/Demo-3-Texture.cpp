// Demo-Texture.cpp

#include <glad.h>                                         
#include <glfw3.h>                                        
#include "GLXtras.h"  
#include "Misc.h"                                                   // LoadTexture

GLuint program = 0;
float scale = .5;
GLuint textureName = 0, textureUnit = 0;                            // OpenGL identifiers
const char *pixFile = "C:/Users/jacob/Graphics/Assests/background.png";       // bmp, jpg, png, tga supported

const char *vertexShader = R"(
	#version 130
	uniform float scale = 1;                                        // from CPU
	in vec2 point, uv;                                              // from GPU
	out vec2 vuv;           
	void main() {
		gl_Position = vec4(scale*point, 0, 1);                      // transform
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

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
	scale *= spin > 0? 1.1f : .9f;
}

void Display() {
	glClearColor(0, 0, 1, 1);                                       // set background color
	glClear(GL_COLOR_BUFFER_BIT);                                   // clear background
	SetUniform(program, "scale", scale);                            // set scale for vertex shader
	SetUniform(program, "textureImage", (int) textureUnit);         // set texture map for pixel shader
	glDrawArrays(GL_QUADS, 0, 4);                                   // draw object                     
	glFlush();                                                      // finish scene
}

int main() {
	glfwInit();
	GLFWwindow *w = glfwCreateWindow(600, 600, "Scale & Texture", NULL, NULL);
	glfwSetWindowPos(w, 200, 200);
	glfwMakeContextCurrent(w);
	glfwSetScrollCallback(w, MouseWheel);
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
