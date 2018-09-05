// -----------------------------------------------------
// extended standard condition_variable to deal with
// interrupt tokens and jthread
// -----------------------------------------------------
#ifndef CONDITION_VARIABLE2_HPP
#define CONDITION_VARIABLE2_HPP

#include "jthread_fwd.hpp"

namespace std {

//***************************************** 
//* implementation of class condition_variable2
//***************************************** 

template<class Predicate>
inline void condition_variable2::wait_or_throw(unique_lock<mutex>& lock, Predicate pred) {
    std::this_thread::get_interrupt_token().registerCV(this);
    try {
      while(!pred()) {
        this_thread::throw_if_interrupted();
        cv.wait(lock, [&pred] {
                        return pred() || this_thread::is_interrupted();
                      });
        //std::cout.put(this_thread::is_interrupted() ? 'i' : '.').flush();
      }
    }
    catch (...) {
      std::this_thread::get_interrupt_token().unregisterCV(this);
      throw;
    }
    std::this_thread::get_interrupt_token().unregisterCV(this);
}

inline void condition_variable2::wait_or_throw(unique_lock<mutex>& lock) {
    std::this_thread::get_interrupt_token().registerCV(this);
    try {
      this_thread::throw_if_interrupted();
      cv.wait(lock);
      this_thread::throw_if_interrupted();
      //std::cout.put(this_thread::is_interrupted() ? 'i' : '.').flush();
    }
    catch (...) {
      std::this_thread::get_interrupt_token().unregisterCV(this);
      throw;
    }
    std::this_thread::get_interrupt_token().unregisterCV(this);

#ifdef OLD
    // Because we call a loop of wait_for() calls,
    // it might happen that notifications arrive between two of these calls
    // so they get los.
    // As a consequence, we usually cause a spurious wakeup and let this
    // handle in the outer loop that calls this function. 
    cv.wait_for(lock, std::chrono::milliseconds{10},
                [] {
                  this_thread::throw_if_interrupted();
                  return false;
                });
    //std::cout.put(this_thread::is_interrupted() ? 'i' : '.').flush();
#endif
}

} // std

#endif // CONDITION_VARIABLE2_HPP
