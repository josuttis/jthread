#ifndef INTERRUPT_TOKEN_HPP
#define INTERRUPT_TOKEN_HPP

#include "interrupted.hpp"
#include <memory>
#include <atomic>
#include <mutex>
#include <cassert>

namespace std {

//***************************************** 
//* new class for interrupt tokens
//* - very cheap to create
//* - cheap to copy
//* - for both interrupter and interruptee
//***************************************** 

// forward declarations to avoid including header file:
class condition_variable2;

class interrupt_token {
 private:
  struct SharedData {
    std::atomic<bool> interrupted;
    condition_variable2* cvPtr;
    std::mutex cvMutex{};      // for any concurrent access to cv, which might be two operations
  };
  std::shared_ptr<SharedData> _ip{nullptr};

 public:
  // default constructor is cheap:
  explicit interrupt_token() noexcept = default;
  // enable interrupt mechanisms by passing a bool (usually false):
  explicit interrupt_token(bool initial_state)
   : _ip{new SharedData{initial_state,nullptr}} {
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
  void throw_if_interrupted() const {
    if (_ip && _ip->interrupted.load()) {
      throw ::std::interrupted();
    }
  }

  bool interrupt() noexcept {
    //return _ip && _ip->interrupted.exchange(true);
    if (!valid()) return false;
    auto wasInterrupted = _ip->interrupted.exchange(true);
    if (!wasInterrupted) {
      if(std::scoped_lock lg{_ip->cvMutex}; _ip->cvPtr) {
        notify_from_interrupt_token(*this);
      }
    }
    return wasInterrupted;
  }
  
  friend bool operator== (const interrupt_token& lhs, const interrupt_token& rhs);
 
 private:
  // stuff to registered condition variables for notofication: 
  friend class ::std::condition_variable2;
  bool registerCV(condition_variable2* cvPtr) noexcept {
    if (!valid()) return false;
    std::scoped_lock lg{_ip->cvMutex};
    _ip->cvPtr = cvPtr;
    return _ip->interrupted.load();
  }
  bool unregisterCV(condition_variable2* cvPtr) noexcept {
    if (!valid()) return false;
    std::scoped_lock lg{_ip->cvMutex};
    assert(_ip->cvPtr == cvPtr);
    _ip->cvPtr = nullptr;
    return _ip->interrupted.load();
  }
  template <typename IT> friend void notify_from_interrupt_token(IT& it) noexcept;

};

bool operator== (const interrupt_token& lhs, const interrupt_token& rhs) {
  return (!lhs.valid() && !rhs.valid())
         || (lhs._ip.get() == rhs._ip.get());
}
bool operator!= (const interrupt_token& lhs, const interrupt_token& rhs) {
  return !(lhs==rhs);
}

} // namespace std

#include "condition_variable2.hpp"
namespace std {
template<typename IT>
void notify_from_interrupt_token(IT& it) noexcept
{
  assert(it._ip && it._ip->cvPtr);
  it._ip->cvPtr->notify_all();
}

} // namespace std

#endif // INTERRUPT_TOKEN_HPP
