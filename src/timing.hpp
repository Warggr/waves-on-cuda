#pragma once

#include <chrono>
#include <iostream>

using timer_clock = std::chrono::high_resolution_clock;

struct ScopeTimer {
    decltype(timer_clock::now()) t1;

    inline ScopeTimer(){
        t1 = timer_clock::now();
    }
    inline ~ScopeTimer(){
        auto t2 = timer_clock::now();
        std::chrono::duration<double, std::milli> runtime = t2 - t1;
        std::cout << runtime.count() << std::endl;
    }
};
