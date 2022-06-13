// BezierLOD.cpp - faceted level of detail patch display

#include <glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <time.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "Text.h"
#include "VecMat.h"
#include "Widgets.h"

// window, shader
int             winWidth = 800, winHeight = 800;
Camera          camera(winWidth, winHeight, vec3(0,0,0), vec3(0,0,-5));
GLuint          shader = 0;

// Bezier patch to tessellate into triangles
vec3            ctrlPts[4][4];                          // 16 control points, indexed [s][t]
vec3            coeffs[4][4];                           // alternative 16 poly coeffs in x, y, z, indexed [s][t]

// movable light
vec3            light(-.2f, .4f, .3f);

// UI
int             xCursorOffset = -7, yCursorOffset = -3;
Mover           mover;
void           *picked = NULL, *hover = NULL;
Magnifier       magnifier(int2(400, 400), int2(300, 300), 10);

// display options
bool            viewMesh = true, useLod = true, outline = true, magnify = true, zfight = false;
Toggler         zfightTog(&zfight, "z-fight", 20, 125, 12),
                outlineTog(&outline, "outline", 20, 100, 12),
                useLodTog(&useLod, "use LOD", 20, 75, 12),
                viewMeshTog(&viewMesh, "control mesh", 20, 50, 12),
				magnifyTog(&magnify, "magnify", 20, 25, 12);
Toggler		   *togs[] = { &outlineTog, &zfightTog, &useLodTog, &viewMeshTog, &magnifyTog };
int             res = 25, ntogs = sizeof(togs)/sizeof(Toggler *);
float           outlineWidth = 1, outlineTransition = 1;
time_t          tEvent = clock();

// vertex shader
const char *vShaderCode = R"(
    #version 130
    void main() {
        gl_Position = vec4(0);
    }
)";

// tessellation control
const char *tcShaderCode = R"(
    #version 400 core
    layout (vertices = 4) out;
    uniform vec3 ctrlPts[16];
    uniform mat4 modelview;
    uniform mat4 persp;
    uniform mat4 viewportMatrix;
    uniform int pixelsPerEdge = 0;  // 0 for do not use LOD
    uniform int fixedRes = 10;      // if do not use LOD
    void main() {
        if (gl_InvocationID == 0) { // only set once
            // see www.khronos.org/opengl/wiki/Tessellation
            // outerLevels: left, bottom, right, top edges
            // innerLevels: left/right, bottom/top divisions
            if (pixelsPerEdge > 0) {
                mat4 m = viewportMatrix*persp*modelview;
                // test distance bet pairs of corner ctrl pts
                // *** should use control mesh outer edge lengths
                vec3 quad[] = vec3[](ctrlPts[12], ctrlPts[0], ctrlPts[3], ctrlPts[15]);
                vec2 quadS[4];  // in screen space
                for (int i = 0; i < 4; i++) {
                    vec4 h = m*vec4(quad[i], 1);
                    quadS[i] = h.xy/h.w;
                }
                // set outer res
                for (int i = 0; i < 4; i++) {
                    float d = distance(quadS[i], quadS[(i+1)%4]);
                    gl_TessLevelOuter[i] = max(2, d/pixelsPerEdge);
                }
                // set inner res as average outer res
                gl_TessLevelInner[0] = .5*(gl_TessLevelOuter[0]+gl_TessLevelOuter[2]);
                gl_TessLevelInner[1] = .5*(gl_TessLevelOuter[1]+gl_TessLevelOuter[3]);
            }
            else
                for (int i = 0; i < 4; i++)
                    gl_TessLevelInner[i%2] = gl_TessLevelOuter[i] = fixedRes;
        }
    }
)";

