// ShadeMeshOBJ.cpp: Phong shade .obj mesh

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "Camera.h"
#include "GLXtras.h"
#include "Mesh.h"

// Application Data

char        *objFilename = "C:"; ***

vector<vec3> points;                // 3D mesh vertices
vector<vec3> normals;               // vertex normals
vector<int3> triangles;             // triplets of vertex indices

vec3         lightSource(1, 1, 0);  // for Phong shading
GLuint       vBuffer = 0;           // GPU vertex buffer ID
GLuint       program = 0;           // GLSL program ID


int          winW = 400, winH = 400;
Camera       camera((float)winW/winH, vec3(0,0,0), vec3(0,0,-5));

// Shaders

char *vertexShader = R"(
    #version 130
    in vec3 point;
    in vec3 normal;
    out vec3 vPoint;
    out vec3 vNormal;
    uniform mat4 modelview;
    uniform mat4 persp;
    void main() {
        vPoint = (modelview*vec4(point, 1)).xyz;
        vNormal = (modelview*vec4(normal, 0)).xyz;
        gl_Position = persp*vec4(vPoint, 1);
    }
)";

char *pixelShader = R"(
    #version 130
    in vec3 vPoint;
    in vec3 vNormal;
    out vec4 pColor;
    uniform vec3 light;
    uniform vec4 color = vec4(.7, .7, 0, 1);
    void main() {
        vec3 N = normalize(vNormal);       // surface normal
        vec3 L = normalize(light-vPoint);  // light vector
        vec3 E = normalize(vPoint);        // eye vertex
        vec3 R = reflect(L, N);            // highlight vector
        float d = abs(dot(N, L));          // two-sided diffuse
        float s = abs(dot(R, E));          // two-sided specular
        float intensity = clamp(d+pow(s, 50), 0, 1);
        pColor = vec4(intensity*color.rgb, color.a);
    }
)";

// Initialization

void InitVertexBuffer() {
    // create GPU buffer, make it the active buffer
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // allocate memory for vertex positions and normals
    *** send vertex data to GPU
}

// Interactive Rotation

int WindowHeight(GLFWwindow *w) {
    int width, height;
    glfwGetWindowSize(w, &width, &height);
    return height;
}

bool Shift(GLFWwindow *w) {
    return glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
           glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
}

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
    double x, y;
    glfwGetCursorPos(w, &x, &y);
    y = WindowHeight(w)-y;
    if (action == GLFW_PRESS)
        camera.MouseDown((int) x, (int) y);
    if (action == GLFW_RELEASE)
        camera.MouseUp();
}

void MouseMove(GLFWwindow *w, double x, double y) {
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) { // drag
        y = WindowHeight(w)-y;
        camera.MouseDrag((int) x, (int) y, Shift(w));
    }
}

void MouseWheel(GLFWwindow *w, double xoffset, double direction) {
    camera.MouseWheel(direction, Shift(w));
}

// Application

void Display() {
    glUseProgram(program);
    // update matrices
    SetUniform(program, "modelview", camera.modelview);
    SetUniform(program, "persp", camera.persp);
    // transform light and send to fragment shader
    vec4 hLight = camera.modelview*vec4(lightSource, 1);
    SetUniform(program, "light", vec3(hLight.x, hLight.y, hLight.z));
    // clear screen to grey, enable transparency, use z-buffer
    glClearColor(.5f, .5f, .5f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // link shader inputs with vertex buffer
    *** setup vertex feeder
    // draw triangles and finish
    glDrawElements(GL_TRIANGLES, 3*triangles.size(), GL_UNSIGNED_INT, &triangles[0]);
    glFlush();
}

int main(int argc, char **argv) {
    // init app window and GL context
    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 4); // anti-alias
    GLFWwindow *w = glfwCreateWindow(winW, winH, "Shade OBJ Mesh", NULL, NULL);
    glfwSetWindowPos(w, 100, 100);
    glfwMakeContextCurrent(w);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    // init shader and GPU data
    program = LinkProgramViaCode(&vertexShader, &pixelShader);
    if (!ReadAsciiObj((char *) objFilename, points, triangles, &normals)) {
        printf("failed to read obj file\n");
        getchar();
        return 1;
    }
    printf("%i vertices, %i triangles, %i normals\n", points.size(), triangles.size(), normals.size());
    Normalize(points, .8f); // scale/move model to +/-1 space
    InitVertexBuffer();
    // callbacks
    glfwSetCursorPosCallback(w, MouseMove);
    glfwSetMouseButtonCallback(w, MouseButton);
    glfwSetScrollCallback(w, MouseWheel);
    // event loop
    while (!glfwWindowShouldClose(w)) {
        Display();
        glfwPollEvents();
        glfwSwapBuffers(w);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vBuffer);
    glfwDestroyWindow(w);
    glfwTerminate();
}
