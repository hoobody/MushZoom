// Demo-ClearScreen.cpp

#include <glad.h>                                         
#include <glfw3.h>                                        
#include "GLXtras.h" 

const char *vertexShader = R"(
	#version 130                                           
	in vec2 point;
	void main() { gl_Position = vec4(point, 0, 1); }
)";

const char *pixelShader = R"(
	#version 130                                           
	out vec4 pColor;                                       
	void main() { pColor = vec4(0, 1, 0, 1); }
)";

int main() {
	glfwInit();
	GLFWwindow *w = glfwCreateWindow(300, 300, "Clear", NULL, NULL);
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	GLuint program = LinkProgramViaCode(&vertexShader, &pixelShader);
	glUseProgram(program);
	GLuint vBuffer = 0;
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	vec2 pts[] = { {-1,-1}, {-1,1}, {1,1}, {1,-1} };
	glBufferData(GL_ARRAY_BUFFER, sizeof(pts), pts, GL_STATIC_DRAW);
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);
	while (!glfwWindowShouldClose(w)) {
		glDrawArrays(GL_QUADS, 0, 4);                     
		glFlush();                                        
		glfwSwapBuffers(w);                          
		glfwPollEvents();
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
