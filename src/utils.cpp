#include "utils.h"
#include <sys/time.h>
#include <stdio.h>

long Utils::get_current_timestamp()                                                                                                   
{       
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long cur_time = tv.tv_sec * 1000000 + tv.tv_usec;                                                                                    
    return cur_time;
} 
