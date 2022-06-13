// Stub-BezierTess.cpp - textured Bezier patch using tessellation shader

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "Misc.h"
#include "Widgets.h"
#include "VecMat.h"

// display parameters
int         winWidth = 800, winHeight = 600;
Camera      camera(winWidth, winHeight, vec3(0, 0, 0), vec3(0, 0, -2));

// 16 movable control points and movable light
const int	npts = 16;
vec3        ctrlPts[npts];
vec3        light(-.2f, .4f, .3f);

// UI
void       *picked = NULL;
bool        viewMesh = true;
Mover       mover;

// shading
GLuint      program = 0;
int			textureName = 0, textureUnit = 0;
const char *textureFilename = "C:/Users/You/YourTexture.tga";

// vertex shader (no op)
const char *vShaderCode = "void main() { gl_Position = vec4(0); } // no-op";

// tessellation evaluation
const char *teShaderCode = R"(
    #version 400 core
    layout (quads, equal_spacing, ccw) in;
    uniform mat4 modelview, persp;
    out vec3 point, normal;
    out vec2 uv;
    void main() {
        uv = gl_TessCoord.st;
		vec3 p, n;
		point = (modelview*vec4(p, 1)).xyz;
		normal = (modelview*vec4(n, 0)).xyz;
        gl_Position = persp*vec4(point, 1);
    }
)";

// pixel shader
const char *pShaderCode = R"(
    #version 130 core
    in vec3 point, normal;
    in vec2 uv;
    uniform sampler2D textureMap;
    uniform vec3 light;
    void main() {
        vec3 N = normalize(normal);             // surface normal
        vec3 L = normalize(light-point);        // light vector
        vec3 E = normalize(pos);                // eye vertex
        vec3 R = reflect(L, N);                 // highlight vector
        float dif = max(0, dot(N, L));          // one-sided diffuse
        float spec = pow(max(0, dot(E, R)), 50);
        float ad = clamp(.15+dif, 0, 1);
        vec3 texColor = texture(textureMap, uv).rgb;
        gl_FragColor = vec4(ad*texColor+vec3(spec), 1);
    }
)";

// display

int WindowHeight(GLFWwindow *w) {
    int width, height;
    glfwGetWindowSize(w, &width, &height);
    return height;
}

void Display() {
    // background, blending, zbuffer
    glClearColor(.6f, .6f, .6f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    // update control points
    GLint id = glGetUniformLocation(program, "ctrlPts");
    glUniform3fv(id, npts, (float *) ctrlPts);
    // update matrices
    SetUniform(program, "modelview", camera.modelview);
    SetUniform(program, "persp", camera.persp);
	// set texture
	SetUniform(program, "textureMap", textureUnit);
    glActiveTexture(GL_TEXTURE0+textureUnit);       // active texture corresponds with textureUnit
	glBindTexture(GL_TEXTURE_2D, textureName);      // bind active texture to textureName
	// transform light and send to pixel shader
    vec4 hLight = camera.modelview*vec4(light, 1);
    glUniform3fv(glGetUniformLocation(program, "light"), 1, (float *) &hLight);
    // tessellate patch
    glPatchParameteri(GL_PATCH_VERTICES, 4);
    float res = 25, outerLevels[] = {res, res, res, res}, innerLevels[] = {res, res};
    glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outerLevels);
    glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, innerLevels);
    glDrawArrays(GL_PATCHES, 0, 4);
    // control mesh and light
    glDisable(GL_DEPTH_TEST);
    UseDrawShader(camera.fullview);
    if (viewMesh) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 3; j++) {
                Line(ctrlPts[4*i+j], ctrlPts[4*i+j+1], 1.25f, vec3(1, 1, 0));
                Line(ctrlPts[4*j+i], ctrlPts[4*j+1+i], 1.25f, vec3(1, 1, 0));
            }
        for (int i = 0; i < npts; i++)
            Disk(ctrlPts[i], 7, vec3(1,1,0));
    }
    Disk(light, 12, vec3(1, 0, 0));
    glFlush();
}

// mouse

bool Shift(GLFWwindow *w) {
    return glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
           glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
}

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
    double x, y;
    glfwGetCursorPos(w, &x, &y);
    y = WindowHeight(w)-y; // invert y for upward-increasing screen space
    picked = NULL;
    if (action == GLFW_RELEASE)
        camera.MouseUp();
    if (action == GLFW_PRESS) {
		vec3 *pp = viewMesh? PickControlPoint(x, y) : NULL;
		if (pp) {
			// pick or deselect control point
			if (butn == GLFW_MOUSE_BUTTON_LEFT) {
				mover.Down(pp, (int) x, (int) y, camera.modelview, camera.persp);
				picked = &mover;
			}
		}
		else if (MouseOver(x, y, light, camera.fullview)) {
			mover.Down(&light, (int) x, (int) y, camera.modelview, camera.persp);
			picked = &mover;
		}
		else {
			picked = &camera;
			camera.MouseDown(x, y);
		}
	}
}

void MouseMove(GLFWwindow *w, double x, double y) {
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)) {
		y = WindowHeight(w)-y;
		if (picked == &mover)
			mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
		if (picked == &camera)
			camera.MouseDrag((int) x, (int) y, Shift(w));
	}
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
    camera.MouseWheel(spin > 0, Shift(w));
}

// patch

void DefaultControlPoints() {
	*** // set default points here (see suggestion in exercise 18.7)
}

// application

void Keyboard(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS && key == 'C')
		viewMesh = !viewMesh;
}

void Resize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main(int ac, char **av) {
    // init app window
    if (!glfwInit())
        return 1;
    GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "Bezier Texture Tess", NULL, NULL);
    glfwSetWindowPos(w, 100, 100);
    glfwMakeContextCurrent(w);
    // init OpenGL, shader program, patch, texture
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    program = LinkProgramViaCode(&vShaderCode, NULL, &teShaderCode, NULL, &pShaderCode);
    textureName = LoadTexture(textureFilename, textureUnit);
    DefaultControlPoints();
    // callbacks
    glfwSetCursorPosCallback(w, MouseMove);
    glfwSetMouseButtonCallback(w, MouseButton);
    glfwSetScrollCallback(w, MouseWheel);
    glfwSetKeyCallback(w, Keyboard);
    glfwSetWindowSizeCallback(w, Resize);
    // event loop
    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(w)) {
        Display();
        glfwPollEvents();
        glfwSwapBuffers(w);
    }
    glfwDestroyWindow(w);
    glfwTerminate();
}
