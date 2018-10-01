// -----------------------------------------------------
// cooperative interruptable and joining thread:
// -----------------------------------------------------
#ifndef JTHREAD_HPP
#define JTHREAD_HPP

#include "interrupt_token.hpp"
#include <thread>
#include <future>
#include <type_traits>
#include <functional>  // for invoke()
#include <iostream>    // for debugging output

namespace std {

//***************************************** 
//* class jthread
//* - joining std::thread with interrupt support 
//***************************************** 
class jthread
{
  public:
    //***************************************** 
    //* standardized API:
    //***************************************** 
    // - cover full API of std::thread
    //   to be able to switch from std::thread to std::jthread

    // types are those from std::thread:
    using id = ::std::thread::id;
    using native_handle_type = ::std::thread::native_handle_type;

    // construct/copy/destroy:
    jthread() noexcept;
    //template <typename F, typename... Args> explicit jthread(F&& f, Args&&... args);
    // THE constructor that starts the thread:
    // - NOTE: does SFINAE out copy constructor semantics
    template <typename Callable, typename... Args,
              typename = ::std::enable_if_t<!::std::is_same_v<::std::decay_t<Callable>, jthread>>>
    explicit jthread(Callable&& cb, Args&&... args);
    ~jthread();

    jthread(const jthread&) = delete;
    jthread(jthread&&) noexcept = default;
    jthread& operator=(const jthread&) = delete;
    jthread& operator=(jthread&&) noexcept = default;

    // members:
    void swap(jthread&) noexcept;
    bool joinable() const noexcept;
    void join();
    void detach();

    id get_id() const noexcept;
    native_handle_type native_handle();

    // static members:
    static unsigned hardware_concurrency() noexcept {
      return ::std::thread::hardware_concurrency();
    };

    //***************************************** 
    // - supplementary API:
    //   - for the calling thread:
    interrupt_token get_original_interrupt_token() const noexcept;
    bool interrupt() noexcept {
      return get_original_interrupt_token().interrupt();
    }


  //***************************************** 
  //* implementation:
  //***************************************** 

  private:
    //*** API for the starting thread:
    interrupt_token _thread_it{};              // interrupt token for started thread
    ::std::thread _thread{::std::thread{}};    // started thread (if any)

    //*** API for the started thread (TLS stuff):
    inline static thread_local interrupt_token _this_thread_it{}; // int.token for this thread
    friend interrupt_token this_thread::get_interrupt_token() noexcept;
};


//**********************************************************************

//***************************************** 
//* implementation of class jthread
//***************************************** 

// default constructor:
inline jthread::jthread() noexcept {
}

// THE constructor that starts the thread:
// - NOTE: declaration does SFINAE out copy constructor semantics
template <typename Callable, typename... Args,
          typename >
inline jthread::jthread(Callable&& cb, Args&&... args)
 : _thread_it{false},                             // initialize interrupt token
   _thread{[] (interrupt_token it, auto&& cb, auto&&... args) {   // called lambda in the thread
                 // pass the interrupt_token to the started thread
                 _this_thread_it = it;

                 // perform tasks of the thread:
                 if constexpr(std::is_invocable_v<Callable, interrupt_token, Args...>) {
                   // pass the interrupt_token as first argument to the started thread:
                   ::std::invoke(::std::forward<decltype(cb)>(cb),
                                 std::move(it),
                                 ::std::forward<decltype(args)>(args)...);
                 }
                 else {
                   // started thread does not expect an interrupt token:
                   ::std::invoke(::std::forward<decltype(cb)>(cb),
                                 ::std::forward<decltype(args)>(args)...);
                 }
               },
               _thread_it,   // not captured due to possible races if immediately set
               ::std::forward<Callable>(cb),  // pass callable
               ::std::forward<Args>(args)...  // pass arguments for callable
           }
{
}

// destructor:
jthread::~jthread() {
  if (joinable()) {   // if not joined/detached, interrupt and wait for end:
    interrupt();
    join();
  }
}


// others:
inline bool jthread::joinable() const noexcept {
  return _thread.joinable();
}
inline void jthread::join() {
  _thread.join();
}
inline void jthread::detach() {
  _thread.detach();
}
inline typename jthread::id jthread::get_id() const noexcept {
  return _thread.get_id();
}
inline typename jthread::native_handle_type jthread::native_handle() {
  return _thread.native_handle();
}

inline interrupt_token jthread::get_original_interrupt_token() const noexcept {
  return _thread_it;
}

void jthread::swap(jthread& t) noexcept {
    std::swap(_thread_it, t._thread_it);
    std::swap(_thread, t._thread);
}


//***************************************** 
//* implementation of new this_thread API for interrupts:
//***************************************** 
namespace this_thread {
  static interrupt_token get_interrupt_token() noexcept {
    return ::std::jthread::_this_thread_it;
  }
  static bool is_interrupted() noexcept {
    return get_interrupt_token().is_interrupted();
  }
} // this_thread

} // std

#endif // JTHREAD_HPP
