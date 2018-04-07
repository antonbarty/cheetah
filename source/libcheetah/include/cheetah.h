//
//  cheetah.h
//  cheetah
//
//  Created by Anton Barty on 11/04/12.
//  Copyright (c) 2012 CFEL. All rights reserved.
//

#ifndef CHEETAH_H
#define CHEETAH_H
#include "cheetah/peakfinders.h"
#include "cheetah/detectorObject.h"
#include "cheetah/cheetahGlobal.h"
#include "cheetah/cheetahEvent.h"
#include "cheetah/cheetahmodules.h"


int cheetahInit(cGlobal *);
void cheetahNewRun(cGlobal *);
cEventData* cheetahNewEvent(cGlobal	*global);
void cheetahProcessEvent(cGlobal *, cEventData *);
void cheetahProcessEventMultithreaded(cGlobal *, cEventData *);
void cheetahDestroyEvent(cEventData *);
void cheetahExit(cGlobal *);
void cheetahUpdateGlobal(cGlobal *, cEventData *);
#endif
