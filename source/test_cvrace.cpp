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
    std::jthread t1([&ready, &readyMutex, &readyCV] (std::stop_token it) {
                        std::cout << "\n" <<std::this_thread::get_id()<<": t1: lock "<<&readyMutex << std::endl;
                      std::unique_lock<std::mutex> lg{readyMutex};
                      std::cout << "\n" <<std::this_thread::get_id()<<": t1: wait" << std::endl;
                      readyCV.wait_until(lg,
                                         [&ready] { return ready; },
                                         it);
                      if (it.stop_requested()) {
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

#ifndef ORIG_TEST
std::condition_variable_any2* cv;
std::mutex m;
bool f_ready = false;
bool g_ready = false;

void f() {
    m.lock();
    f_ready = true;
    cv->notify_one();
    delete cv;
    std::memset(cv, 0x55, sizeof(*cv)); // UB but OK to ensure the check
    m.unlock();
}

void g() {
    m.lock();
    g_ready = true;
    cv->notify_one();
    while (!f_ready)
        cv->wait(m);
    m.unlock();
}

void testCVAnyMutex() 
{
    cv = new std::condition_variable_any2;
    std::thread th2(g);
    m.lock();
    while (!g_ready)
        cv->wait(m);
    m.unlock();
    std::thread th1(f);
    th1.join();
    th2.join();
}
#else

void testCVAnyMutex() 
{
  std::condition_variable_any2* cv = new std::condition_variable_any2;
  std::mutex m;
  bool f_ready = false;
  bool g_ready = false;

  std::thread t2([&] {
                    std::unique_lock ul{m};
                    g_ready = true;
                    cv->notify_one();
                    while (!f_ready) {
                        cv->wait(ul);
                    }
                 });
  {
    std::unique_lock ul{m};
    while (!g_ready) {
      cv->wait(ul);
    }
  }
  std::thread t1([&] {
                    std::unique_lock ul{m};
                    f_ready = true;
                    cv->notify_one();
                    delete cv;
                    std::memset(cv, 0x55, sizeof(*cv)); // UB but OK to ensure the check
                 });
  t1.join();
  t2.join();
}
#endif

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
  testCVAnyMutex(); 
  std::cout << "\n\n**************************\n";
 }
 catch (const std::exception& e) {
   std::cerr << "EXCEPTION: " << e.what() << std::endl;
 }
 catch (...) {
   std::cerr << "EXCEPTION" << std::endl;
 }
}