// tessellation evaluation - note comparison coeffs vs control points
const char *teShaderCode = R"(
    #version 400 core
    layout (quads, equal_spacing, ccw) in;
    uniform vec3 ctrlPts[16];
    uniform vec3 coeffs[16];
    uniform mat4 modelview;
    uniform mat4 persp;
    out vec3 vPosition;
    vec3 CurvePoint(float t, vec3 b1, vec3 b2, vec3 b3, vec3 b4) {
        float t2 = t*t, t3 = t*t2;
        return (-t3+3*t2-3*t+1)*b1+(3*t3-6*t2+3*t)*b2+(3*t2-3*t3)*b3+t3*b4;
        // 22 ops/coord
    }
    vec3 PatchPoint1(float s, float t) { // form 1 curve across 4 curves via CurvePoint, ctrlPts
        vec3 spts[4];
        for (int i = 0; i < 4; i++)
            spts[i] = CurvePoint(s, ctrlPts[4*i], ctrlPts[4*i+1], ctrlPts[4*i+2], ctrlPts[4*i+3]);
        return CurvePoint(t, spts[0], spts[1], spts[2], spts[3]);
        // 5 calls to CurvePoint, total 110 ops/coord
    }
    vec3 PatchPoint2(float s, float t) { // form 1 curve across 4 curves via coeffs
        float s2 = s*s, s3 = s*s2, t2 = t*t, t3 = t*t2;
        vec3 spts[4];
        for (int i = 0; i < 4; i++)
            spts[i] = s3*coeffs[4*i]+s2*coeffs[4*i+1]+s*coeffs[4*i+2]+coeffs[4*i+3];
        return t3*spts[0]+t2*spts[1]+t*spts[2]+spts[3];
        // 34 ops/coord
    }
    vec3 PatchPoint3(float s, float t) { // direct evaluation of coeffs
        float s2 = s*s, s3 = s*s2, t2 = t*t, ta[] = float[](t*t2, t2, t, 1);
        vec3 ret = vec3(0, 0, 0);
        for (int i = 0; i < 4; i++)
            ret += ta[i]*(s3*coeffs[4*i]+s2*coeffs[4*i+1]+s*coeffs[4*i+2]+coeffs[4*i+3]);
        return ret;
        // 32 ops/coord
    }
    void main() {
        vec3 p = PatchPoint3(gl_TessCoord.st.s, gl_TessCoord.st.t);
        vPosition = (modelview*vec4(p, 1)).xyz;
        gl_Position = persp*vec4(vPosition, 1);
    }
)";

// geometry shader
const char *gShaderCode = R"(
    #version 330 core
    layout (triangles) in;
    layout (triangle_strip, max_vertices = 3) out;
    in vec3 vPosition[];
    out vec3 gPosition;
    noperspective out vec3 gEdgeDistance;
    uniform mat4 viewportMatrix;
    vec3 ViewSpacePoint(int i) {
        return vec3(viewportMatrix*(gl_in[i].gl_Position/gl_in[i].gl_Position.w));
    }
    void main() {
        float ha = 0, hb = 0, hc = 0;
        // from OpenGL4 Cookbook by Wolff, p 198 (also NVidia 2007 white paper Solid Wireframe)
        // transform each vertex into viewport space
        vec3 p0 = ViewSpacePoint(0), p1 = ViewSpacePoint(1), p2 = ViewSpacePoint(2);
        // find altitudes ha, hb, hc
        float a = length(p2-p1), b = length(p2-p0), c = length(p1-p0);
        float alpha = acos((b*b+c*c-a*a)/(2.*b*c));
        float beta = acos((a*a+c*c-b*b)/(2.*a*c));
        ha = abs(c*sin(beta));
        hb = abs(c*sin(alpha));
        hc = abs(b*sin(alpha));
        // send triangle vertices and edge distances
        for (int i = 0; i < 3; i++) {
            gEdgeDistance = i==0? vec3(ha, 0, 0) : i==1? vec3(0, hb, 0) : vec3(0, 0, hc);
            gPosition = vPosition[i];
            gl_Position = gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
)";

// pixel shader
const char *pShaderCode = R"(
    #version 130
    in vec3 gPosition;
    noperspective in vec3 gEdgeDistance;
    out vec4 fColor;
    uniform vec3 lightV;
    uniform vec4 shadeColor = vec4(1, 1, 1, 1);
    uniform vec4 outlineColor = vec4(0, 0, 0, 1);
    uniform float outlineWidth = 1;
    uniform float transition = 1;
    uniform bool outlineOn = true;
    void main() {
        vec3 dx = dFdy(gPosition.xyz);
        vec3 dy = dFdx(gPosition.xyz);
        vec3 n = normalize(cross(dx, dy));                  // faceted shading
        float intensity = clamp(abs(dot(n, normalize(lightV))), 0, 1);
        fColor = shadeColor;
        fColor.rgb *= intensity;
        if (outlineOn) {
            float minDist = min(gEdgeDistance.x, gEdgeDistance.y);
            minDist = min(minDist, gEdgeDistance.z);
            float t = smoothstep(outlineWidth-transition, outlineWidth+transition, minDist);
            // mix edge and surface colors(t=0: edgeColor, t=1: surfaceColor)
            fColor = mix(outlineColor, fColor, t);
        }
    }
)";

// Bezier control points

