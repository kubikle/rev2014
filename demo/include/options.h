#pragma once
#ifndef OPTIONS_H
#define OPTIONS_H
#include <d3d11.h>

void CalculateScreenSize();

typedef struct {
	double dWidth;
	double dHeight;
	UINT bpp;
	int fullscreen;
	double aspectRatio;
	double iWidth;
	double iHeight;
	double offsetX; 
	double offsetY;
	double sWidth; 
	double sHeight;
	bool saveImages;
	double saveFrameRate;
} DemoOptions; 

#endif  