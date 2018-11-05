//*****************************************************************************
// forward declarations of
// - class interrupt_token
// - class condition_variable_any2
// due to cyclic dependencies
//*****************************************************************************
#ifndef ITOKEN_AND_CV_FWD_HPP
#define ITOKEN_AND_CV_FWD_HPP

#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <list>

namespace std {

// forward declarations to avoid including header file:
class condition_variable_any2;
class register_guard;

//***************************************** 
//* new class for interrupt tokens
//* - very cheap to create
//* - cheap to copy
//* - for both interrupter and interruptee
//***************************************** 

class interrupt_token {
 private:
  struct CVData {
    condition_variable_any2* cvPtr;         // currently waiting CVs
    recursive_mutex*         cvMxPtr;       // associated mutex
    std::atomic<bool>        readyToErase;  // CV entry no longer needed (for interrupt())
    CVData(condition_variable_any2* cvp, recursive_mutex* cvmxp)
     : cvPtr{cvp}, cvMxPtr{cvmxp}, readyToErase{false} {
    }
  };
  struct SharedData {
    ::std::atomic<bool> interrupted;  // true if interrupt signaled
    ::std::list<CVData> cvData{};     // currently waiting CVs and its lock
    ::std::mutex cvDataMutex{};       // we have multistep concurrent access to cvPtrs
    // make polymorphic class for future binary-compatible interrupt_token extensions:
    SharedData(bool initial_state)
     : interrupted{initial_state} {
    }
    virtual ~SharedData() = default;  // make polymorphic
    SharedData(const SharedData&) = delete;
    SharedData(SharedData&&) = delete;
    SharedData& operator= (const SharedData&) = delete;
    SharedData& operator= (SharedData&&) = delete;
  };
  ::std::shared_ptr<SharedData> _ip{nullptr};

 public:
  // default constructor is cheap:
  explicit interrupt_token() noexcept = default;
  // enable interrupt mechanisms by passing a bool (usually false):
  explicit interrupt_token(bool initial_state)
   : _ip{new SharedData{initial_state}} {
  }

  // special members (default OK):
  //interrupt_token(const interrupt_token&) noexcept;
  //interrupt_token(interrupt_token&&) noexcept;
  //interrupt_token& operator=(const interrupt_token&) noexcept;
  //interrupt_token& operator=(interrupt_token&&) noexcept;
  void swap(interrupt_token& it) noexcept {
    _ip.swap(it._ip);
  }

  // interrupt handling:
  bool valid() const {
    return _ip != nullptr;
  }
  bool is_interrupted() const noexcept {
    return _ip && _ip->interrupted.load();
  }

  bool interrupt();
  
  friend bool operator== (const interrupt_token& lhs, const interrupt_token& rhs);
 
 private:
  // stuff to registered condition variables for notofication: 
  friend class ::std::condition_variable_any2;
  friend class ::std::register_guard;
  void registerCV(condition_variable_any2* cvPtr, recursive_mutex* cvMxPtr);
  void unregisterCV(condition_variable_any2* cvPtr);
};

bool operator== (const interrupt_token& lhs, const interrupt_token& rhs) {
  return (!lhs.valid() && !rhs.valid())
         || (lhs._ip.get() == rhs._ip.get());
}
bool operator!= (const interrupt_token& lhs, const interrupt_token& rhs) {
  return !(lhs==rhs);
}


//***************************************** 
//* class condition_variable_any2
//* - joining std::thread with interrupt support 
//***************************************** 
class condition_variable_any2
{
  public:
    //***************************************** 
    //* standardized API for condition_variable_any:
    //***************************************** 

    condition_variable_any2()
     : cv{} {
    }
    ~condition_variable_any2() {
    }
    condition_variable_any2(const condition_variable_any2&) = delete;
    condition_variable_any2& operator=(const condition_variable_any2&) = delete;

    void notify_one() noexcept {
      cv.notify_one();
    }
    void notify_all() noexcept {      
      cv.notify_all();
    }

    template<class Lock>
    void wait(Lock& lock) {
      cv.wait(lock);
    }
    template<class Lock, class Predicate>
     void wait(Lock& lock, Predicate pred) {
      cv.wait(lock,pred);
    }
    template<class Lock, class Clock, class Duration>
     cv_status wait_until(Lock& lock,
                          const chrono::time_point<Clock, Duration>& abs_time) {
      return cv.wait_until(lock, abs_time);
    }
    template<class Lock, class Clock, class Duration, class Predicate>
     bool wait_until(Lock& lock,
                     const chrono::time_point<Clock, Duration>& abs_time,
                     Predicate pred) {
      return cv.wait_until(lock, abs_time, pred);
    }
    template<class Lock, class Rep, class Period>
     cv_status wait_for(Lock& lock,
                        const chrono::duration<Rep, Period>& rel_time) {
      return cv.wait_for(lock, rel_time);
    }
    template<class Lock, class Rep, class Period, class Predicate>
     bool wait_for(Lock& lock,
                   const chrono::duration<Rep, Period>& rel_time,
                   Predicate pred) {
      return cv.wait_for(lock, rel_time, pred);
    }

    //***************************************** 
    //* supplementary API:
    //***************************************** 

    // x.6.2.1 dealing with interrupts:

    // return:
    // - true if pred() yields true
    // - false otherwise (i.e. on interrupt)
    template <class Predicate>
      bool wait_until(unique_lock<recursive_mutex>& lock,
                      Predicate pred,
                      interrupt_token itoken);

    // return:
    // - true if pred() yields true
    // - false otherwise (i.e. on timeout or interrupt)
    template <class Clock, class Duration, class Predicate>
      bool wait_until(unique_lock<recursive_mutex>& lock,
                      const chrono::time_point<Clock, Duration>& abs_time,
                      Predicate pred,
                      interrupt_token itoken);
    // return:
    // - true if pred() yields true
    // - false otherwise (i.e. on timeout or interrupt)
    template <class Rep, class Period, class Predicate>
      bool wait_for(unique_lock<recursive_mutex>& lock,
                    const chrono::duration<Rep, Period>& rel_time,
                    Predicate pred,
                    interrupt_token itoken);

  //***************************************** 
  //* implementation:
  //***************************************** 

  private:
    //*** API for the starting thread:
    condition_variable_any cv;
};


} // std

#endif // ITOKEN_AND_CV_FWD_HPP
