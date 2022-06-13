// FontTest.cpp: test FreeType - see Text.cpp

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "Draw.h"
#include "GLXtras.h"
#include "Text.h"

GLuint program;

void DisplayText(const char *fontName, const char *text, int y) {
    SetFont(fontName, 16, 60);  // exact affect of charRes, pixelRes unclear
    Text(20, y, vec3(1,0,0), 10, text);
}

void Display() {
    glClearColor(.8f, .8f, .8f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // total build time for following 14 fonts ~ .25 secs
    DisplayText("C:/Fonts/OpenSans/OpenSans-Bold.ttf", "OpenSans-Bold", 640);
    DisplayText("C:/Fonts/OpenSans/OpenSans-BoldItalic.ttf", "OpenSans-BoldItalic", 600);
    DisplayText("C:/Fonts/OpenSans/OpenSans-ExtraBold.ttf", "OpenSans-ExtraBold", 560);
    DisplayText("C:/Fonts/OpenSans/OpenSans-ExtraBoldItalic.ttf", "OpenSans-ExtraBoldItalic", 520);
    DisplayText("C:/Fonts/OpenSans/OpenSans-Italic.ttf", "OpenSans-Italic", 480);
    DisplayText("C:/Fonts/OpenSans/OpenSans-Light.ttf", "OpenSans-Light", 440);
    DisplayText("C:/Fonts/OpenSans/OpenSans-LightItalic.ttf", "OpenSans-LightItalic", 400);
    DisplayText("C:/Fonts/OpenSans/OpenSans-Regular.ttf", "OpenSans-Regular", 360);
    DisplayText("C:/Fonts/OpenSans/OpenSans-SemiBold.ttf", "OpenSans-SemiBold", 320);
    DisplayText("C:/Fonts/OpenSans/OpenSans-SemiBoldItalic.ttf", "OpenSans-SemiBoldItalic", 280);
    DisplayText("C:/Fonts/SansSerifHeavy.ttf", "SansSerifHeavy", 240);
    DisplayText("C:/Fonts/SansSerif.ttf", "SansSerif", 200);
    DisplayText("C:/Fonts/SansSerifLight.otf", "SansSerifLight", 160);
    DisplayText("C:/Fonts/Blazed.ttf", "Blazed", 120);
    glFlush();
}

static void ErrorGFLW(int id, const char *reason) {
    printf("GFLW error %i: %s\n", id, reason);
}

static void Keyboard(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void Resize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main(void) {
    int winWidth = 720, winHeight = 720;
    glfwSetErrorCallback(ErrorGFLW);
    if (!glfwInit())
        return 1;
    GLFWwindow *window = glfwCreateWindow(winWidth, winHeight, "Font Test", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    PrintGLErrors();
    glViewport(0, 0, winWidth, winHeight);
    glfwSetKeyCallback(window, Keyboard);
    glfwSetWindowSizeCallback(window, Resize);
    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(window)) {
        Display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}
