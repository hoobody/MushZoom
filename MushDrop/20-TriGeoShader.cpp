// TriGeoShader.cpp: use geometry shader to subdivide and outline triangle

#include <glad.h>
#include <glfw/glfw3.h>
#include <stdio.h>
#include <time.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "Text.h"
#include "Widgets.h"

// shader ids
GLuint      vBuffer = 0, program = 0;

// equilateral triangle
float       h = 1.f/(float)sqrt(3), points[][3] = {{.5f, -h/2, 0}, {0, h, 0}, {-.5f, -h/2, 0}};

// display
int         winWidth = 800, winHeight = 800;
Camera      camera(winWidth, winHeight, vec3(70,0,0), vec3(0,0,-5));

// display params
enum Mode   { M_Shade = 0, M_Line, M_HLE } mode = M_Shade;
float       lineWidth = 5, outlineTransition = 2; // in pixels
vec3        light(.3f, .2f, -.2f);

// interaction
void       *picked = NULL, *hover = NULL;
Mover       mover;

const char *vertexShader = R"(
    #version 130
    uniform mat4 modelview;
    in vec3 pt;
    out vec3 vPt;
    void main() {
        vPt = (modelview*vec4(pt, 1)).xyz;
    }
)";

const char *geometryShader = R"(
    #version 330 core
    layout (triangles) in;
    layout (triangle_strip, max_vertices = 12) out;
    uniform mat4 persp;
    uniform mat4 viewpt;
    in vec3 vPt[];
    out vec3 gPt;
    out vec3 gNrm;
    out vec3 gColor;
    noperspective out vec3 gEdgeDistance;
    vec2 ScreenPoint(vec4 h) {
        // perspective divide and viewport transform:
        return (viewpt*vec4(h.xyz/h.w, 1)).xy;
    }
    void EmitTriangle(vec3 v0, vec3 v1, vec3 v2, vec3 color) {
        // transform each vertex into perspective and into screen space
        vec4 x0 = persp*vec4(v0, 1), x1 = persp*vec4(v1, 1), x2 = persp*vec4(v2, 1);
        vec2 p0 = ScreenPoint(x0), p1 = ScreenPoint(x1), p2 = ScreenPoint(x2);
        // find altitudes ha, hb, hc (courtesy OpenGL4 Cookbook by Wolff)
        float a = length(p2-p1), b = length(p2-p0), c = length(p1-p0);
        float alpha = acos((b*b+c*c-a*a)/(2*b*c));
        float beta = acos((a*a+c*c-b*b)/(2*a*c));
        float ha = abs(c*sin(beta)), hb = abs(c*sin(alpha)), hc = abs(b*sin(alpha));
        // tetrahedron is faceted, so output same normal for triangle vertices
        gNrm = normalize(cross(v2-v1, v1-v0));
        // output edge dist, color, and homog & non-homog loc for three vertices
        for (int i = 0; i < 3; i++) {
            gEdgeDistance = i==0? vec3(ha, 0, 0) : i==1? vec3(0, hb, 0) : vec3(0, 0, hc);
            gColor = color;
            gl_Position = i == 0? x0 : i == 1? x1 : x2;
            gPt = i == 0? v0 : i == 1? v1 : v2;
            EmitVertex();
        }
        EndPrimitive();
    }
    void main() {
        // compute fourth vertex
        vec3 center = (vPt[0]+vPt[1]+vPt[2])/3;
        vec3 n = normalize(cross(vPt[2]-vPt[1], vPt[1]-vPt[0]));
        float sLen = length(vPt[1]-vPt[0]);
        vec3 pt3 = center+sLen*sqrt(2)/sqrt(3)*n;
        // output tetrahedron, each face different color
        EmitTriangle(vPt[2], vPt[1], vPt[0], vec3(1,0,0));
        EmitTriangle(vPt[0], vPt[1], pt3, vec3(0,1,0));
        EmitTriangle(vPt[1], vPt[2], pt3, vec3(0,0,1));
        EmitTriangle(vPt[2], vPt[0], pt3, vec3(1,1,0));
    }
)";

