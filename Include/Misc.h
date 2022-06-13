// Misc.h (c) 2019-2022 Jules Bloomenthal

#ifndef MISC_HDR
#define MISC_HDR

#include <glad.h>
#include <string.h>
#include <time.h>
#include "VecMat.h"

// Misc

bool KeyDown(int button);
bool Shift();
bool Control();
std::string GetDirectory();
time_t FileModified(const char *name);
bool FileExists(const char *name);

// Sphere

int LineSphere(vec3 ln1, vec3 ln2, vec3 center, float radius, vec3 &p1, vec3 &p2);
	// set points intersected by line with sphere, return # intersection
	// line defined by ln1, ln2
	// sphere defined by center and radius
	// p1 and p2 set according to # hits; return # hits

float RaySphere(vec3 base, vec3 v, vec3 center, float radius);
	// return least pos alpha of ray and sphere (or -1 if none)
	// v presumed unit length

// Image file

unsigned char *MergeFiles(const char *imageName, const char *matteName, int &width, int &height);
	// allocate width*height pixels, set them from an image and matte file, return pointer
	// pixels returned are 4 bytes (rgba)
	// this memory should be freed by the caller

unsigned char *ReadTarga(const char *filename, int *width, int *height, int *bytesPerPixel = NULL);
	// allocate width*height pixels, set them from file, return pointer
	// this memory should be freed by the caller
	// expects 24 bpp
	// *** pixel data is BGR format ***

bool TargaSize(const char *filename, int &width, int &height);

bool WriteTarga(const char *filename, unsigned char *pixels, int width, int height);
	// save raster to named Targa file

bool WriteTarga(const char *filename);
	// as above but with entire application raster

// Texture

// Buffer to GPU
//    GLuint textureName;
//    glGenTextures(1, &textureName);
//    glBindTexture(GL_TEXTURE_2D, textureName); 
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,  GL_RGB, GL_UNSIGNED_BYTE, pixels);
// Display
//    GLuint textureUnit = 0; // arbitrary
//    glActiveTexture(GL_TEXTURE0+textureUnit);
//    glBindTexture(GL_TEXTURE_2D, textureName); // bind GPU buffer to active texture unit
//    SetUniform(“textureImage”, textureUnit);
// In shader
//    uniform sampler2D textureImage;
//    vec4 rgba = texture(textureImage, uv);

GLuint LoadTexture(const char *filename, bool mipmap = true, int *nchannels = NULL);
	// for arbitrary image format, load image file into given texture unit; return texture name

GLuint LoadTargaTexture(const char *targaFilename, bool mipmap = true);
	// load .tga file into given texture unit; return texture name (id)

GLuint LoadTexture(unsigned char *pixels, int width, int height, int bpp, bool bgr = false, bool mipmap = true);
	// bpp is bytes per pixel
	// load pixels into given texture unit; return texture name (id)

void LoadTexture(unsigned char *pixels, int width, int height, int bpp, GLuint textureName, bool bgr, bool mipmap);

// Bump map
unsigned char *GetNormals(unsigned char *depthPixels, int width, int height, float depthIncline = 1);
	// return normal pixels (3 bytes/pixel) that correspond with depth pixels (presumed 3 bytes/pixel)
	// the memory returned should be freed by the caller
	// depthIncline is ratio of distance represented by z range 0-1 to distance represented by width of image

#endif
