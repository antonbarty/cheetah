//
//  myTimer.cpp
//  libcheetah
//
//  Created by Anton Barty on 17.09.18.
//  Copyright © 2018 Anton Barty. All rights reserved.
//


#include <stdlib.h>
#include <ctime>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <pthread.h>

#include "myTimer.h"

//#include <chrono>
//#include <atomic>


/*
 Standard C:
 https://stackoverflow.com/questions/361363/how-to-measure-time-in-milliseconds-using-ansi-c
 struct timespec ts;
 clock_gettime (CLOCK_MONOTONIC, &ts)
 return (uint64_t) (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
 time_diff = time_value - prev_time_value;
 
 c++ 11 using std::chrono
 Some examples of using high resolution clock timer
 
 cout << chrono::high_resolution_clock::period::den << endl;
 auto start_time = chrono::high_resolution_clock::now();
 auto end_time = chrono::high_resolution_clock::now();
 cout << chrono::duration_cast<chrono::seconds>(end_time - start_time).count() << ":";
 cout << chrono::duration_cast<chrono::microseconds>(end_time - start_time).count() << ":";
 
 
 std::chrono::duration<double> elapsed_seconds = end-start;
 std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 std::cout << "finished computation at " << std::ctime(&end_time) << "elapsed time: " << elapsed_seconds.count() << "s\n";
 
 auto start = std::chrono::high_resolution_clock::now();
 auto end = std::chrono::high_resolution_clock::now();
 std::chrono::duration<double> diff = end-start;
 
 
 http://www.cplusplus.com/reference/chrono/high_resolution_clock/now/
 high_resolution_clock::time_point t1 = high_resolution_clock::now();
 high_resolution_clock::time_point t2 = high_resolution_clock::now();
 duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
 std::cout << "It took me " << time_span.count() << " seconds.";
 std::cout << std::endl;
 */


/*
 *  Simple duration timer
 *  Time between start and stop in (double precision) seconds
 *  Would use std::chrono, but this causes errors compiling psana so we use an ANSI workaround
 */
cMyTimer::cMyTimer(){
    duration = 0;
}

//cMyTimer::~cMyTimer(){
//}


double cMyTimer::getTimeInSeconds_double(void){
    
    // Usage of clock_gettime is pretty straight forward
    if(false) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec + ((float)ts.tv_nsec)*1e-9;
    }
    
    // Fallback strategy is gettimeofday which reports wall time (not CPU time)
    if(true) {
        struct timeval tv;
        if (gettimeofday (&tv, NULL) == 0)
            return tv.tv_sec + ((float)tv.tv_usec)*1e-6;
        else
            return 0;
    }
    
    // High resolution clock requires C++11, which does not play nice with psana
    if(false) {
        //high_resolution_clock::time_point t1 = high_resolution_clock::now();
        //high_resolution_clock::time_point t2 = high_resolution_clock::now();
        //duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    }
}


void cMyTimer::start(void){
    duration = 0;
    start_clock = getTimeInSeconds_double();
    //start_clock = std::chrono::high_resolution_clock::now();
}

void cMyTimer::stop(void){
    end_clock = getTimeInSeconds_double();
    double temp = end_clock - start_clock;
    //end_clock = std::chrono::high_resolution_clock::now();
    //temp = (std::chrono::duration<double>(end_clock - start_clock)).count();
    //std::chrono::duration<double> temp = std::chrono::duration_cast<std::chrono::duration<double>>(end_clock - start_clock);
    duration += temp;
}

void cMyTimer::pause(void){
    end_clock = getTimeInSeconds_double();
    double temp = end_clock - start_clock;
    //end_clock = std::chrono::high_resolution_clock::now();
    //temp = (std::chrono::duration<double>(end_clock - start_clock)).count();
    //duration<double> temp = duration_cast<duration<double>>(end_clock - start_clock);
    duration += temp;
}

void cMyTimer::resume(void){
    start_clock = getTimeInSeconds_double();
    //start_clock = std::chrono::high_resolution_clock::now();
}




/*
 *  Keep track of time spent in different parts of code
 */
cTimingProfiler::cTimingProfiler() {
    pthread_mutex_init(&counter_mutex, NULL);
    resetTimers();
}

// Reset couters to 0
void cTimingProfiler::resetTimers(void) {
    for(long i=0; i<TIMER_NTYPES; i++) {
        elapsed_time[i] = 0;
    }
}


// Add value to the specified timer (thread-safe)
void addToTimer(double time, int field) {
    pthread_mutex_lock(&counter_mutex);
    elapsed_time[field] += time;
    pthread_mutex_unlock(&counter_mutex);
}

// Report on timer status
void reportTimers(void){
    
    // Add up total measured time for percentage calculation
    double total=0;
    double percent;
    for(long i=0; i<TIMER_NTYPES; i++) {
        total += elapsed_time[i];
    }
    
    // Print summary
    printf("Execution time summary: \n");
    for(long i=0; i<TIMER_NTYPES; i++) {
        percent = 100*elapsed_time[i] / total;
        printf("\t%s %0.2lf sec (%0.2lf %%)\n",message[i].c_str(), elapsed_time[i], percent);
    }
}



/*
class cTimingProfiler {
    
public:
    enum {
        TIMER_EVENTWAIT=0,
        TIMER_EVENTDATA,
        TIMER_CALC,
        TIMER_H5WAIT,
        TIMER_H5WRITE,
        TIMER_FLUSH,
        TIMER_NTYPES
    };
    