const char *pixelShader = R"(
    #version 330
    uniform vec3 light;
    uniform float outlineWidth = 1;
    uniform float transition = 1;
    uniform int mode = 0; // 0: Shade, 1: Lines, 2: HLE
    in vec3 gPt;
    in vec3 gNrm;
    in vec3 gColor;
    noperspective in vec3 gEdgeDistance;
    out vec4 pColor;
    void main() {
        // compute triangle normal for faceted shading
        vec3 N = normalize(gNrm), E = -gPt;
        bool sideViewer = dot(E, N) < 0;
        // given local lights, compute total diffuse intensity
        float intensity = .2;
        vec3 L = normalize(light-gPt);
        bool sideLight = dot(L, N) < 0;
        if (sideLight == sideViewer)
            intensity += max(0, dot(N, L));
        intensity = clamp(intensity, 0, 1);
        // get dist to nearest edge and map to 0,1
        float minDist = min(min(gEdgeDistance[0], gEdgeDistance[1]), gEdgeDistance[2]);
        float t = smoothstep(outlineWidth-transition, outlineWidth+transition, minDist);
        // mix edge and surface colors (t=0: edgeColor, t=1: surfaceColor)
        pColor = mix(vec4(0,0,0,1), vec4(intensity*gColor,1), t);
        if (mode == 1)
            pColor = vec4(0,0,0,1-t);
        if (mode == 2)
            pColor = vec4(t,t,t,1);
    }
)";

void Display() {
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // transform light
    vec4 xl = camera.modelview*vec4(light, 1);
    // send uniforms
    SetUniform(program, "light", (vec3 *) &xl);
    SetUniform(program, "viewpt", Viewport());
    SetUniform(program, "modelview", camera.modelview);
    SetUniform(program, "persp", camera.persp);
    SetUniform(program, "outlineWidth", lineWidth);
    SetUniform(program, "transition", outlineTransition);
    SetUniform(program, "mode", (int) mode);
    // z-buffer, blending
    if (mode == M_Line) {
        glDisable(GL_DEPTH_TEST);   // draw all lines
        glEnable(GL_BLEND);         // enable pixel shader alpha value
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    // set vertex fetch, render
    VertexAttribPointer(program, "pt", 3, 0, (void *) 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    // light
    glDisable(GL_DEPTH_TEST);
    UseDrawShader(ScreenMode());
    Text(10,10,vec3(0,0,0),12,"light:%4.3f,%4.3f,%4.3f",light.x,light.y,light.z);
    UseDrawShader(camera.fullview);
    bool lVisible = IsVisible(light, camera.fullview);
    Disk(light, 12, hover == &light? lVisible? vec3(1,0,0) : vec3(1,0,1) : lVisible? vec3(1,1,0) : vec3(0,1,1));
    glFlush();
}

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
    if (action == GLFW_PRESS) {
        double x, y;
        glfwGetCursorPos(w, &x, &y);
        y = WindowHeight(w)-y;
        picked = NULL;
        if (MouseOver(x, y, light, camera.fullview)) {
            mover.Down(&light, (int) x, (int) y, camera.modelview, camera.persp);
            picked = &mover;
        }
        if (picked == NULL) {
            camera.MouseDown(x, y);
            picked = &camera;
        }
    }
    if (action == GLFW_RELEASE)
        camera.MouseUp();
}

void MouseMove(GLFWwindow *w, double x, double y) {
    y = WindowHeight(w)-y;
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (picked == &mover)
            mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
        if (picked == &camera)
            camera.MouseDrag((int) x, (int) y, Shift(w));
    }
    else
        hover = MouseOver(x, y, light, camera.fullview)? (void *) &light : NULL;
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
    camera.MouseWheel(spin > 0, Shift(w));
}

void Resize(GLFWwindow *w, int width, int height) {
    camera.Resize(winWidth = width, winHeight = height);
    glViewport(0, 0, winWidth, winHeight);
}

void Keyboard(GLFWwindow *w, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        bool shift = mods & GLFW_MOD_SHIFT;
        if (key == 'T')
            outlineTransition *= (shift? .8f : 1.2f);
        if (key == 'W')
            lineWidth *= (shift? .8f : 1.2f);
        if (key == 'M')
            mode = (Mode) ((mode+1)%3);
    }
}

const char *usage = "\n\
    T: outline transition\n\
    W: line width\n\
    M: Shade/Line/HLE\n";

int main() {
    glfwInit();
    GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "Geometry Shader Demo", NULL, NULL);
    glfwMakeContextCurrent(w);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    program = LinkProgramViaCode(&vertexShader, NULL, NULL, &geometryShader, &pixelShader);
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    glfwSetCursorPosCallback(w, MouseMove);
    glfwSetMouseButtonCallback(w, MouseButton);
    glfwSetScrollCallback(w, MouseWheel);
    glfwSetWindowSizeCallback(w, Resize);
    glfwSetKeyCallback(w, Keyboard);
    printf("Usage: %s\n", usage);
    while (!glfwWindowShouldClose(w)) {
        Display();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }
    glfwDestroyWindow(w);
    glfwTerminate();
}

