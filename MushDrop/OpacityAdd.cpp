// OpacityAdd.cpp - add opacity layer to 24 bit image
// console only requests directory, image, and matte

#include <stdio.h>
#include <string.h>
#include "Misc.h"
#include "STB_Image.h"
#include "stb_image_write.h"

using std::string;

unsigned char *ReadFile(string fileName, int &width, int &height, int &nChannels) {
	stbi_set_flip_vertically_on_load(false);
	unsigned char *data = stbi_load(fileName.c_str(), &width, &height, &nChannels, 0);
	if (!data) {
		printf("Can't open %s (%s)\n", fileName.c_str(), stbi_failure_reason());
		return NULL;
	}
	return data;
}

float GetVal(unsigned char *data, int x, int y, int w, int nChannels) {
	unsigned char c = data[nChannels == 1? y*w+x : 3*(y*w+x)];
	return (float)c/255;
}

float Lerp(float a, float b, float t) { return a+t*(b-a); }

bool Write32(const char *directory, const char *image, const char *matte, const char *out) {
	const char *ext = strlen(out) > 3? out+strlen(out)-3 : NULL;
	string imageName = string(directory)+"/"+string(image);
	string matteName = string(directory)+"/"+string(matte);
	string outName = string(directory)+"/"+string(out);
	int tWidth, tHeight, tNChannels, mWidth, mHeight, mNChannels;
	unsigned char *tData = ReadFile(imageName, tWidth, tHeight, tNChannels);
	unsigned char *mData = ReadFile(matteName, mWidth, mHeight, mNChannels);
	printf("image nchans=%i, matte nchans=%i\n", tNChannels, mNChannels);
	if (!tData || !mData) return false;
	bool sameSize = tWidth == mWidth && tHeight == mHeight;
	unsigned char *oData = new unsigned char[4*tWidth*tHeight];
	unsigned char *t = tData, *m = mData, *o = oData;
	for (int j = 0; j < tHeight; j++)
		for (int i = 0; i < tWidth; i++) {
			for (int k = 0; k < 3; k++)
				*o++ = *t++;
			if (sameSize) {
				*o++ = *m++;
				if (mNChannels == 3) m += 2;
			}
			else {
				float txf = (float)i/(tWidth-1), tyf = (float)j/(tHeight-1);
				float x = txf*mWidth, y = tyf*mHeight;
				int x0 = (int) floor(x), y0 = (int) floor(y);
				float mxf = x-x0, myf = y-y0;
				bool lerpX = x0 < mWidth-1, lerpY = y0 < mHeight-1;
				float v00 = GetVal(mData, x0, y0, mWidth, mNChannels), v10, v11, v01;
				if (lerpX) v10 = GetVal(mData, x0+1, y0, mWidth, mNChannels);
				if (lerpY) v01 = GetVal(mData, x0, y0+1, mWidth, mNChannels);
				if (lerpX && lerpY) v11 = GetVal(mData, x0+1, y0+1, mWidth, mNChannels);
				float v = v00;
				if (lerpX && !lerpY)
					v = Lerp(v00, v10, mxf);
				if (!lerpX && lerpY)
					v = Lerp(v00, v01, myf);
				if (lerpX && lerpY) {
					float v1 = Lerp(v00, v10, mxf), v2 = Lerp(v01, v11, mxf);
					v = Lerp(v1, v2, myf);
				}
				*o++ = (unsigned char) (255.*v);
			}
		}
	const char *oName = outName.c_str();
	int r = !strcmp(ext, "png")? stbi_write_png(oName, tWidth, tHeight, 4, oData, tWidth*4) :
			!strcmp(ext, "tga")? stbi_write_tga(oName, tWidth, tHeight, 4, oData) :
			0; // bmp, jpg do not (?) support 4 channels
	printf("%s %s\n", oName, r? "written" : "write failure");
	return r != 0;
}

void GetInput(const char *prompt, char *buf) {
	printf("%s: ", prompt);
	fgets(buf, 500, stdin);
	if (strlen(buf) > 1) buf[strlen(buf)-1] = 0; // remove <cr>
}

int main(int ac, char **av) {
	char directory[500], picture[500], matte[500], out32[500];
	strcpy(directory, "/Users/Jules/Code/Aids");
	strcpy(picture, "Lily.tga");
	strcpy(matte, "1.tga");
	strcpy(out32, "FRABA.tga");
//	GetInput("directory", directory);
//	GetInput("picture (any type)", picture);
//	GetInput("matte (any type)", matte);
//	GetInput("output (png or tga only)", out32);
	bool ok = Write32(directory, picture, matte, out32);
	printf("%s %s\n", out32, ok? "written" : "write failed");
	getchar();
}
