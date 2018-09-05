// -----------------------------------------------------
// extended standard condition_variable to deal with
// interrupt tokens and jthread
// -----------------------------------------------------
#ifndef CONDITION_VARIABLE2_HPP
#define CONDITION_VARIABLE2_HPP

#include "this_thread.hpp"
#include "interrupt_token.hpp"
#include <condition_variable>

namespace std {

//***************************************** 
//* new cv_status interrupted
//***************************************** 
enum class cv_status2 {
  no_timeout = static_cast<int>(cv_status::no_timeout),
  timeout = static_cast<int>(cv_status::timeout),
  interrupted = static_cast<int>(cv_status::no_timeout)
                 + static_cast<int>(cv_status::timeout)
                 + 1,
};


//***************************************** 
//* class condition_variable2
//* - joining std::thread with interrupt support 
//***************************************** 
class condition_variable2
{
  public:
    //***************************************** 
    //* standardized API for condition_variable:
    //***************************************** 

    condition_variable2()
     : cv{} {
    }
    ~condition_variable2() {
    }
    condition_variable2(const condition_variable2&) = delete;
    condition_variable2& operator=(const condition_variable2&) = delete;

    void notify_one() noexcept {
      cv.notify_one();
    }
    void notify_all() noexcept {      
      cv.notify_all();
    }

    void wait(unique_lock<mutex>& lock) {
      cv.wait(lock);
    }
    template<class Predicate>
     void wait(unique_lock<mutex>& lock, Predicate pred) {
      cv.wait(lock,pred);
    }
    template<class Clock, class Duration>
     cv_status wait_until(unique_lock<mutex>& lock,
                          const chrono::time_point<Clock, Duration>& abs_time) {
      return cv.wait_until(lock, abs_time);
    }
    template<class Clock, class Duration, class Predicate>
     bool wait_until(unique_lock<mutex>& lock,
                     const chrono::time_point<Clock, Duration>& abs_time,
                     Predicate pred) {
      return cv.wait_until(lock, abs_time, pred);
    }
    template<class Rep, class Period>
     cv_status wait_for(unique_lock<mutex>& lock,
                        const chrono::duration<Rep, Period>& rel_time) {
      return cv.wait_for(lock, rel_time);
    }
    template<class Rep, class Period, class Predicate>
     bool wait_for(unique_lock<mutex>& lock,
                   const chrono::duration<Rep, Period>& rel_time,
                   Predicate pred) {
      return cv.wait_for(lock, rel_time, pred);
    }

    //***************************************** 
    //* supplementary API:
    //***************************************** 

    // x.6.2.1 dealing with interrupts:
    // throw std::interrupted on interrupt:
    void wait_or_throw(unique_lock<mutex>& lock);
    template<class Predicate>
    void wait_or_throw(unique_lock<mutex>& lock, Predicate pred);

  //***************************************** 
  //* implementation:
  //***************************************** 

  private:
    //*** API for the starting thread:
    condition_variable cv;
};


//**********************************************************************

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
