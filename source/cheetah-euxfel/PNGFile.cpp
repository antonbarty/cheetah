//
//  Image.cpp
//  CellCounter
//
//  Created by Helen Ginn on 07/08/2015.
//  Copyright (c) 2015 Division of Structural Biology Oxford. All rights reserved.
//

#include "PNGFile.h"
#include <string>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include "TextManager.h"

int PNGFile::writeImage(std::string filename, int width, int height, std::string title)
{
    int code = 0;
    FILE *fp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    
    // Open file for writing (binary mode)
    fp = fopen(filename.c_str(), "wb");
    if (fp == NULL) {
        fprintf(stderr, "Could not open file %s for writing\n", filename.c_str());
        code = 1;
        goto finalise;
    }
    
    // Initialize write structure
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fprintf(stderr, "Could not allocate write struct\n");
        code = 1;
        goto finalise;
    }
    
    // Initialize info structure
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fprintf(stderr, "Could not allocate info struct\n");
        code = 1;
        goto finalise;
    }
    
    // Setup Exception handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error during png creation\n");
        code = 1;
        goto finalise;
    }
    
    png_init_io(png_ptr, fp);
    
    // Write header (8 bit colour depth)
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    
    // Set title
    if (title.length() > 0) {
        png_text title_text;
        title_text.compression = PNG_TEXT_COMPRESSION_NONE;
        title_text.key = (char *)"Title";
        title_text.text = (char *)title.c_str();
        png_set_text(png_ptr, info_ptr, &title_text, 1);
    }
    
    png_write_info(png_ptr, info_ptr);
    
    // Write image data

    for (int y = 0 ; y < height ; y++)
    {
        png_bytep row = &data[y * pixelsPerRow * bytesPerPixel];
        png_write_row(png_ptr, row);
    }
    
    // End write
    png_write_end(png_ptr, NULL);
    
finalise:
    if (fp != NULL)
    {
        fclose(fp);
    }
        
    if (info_ptr != NULL)
    {
        png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    }
    
    if (png_ptr != NULL)
    {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

    }
    
    return code;
}

void PNGFile::RGB_to_HSB(float red, float green, float blue,
						 float *brightness, float *saturation, float *hue)
{
	int cmax = (red > green) ? red : green;

	if (blue > cmax)
	{
		cmax = blue;
	}

	int cmin = (red < green) ? red : green;

	if (blue < cmin)
	{
		cmin = blue;
	}

	*brightness = ((float) cmax) / 255.0f;

	if (cmax != 0)
	{
		*saturation = ((float) (cmax - cmin)) / ((float) cmax);
	}
	else
	{
		saturation = 0;
	}
	if (saturation == 0)
	{
		hue = 0;
	}
	else
	{
		float redc = ((float) (cmax - red)) / ((float) (cmax - cmin));
		float greenc = ((float) (cmax - green)) / ((float) (cmax - cmin));
		float bluec = ((float) (cmax - blue)) / ((float) (cmax - cmin));

		if (red == cmax)
		{
			*hue = bluec - greenc;
		}
		else if (green == cmax)
		{
			*hue = 2.0f + redc - bluec;
		}
		else
		{
			*hue = 4.0f + greenc - redc;
		}

		*hue = *hue / 6.0f;

		if (hue < 0)
		{
			*hue = *hue + 1.0f;
		}

		*hue *= 360;
	}
}

