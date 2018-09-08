#include "condition_variable2.hpp"
#include "jthread.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <mutex>
#include <atomic>
#include <cassert>
using namespace::std::literals;

void cvWaitOrThrow (int id, bool& ready, std::mutex& readyMutex, std::condition_variable2& readyCV, bool notifyCalled) {
  std::ostringstream strm;
  strm <<"\ncvWaitOrThrow(" << std::to_string(id) << ") called in thread "
       << std::this_thread::get_id();
  std::cout << strm.str() << std::endl;

  try {
    {
      std::unique_lock lg{readyMutex};
      while(!ready) {
        //std::cout.put(static_cast<char>('0'+id)).flush();
        readyCV.wait_or_throw(lg);
      }
    }
    if (std::this_thread::is_interrupted()) {
      std::string msg = "\ncvWaitOrThrow(" + std::to_string(id) + "): interrupted";
      std::cout << msg << std::endl;
      assert(false);
    }
    else {
      std::string msg = "\ncvWaitOrThrow(" + std::to_string(id) + "): ready";
      std::cout << msg << std::endl;
    }
    assert(notifyCalled);
  }
  catch (const std::interrupted& e) {
    std::string msg = "\nINTERRUPT in cvWaitOrThrow(" + std::to_string(id) + "): " + e.what();
    std::cerr << msg << std::endl;
    assert(!notifyCalled);
    throw;
  }
  catch (const std::exception& e) {
    std::string msg = "\nEXCEPTION in cvWaitOrThrow(" + std::to_string(id) + "): " + e.what();
    std::cerr << msg << std::endl;
    assert(false);
  }
  catch (...) {
    std::string msg = "\nEXCEPTION in cvWaitOrThrow(" + std::to_string(id) + ")";
    std::cerr << msg << std::endl;
    assert(false);
  }
}

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

