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
#include <thread>

namespace std {

bool interrupt_token::interrupt()
{
    std::cout<<std::this_thread::get_id()<<": Interrupting "<<std::endl;
  //std::cout.put('I').flush();
  if (!valid()) return false;
  auto wasInterrupted = _ip->interrupted.exchange(true);
  if (!wasInterrupted) {
      ::std::unique_lock lg{_ip->cbDataMutex};  // might throw
      // note, we no longer accept new callbacks here in the list
      // but other callbacks might be unregistered
      while(!_ip->cbData.empty()) {
        std::cout<<std::this_thread::get_id()<<": Notifying "<<std::endl;
        auto elem = _ip->cbData.front();
        _ip->cbData.pop_front();
        lg.unlock();
        elem->run();  // don't call the callback locked
        lg.lock();
      }
  }
  //std::cout.put('i').flush();
  return wasInterrupted;
}

void interrupt_token::registerCB(interrupt_callback_base* cbPtr) {
  //std::cout.put('R').flush();
  if (!valid()) return;

  std::unique_lock lg{_ip->cbDataMutex};
  if (_ip->interrupted.load()) {
    // if already interrupted, make sure the callback is durectly called
    // - but not blocking others
    lg.unlock();
    cbPtr->run();
  }
  else {
    _ip->cbData.emplace_front(cbPtr);  // might throw
  }
  //std::cout.put('r').flush();
}

void interrupt_token::unregisterCB(interrupt_callback_base* cbPtr) {
  //std::cout.put('U').flush();
  if (!valid()) return;

  {
    std::scoped_lock lg{_ip->cbDataMutex};
    // remove the matching CB
    for (auto pos = _ip->cbData.begin(); pos != _ip->cbData.end(); ++pos) {
      if (*pos == cbPtr) {
        _ip->cbData.erase(pos);
        break;
      }
    }
  }
  //std::cout.put('u').flush();
}


} // namespace std

#endif // INTERRUPT_TOKEN_HPP