// Hue should be between 0 and 360 degrees (rainbow)
// Saturation between 0 and 1
// Brightness between 0 and 1
void PNGFile::HSB_to_RGB(float hue, float sat, float bright, // or light
                         png_byte *red, png_byte *green, png_byte *blue)
{
    while (hue < 0)
    {
        hue += 360.;
    }
    
    while (hue > 360)
    {
        hue -= 360.;
    }
    
    double c = bright * sat;
    double m = bright - c;
    
    //double c = (1 - fabs(2 * bright - 1)) * sat;
    //double m = bright - c;
    
    double x = c * (1 - fabs(fmod((hue / 60), 2) - 1));
    
    double tempRed = 0;
    double tempGreen = 0;
    double tempBlue = 0;
    
    if (hue < 60)
    {
        tempRed = c;
        tempGreen = x;
        tempBlue = 0;
    }
    else if (hue < 120)
    {
        tempRed = x;
        tempGreen = c;
        tempBlue = 0;
    }
    else if (hue < 180)
    {
        tempRed = 0;
        tempGreen = c;
        tempBlue = x;

    }
    else if (hue < 240)
    {
        tempRed = 0;
        tempGreen = x;
        tempBlue = c;
    }
    else if (hue < 300)
    {
        tempRed = x;
        tempGreen = 0;
        tempBlue = c;

    }
    else if (hue <= 360)
    {
        tempRed = c;
        tempGreen = 0;
        tempBlue = x;
    }

    *red = (tempRed + m) * 255;
    *green = (tempGreen + m) * 255;
    *blue = (tempBlue + m) * 255;
    
}

std::string getFilename(std::string filename)
{
	size_t pos = filename.rfind("/");
	if(pos == std::string::npos)  //No path.
		return filename;

	return filename.substr(pos + 1, filename.length());
}

std::string getBaseFilename(std::string filename)
{
	std::string fName = getFilename(filename);
	size_t pos = fName.rfind(".");
	if(pos == std::string::npos)  //No extension.
		return fName;

	if(pos == 0)    //. is at the front. Not an extension.
		return fName;

	return fName.substr(0, pos);
}

PNGFile::PNGFile(std::string filename, int width, int height)
{
    // set data using libpng...
    // data =
    
    plain = false;
    this->height = height;
    bytesPerPixel = 3; // change
    pixelsPerRow = width; // change
    this->filename = filename;
    
    size_t arraySize = sizeof(png_byte) * bytesPerPixel * height * pixelsPerRow;
    
    data = (png_bytep)malloc(arraySize);
    memset(data, 255, arraySize);
    
    rootName = getBaseFilename(filename);
}

void PNGFile::writeImageOutput()
{
    std::string title = rootName;

    writeImage(filename, pixelsPerRow, height, title);
}

png_byte PNGFile::valueAt(int x, int y)
{
    png_byte *bytes = new png_byte[bytesPerPixel];
    pixelAt(x, y, &bytes);
    int value = 0;
    
    for (int i = 0; i < bytesPerPixel; i++)
    {
        value += bytes[i];
    }
    
    value /= bytesPerPixel;
    
    return value;
}

void PNGFile::pixelAt(int x, int y, png_byte **bytes)
{
    int offset = y * bytesPerPixel * pixelsPerRow +  x * bytesPerPixel;
    
    *bytes = &data[offset];
    
}

void PNGFile::moveCoordRelative(int *x, int *y)
{
    if (plain)
    {
        *x += centreX;
        *y += centreY;

		*y = height - *y;
        return;
    }
    
    *x += pixelsPerRow / 2 - centreX;
    *y += height / 2 - centreY;
}

void PNGFile::invertColourRelative(int x, int y)
{
	moveCoordRelative(&x, &y);

//	setPixelColour(x, y, 255, 255, 255);

	png_byte *bytes;
	float saturation, brightness, hue;

	pixelAt(x, y, &bytes);

//	std::cout << (int)bytes[0] << " " << (int)bytes[1] << " " << (int)bytes[2] << " to ";

	RGB_to_HSB(bytes[0], bytes[1], bytes[2], &brightness, &saturation, &hue);
	double flat_hue = fmod(hue + 180, 360);
	png_byte red, blue, green;

	HSB_to_RGB(flat_hue, saturation, brightness, &red, &green, &blue);

//	std::cout << (int)red << " " << (int)blue << " " << (int)green << std::endl;

	setPixelColour(x, y, 0, 0, 0);
}

