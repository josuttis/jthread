#include "condition_variable2.hpp"
#include "jthread.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <mutex>
#include <atomic>
#include <cassert>
using namespace::std::literals;

//------------------------------------------------------

void testStdCV(bool doNotify)
{
  // test the basic jthread API
  std::cout << "*** start testStdCV(doNotify=" << doNotify << ")" << std::endl;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, doNotify] {
                      {
                        std::unique_lock lg{readyMutex};
                        while (!std::this_thread::is_interrupted() && !ready) {
                          readyCV.wait_for(lg, 100ms);
                          std::cout.put('.').flush();
                        }
                      }
                      if (std::this_thread::is_interrupted()) {
                        std::cout << "t1: interrupted" << std::endl;
                      }
                      else {
                        std::cout << "t1: ready" << std::endl;
                      }
                      assert(doNotify != std::this_thread::is_interrupted());
                    });
    
    std::this_thread::sleep_for(1s);
    if (doNotify) {
      {
        std::lock_guard lg(readyMutex);
        ready = true;
      } // release lock
      std::cout << "- call notify_one()" << std::endl;
      readyCV.notify_one();
      t1.join();
    }
  } // leave scope of t1 without join() or detach() (signals cancellation)
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testCVNoPred(bool doNotify)
{
  // test the basic jthread API
  std::cout << "*** start testCVNoPred(doNotify=" << doNotify << ")" << std::endl;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, doNotify] {
                      try {
                        {
                          std::unique_lock lg{readyMutex};
                          while(!ready) {
                            readyCV.wait_or_throw(lg);
                          }
                        }
                        assert(doNotify);
                      }
                      catch (const std::exception&) {  // should be no std::exception
                        assert(false);
                      }
                      catch (const std::interrupted&) {
                        assert(!doNotify);
                      }
                      catch (...) {
                        assert(false);
                      }
                      if (std::this_thread::is_interrupted()) {
                        std::cout << "t1: interrupted" << std::endl;
                      }
                      else {
                        std::cout << "t1: ready" << std::endl;
                      }
                      assert(doNotify != std::this_thread::is_interrupted());
                    });
    
    std::this_thread::sleep_for(1s);
    if (doNotify) {
      {
        std::lock_guard lg(readyMutex);
        ready = true;
      } // release lock
      std::cout << "- call notify_one()" << std::endl;
      readyCV.notify_one();
      t1.join();
    }
  } // leave scope of t1 without join() or detach() (signals cancellation)
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testCVPred(bool doNotify)
{
  // test the basic jthread API
  std::cout << "*** start testCVPred(doNotify=" << doNotify << ")" << std::endl;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, doNotify] {
                      try {
                        std::unique_lock lg{readyMutex};
                        readyCV.wait_or_throw(lg, [&ready] { return ready; });
                        assert(doNotify);
                      }
                      catch (const std::exception&) {  // should be no std::exception
                        assert(false);
                      }
                      catch (const std::interrupted&) {
                        assert(!doNotify);
                      }
                      catch (...) {
                        assert(false);
                      }
                      if (std::this_thread::is_interrupted()) {
                        std::cout << "t1: interrupted" << std::endl;
                      }
                      else {
                        std::cout << "t1: ready" << std::endl;
                      }
                      assert(doNotify != std::this_thread::is_interrupted());
                    });
    
    std::this_thread::sleep_for(1s);
    if (doNotify) {
      {
        std::lock_guard lg(readyMutex);
        ready = true;
      } // release lock
      std::cout << "- call notify_one()" << std::endl;
      readyCV.notify_one();
      t1.join();
    }
  } // leave scope of t1 without join() or detach() (signals cancellation)
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testCVStdThreadPred(bool doNotify)
{
  // test the basic jthread API
  std::cout << "*** start testCVStdThread(doNotify=" << doNotify << ")" << std::endl;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  
  std::interrupt_token it{false};
  {
    std::thread t1([&ready, &readyMutex, &readyCV, it, doNotify] {
                      //std::cout.put('a').flush();
                      bool ret;
                      {
                        std::unique_lock lg{readyMutex};
                        //std::cout.put('b').flush();
                        ret = readyCV.wait_until(lg,
                                                 [&ready] { return ready; },
                                                 it);
                      }
                      //std::cout.put('c').flush();
                      if (ret) {
                        std::cout << "t1: ready" << std::endl;
                        assert(!it.is_interrupted());
                      }
                      else {
                        std::cout << "t1: interrupted" << std::endl;
                        assert(it.is_interrupted());
                      }
                      //std::cout.put('d').flush();
                    });
    
    std::this_thread::sleep_for(1s);
    assert(!it.is_interrupted());
    std::this_thread::sleep_for(1s);
    if (doNotify) {
      {
        std::lock_guard lg(readyMutex);
        ready = true;
      } // release lock
      std::cout << "- call notify_one()" << std::endl;
      readyCV.notify_one();
    }
    else {
      std::cout << "- signal interrupt" << std::endl;
      it.interrupt();
    }
    t1.join();
  } // leave scope of t1 without join() or detach() (signals cancellation)
  std::cout << "\n*** OK" << std::endl;
}


//------------------------------------------------------

int main()
{
  std::set_terminate([](){
                       std::cout << "ERROR: terminate() called" << std::endl;
                       std::this_thread::sleep_for(10s);
                       assert(false);
                     });

  std::cout << std::boolalpha;

  std::cout << "\n\n**************************\n";
  testStdCV(false);  // signal cancellation
  std::cout << "\n\n**************************\n";
  testStdCV(true);   // call notify()

  std::cout << "\n\n**************************\n";
  testCVPred(false);  // signal cancellation
  std::cout << "\n\n**************************\n";
  testCVPred(true);   // call notify()

  std::cout << "\n\n**************************\n";
  testCVNoPred(false);  // signal cancellation
  std::cout << "\n\n**************************\n";
  testCVNoPred(true);   // call notify()

  std::cout << "\n\n**************************\n";
  testCVStdThreadPred(false);  // signal cancellation
  std::cout << "\n\n**************************\n";
  testCVStdThreadPred(true);   // call notify()
}

