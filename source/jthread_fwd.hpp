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


//***************************************** 
//* new class for interrupt tokens
//* - very cheap to create
//* - cheap to copy
//* - for both interrupter and interruptee
//***************************************** 

// callbacks objects have different types => type erase them
struct interrupt_callback_base {
  std::atomic<bool> finishedExecuting{false};
  virtual void run() noexcept = 0;
  virtual ~interrupt_callback_base() = default;
};


struct SharedData {
    ::std::atomic<bool> interrupted;  // true if interrupt signaled

    ::std::list<interrupt_callback_base*> cbData{};     // currently waiting CVs and its lock
    ::std::mutex cbDataMutex{};       // we have multistep concurrent access to cvPtrs

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

template <typename Callback>
class interrupt_callback;

class interrupt_token {
 private:
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
  template <typename T>
  friend class ::std::interrupt_callback;
  void registerCB(interrupt_callback_base* cvPtr);
  void unregisterCB(interrupt_callback_base* cvPtr);
};

bool operator== (const interrupt_token& lhs, const interrupt_token& rhs) {
  return (!lhs.valid() && !rhs.valid())
         || (lhs._ip.get() == rhs._ip.get());
}
bool operator!= (const interrupt_token& lhs, const interrupt_token& rhs) {
  return !(lhs==rhs);
}

template <typename Callback>
struct interrupt_callback : interrupt_callback_base
{
  private:
    interrupt_token itoken;
    Callback callback;
  public:
    interrupt_callback(interrupt_token it, Callback&& cb)
     : itoken{std::move(it)}, callback{std::forward<Callback>(cb)} {
        if (itoken.valid()) {
          itoken.registerCB(this);
        }
    }
    ~interrupt_callback() {
        if (itoken.valid()) {
          itoken.unregisterCB(this);
        }
    }
    interrupt_callback(const interrupt_callback&) = delete;
    interrupt_callback(interrupt_callback&&) = delete;
    interrupt_callback& operator=(const interrupt_callback&) = delete;
    interrupt_callback& operator=(interrupt_callback&&) = delete;
  private:
    virtual void run() noexcept override {
      callback();  //or: std::invoke(callback);
    }    
};



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


} // std

#endif // ITOKEN_AND_CV_FWD_HPP
