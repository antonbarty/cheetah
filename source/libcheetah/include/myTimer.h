//
//  myTimer.h
//  libcheetah
//
//  Created by Anton Barty on 17.09.18.
//  Copyright (c) 2018 Anton Barty. All rights reserved.
//

#ifndef myTimer_h
#define myTimer_h

#include <string>
#include <pthread.h>



/*
 *  Simple start/stop timer
 */
class cMyTimer {

public:
    cMyTimer();

    void start(void);
    void stop(void);
    void pause(void);
    void resume(void);

    double duration;

private:
    // Ansi-C version
    double start_clock;
    double end_clock;
    double getTimeInSeconds_double(void);


    // std::chrono needs c++11, not compatible with psana?
    //std::chrono::high_resolution_clock::time_point start_clock;
    //std::chrono::high_resolution_clock::time_point end_clock;
};



/*
 *  Housekeeper for keeping track of how long spent in different parts of code
 */
class cTimingProfiler {

public:
    enum {
        TIMER_EVENTWAIT=0,
        TIMER_EVENTDATA,
        TIMER_EVENTCOPY,
        TIMER_WAITFORWORKERTHREAD,
        TIMER_WORKERRUN,
        TIMER_H5WAIT,
        TIMER_H5WRITE,
        TIMER_FLUSH,
        TIMER_NTYPES
    };

private:
    std::string message[TIMER_NTYPES] = {
        "Waiting for next event: ",
        "Reading event data: ",
        "Copying event data: ",
        "Waiting for available worker thread: ",
        "Cheetah worker execution (multithreaded): ",
        "Waiting to write .cxi/.h5 file: ",
        "Writing .cxi/h5 file: ",
        "Flushing files: "
    };


public:
    cTimingProfiler();
    
    void addToTimer(double, int);
    void reportTimers(void);
    void resetTimers(void);

private:
    double   elapsed_time[TIMER_NTYPES];
    pthread_mutex_t counter_mutex;
    
};

#endif /* myTimer_h */
