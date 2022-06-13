// 7-CubePersp.cpp - draw cube with perspective

#include <glad.h>
#include <GLFW/glfw3.h>
#include <string.h>
#include <stdio.h>
#include "Draw.h"
#include "GLXtras.h"
#include "VecMat.h"

// cube
float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1;
float points[][3] = {{l, b, n}, {l, b, f}, {l, t, n}, {l, t, f}, {r, b, n}, {r, b, f}, {r, t, n}, {r, t, f}}; // 8 points
float colors[][3] = {{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1, 1}, {1, 0, 0}, {1, 0, 1}, {1, 1, 0}, {1, 1, 1}}; // 8 colors
int faces[][4] = {{1, 3, 2, 0}, {6, 7, 5, 4}, {4, 5, 1, 0}, {3, 7, 6, 2}, {2, 6, 4, 0}, {5, 7, 3, 1}};        // 6 faces

// IDs for vertex buffer, shader program
GLuint vBuffer = 0, program = 0;

// display parameters
float cubeSize = .15f;
int screenWidth = 1100, screenHeight = 500;

// Shaders

const char *vertexShader = R"(
    #version 130
    in vec3 point;
    in vec3 color;
    out vec4 vColor;
    uniform mat4 view;
    void main() {
        gl_Position = view*vec4(point, 1);
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

// Mouse

vec2    mouseDown;                          // mouse drag reference
vec2    rotOld(245, 205), rotNew(rotOld);   // .x is rotation about Y-axis, .y about X-axis
float   rotSpeed = .3f;                     // modify drag speed
vec3    tranOld, tranNew(0, 0, -1.3f);      // old/new translate (initial so ortho/persp about same size)
float   tranSpeed = .003f;                  // modify drag speed

int WindowHeight(GLFWwindow *w) {
    int width, height;
    glfwGetWindowSize(w, &width, &height);
    return height;
}

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
    if (action == GLFW_PRESS) {
        double x, y;
        glfwGetCursorPos(w, &x, &y);
        y = WindowHeight(w)-y;
        mouseDown = vec2((float) x, (float) y); // save reference for MouseDrag
    }
    if (action == GLFW_RELEASE) {
        rotOld = rotNew;                            // save reference for MouseDrag
        tranOld = tranNew;
    }
}

void MouseMove(GLFWwindow *w, double x, double y) {
    y = WindowHeight(w)-y;
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) { // drag
        vec2 mouse((float) x, (float) y), dif = mouse-mouseDown;
        if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            tranNew = tranOld+tranSpeed*vec3(dif.x, dif.y, 0);  // SHIFT key: translate
        else
            rotNew = rotOld+rotSpeed*dif;                       // rotate
    }
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
    tranNew.z += spin > 0? -.1f : .1f;   // dolly in/out
    tranOld = tranNew;
}

// App Display

void Display(GLFWwindow *w) {
    // clear screen, enable z-buffer
    glClearColor(.7f, .7f, .7f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // init shader program, set vertex feed for points and colors
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    VertexAttribPointer(program, "point", 3, 0, (void *) 0);
    VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
    // get screen size, divide into two viewports with equal aspect ratios
    glfwGetWindowSize(w, &screenWidth, &screenHeight);
    float aspectRatio = (float) screenWidth/2 / (float) screenHeight;
    // compute projection and modelview matrices
    mat4 persp = Perspective(30, aspectRatio, 0.001f, 500);
    mat4 scale = Scale(cubeSize);
    mat4 rot = RotateY(rotNew.x)*RotateX(rotNew.y);
    mat4 tran = Translate(tranNew);
    mat4 modelview = tran*rot*scale;
    // send final view to shader
    SetUniform(program, "view", persp*modelview);
    // draw shaded cube on left
    glViewport(0, 0, screenWidth/2, screenHeight);
    glEnable(GL_MULTISAMPLE);
    glDrawElements(GL_QUADS, sizeof(faces)/sizeof(int), GL_UNSIGNED_INT, faces);
    // draw smooth lined cube on right
    glViewport(screenWidth/2, 0, screenWidth/2, screenHeight);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glDisable(GL_MULTISAMPLE); // blend is better for points, but multisample overrides
    glPointSize(5);
    glDrawArrays(GL_POINTS, 0, 8);
    glLineWidth(5);
    for (int i = 0; i < 6; i++)
        glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, &faces[i]);
    glFlush();
}

// Vertex Buffer

void InitVertexBuffer() {
    // create GPU buffer to hold points and colors, and make it the active vertex buffer
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // allocate memory for vertex points and colors, and load data
    glBufferData(GL_ARRAY_BUFFER, sizeof(points)+sizeof(colors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
}

// Application

void Resize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, screenWidth = width, screenHeight = height);
}

int main(int ac, char **av) {
    // init app window and GL context
    glfwInit();
    GLFWwindow *w = glfwCreateWindow(screenWidth, screenHeight, "Cube Perspective", NULL, NULL);
    glfwSetWindowPos(w, 100, 100);
    glfwMakeContextCurrent(w);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    // init shader and GPU data
    program = LinkProgramViaCode(&vertexShader, &pixelShader);
    InitVertexBuffer();
    // callbacks
    glfwSetCursorPosCallback(w, MouseMove);
    glfwSetMouseButtonCallback(w, MouseButton);
    glfwSetScrollCallback(w, MouseWheel);
    glfwSetWindowSizeCallback(w, Resize);
    // event loop
    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(w)) {
        glfwPollEvents();
        Display(w);
        glfwSwapBuffers(w);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vBuffer);
    glfwDestroyWindow(w);
    glfwTerminate();
}
