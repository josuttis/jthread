// -----------------------------------------------------
// extended standard condition_variable to deal with
// interrupt tokens and jthread
// -----------------------------------------------------
#ifndef CONDITION_VARIABLE2_HPP
#define CONDITION_VARIABLE2_HPP

//*****************************************************************************
// forward declarations are in separate header due to cyclic type dependencies:
//*****************************************************************************
#include "jthread_fwd.hpp"
#include <iostream>

namespace std {

//*****************************************************************************
//* implementation of class condition_variable2
//*****************************************************************************

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
}

// return std::cv_status::interrupted on interrupt:
inline cv_status2 condition_variable2::wait_until(unique_lock<mutex>& lock,
                                                  interrupt_token itoken) {
    if (itoken.is_interrupted()) {
      return cv_status2::interrupted;
    }
    itoken.registerCV(this);
    try {
      cv.wait(lock);
      //std::cout.put(itoken.is_interrupted() ? 'i' : '.').flush();
    }
    catch (...) {
      itoken.unregisterCV(this);
      throw;
    }
    itoken.unregisterCV(this);
    return itoken.is_interrupted() ? cv_status2::interrupted : cv_status2::no_timeout;
}

// only returns if pred or on signaled interrupt:
// return value:
//   true:  pred() yields true
//   false: pred() yields false (other reason => interrupt signaled)
template <class Predicate>
inline bool condition_variable2::wait_until(unique_lock<mutex>& lock,
                                            Predicate pred,
                                            interrupt_token itoken)
{
    if (itoken.is_interrupted()) {
      return pred();
    }
    itoken.registerCV(this);
    try {
      while(!pred() && !itoken.is_interrupted()) {
        //std::cout.put(itoken.is_interrupted() ? 'i' : '.').flush();
        cv.wait(lock, [&pred, &itoken] {
                        return pred() || itoken.is_interrupted();
                      });
      }
    }
    catch (...) {
      itoken.unregisterCV(this);
      throw;
    }
    itoken.unregisterCV(this);
    // return true if pred() true:
    return pred();
}

} // std

#endif // CONDITION_VARIABLE2_HPP