void PNGFile::setPixelColourRelative(int x, int y, png_byte red, png_byte green, png_byte blue)
{
    moveCoordRelative(&x, &y);
    
    setPixelColour(x, y, red, green, blue);
}

void PNGFile::setPixelColour(int x, int y, png_byte red, png_byte green, png_byte blue, float transparency)
{
    int offset = y * bytesPerPixel * pixelsPerRow +  x * bytesPerPixel;
    
    if (offset < 0 || offset >= bytesPerPixel * pixelsPerRow * height)
    {
        return;
    }
    
    if (transparency < 1)
    {
        png_byte currentRed = data[offset];
        png_byte currentGreen = data[offset + 1];
        png_byte currentBlue = data[offset + 2];
        
        red = currentRed + (red - currentRed) * transparency;
        green = currentGreen + (green - currentGreen) * transparency;
        blue = currentBlue + (blue - currentBlue) * transparency;
    }
    
    data[offset] = red;
    data[offset + 1] = green;
    data[offset + 2] = blue;
}


void PNGFile::setPixelForChannel(int x, int y, int channel, png_byte value)
{
    int offset = y * bytesPerPixel * pixelsPerRow +  x * bytesPerPixel + channel;
    
    data[offset] = value;
}

void PNGFile::drawCircleAroundPixel(int x, int y, float radius, float transparency, png_byte red, png_byte green, png_byte blue, float thickness)
{
    moveCoordRelative(&x, &y);
    double radiusSqr = radius * radius;
    double minDiff = radiusSqr - pow(radius - thickness / 2, 2);
    
    for (int i = x - radius; i <= x + radius; i++)
    {
        for (int j = y - radius; j <= y + radius; j++)
        {
            double distSqr = pow(i - x, 2) + pow(j - y, 2);
            double diff = fabs(distSqr - radiusSqr);
            
            if (diff < minDiff)
            {
                setPixelColour(i, j, red, green, blue, transparency);
            }
        }
    }
}

void PNGFile::drawText(std::string text, int xCentre, int yCentre, png_byte red, png_byte green, png_byte blue)
{
    png_byte *textPixels;
    int width = 0;
    int height = 0;
    
    moveCoordRelative(&xCentre, &yCentre);
    TextManager::text_malloc(&textPixels, text, &width, &height);
    
    int left = xCentre - width / 2;
    int top = yCentre - height / 2;
    
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            int pos = j * width + i;
            int x = left + i;
            int y = top + j;
            
            png_byte transByte = textPixels[pos];
            float transparency = (float)transByte / 255.;
            setPixelColour(x, y, red, green, blue, transparency);
        }
    }
    
    TextManager::text_free(&textPixels);
}

void PNGFile::drawLine(int x1, int y1, int x2, int y2, float transparency, png_byte red, png_byte green, png_byte blue)
{
    moveCoordRelative(&x1, &y1);
    moveCoordRelative(&x2, &y2);
    double xTravel = x2 - x1;
    double yTravel = y2 - y1;
    double *bigger = (fabs(yTravel) > fabs(xTravel)) ? &yTravel : &xTravel;
    double *smaller = (bigger == &xTravel) ? &yTravel : &xTravel;
    
    const double bigShift = 0.25 * (*bigger > 0 ? 1 : -1);
    int intervals = fabs(fabs(*bigger) / bigShift + 0.5);
    double smallShift = *smaller / intervals;
    
    double xCurr = x1;
    double yCurr = y1;
    
    for (int i = 0; i < intervals + 1; i++)
    {
        setPixelColour(xCurr, yCurr, red, green, blue, 1 - transparency);
        xCurr += (bigger == &xTravel) ? bigShift : smallShift;
        yCurr += (smaller == &xTravel) ? bigShift : smallShift;
    }
    
}

void PNGFile::dropImage()
{
    free(data);
}
