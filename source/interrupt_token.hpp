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
    // for each CV try to lock and notify and remove the CV from the list
    // HOWEVER: We have to avoid deadlocks.
    //          CV's might be locked (e.g. while trying to (un)register).
    //          In that case delay notifications until we get access. 
    // Thus: We LOOP until we got access to all CVs
    bool oneCVNotNotified = true;
    while(oneCVNotNotified) {
      oneCVNotNotified = false;
      {
        ::std::scoped_lock lg{_ip->cvDataMutex};  // might throw
        for (auto& cvd : _ip->cvData) {
          if (!cvd.readyToErase) {
            // We have to ensure that notify() is not called between CV's check
            //  for is_interrupted() and the wait call:
            // Thus, we (try to) lock the CV mutex before we call notify():
            std::unique_lock ul{*(cvd.cvMxPtr), std::try_to_lock};
            if (ul) {
              cvd.cvPtr->notify_all();
              cvd.readyToErase = true;
            }
            else {
              oneCVNotNotified = true;
            }
          }
        }
      } // give up lock on cvDataMutex and yield 
      if (oneCVNotNotified) {
        std::this_thread::yield();
      }
    }
    
    // NOTE: We remove all registered CV's here because we have to avoid the
    //       following deadlocks or unnecessary locks:
    //       - we locked the token mutex and wait here for the CV mutex
    //       - register() or unregister() have the CV mutex and wait for the token mutex
    //       So register() and unregister() can return immediately
    //       when interrupt is signaled and we are here in this loop
    _ip->cvData.clear();
  }
  //std::cout.put('i').flush();
  return wasInterrupted;
}

void interrupt_token::registerCV(condition_variable2* cvPtr, mutex* cvMxPtr) {
  //std::cout.put('R').flush();
  if (!valid()) return;

  // don't register anymore when already interrupted
  // - important to avoid the deadlock descibed in interrupt()
  if (_ip->interrupted.load()) return;

  {
    std::scoped_lock lg{_ip->cvDataMutex};
    _ip->cvData.emplace_front(cvPtr, cvMxPtr);  // might throw
  }
  //std::cout.put('r').flush();
}

void interrupt_token::unregisterCV(condition_variable2* cvPtr) {
  //std::cout.put('U').flush();
  if (!valid()) return;

  // don't register anymore when already interrupted
  // - important to avoid the deadlock descibed in interrupt()
  if (_ip->interrupted.load()) return;

  {
    std::scoped_lock lg{_ip->cvDataMutex};
    // remove the FIRST matching CV
    for (auto pos = _ip->cvData.begin(); pos != _ip->cvData.end(); ++pos) {
      if (pos->cvPtr == cvPtr) {
        _ip->cvData.erase(pos);
        break;
      }
    }
  }
  //std::cout.put('u').flush();
}


} // namespace std

#endif // INTERRUPT_TOKEN_HPP
