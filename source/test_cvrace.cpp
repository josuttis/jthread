#include "jthread.hpp"
#include "condition_variable_any2.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>
#include <cassert>
#include <cstring>
using namespace::std::literals;


//------------------------------------------------------

void testCVDeadlock()
{
  // test the basic jthread API
  std::cout << "*** start testCVDeadlock()" << std::endl;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable_any2 readyCV;
  
// T1:
//  given cv1 with cvMx1
//  currently waiting
// T2:
//  locks cvMx1
//  interrupt
//  - block and try to notify cv1
  
  {
    std::jthread t1([&ready, &readyMutex, &readyCV] (std::stop_token st) {
                      std::cout << "\n" <<std::this_thread::get_id()<<": t1: lock "<<&readyMutex << std::endl;
                      std::unique_lock<std::mutex> lg{readyMutex};
                      std::cout << "\n" <<std::this_thread::get_id()<<": t1: wait" << std::endl;
                      readyCV.wait(lg,
                                   st,
                                   [&ready] { return ready; });
                      if (st.stop_requested()) {
                        std::cout << "\n" <<std::this_thread::get_id()<<": t1: signal stop" << std::endl;
                      }
                      else {
                        std::cout << "\n" <<std::this_thread::get_id()<<": t1: ready" << std::endl;
                      }
                    });

    auto t1StopSource = t1.get_stop_source();

    std::this_thread::sleep_for(1s);
    std::jthread t2([&ready, &readyMutex, &readyCV, &t1StopSource] (std::stop_token /*t2_stop_token_not_used*/) {
                        std::cout << "\n" <<std::this_thread::get_id()<<": t2: lock " <<&readyMutex << std::endl;
                      std::unique_lock<std::mutex> lg{readyMutex};
                      std::cout << "\n" <<std::this_thread::get_id()<<": t2: signal stop" << std::endl;
		      t1StopSource.request_stop();
                      std::cout << "\n" <<std::this_thread::get_id()<<": t2: signal-stop done" << std::endl;
                    });

    std::this_thread::sleep_for(1s);
    // set ready and notify:
    {
      std::lock_guard lg{readyMutex};
      ready = true;
      readyCV.notify_one();
    }

    t2.join();
    
  } // leave scope of t1 without join() or detach() (signals cancellation)
  std::cout << "\n*** OK" << std::endl;
}




//------------------------------------------------------

int main()
{
 try {
  std::set_terminate([](){
                       std::cout << "ERROR: terminate() called" << std::endl;
                       assert(false);
                     });

  std::cout << std::boolalpha;

  std::cout << "\n\n**************************\n";
  testCVDeadlock();
  std::cout << "\n\n**************************\n";
 }
 catch (const std::exception& e) {
   std::cerr << "EXCEPTION: " << e.what() << std::endl;
 }
 catch (...) {
   std::cerr << "EXCEPTION" << std::endl;
 }
}

