// BezierPatch.cpp - interactive patch design

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "VecMat.h"
#include "Widgets.h"

// display
GLFWwindow *window;
int         winWidth = 930, winHeight = 800;
Camera      camera(winWidth, winHeight, vec3(0,0,0), vec3(0,0,-5));

// widgets
int         xCursorOffset = -7, yCursorOffset = -3;
Mover       mover;
void       *picked = NULL, *hover = NULL;

// patch
int         res = 10;               // res*res quadrilaterals (res+1)**2 #vertices
vec3        ctrlPts[4][4];          // 16 control points, indexed [s][t]

// shading
GLuint      shader = 0, vBufferId = -1;
vec3        lightSource(.6f, .4f, .2f);

// Bezier operations

vec3 BezTangent(float t, vec3 b1, vec3 b2, vec3 b3, vec3 b4) {
    *** // return the tangent of the curve given parameter t
    // HERE
}

vec3 BezPoint(float t, vec3 b1, vec3 b2, vec3 b3, vec3 b4) {
    *** // return the position of the curve given parameter t
    // HERE
}

void BezierPatch(float s, float t, vec3 *point, vec3 *normal) {
    *** // set point and normal at patch (s, t)
    // HERE
};

const char *vShader = R"(
    #version 150
    in vec3 point;
    in vec3 normal;
    out vec3 vPoint;
    out vec3 vNormal;
    uniform mat4 modelview;
    uniform mat4 persp;
    void main() {
        vNormal = (modelview*vec4(normal, 0)).xyz;
        vPoint = (modelview*vec4(point, 1)).xyz;
        gl_Position = persp*vec4(vPoint, 1);
    }
)";

const char *pShader = R"(
    #version 150
    in vec3 vPoint;
    in vec3 vNormal;
    out vec4 pColor;
    uniform vec3 light;
    uniform vec4 color = vec4(0,.7,0,1);    // default
    void main() {
        vec3 N = normalize(vNormal);        // surface normal
        vec3 L = normalize(light-vPoint);   // light vector
        vec3 E = normalize(vPoint);         // eye vertex
        vec3 R = reflect(L, N);             // highlight vector
        float d = abs(dot(N, L));           // two-sided diffuse
        float s = abs(dot(R, E));           // two-sided specular
        float intensity = clamp(d+pow(s, 50), 0, 1);
        pColor = vec4(intensity*color.rgb, 1);
    }
)";

// vertex buffer

int nQuadrilaterals = 0;

void SetVertices(int res, bool init = false) {
    // activate GPU vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vBufferId);
    // get pointers to GPU memory for vertices, normals
    nQuadrilaterals = res*res;
    int sizeBuffer = 2*4*nQuadrilaterals*sizeof(vec3);
    if (init)
        glBufferData(GL_ARRAY_BUFFER, sizeBuffer, NULL, GL_STATIC_DRAW);
    *** // set the 4 vertices for each quadrilateral and save to GPU
    // HERE
}

// display

