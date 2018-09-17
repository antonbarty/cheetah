//
//  myTimer.hpp
//  libcheetah
//
//  Created by Anton Barty on 17.09.18.
//  Copyright © 2018 Anton Barty. All rights reserved.
//

#ifndef myTimer_h
#define myTimer_h

//#include <stdlib.h>
//#include <chrono>




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

    
    // Needs c++11, not compatible with psana?
    //std::chrono::high_resolution_clock::time_point start_clock;
    //std::chrono::high_resolution_clock::time_point end_clock;
};



#endif /* myTimer_h */