void SetCoeffs() {
    // set coeffs from ctrlPts
    mat4 m(vec4(-1, 3, -3, 1), vec4(3, -6, 3, 0), vec4(-3, 3, 0, 0), vec4(1, 0, 0, 0)), g;
    for (int k = 0; k < 3; k++) {
        for (int i = 0; i < 16; i++)
            g[i/4][i%4] = ctrlPts[i/4][i%4][k];
        mat4 c = m*g*m;
        for (int i = 0; i < 16; i++)
            coeffs[i/4][i%4][k] = c[i/4][i%4];
    }
}

void DefaultControlPoints() {
    float vals[] = {-.6f, -.2f, .2f, .6f};
    for (int i = 0; i < 16; i++)
        ctrlPts[i/4][i%4] = vec3(2*vals[i%4], vals[i/4], 0);
    SetCoeffs();
}

// display

vec3 BezPoint(float t, vec3 b1, vec3 b2, vec3 b3, vec3 b4) {
    float t2 = t*t, t3 = t*t2, T = 1-t, T2 = T*T, T3 = T*T2;
    return T3*b1+(3*t*T2)*b2+(3*t2*T)*b3+t3*b4;
}

void DrawCurve(vec3 b1, vec3 b2, vec3 b3, vec3 b4, vec3 color, float width) {
    for (int i = 0; i < res; i++)
        Line(BezPoint((float)i/res, b1, b2, b3, b4), BezPoint((float)(i+1)/res, b1, b2, b3, b4), width, color, 1);
}

void DrawGrid(vec3 color, float width) {
	for (int i = 0; i <= res; i++) {
		float a = (float)i/res;
		vec3 spts[4], tpts[4];
		for (int i = 0; i < 4; i++) {
			spts[i] = BezPoint(a, ctrlPts[i][0], ctrlPts[i][1], ctrlPts[i][2], ctrlPts[i][3]);
			tpts[i] = BezPoint(a, ctrlPts[0][i], ctrlPts[1][i], ctrlPts[2][i], ctrlPts[3][i]);
		}
		DrawCurve(spts[0], spts[1], spts[2], spts[3], color, width);
		DrawCurve(tpts[0], tpts[1], tpts[2], tpts[3], color, width);
	}
}

void Display() {
    // background, blending, zbuffer
    glClearColor(1,1,1,1); // .6f, .6f, .6f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);
    glUseProgram(shader);
    // send matrices to vertex shader
    SetUniform(shader, "viewportMatrix", Viewport());
    SetUniform(shader, "modelview", camera.modelview);
    SetUniform(shader, "persp", camera.persp);
    // send LOD params
    SetUniform(shader, "pixelsPerEdge", useLod? 100 : 0);
    SetUniform(shader, "fixedRes", res);
    // send ctrlPts and coeffs (see comparison in tess eval shader)
    SetUniform3v(shader, "ctrlPts", 16, (float *) &ctrlPts[0][0]);
    SetUniform3v(shader, "coeffs", 16, (float *) &coeffs[0][0]);
    // transform light, send to fragment shader
    vec4 xLightV = camera.modelview*vec4(light, 0);
    SetUniform3v(shader, "lightV", 1, (float *) &xLightV.x);
    // send HLE params
    SetUniform(shader, "outlineOn", outline);
    SetUniform(shader, "outlineWidth", outlineWidth);
    SetUniform(shader, "transition", outlineTransition);
    // tessellate and render patch
    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glDrawArrays(GL_PATCHES, 0, 4);
    // mesh and buttons without z-test
    UseDrawShader(camera.fullview);
	if (zfight)
		DrawGrid(vec3(0, 0, 0), 2*outlineWidth);
    glDisable(GL_DEPTH_TEST);
    // control mesh (disks and dashed lines)
    if (viewMesh) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 3; j++) {
                LineDash(ctrlPts[i][j], ctrlPts[i][j+1], camera.fullview, 1.25f, vec3(1,1,0), vec3(1,1,0));
                LineDash(ctrlPts[j][i], ctrlPts[j+1][i], camera.fullview, 1.25f, vec3(1,1,0), vec3(1,1,0));
            }
        for (int i = 0; i < 16; i++)
            Disk(ctrlPts[i/4][i%4], 7, vec3(1,1,0));
    }
    if ((float) (clock()-tEvent)/CLOCKS_PER_SEC < 1)
        Disk(light, 12, hover == (void *) &light? vec3(0,1,1) : IsVisible(light, camera.fullview)? vec3(1,0,0) : vec3(0,0,1));
    // draw controls in 2D pixel space
    UseDrawShader(ScreenMode());
	for (int i = 0; i < ntogs; i++)
		togs[i]->Draw(NULL, 10);
    Text(20, 20, vec3(0, 0, 0), 1, "res = %i", res);
    if (magnify)
        magnifier.Display(int2(10, 490));
    glFlush();
}