void Display() {
    // background, blending, zbuffer
    glClearColor(.6f, .6f, .6f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    // invoke Phong shader
    glUseProgram(shader);
    // send matrices to vertex shader
    SetUniform(shader, "modelview", camera.modelview);
    SetUniform(shader, "persp", camera.persp);
    // set vertex feed
    glBindBuffer(GL_ARRAY_BUFFER, vBufferId);
    // just send fixed light to fragment shader (in previous assignments we have transformed the light
    // by the modelview matrix in order that the light be part of the same scene containing the object)
    // here, the light doesn't move, even though the object may be rotated, giving the impression of
    // something be examined, rather than the camera moving around a scene
    vec4 hLight = camera.modelview*vec4(lightSource, 1);
    SetUniform(shader, "light", vec3(hLight.x, hLight.y, hLight.z));
    // connect patch points and normals to vertex shader
    VertexAttribPointer(shader, "point", 3, 2*sizeof(vec3), (void *) 0);
    VertexAttribPointer(shader, "normal", 3, 2*sizeof(vec3), (void *) sizeof(vec3));
    // shade patch
    glDrawArrays(GL_QUADS, 0, 4*nQuadrilaterals);
    UseDrawShader(camera.fullview);
    glDisable(GL_DEPTH_TEST);
    Disk(lightSource, 12, hover == (void *) &lightSource? vec3(0,1,1) : IsVisible(lightSource, camera.fullview)? vec3(1,0,0) : vec3(0,0,1));
    *** // draw 16 movable points and lines between control mesh
    // HERE
    glFlush();
}

// mouse

vec3 *PickControlPoint(int x, int y, bool rightButton) {
    for (int k = 0; k < 16; k++)
        if (MouseOver(x, y, ctrlPts[k/4][k%4], camera.fullview, xCursorOffset, yCursorOffset))
            return &ctrlPts[k/4][k%4];
    return NULL;
}

int WindowHeight() {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    return height;
}

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
    double x, y;
    glfwGetCursorPos(w, &x, &y);
    y = WindowHeight()-y; // invert y for upward-increasing screen space
    hover = picked = NULL;
    if (action == GLFW_RELEASE)
        camera.MouseUp();
    if (action == GLFW_PRESS) {
        vec3 *pp = PickControlPoint(x, y, butn == GLFW_MOUSE_BUTTON_RIGHT);
        if (pp) {
            // pick or deselect control point
            if (butn == GLFW_MOUSE_BUTTON_LEFT) {
                mover.Set(pp, x, y, camera.modelview, camera.persp);
                picked = &mover;
            }
        }
        else if (MouseOver(x, y, lightSource, camera.fullview, xCursorOffset, yCursorOffset)) {
            mover.Set(&lightSource, x, y, camera.modelview, camera.persp);
            picked = &mover;
        }
        else {
            picked = &camera;
            camera.MouseDown(x, y);
        }
    }
}

void MouseMove(GLFWwindow *w, double x, double y) {
    y = WindowHeight()-y; // invert y for upward-increasing screen space
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) { // drag
        bool shift = glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                     glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        if (picked == &mover) {
            mover.Drag(x, y, camera.modelview, camera.persp);
            SetVertices(res);
        }
        if (picked == &camera)
            camera.MouseDrag((int) x, (int) y, shift);
    }
    else
        hover = MouseOver(x, y, lightSource, camera.fullview, xCursorOffset, yCursorOffset)? (void *) &lightSource : NULL;
}

void MouseWheel(GLFWwindow *w, double xoffset, double yoffset) {
    camera.MouseWheel((int) yoffset, Shift(w));
}

// patch

void DefaultControlPoints() {
    vec3 p0(.5f, -.25f, 0), p1(.5f, .25f, 0), p2(-.5f, -.25f, 0), p3(-.5f, .25f, 0);
    float vals[] = {0, 1/3.f, 2/3.f, 1.};
    for (int i = 0; i < 16; i++) {
        float ax = vals[i%4], ay = vals[i/4];
        vec3 p10 = p0+ax*(p1-p0), p32 = p2+ax*(p3-p2);
        ctrlPts[i/4][i%4] = p10+ay*(p32-p10);
    }
}

// application

static void Keyboard(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == 'R' && action == GLFW_PRESS) {
        res += mods & GLFW_MOD_SHIFT? -1 : 1;
        res = res < 1? 1 : res;
        SetVertices(res, true);
    }
}

void Resize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main(int ac, char **av) {
    // init app window
    glfwInit();
    window = glfwCreateWindow(winWidth, winHeight, "Simple Bezier Patch", NULL, NULL);
    glfwSetWindowPos(window, 100, 100);
    glfwMakeContextCurrent(window);
    // init OpenGL
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glViewport(0, 0, winWidth, winHeight);
    // make shader program
    shader = LinkProgramViaCode(&vShader, &pShader);
    // init patch
    DefaultControlPoints();
    // make vertex buffer
    glGenBuffers(1, &vBufferId);
    SetVertices(res, true);
    // callbacks
    glfwSetCursorPosCallback(window, MouseMove);
    glfwSetMouseButtonCallback(window, MouseButton);
    glfwSetScrollCallback(window, MouseWheel);
    glfwSetKeyCallback(window, Keyboard);
    glfwSetWindowSizeCallback(window, Resize);
    // event loop
    while (!glfwWindowShouldClose(window)) {
        Display();
        glfwPollEvents();
        glfwSwapBuffers(window);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}
