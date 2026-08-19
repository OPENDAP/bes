#ifndef _thread_count_h
#define _thread_count_h 1

#include <chrono>
#include <future>
#include <list>
#include "BESInternalError.h"

namespace dmrpp {

class ThreadCount {

private: 

    unsigned int tc_count = 0;
    unsigned int tc_max_num_threads;
    std::mutex tc_mtx;

    void release_thread_slot() {

        unique_lock<std::mutex> tc_lock(tc_mtx);
        if (tc_count > 0)
            tc_count--;

    } 
    
public:
    
    explicit ThreadCount(unsigned int max_num_threads): tc_max_num_threads(max_num_threads) {}
    template <typename ThreadTask> bool start_future(std::list<std::future<bool>> &futures, ThreadTask task){

        unique_lock<mutex> tc_lock(tc_mtx);
        if (tc_count >= tc_max_num_threads)
            return false;
        tc_count++;
        tc_lock.unlock();
    
        // Use a lambda and try to handle exception gracefully;tested and it works for a sample file.
        futures.push_back(async(launch::async,
                          [this](ThreadTask t) -> bool {
                                 struct ThreadExceptionGuard {
                                        ThreadCount *tc;
                                        ~ThreadExceptionGuard() { tc->release_thread_slot(); }
                                 } finish_exit{this};
                                 return t();
                                 },
                           std::move(task)));
        return true;

    }
    template <typename Rep, typename Period> bool wait_for_one(std::list<std::future<bool>> &futures, const std::chrono::duration<Rep, Period> &timeout) {

        while (!futures.empty()) {
            for (auto it = futures.begin(); it != futures.end(); ++it) {
                if (!it->valid()) {
                    futures.erase(it);
                    return true;
                }
                if (it->wait_for(timeout) != std::future_status::timeout) {
                    bool success = it->get();
                    futures.erase(it);
                    if (!success)
                        throw BESInternalError("A parallel thread task failed.", __FILE__, __LINE__);
                    return true;
                }
            }
       }
       return false;
    }
    void release_all_threads(std::list<std::future<bool>> &futures) {

        while (!futures.empty()) {
            try {
                if (futures.back().valid())
                    futures.back().get();
            } catch (...) {
                // Thread is released already by ThreadExceptionGuard.
            }
            futures.pop_back();
        }
    }

};

}
#endif