// mouse

int WindowHeight(GLFWwindow *w) {
    int width, height;
    glfwGetWindowSize(w, &width, &height); // GetFramebufferSize?
    return height;
}

bool Shift(GLFWwindow *w) {
    return glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
           glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
}

vec3 *PickControlPoint(int x, int y) {
    for (int k = 0; k < 16; k++)
        if (MouseOver(x, y, ctrlPts[k/4][k%4], camera.fullview))
            return &ctrlPts[k/4][k%4];
    return NULL;
}

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
    double x, y;
    glfwGetCursorPos(w, &x, &y);
    y = WindowHeight(w)-y;
    x += xCursorOffset;
    y += yCursorOffset;
	int ix = (int) x, iy = (int) y;
    hover = picked = NULL;
    if (action == GLFW_RELEASE)
        camera.MouseUp();
    if (action == GLFW_PRESS && butn == GLFW_MOUSE_BUTTON_LEFT) {
		for (int i = 0; i < ntogs; i++)
			if (togs[i]->DownHit(x, y, action)) {
				picked = togs[i];
				if (picked == &zfightTog) { outlineTog.Set(false); useLodTog.Set(false); }
				if (picked == &outlineTog || picked == &useLodTog) zfightTog.Set(false);
			}
        vec3 *pp = !picked && viewMesh? PickControlPoint(ix, iy) : NULL;
        if (pp) {
            mover.Down(pp, ix, iy, camera.modelview, camera.persp);
            picked = &mover;
        }
        if (!picked && MouseOver(ix, iy, light, camera.fullview)) {
            mover.Down(&light, ix, iy, camera.modelview, camera.persp);
            picked = &mover;
        }
        if (!picked && magnifier.Hit(ix, iy)) {
            picked = &magnifier;
            magnifier.Down(ix, iy);
        }
        if (!picked) {
            picked = &camera;
            camera.MouseDown(x, y);
        }
    }
}

void MouseMove(GLFWwindow *w, double x, double y) {
    tEvent = clock();
    x += xCursorOffset;
    y += yCursorOffset;
    y = WindowHeight(w)-y; // invert y for upward-increasing screen space
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) { // drag
        if (picked == &mover) {
            mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
            SetCoeffs();
        }
        if (picked == &magnifier)
            magnifier.Drag((int) x, (int) y);
        if (picked == &camera)
            camera.MouseDrag(x, y, Shift(w));
    }
    else
        hover = MouseOver(x, y, light, camera.fullview)? (void *) &light : NULL;
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
    camera.MouseWheel(spin > 0, Shift(w));
}

// application

void Resize(GLFWwindow *w, int width, int height) {
    camera.Resize(width, height);
    glViewport(0, 0, width, height);
}

void Keyboard(GLFWwindow *w, int c, int scancode, int action, int mods) {
    bool shift = mods & GLFW_MOD_SHIFT;
    if (action == GLFW_PRESS)
        switch (c) {
            case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(w, GLFW_TRUE); break;
            case 'B': magnifier.blockSize += shift? -1 : 1; break;
            case 'R': res += shift? -1 : 1; res = res < 1? 1 : res; break;
            case 'T': outlineTransition *= (shift? .8f : 1.2f); break;
            case 'W': outlineWidth *= (shift? .8f : 1.2f); break;
        }
}

const char *usage = "\
    b/B: +/- magnifier block size\n\
    t/T: +/- outline transition\n\
    w/W: +/- lineWidth\n\
    r/R: +/- patch res\n\
";

int main(int ac, char **av) {
    // init app window
    glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 4);
	GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "Bezier Patch with LOD", NULL, NULL);
    if (!w) {
        glfwTerminate();
        return 1;
    }
    glfwSetWindowPos(w, 100, 100);
    glfwMakeContextCurrent(w);
    // init OpenGL
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    PrintGLErrors();
    glViewport(0, 0, winWidth, winHeight);
    // init shader programs
    shader = LinkProgramViaCode(&vShaderCode, &tcShaderCode, &teShaderCode, &gShaderCode, &pShaderCode);
    if (!shader) {
        printf("Can't link shader program\n");
        getchar();
        return 1;
    }
    // init Bezier patch
    DefaultControlPoints();
    // callbacks
    glfwSetCursorPosCallback(w, MouseMove);
    glfwSetMouseButtonCallback(w, MouseButton);
    glfwSetScrollCallback(w, MouseWheel);
    glfwSetKeyCallback(w, Keyboard);
    glfwSetWindowSizeCallback(w, Resize);
    printf("Usage:\n%s\n", usage);
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
