//
//  TextManager.h
//  cppxfel
//
//  Created by Helen Ginn on 22/02/2017.
//  Copyright (c) 2017 Division of Structural Biology Oxford. All rights reserved.
//

#ifndef __cppxfel__TextManager__
#define __cppxfel__TextManager__

#include <stdio.h>
#include <png.h>
#include <string>

class TextManager
{
private:
    static TextManager textManager;
    
    TextManager();
    
public:
    static void text_malloc(png_byte **pointer, std::string text, int *width, int *height);
    static void text_free(png_byte **pointer);
};

#endif /* defined(__cppxfel__TextManager__) */
