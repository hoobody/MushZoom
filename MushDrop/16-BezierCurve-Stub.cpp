// BezierCurve.cpp - interactive curve design

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "Camera.h"
#include "Draw.h"           // for Line()
#include "GLXtras.h"
#include "Widgets.h"

int     winW = 900, winH = 800;
Camera  camera(winW, winH, vec3(0,0,0), vec3(0,0,-5));
Mover   ptMover;

// Bezier curve

class Bezier {
public:
    vec3 p1, p2, p3, p4;    // control points
    int res;                // display resolution
    Bezier(const vec3 &p1, const vec3 &p2, const vec3 &p3, const vec3 &p4, int res = 50) :
        p1(p1), p2(p2), p3(p3), p4(p4), res(res) { }
    vec3 Point(float t) {
        // return a point on the Bezier curve given parameter t, in (0,1)
    }
    void Draw(vec3 color, float width) {
        // break the curve into res number of straight pieces
        // *** render each piece with Line() ***
    }
    void DrawControlMesh(vec3 pointColor, vec3 meshColor, float opacity, float width) {
        // draw the four control points and the mesh that connects them
    }
    vec3 *PickPoint(int x, int y, mat4 view) {
        // return pointer to nearest control point, if within 10 pixels of mouse (x,y), else NULL
        // hint: use ScreenDistSq
        return NULL;
    }
};

Bezier  curve();//vec3(???), vec3(???), vec3(???), vec3(???));

// Display

void Display() {
    // background, blending, zbuffer
    glClearColor(.6f, .6f, .6f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    // draw curve and control polygon
    UseDrawShader(camera.fullview); // no shading, so single matrix
    curve.Draw(vec3(.7f, .2f, .5f), 3.5f);
    curve.DrawControlMesh(vec3(0, .4f, 0), vec3(1, 1, 0), 1, 2.5f);
    glFlush();
}

// Mouse

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
    y = WindowHeight(w)-y;              // invert y for upward-increasing screen space
    if (action == GLFW_RELEASE) {
        camera.MouseUp();               // reset rotation accumulation
        ptMover.Unset();                // deselect control point
    }
    if (action == GLFW_PRESS) {
        vec3 *pp = curve.PickPoint(x, y, camera.fullview);
        if (pp) {
            if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
                ptMover.Down(pp, x, y, camera.modelview, camera.persp);
        }
        else
            camera.MouseDown(x, y);
    }
}

void MouseMove(GLFWwindow *w, double x, double y) {
    y = WindowHeight(w)-y;
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) { // drag
        if (ptMover.IsSet())
            ptMover.Drag((int) x, (int) y, camera.modelview, camera.persp);
        else
            camera.MouseDrag((int) x, (int) y, Shift(w));
    }
}

void MouseWheel(GLFWwindow *w, double xoffset, double yoffset) {
    camera.MouseWheel(yoffset, Shift(w));
}

// Application

void Resize(GLFWwindow *w, int width, int height) {
    glViewport(0, 0, winW = width, winH = height);
    camera.Resize(width, height);
}

int main(int ac, char **av) {
    // init app window and GL context
    glfwInit();
    GLFWwindow *w = glfwCreateWindow(winW, winH, "Bezier Curve", NULL, NULL);
    glfwSetWindowPos(w, 100, 100);
    glfwMakeContextCurrent(w);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    // callbacks
    glfwSetCursorPosCallback(w, MouseMove);
    glfwSetMouseButtonCallback(w, MouseButton);
    glfwSetScrollCallback(w, MouseWheel);
    glfwSetWindowSizeCallback(w, Resize);
   // event loop
    while (!glfwWindowShouldClose(w)) {
        Display();
        glfwPollEvents();
        glfwSwapBuffers(w);
    }
    glfwDestroyWindow(w);
    glfwTerminate();
}
