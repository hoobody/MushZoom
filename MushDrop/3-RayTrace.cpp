// 3-RayTrace.cpp: non-interactive, ray-trace animation; needs 3-RayTrace.glsl

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <time.h>
#include "GLXtras.h"

// one light, six planes, three spheres
vec3 light(2, 0, 1);
vec4 planes[] = {vec4(-1,0,0,-3), vec4(1,0,0,-3), vec4(0,-1,0,-3), vec4(0,1,0,-3), vec4(0,0,-1,-3), vec4(0,0,1,-3)};
vec4 spheres[] = {vec4(-1.7f,-.3f,2,.6f), vec4(0,.1f,2,.9f), vec4(1.3f,0,2,.4f)};

// shader program ID, window size, application start time
GLuint shader = 0;
int winWidth = 800, winHeight = 800;
time_t start = clock();

void Display() {
    glUseProgram(shader);
    // set window, aspectRatio, let viewPoint and viewDirection default
    SetUniform(shader, "windowWidth", (float) winWidth);
    SetUniform(shader, "windowHeight", (float) winHeight);
    // send planes, light to GPU
    SetUniform4v(shader, "planes", 6, &planes[0].x);
    SetUniform(shader, "light", light);
    vec4 ave = (spheres[0]+spheres[1]+spheres[2])/3;
    vec3 com(ave.x, ave.y, ave.z); // center of mass
    // animate spheres at 60 degrees/second
    float tot = (float)(clock()-start)/CLOCKS_PER_SEC;
    float a = 3.1415f*(-60.f*tot)/180.f, c = cos(a), s = sin(a);
    vec4 xSpheres[3];
    for (int i = 0; i < 3; i++) {
        // revolve sphere around cen (move origin to cen, rotate, move back), set xSphere
        vec4 sph = spheres[i];
        vec3 q = vec3(sph.x, sph.y, sph.z)-com, xq = vec3(q.x*c-q.z*s, q.y, q.x*s+q.z*c)+com;
        xSpheres[i] = vec4(xq.x, xq.y, xq.z, sph.w);
    }
    // transfer sphere data to shaders, redraw
    SetUniform4v(shader, "spheres", 3, &xSpheres[0].x);
    glDrawArrays(GL_QUADS, 0, 4);
    glFlush();
}

int main(int ac, char **av) {
    // init window
    if (!glfwInit())
        return 1;
    GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "Ray Trace", NULL, NULL);
    if (!w) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(w);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    printf("GL version: %s\n", glGetString(GL_VERSION));
    PrintGLErrors();
    // make shader program
    const char *quadVertexShader = R"(
        #version 130
        vec2 pts[] = vec2[4](vec2(-1,-1), vec2(-1,1), vec2(1,1), vec2(1,-1));
        void main() { gl_Position = vec4(pts[gl_VertexID], 0, 1); }
    )";
    int v = CompileShaderViaCode(&quadVertexShader, GL_VERTEX_SHADER);
    int f = CompileShaderViaFile("3-RayTrace.glsl", GL_FRAGMENT_SHADER);
    shader = LinkProgram(v, f);
    if (!shader) {
        printf("*** can't link shader program\n");
        getchar();
    }
    while (!glfwWindowShouldClose(w)) {
        Display();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }
    glfwDestroyWindow(w);
    glfwTerminate();
}
