// 4-ColorfulTriangle.cpp - draw RGB triangle via GLSL, vBuffer

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include "GLXtras.h"

// GPU identifiers
GLuint vBuffer = 0;
GLuint program = 0;

// a triangle (3 2D locations, 3 RGB colors)
float  points[][2] = {{-.9f, -.6f}, {0, .6f}, {.9f, -.6f}};
float  colors[][3] = {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}};

const char *vertexShader = R"(
    #version 130
    in vec2 point;
    in vec3 color;
    out vec4 vColor;
    void main() {
        gl_Position = vec4(point, 0, 1);
        vColor = vec4(color, 1);
    }
)";

const char *pixelShader = R"(
    #version 130
    in vec4 vColor;
    out vec4 pColor;
    void main() {
        pColor = vColor;
    }
)";

void Display() {
    // clear background
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);
    // access GPU vertex buffer
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // associate position input to shader with position array in vertex buffer
    VertexAttribPointer(program, "point", 2, 0, (void *) 0);
    // associate color input to shader with color array in vertex buffer
    VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
    // render three vertices as a triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glFlush();
}

void InitVertexBuffer() {
    // make GPU buffer for points & colors, set it active buffer
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // allocate buffer memory to hold vertex locations and colors
    int sPnts = sizeof(points), sCols = sizeof(colors);
    glBufferData(GL_ARRAY_BUFFER, sPnts+sCols, NULL, GL_STATIC_DRAW);
    // load data to the GPU
    glBufferSubData(GL_ARRAY_BUFFER, 0, sPnts, points);
        // start at beginning of buffer, for length of points array
    glBufferSubData(GL_ARRAY_BUFFER, sPnts, sCols, colors);
        // start at end of points array, for length of colors array
}

bool InitShader() {
    program = LinkProgramViaCode(&vertexShader, &pixelShader);
    if (!program)
        printf("can't init shader program\n");
    return program != 0;
}

// application

void Keyboard(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void ErrorGFLW(int id, const char *reason) {
    printf("GFLW error %i: %s\n", id, reason);
}

void Close() {
    // unbind vertex buffer and free GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vBuffer);
}

int main() {
    glfwSetErrorCallback(ErrorGFLW);
    if (!glfwInit())
        return 1;
    GLFWwindow *w = glfwCreateWindow(300, 300, "Colorful Triangle", NULL, NULL);
    if (!w) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(w);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    printf("GL version: %s\n", glGetString(GL_VERSION));
    PrintGLErrors();
    if (!InitShader())
        return 0;
    InitVertexBuffer();
    glfwSetKeyCallback(w, Keyboard);
    glfwSwapInterval(1); // ensure no generated frame backlog
    // event loop
    while (!glfwWindowShouldClose(w)) {
        Display();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }
    Close();
    glfwDestroyWindow(w);
    glfwTerminate();
}
