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
//* implementation of class condition_variable_any2
//*****************************************************************************

// wait_until(): wait with interrupt handling 
// - returns on interrupt
// return value:
// - true if pred() yields true
// - false otherwise (i.e. on interrupt)
template <class Lockable, class Predicate>
inline bool condition_variable_any2::wait_until(Lockable& lock,
                                            Predicate pred,
                                            interrupt_token itoken)
{
    if (itoken.is_interrupted()) {
      return pred();
    }
    interrupt_callback cb(itoken, [this] { this->notify_all(); });
    //register_guard rg{itoken, this};
    while(!pred()) {
        std::unique_lock<std::mutex> first_internal_lock(mut);
        if(itoken.is_interrupted())
            break;
        unlock_guard<Lockable> unlocker(lock);
        std::unique_lock<std::mutex> second_internal_lock(std::move(first_internal_lock));
        cv.wait(second_internal_lock);
    }
    return pred();
}

// wait_until(): timed wait with interrupt handling 
// - returns on interrupt
// return:
// - true if pred() yields true
// - false otherwise (i.e. on timeout or interrupt)
template <class Lockable, class Clock, class Duration, class Predicate>
inline bool condition_variable_any2::wait_until(Lockable& lock,
                                            const chrono::time_point<Clock, Duration>& abs_time,
                                            Predicate pred,
                                            interrupt_token itoken)
{
    if (itoken.is_interrupted()) {
      return pred();
    }
    interrupt_callback cb(itoken, [this] { this->notify_all(); });
    //register_guard rg{itoken, this};
    while(!pred()  && Clock::now() < abs_time) {
        std::unique_lock<std::mutex> first_internal_lock(mut);
        if(itoken.is_interrupted())
            break;
        unlock_guard<Lockable> unlocker(lock);
        std::unique_lock<std::mutex> second_internal_lock(std::move(first_internal_lock));
        cv.wait_until(second_internal_lock,abs_time);
    }
    return pred();
}

// wait_for(): timed wait with interrupt handling 
// - returns on interrupt
// return:
// - true if pred() yields true
// - false otherwise (i.e. on timeout or interrupt)
template <class Lockable,class Rep, class Period, class Predicate>
inline bool condition_variable_any2::wait_for(Lockable& lock,
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
