// 3-RayTraceInteractive.cpp: interactive ray-trace demo

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <time.h>
#include "GLXtras.h"

// Camera, Scene
int winWidth = 800, winHeight = 800;
vec3 viewPnt(0, 0, -1), viewDir(0, 0, 1), upDir(0, 1, 0);
float fov = 30;
GLFWwindow *window;

// Light, Objects, Shader Program
vec3 light(2, 0, 1);

// Objects
vec4 planes[] = {vec4(-1,0,0,-3), vec4(1,0,0,-3), vec4(0,-1,0,-3), vec4(0,1,0,-3), vec4(0,0,-1,-3), vec4(0,0,1,-3)};
vec4 spheres[] = {vec4(-1.7f, -.3f, 2, .6f), vec4(0, .1f, 2, .9f), vec4(1.3f, 0, 2, .4f)};
vec4 xSpheres[3];

// Shaders
GLuint shader = 0;

// Interaction

vec2 mouseDown, rotOld, rotNew;
bool cameraDown = false, fullscreen = false;

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
    if (action == GLFW_PRESS) {
        double x, y;
        glfwGetCursorPos(w, &x, &y);
        cameraDown = true;
        mouseDown = vec2((float) x, (float) y); // save reference for MouseDrag
    }
    if (action == GLFW_RELEASE)
        rotOld = rotNew;                        // save reference for MouseDrag
}

void MouseMove(GLFWwindow *w, double x, double y) {
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) { // drag
        if (cameraDown) {
            rotNew = rotOld+.3f*(vec2((float) x, (float) y)-mouseDown);
            mat4 rotM = RotateY(-rotNew.x)*RotateX(rotNew.y);
            // first-person movement
             vec4 vDir = rotM*vec4(0, 0, 1, 0);
             vec4 vUp = rotM*vec4(0, 1, 0, 0);
             viewDir = normalize(vec3(vDir.x, vDir.y, vDir.z));
             upDir = normalize(vec3(vUp.x, vUp.y, vUp.z));
        }
    }
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
    vec3 p = viewPnt-viewDir*(spin > 0? .1f : -.1f);
    for (int i = 0; i < 6; i++)
        if (dot(planes[i], vec4(p.x, p.y, p.z, 1)) > 0)
            return;
    viewPnt = p;
}

// Application

time_t start = clock();

void Display() {
    glUseProgram(shader);
    // send window and camera specs
    float w = static_cast<float>(winWidth), h = static_cast<float>(winHeight), aspect = w/h;
    SetUniform(shader, "windowWidth", w);
    SetUniform(shader, "windowHeight", h);
    SetUniform(shader, "viewPnt", viewPnt);
    SetUniform(shader, "viewDir", viewDir);
    // compute, send screen vector specs
    float sc = sin(3.1451592f*fov/180);
    vec3 right = sc*normalize(cross(upDir, viewDir));
    vec3 up = (sc/aspect)*upDir;
    SetUniform(shader, "right", right);
    SetUniform(shader, "up", up);
    // send planes, light
    SetUniform4v(shader, "planes", 6, &planes[0].x);
    SetUniform(shader, "light", light);
    // animate spheres and send
    vec4 ave = (spheres[0]+spheres[1]+spheres[2])/3;
    vec3 cen(ave.x, ave.y, ave.z); // center of mass
    float tot = (float)(clock()-start)/CLOCKS_PER_SEC;
    float a = 3.1415f*(-60.f*tot)/180.f, c = cos(a), s = sin(a); // 60 deg/sec
    for (int i = 0; i < 3; i++) {
        // revolve sphere around cen (move origin to cen, rotate, move back), set xSphere
        vec4 sph = spheres[i];
        vec3 q = vec3(sph.x, sph.y, sph.z)-cen, xq = vec3(q.x*c-q.z*s, q.y, q.x*s+q.z*c)+cen;
        xSpheres[i] = vec4(xq.x, xq.y, xq.z, sph.w);
    }
    glUniform4fv(glGetUniformLocation(shader, "spheres"), 3, &xSpheres[0].x);
    // redraw
    glDrawArrays(GL_QUADS, 0, 4);
    glFlush();
}

void Resize(GLFWwindow *window, int width, int height) {
    winWidth = width;
    winHeight = height;
    glViewport(0, 0, width, height);
}

int main(int ac, char **av) {
    // init window
    glfwInit();
    window = glfwCreateWindow(winWidth, winHeight, "Ray-Trace Demo", NULL, NULL);
    glfwSetWindowPos(window, 100, 100);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    // callbacks
    glfwSetCursorPosCallback(window, MouseMove);
    glfwSetMouseButtonCallback(window, MouseButton);
    glfwSetScrollCallback(window, MouseWheel);
    glfwSetWindowSizeCallback(window, Resize);
    // make shader program
    const char *quadVertexShader = R"(
        #version 130
        vec2 pts[] = vec2[4](vec2(-1,-1), vec2(-1,1), vec2(1,1), vec2(1,-1));
        void main() { gl_Position = vec4(pts[gl_VertexID], 0, 1); }
    )";
    int v = CompileShaderViaCode(&quadVertexShader, GL_VERTEX_SHADER);
    int f = CompileShaderViaFile("C:/Users/Jules/Code/Exe/3-RayTrace.glsl", GL_FRAGMENT_SHADER);
    shader = LinkProgram(v, f);
    while (!glfwWindowShouldClose(window)) {
        Display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}
