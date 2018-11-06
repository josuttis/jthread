// -----------------------------------------------------
// extended standard condition_variable to deal with
// interrupt tokens and jthread
// -----------------------------------------------------
#ifndef CONDITION_VARIABLE2_HPP
#define CONDITION_VARIABLE2_HPP

//*****************************************************************************
// forward declarations are in separate header due to cyclic type dependencies:
//*****************************************************************************
#include "interrupt_token.hpp"
#include <condition_variable>
#include <iostream>

namespace std {


//***************************************** 
//* class condition_variable_any2
//* - joining std::thread with interrupt support 
//***************************************** 
class condition_variable_any2
{
    template<typename Lockable>
    struct unlock_guard{
        unlock_guard(Lockable& mtx_):
            mtx(mtx_){
            mtx.unlock();
        }
        ~unlock_guard(){
            mtx.lock();
        }
        unlock_guard(unlock_guard const&)=delete;
        unlock_guard(unlock_guard&&)=delete;
        unlock_guard& operator=(unlock_guard const&)=delete;
        unlock_guard& operator=(unlock_guard&&)=delete;
        
    private:
        Lockable& mtx;
    };
    
  public:
    //***************************************** 
    //* standardized API for condition_variable_any:
    //***************************************** 

    condition_variable_any2()
     : cv{}, mut{} {
    }
    ~condition_variable_any2() {
    }
    condition_variable_any2(const condition_variable_any2&) = delete;
    condition_variable_any2& operator=(const condition_variable_any2&) = delete;

    void notify_one() noexcept {
        std::lock_guard<std::mutex> guard(mut);
      cv.notify_one();
    }
    void notify_all() noexcept {      
        std::lock_guard<std::mutex> guard(mut);
      cv.notify_all();
    }

    template<typename Lockable>
    void wait(Lockable& lock) {
        std::unique_lock<std::mutex> first_internal_lock(mut);
        unlock_guard<Lockable> unlocker(lock);
        std::unique_lock<std::mutex> second_internal_lock(std::move(first_internal_lock));
      cv.wait(second_internal_lock);
    }
    template<class Lockable,class Predicate>
     void wait(Lockable& lock, Predicate pred) {
        std::unique_lock<std::mutex> first_internal_lock(mut);
        unlock_guard<Lockable> unlocker(lock);
        std::unique_lock<std::mutex> second_internal_lock(std::move(first_internal_lock));
      cv.wait(second_internal_lock,pred);
    }
    template<class Lockable, class Clock, class Duration>
     cv_status wait_until(Lockable& lock,
                          const chrono::time_point<Clock, Duration>& abs_time) {
        std::unique_lock<std::mutex> first_internal_lock(mut);
        unlock_guard<Lockable> unlocker(lock);
        std::unique_lock<std::mutex> second_internal_lock(std::move(first_internal_lock));
      return cv.wait_until(second_internal_lock, abs_time);
    }
    template<class Lockable,class Clock, class Duration, class Predicate>
     bool wait_until(Lockable& lock,
                     const chrono::time_point<Clock, Duration>& abs_time,
                     Predicate pred) {
        std::unique_lock<std::mutex> first_internal_lock(mut);
        unlock_guard<Lockable> unlocker(lock);
        std::unique_lock<std::mutex> second_internal_lock(std::move(first_internal_lock));
      return cv.wait_until(second_internal_lock, abs_time, pred);
    }
    template<class Lockable,class Rep, class Period>
     cv_status wait_for(Lockable& lock,
                        const chrono::duration<Rep, Period>& rel_time) {
        std::unique_lock<std::mutex> first_internal_lock(mut);
        unlock_guard<Lockable> unlocker(lock);
        std::unique_lock<std::mutex> second_internal_lock(std::move(first_internal_lock));
      return cv.wait_for(second_internal_lock, rel_time);
    }
    template<class Lockable,class Rep, class Period, class Predicate>
     bool wait_for(Lockable& lock,
                   const chrono::duration<Rep, Period>& rel_time,
                   Predicate pred) {
        std::unique_lock<std::mutex> first_internal_lock(mut);
        unlock_guard<Lockable> unlocker(lock);
        std::unique_lock<std::mutex> second_internal_lock(std::move(first_internal_lock));
      return cv.wait_for(second_internal_lock, rel_time, pred);
    }

    //***************************************** 
    //* supplementary API:
    //***************************************** 

    // x.6.2.1 dealing with interrupts:

    // return:
    // - true if pred() yields true
    // - false otherwise (i.e. on interrupt)
    template <class Lockable,class Predicate>
      bool wait_until(Lockable& lock,
                      Predicate pred,
                      interrupt_token itoken);

    // return:
    // - true if pred() yields true
    // - false otherwise (i.e. on timeout or interrupt)
    template <class Lockable, class Clock, class Duration, class Predicate>
      bool wait_until(Lockable& lock,
                      const chrono::time_point<Clock, Duration>& abs_time,
                      Predicate pred,
                      interrupt_token itoken);
    // return:
    // - true if pred() yields true
    // - false otherwise (i.e. on timeout or interrupt)
    template <class Lockable, class Rep, class Period, class Predicate>
      bool wait_for(Lockable& lock,
                    const chrono::duration<Rep, Period>& rel_time,
                    Predicate pred,
                    interrupt_token itoken);

  //***************************************** 
  //* implementation:
  //***************************************** 

  private:
    //*** API for the starting thread:
    condition_variable cv;
    std::mutex mut;
};



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
