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

// RAII helper to ensure interrupt_token::unregister(cv) is called 
class register_guard
{
  private:
    interrupt_token*  itoken;
    condition_variable2*  cvPtr;
  public:
    explicit register_guard(interrupt_token& i, condition_variable2* cp)
     : itoken{&i}, cvPtr{cp} {
        itoken->registerCV(cvPtr);
    }
    ~register_guard() {
        itoken->unregisterCV(cvPtr);
    }
    register_guard(register_guard&) = delete;
    register_guard& operator=(register_guard&) = delete;
};


//*****************************************************************************
//* implementation of class condition_variable2
//*****************************************************************************

// wait_until(): wait with interrupt handling 
// - returns on interrupt
// return value:
// - true if pred() yields true
// - false otherwise (i.e. on interrupt)
template <class Predicate>
inline bool condition_variable2::wait_until(unique_lock<mutex>& lock,
                                            Predicate pred,
                                            interrupt_token itoken)
{
    if (itoken.is_interrupted()) {
      return pred();
    }
    register_guard rg{itoken, this};
    // Note: Only after registration a notification is guaranteed.
    //       Thus, to avoid a race, we have to check for is_interrupted() again:
    while(!pred() && !itoken.is_interrupted()) {
      cv.wait(lock, [&pred, &itoken] {
                      return pred() || itoken.is_interrupted();
                    });
    }
    return pred();
}

// wait_until(): timed wait with interrupt handling 
// - returns on interrupt
// return:
// - true if pred() yields true
// - false otherwise (i.e. on timeout or interrupt)
template <class Clock, class Duration, class Predicate>
inline bool condition_variable2::wait_until(unique_lock<mutex>& lock,
                                            const chrono::time_point<Clock, Duration>& abs_time,
                                            Predicate pred,
                                            interrupt_token itoken)
{
    register_guard rg{itoken, this};
    // Note: Only after registration a notification is guaranteed.
    //       Thus, to avoid a race, we have to check for is_interrupted() again:
    while(!pred() && !itoken.is_interrupted() && Clock::now() < abs_time) {
      //std::cout.put(itoken.is_interrupted() ? 'i' : '.').flush();
      cv.wait_until(lock,
                    abs_time,
                    [&pred, &itoken] {
                      return pred() || itoken.is_interrupted();
                    });
    }
    return pred();
}

// wait_for(): timed wait with interrupt handling 
// - returns on interrupt
// return:
// - true if pred() yields true
// - false otherwise (i.e. on timeout or interrupt)
template <class Rep, class Period, class Predicate>
inline bool condition_variable2::wait_for(unique_lock<mutex>& lock,
                                          const chrono::duration<Rep, Period>& rel_time,
                                          Predicate pred,
                                          interrupt_token itoken)
{
  auto abs_time = std::chrono::steady_clock::now() + rel_time;
  return wait_until(lock,
                    abs_time,
                    std::move(pred),
                    std::move(itoken));
}


} // std

#endif // CONDITION_VARIABLE2_HPP
