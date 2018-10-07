#ifndef INTERRUPT_TOKEN_HPP
#define INTERRUPT_TOKEN_HPP

//*****************************************************************************
// forward declarations are in separate header due to cyclic type dependencies:
//*****************************************************************************
#include "jthread_fwd.hpp"


//*****************************************************************************
//* implementation of interrupt_token member functions:
//*****************************************************************************
#include <cassert>
#include <iostream> // in case we enable the debug output

namespace std {

bool interrupt_token::interrupt()
{
  //std::cout.put('I').flush();
  if (!valid()) return false;
  auto wasInterrupted = _ip->interrupted.exchange(true);
  if (!wasInterrupted) {
    ::std::scoped_lock lg{_ip->cvDataMutex};  // might throw
    for (const auto& cvd : _ip->cvData) {
      // We have to ensure that notify() is not called between CV's check
      //  for is_interrupted() and the wait call:
      // Thus, we lock the CV mutex before we call notify():
      std::lock_guard sl{*(cvd.cvMxPtr)};
      cvd.cvPtr->notify_all();
    }
  }
  //std::cout.put('i').flush();
  return wasInterrupted;
}

bool interrupt_token::registerCV(condition_variable2* cvPtr, mutex* cvMxPtr) {
  //std::cout.put('R').flush();
  if (!valid()) return false;
  {
    std::scoped_lock lg{_ip->cvDataMutex};
    _ip->cvData.emplace_front(cvPtr, cvMxPtr);  // might throw
  }
  //std::cout.put('r').flush();
  return _ip->interrupted.load();
}

bool interrupt_token::unregisterCV(condition_variable2* cvPtr) {
  //std::cout.put('U').flush();
  if (!valid()) return false;
  {
    std::scoped_lock lg{_ip->cvDataMutex};
    // remove the FIRST found cv
    for (auto pos = _ip->cvData.begin(); pos != _ip->cvData.end(); ++pos) {
      if (pos->cvPtr == cvPtr) {
        _ip->cvData.erase(pos);
        break;
      }
    }
  }
  //std::cout.put('u').flush();
  return _ip->interrupted.load();
}


} // namespace std

#endif // INTERRUPT_TOKEN_HPP
