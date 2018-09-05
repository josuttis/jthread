#ifndef INTERRUPT_TOKEN_HPP
#define INTERRUPT_TOKEN_HPP

//*****************************************************************************
// forward declarations are in separate header due to cyclic type dependencies:
//*****************************************************************************
#include "jthread_fwd.hpp"


//*****************************************************************************
//* implementation of new this_thread API for interrupts:
//*****************************************************************************
#include "interrupted.hpp"
#include <cassert>

namespace std {

void interrupt_token::throw_if_interrupted() const
{
  if (_ip && _ip->interrupted.load()) {
    throw ::std::interrupted();
  }
}

bool interrupt_token::interrupt() noexcept
{
  //return _ip && _ip->interrupted.exchange(true);
  if (!valid()) return false;
  auto wasInterrupted = _ip->interrupted.exchange(true);
  if (!wasInterrupted) {
    if(::std::scoped_lock lg{_ip->cvMutex}; _ip->cvPtr) {
      _ip->cvPtr->notify_all();
    }
  }
  return wasInterrupted;
}

bool interrupt_token::registerCV(condition_variable2* cvPtr) noexcept {
  if (!valid()) return false;
  std::scoped_lock lg{_ip->cvMutex};
  _ip->cvPtr = cvPtr;
  return _ip->interrupted.load();
}

bool interrupt_token::unregisterCV(condition_variable2* cvPtr) noexcept {
  if (!valid()) return false;
  std::scoped_lock lg{_ip->cvMutex};
  assert(_ip->cvPtr == cvPtr);
  _ip->cvPtr = nullptr;
  return _ip->interrupted.load();
}


} // namespace std

#endif // INTERRUPT_TOKEN_HPP
