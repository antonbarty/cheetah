#!/usr/bin/env python
import argparse
parser = argparse.ArgumentParser()
parser.add_argument("cheetahroot", help="root cheetah directory (where README.md is located)")
args = parser.parse_args()

libcheetahdir = args.cheetahroot+'/source/libcheetah/'
extdir = 'cheetah_extensions_yaroslav'

incfiles = libcheetahdir+'include/*.h'
srcfiles = libcheetahdir+'src/*.cpp'
extincfiles = libcheetahdir+'include/'+extdir+'/*.h'
extsrcfiles = libcheetahdir+'src/'+extdir+'/*.cpp'

import os
os.system('rm include/*.h')
os.system('rm src/*.cpp')

import glob
incfiles = glob.glob(incfiles)
srcfiles = glob.glob(srcfiles)
extincfiles = glob.glob(extincfiles)
extsrcfiles = glob.glob(extsrcfiles)

replacements = [
                ['include "','include "cheetah/'],
                ['<Python.h>','"python/Python.h"'],
                ['#include "cheetah/testFileHandling.h"',''],
                #['#include "cheetah/cheetahLegacy_peakFinders.h"',''],
                ['#include "cheetah/cheetah_extensions_yaroslav/','#include "cheetah/'],
                ['#include <processRateMonitor.h>','#include "cheetah/processRateMonitor.h"'],
                ['#include <Eigen','#include <eigen3/Eigen'],
                ['#include <Point2D.h>','#include "cheetah/Point2D.h"'],
                ['#include <saveCXI.h>','#include "cheetah/saveCXI.h"'],
                ['#include <detectorObject.h>','#include "cheetah/detectorObject.h"'],
                ['#include <cheetahGlobal.h>','#include "cheetah/cheetahGlobal.h"'],
                ['#include <cheetahEvent.h>','#include "cheetah/cheetahEvent.h"'],
                ['#include <cheetahmodules.h>','#include "cheetah/cheetahmodules.h"'],
                ['#include <median.h>','#include "cheetah/median.h"']]
                #['#include <hdf5.h>','#include "hdf5/hdf5.h"']

skipfiles=['cheetahExtensions.cpp','setup.cpp']

for f in incfiles+srcfiles+extincfiles+extsrcfiles:
    dir,fname = os.path.split(f)
    print fname
    if fname in skipfiles:
        print '*** SKIPPING',fname
        continue
    f = open(f,'r')
    if fname.endswith('cpp'):
        fnew = open('src/'+fname,'w')
    else:
        fnew = open('include/'+fname,'w')
    for line in f:
        for r in replacements:
            line = line.replace(r[0],r[1])
        fnew.write(line)
