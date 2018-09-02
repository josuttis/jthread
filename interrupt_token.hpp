#ifndef ITOKEN_HPP
#define ITOKEN_HPP

#include "interrupted.hpp"
#include <memory>
#include <atomic>

namespace std {

//***************************************** 
//* new class for interrupt tokens
//* - very cheap to create
//* - cheap to copy
//* - for both interrupter and interruptee
//***************************************** 
class interrupt_token {
 private:
  std::shared_ptr<std::atomic<bool>> _ip{nullptr};
 public:
  // default constructor is cheap:
  explicit interrupt_token() noexcept = default;
  // enable interrupt mechanisms by passing a bool (usually false):
  explicit interrupt_token(bool initial_state)
   : _ip{new std::atomic<bool>{initial_state}} {
  }

  // special members (default OK):
  //interrupt_token(const interrupt_token&) noexcept;
  //interrupt_token(interrupt_token&&) noexcept;
  //interrupt_token& operator=(const interrupt_token&) noexcept;
  //interrupt_token& operator=(interrupt_token&&) noexcept;
  void swap(interrupt_token& it) noexcept {
    _ip.swap(it._ip);
  }

  bool valid() const {
    return _ip != nullptr;
  }
  // interrupt handling:
  bool interrupt() noexcept {
    return _ip && _ip->exchange(true);
  }
  bool is_interrupted() const noexcept {
    return _ip && _ip->load();
  }
  void throw_if_interrupted() const {
    if (_ip && _ip->load()) {
      throw ::std::interrupted();
    }
  }
  
  friend bool operator== (const interrupt_token& lhs, const interrupt_token& rhs);
};

bool operator== (const interrupt_token& lhs, const interrupt_token& rhs) {
  return (!lhs.valid() && !rhs.valid())
         || (lhs._ip.get() == rhs._ip.get());
}
bool operator!= (const interrupt_token& lhs, const interrupt_token& rhs) {
  return !(lhs==rhs);
}

} // namespace std

#endif // ITOKEN_HPP