void testCVStdThreadNoPred(bool doNotify)
{
  // test the basic jthread API
  std::cout << "*** start testCVStdThreadNoPred(doNotify=" << doNotify << ")" << std::endl;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  
  std::interrupt_token it{false};
  {
    std::thread t1([&ready, &readyMutex, &readyCV, it, doNotify] {
                      {
                        std::unique_lock lg{readyMutex};
                        while (!ready && !it.is_interrupted()) {
                          auto ret = readyCV.wait_until(lg,
                                                        it);
                          switch (ret) {
                           case std::cv_status2::no_timeout:
                            std::cout << "t1: ready" << std::endl;
                            assert(!it.is_interrupted());
                            break;
                           case std::cv_status2::interrupted:
                            std::cout << "t1: interrupted" << std::endl;
                            assert(it.is_interrupted());
                            break;
                           case std::cv_status2::timeout:
                            assert(false);
                            break;
                          }
                        }
                      }
                      assert(doNotify != it.is_interrupted());
                    });
    
    std::this_thread::sleep_for(0.5s);
    assert(!it.is_interrupted());
    std::this_thread::sleep_for(0.5s);
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

void testCVStdThreadPred(bool doNotify)
{
  // test the basic jthread API
  std::cout << "*** start testCVStdThreadPred(doNotify=" << doNotify << ")" << std::endl;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  
  std::interrupt_token it{false};
  {
    std::thread t1([&ready, &readyMutex, &readyCV, it, doNotify] {
                      bool ret;
                      {
                        std::unique_lock lg{readyMutex};
                        ret = readyCV.wait_until(lg,
                                                 [&ready] { return ready; },
                                                 it);
                        if (ret) {
                          std::cout << "t1: ready" << std::endl;
                          assert(!it.is_interrupted());
                        }
                        else {
                          std::cout << "t1: interrupted" << std::endl;
                          assert(it.is_interrupted());
                        }
                      }
                      assert(doNotify != it.is_interrupted());
                    });
    
    std::this_thread::sleep_for(0.5s);
    assert(!it.is_interrupted());
    std::this_thread::sleep_for(0.5s);
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

template <int numExtraCV>
void testManyCV(bool callNotify, bool callInterrupt)
{
  // test interrupt tokens interrupting multiple CVs
  std::cout << "*** start testManyCV(callNotify=" << callNotify
            << ", callInterrupt=" << callInterrupt << ")"
            << ", numExtraCV=" << numExtraCV << ")" << std::endl;

  std::interrupt_token t0itoken;
  {
    // thread t0 with CV:
    bool ready = false;
    std::mutex readyMutex;
    std::condition_variable2 readyCV;
    std::jthread t0(cvWaitOrThrow, 0, std::ref(ready), std::ref(readyMutex), std::ref(readyCV), callNotify);
    {  
      t0itoken = t0.get_original_interrupt_token();
      std::this_thread::sleep_for(0.5s);

      // starts thread concurrently calling interrupt() for the same token:
      std::cout << "\n- loop to start " << numExtraCV << " threads sharing the token and waiting concurently" << std::endl;
      std::array<bool,numExtraCV> arrReady{};  // don't forget to initialize with {} here !!!
      std::array<std::mutex,numExtraCV> arrReadyMutex{};
      std::array<std::condition_variable2,numExtraCV> arrReadyCV{};

      std::vector<std::jthread> vThreads;
      for (int idx = 0; idx < numExtraCV; ++idx) {
        std::this_thread::sleep_for(0.1ms);
        std::jthread t([idx, t0itoken, &arrReady, &arrReadyMutex, &arrReadyCV, callNotify] {
                         // use interrupt token of t0 instead
                         // NOTE: disables signaling interrupts directly to the thread
                         std::this_thread::exchange_interrupt_token(t0itoken);

                         cvWaitOrThrow(idx+1, arrReady[idx], arrReadyMutex[idx], arrReadyCV[idx], callNotify);
                       });
        vThreads.push_back(std::move(t));
      }

      std::cout << "\n- sleep" << std::endl;
      std::this_thread::sleep_for(2s);

      if (callNotify) {
        std::cout << "\n- set predicate and call notify on t0" << std::endl;
        {
          std::lock_guard lg(readyMutex);
          ready = true;
        } // release lock
        readyCV.notify_one();
        t0.join();

        std::cout << "\n- call notify_one() on other threads" << std::endl;
        for (int idx = 0; idx < numExtraCV; ++idx) {
          {
            std::lock_guard lg(arrReadyMutex[idx]);
            arrReady[idx] = true;
            arrReadyCV[idx].notify_one();
          } // release lock
        }
      }
      else if (callInterrupt) {
        std::cout << "\n- signal interrupt" << std::endl;
        t0.interrupt();
      }
      else {
        // without detach the destructors of the thgreads will block
        // BECAUSE: by replacing the usual interrupt token, we have
        //          disabled ability to signal interrupts via the
        //          default interrupt token originally attached.
        //          Thus, the interrupt signal of the destructor of the
        //          jthread has no effect without any notification.
        for (auto& t : vThreads) {
          if (t.joinable()) {
            t.detach();
          }
        }
      }
      std::cout << "\n- leaving scope of additional threads" << std::endl;
    } // leave scope of additional threads (already notified/interrupted or detached)
    std::cout << "\n- join thread t0 by leaving scope" << std::endl;
  } // leave scope of t0 without join() or detach() (signals cancellation)
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
  testCVStdThreadNoPred(false);  // signal cancellation
  std::cout << "\n\n**************************\n";
  testCVStdThreadNoPred(true);   // call notify()

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

  std::cout << "\n\n**************************\n";
  testManyCV<9>(true, false);  // call notify(), don't call interrupt()
  std::cout << "\n\n**************************\n";
  testManyCV<9>(false, true);  // don't call notify, call interrupt()
  std::cout << "\n\n**************************\n";
  testManyCV<9>(false, false); // don't call notify, don't call interrupt() (implicit interrupt)
 }
 catch (const std::exception& e) {
   std::cerr << "EXCEPTION: " << e.what() << std::endl;
 }
 catch (...) {
   std::cerr << "EXCEPTION" << std::endl;
 }
}

