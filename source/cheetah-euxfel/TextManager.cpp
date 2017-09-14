//
//  TextManager.cpp
//  cppxfel
//
//  Created by Helen Ginn on 22/02/2017.
//  Copyright (c) 2017 Division of Structural Biology Oxford. All rights reserved.
//

#include "TextManager.h"
#include "font.h"
#include <cstdlib>

void TextManager::text_free(png_byte **pointer)
{
    free(*pointer);
}

void TextManager::text_malloc(png_byte **pointer, std::string text, int *width, int *height)
{
    int squish = 3;
    int totalWidth = squish;
    int maxHeight = 0;
    
    for (int i = 0; i < text.length(); i++)
    {
        int charHeight = asciiDimensions[text[i]][0];
        int charWidth = asciiDimensions[text[i]][1];
        totalWidth += charWidth - squish;
        if (charHeight > maxHeight)
        {
            maxHeight = charHeight;
        }
    }
    
    totalWidth += squish;
    
    *width = totalWidth;
    *height = maxHeight;
    
    *pointer = (png_byte *)calloc(maxHeight * totalWidth, sizeof(png_byte));
    
    int currentX = 0;
    
    for (int i = 0; i < text.length(); i++)
    {
        char whichAscii = text[i];
        png_byte *chosenAscii = asciis[whichAscii];
        int chosenHeight = asciiDimensions[whichAscii][0];
        int chosenWidth = asciiDimensions[whichAscii][1];
        
        for (int k = 0; k < chosenHeight; k++)
        {
            for (int j = 0; j < chosenWidth; j++)
            {
                int xPos = currentX + j;
                int newPos = totalWidth * k + xPos;
                int asciiPos = chosenWidth * k + j;
                
                (*pointer)[newPos] += chosenAscii[asciiPos];
            }
        }
        
        currentX += chosenWidth - squish;
    }
}