#include "condition_variable2.hpp"
#include "jthread.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <mutex>
#include <atomic>
#include <cassert>
using namespace::std::literals;

// helper to call iwait() and check some assertions
void cvIWait (int id, bool& ready, std::mutex& readyMutex, std::condition_variable2& readyCV, bool notifyCalled) {
  std::ostringstream strm;
  strm <<"\ncvIWait(" << std::to_string(id) << ") called in thread "
       << std::this_thread::get_id();
  std::cout << strm.str() << std::endl;

  try {
    {
      std::unique_lock lg{readyMutex};
      //std::cout.put(static_cast<char>('0'+id)).flush();
      //readyCV.iwait(lg, [&ready] { return ready; });
      readyCV.wait_until(lg,
                         [&ready] { return ready; },
                         std::this_thread::get_interrupt_token());
      if (std::this_thread::is_interrupted()) {
        throw "interrupted";
      }

    }
    if (std::this_thread::is_interrupted()) {
      std::string msg = "\ncvIWait(" + std::to_string(id) + "): interrupted";
      std::cout << msg << std::endl;
      assert(false);
    }
    else {
      std::string msg = "\ncvIWait(" + std::to_string(id) + "): ready";
      std::cout << msg << std::endl;
    }
    assert(notifyCalled);
  }
  catch (const char* e) {
    std::string msg = "\nINTERRUPT in cvIWait(" + std::to_string(id) + "): " + e;
    std::cerr << msg << std::endl;
    assert(!notifyCalled);
    return;
  }
  catch (const std::exception& e) {
    std::string msg = "\nEXCEPTION in cvIWait(" + std::to_string(id) + "): " + e.what();
    std::cerr << msg << std::endl;
    assert(false);
  }
  catch (...) {
    std::string msg = "\nEXCEPTION in cvIWait(" + std::to_string(id) + ")";
    std::cerr << msg << std::endl;
    assert(false);
  }
}

//------------------------------------------------------

void testStdCV(bool callNotify)
{
  // test the basic jthread API
  std::cout << "*** start testStdCV(callNotify=" << callNotify << ")" << std::endl;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, callNotify] {
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
                      assert(callNotify != std::this_thread::is_interrupted());
                    });
    
    std::this_thread::sleep_for(1s);
    if (callNotify) {
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

void testCVPred(bool callNotify)
{
  // test the basic jthread API
  std::cout << "*** start testCVPred(callNotify=" << callNotify << ")" << std::endl;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, callNotify] {
                      try {
                        std::unique_lock lg{readyMutex};
                        //readyCV.iwait(lg, [&ready] { return ready; });
                        readyCV.wait_until(lg,
                                           [&ready] { return ready; },
                                           std::this_thread::get_interrupt_token());
                        if (std::this_thread::is_interrupted()) {
                          throw "interrupted";
                        }
                        assert(callNotify);
                      }
                      catch (const std::exception&) {  // should be no std::exception
                        assert(false);
                      }
                      catch (const char*) {
                        assert(!callNotify);
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
                      assert(callNotify != std::this_thread::is_interrupted());
                    });
    
    std::this_thread::sleep_for(1s);
    if (callNotify) {
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

void testCVStdThreadNoPred(bool callNotify)
{
  // test the basic jthread API
  std::cout << "*** start testCVStdThreadNoPred(callNotify=" << callNotify << ")" << std::endl;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  
  std::interrupt_token it{false};
  {
    std::thread t1([&ready, &readyMutex, &readyCV, it, callNotify] {
                      {
                        std::unique_lock lg{readyMutex};
                        bool ret = readyCV.wait_until(lg,
                                                      [&ready] { return ready; },
                                                      it);
                        if (ret) {
                            std::cout << "t1: ready" << std::endl;
                            assert(!it.is_interrupted());
                            assert(callNotify);
                        }
                        else if (it.is_interrupted()) {
                            std::cout << "t1: interrupted" << std::endl;
                            assert(!callNotify);
                        }
                      }
                    });
    
    std::this_thread::sleep_for(0.5s);
    assert(!it.is_interrupted());
    std::this_thread::sleep_for(0.5s);
    if (callNotify) {
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

void testCVStdThreadPred(bool callNotify)
{
  // test the basic jthread API
  std::cout << "*** start testCVStdThreadPred(callNotify=" << callNotify << ")" << std::endl;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  
  std::interrupt_token it{false};
  {
    std::thread t1([&ready, &readyMutex, &readyCV, it, callNotify] {
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
                      assert(callNotify != it.is_interrupted());
                    });
    
    std::this_thread::sleep_for(0.5s);
    assert(!it.is_interrupted());
    std::this_thread::sleep_for(0.5s);
    if (callNotify) {
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

void testMinimalWaitFor() 
{
  // test the basic timed CV wait API
  std::cout << "*** start testMinimalWaitFor()" << std::endl;
  auto dur = std::chrono::seconds{5};

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, dur] {
                      std::cout << "\n- start t1" << std::endl;
                      auto t0 = std::chrono::steady_clock::now();
                      {
                        std::unique_lock lg{readyMutex};
                        readyCV.wait_for(lg,
                                         dur,
                                         [&ready] {
                                            return ready;
                                         },
                                         std::this_thread::get_interrupt_token());
                      }
                      assert(std::chrono::steady_clock::now() <  t0 + dur);
                      std::cout << "\n- t1 done" << std::endl;
                    });
    
    std::this_thread::sleep_for(1.5s);
    std::cout << "- leave scope (should signal interrupt and unblock CV wait)" << std::endl;
  } // leave scope of t1 without join() or detach() (signals cancellation)
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

template<typename Dur>
void testTimedCV(bool callNotify, bool callInterrupt, Dur dur)
{
  // test the basic jthread API
  std::cout << "*** start testTimedCV(callNotify=" << callNotify << ", "
            << "callInterrupt=" << callInterrupt << ", "
            << std::chrono::duration<double, std::ratio<1>>(dur).count() << "s)" << std::endl;
  using namespace std::literals;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, callNotify, dur] {
                      std::cout << "\n- start t1" << std::endl;
                      auto t0 = std::chrono::steady_clock::now();
                      int timesDone{0};
                      while (timesDone < 3) {
                        {
                          std::unique_lock lg{readyMutex};
                          //auto end = std::chrono::system_clock::now() + dur;
                          //auto ret = readyCV.wait_until(lg, end,
                          auto ret = readyCV.wait_for(lg, dur,
                                                      [&ready] { return ready; },
                                                      std::this_thread::get_interrupt_token());
                          if (dur > 5s) {
                            assert(std::chrono::steady_clock::now() < t0 + dur);
                          }
                          if (ret) {
                            //std::cout << "t1: ready" << std::endl;
                            std::cout.put('r').flush();
                            assert(ready);
                            assert(!std::this_thread::is_interrupted());
                            assert(callNotify);
                            ++timesDone;
                          }
                          else if (std::this_thread::is_interrupted()) {
                            //std::cout << "t1: interrupted" << std::endl;
                            std::cout.put('i').flush();
                            assert(!ready);
                            assert(!callNotify);
                            ++timesDone;
                          }
                          else {
                            //std::cout << "t1: timeout" << std::endl;
                            std::cout.put('t').flush();
                          }
                        }
                      }
                      std::cout << "\n- t1 done" << std::endl;
                    });
    
    std::this_thread::sleep_for(0.5s);
    assert(!t1.get_original_interrupt_token().is_interrupted());
    std::this_thread::sleep_for(0.5s);
    if (callNotify) {
      std::cout << "\n- set ready" << std::endl;
      {
        std::lock_guard lg(readyMutex);
        ready = true;
      } // release lock
      std::this_thread::sleep_for(1.5s);
      std::cout << "\n- call notify_one()" << std::endl;
      readyCV.notify_one();
    }
    else if (callInterrupt) {
      std::cout << "\n- signal interrupt" << std::endl;
      t1.interrupt();
    }
    else {
      std::cout << "-  interrupt" << std::endl;
      t1.interrupt();
    }
    std::this_thread::sleep_for(1.5s);
    std::cout << "- leave scope (should at latest signal interrupt)" << std::endl;
  } // leave scope of t1 without join() or detach() (signals cancellation)
  std::cout << "\n*** OK" << std::endl;
}


//------------------------------------------------------

template<typename Dur>
void testTimedIWait(bool callNotify, bool callInterrupt, Dur dur)
{
  // test the basic jthread API
  std::cout << "*** start testTimedIWait(callNotify=" << callNotify << ", "
            << "callInterrupt=" << callInterrupt << ", "
            << std::chrono::duration<double, std::ratio<1>>(dur).count() << "s)" << std::endl;
  using namespace std::literals;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable2 readyCV;
  
  enum class State { loop, ready, interrupted };
  State t1Feedback{State::loop};
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, callNotify, dur, &t1Feedback] {
                      std::cout << "\n- start t1" << std::endl;
                      auto t0 = std::chrono::steady_clock::now();
                      int timesDone{0};
                      while (timesDone < 3) {
                        try {
                          std::unique_lock lg{readyMutex};
                          //auto end = std::chrono::system_clock::now() + dur;
                          //auto ret = readyCV.wait_until(lg, end,
                          auto ret = readyCV.wait_for(lg, dur,
                                                      [&ready] { return ready; },
                                                      std::this_thread::get_interrupt_token());
                          if (std::this_thread::is_interrupted()) {
                            throw "interrupted";
                          }
                          if (dur > 5s) {
                            assert(std::chrono::steady_clock::now() < t0 + dur);
                          }
                          if (ret) {
                            //std::cout << "t1: ready" << std::endl;
                            std::cout.put('r').flush();
                            t1Feedback = State::ready;
                            assert(ready);
                            assert(!std::this_thread::is_interrupted());
                            assert(callNotify);
                            ++timesDone;
                          }
                          else {
                            // note: might also be interrupted already!
                            //std::cout << "t1: timeout" << std::endl;
                            std::cout.put(std::this_thread::is_interrupted() ? 'T' : 't').flush();
                          }
                        }
                        catch (const char* e) {
                          //std::cout << "t1: interrupted" << std::endl;
                          std::cout.put('i').flush();
                          t1Feedback = State::interrupted;
                          assert(!ready);
                          assert(!callNotify);
                          ++timesDone;
                          if (timesDone >= 3) {
                            std::cout << "\n- t1 done" << std::endl;
                            return;
                          }
                        }
                        catch (const std::exception& e) {
                          std::string msg = std::string("\nEXCEPTION: ") + e.what();
                          std::cerr << msg << std::endl;
                          assert(false);
                        }
                        catch (...) {
                          std::cerr << "\nEXCEPTION" << std::endl;
                          assert(false);
                        }
                      }
                      std::cout << "\n- t1 done" << std::endl;
                    });
    
    std::this_thread::sleep_for(0.5s);
    assert(!t1.get_original_interrupt_token().is_interrupted());
    std::this_thread::sleep_for(0.5s);
    if (callNotify) {
      std::cout << "\n- set ready" << std::endl;
      {
        std::lock_guard lg(readyMutex);
        ready = true;
      } // release lock
      std::this_thread::sleep_for(1.5s);
      std::cout << "\n- call notify_one()" << std::endl;
      auto t0 = std::chrono::steady_clock::now();
      readyCV.notify_one();
      while (t1Feedback != State::ready) {
        std::this_thread::sleep_for(200ms);
        assert(std::chrono::steady_clock::now() < t0 + 5s);
      }
    }
    else if (callInterrupt) {
      std::cout << "\n- signal interrupt" << std::endl;
      auto t0 = std::chrono::steady_clock::now();
      t1.interrupt();
      while (t1Feedback != State::interrupted) {
        std::this_thread::sleep_for(200ms);
        assert(std::chrono::steady_clock::now() < t0 + 5s);
      }
    }
    else {
      std::cout << "- let destructor signal interrupt" << std::endl;
    }
    std::this_thread::sleep_for(1.5s);
    std::cout << "- leave scope (should at latest signal interrupt)" << std::endl;
  } // leave scope of t1 without join() or detach() (signals cancellation)
  auto t0 = std::chrono::steady_clock::now();
  while (t1Feedback == State::loop) {
        std::this_thread::sleep_for(100ms);
  }
  assert(std::chrono::steady_clock::now() < t0 + 5s);
  std::cout << "\n*** OK" << std::endl;
}


//------------------------------------------------------

template <int numExtraCV>
void testManyCV(bool callNotify, bool callInterrupt)
{
  // test interrupt tokens interrupting multiple CVs
  std::cout << "*** start testManyCV(callNotify=" << callNotify << ", "
            << "callInterrupt=" << callInterrupt << ", "
            << "numExtraCV=" << numExtraCV << ")" << std::endl;

  std::interrupt_token t0itoken;
  {
    // thread t0 with CV:
    bool ready = false;
    std::mutex readyMutex;
    std::condition_variable2 readyCV;
    std::jthread t0(cvIWait, 0, std::ref(ready), std::ref(readyMutex), std::ref(readyCV), callNotify);
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

                         cvIWait(idx+1, arrReady[idx], arrReadyMutex[idx], arrReadyCV[idx], callNotify);
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
  testMinimalWaitFor();

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
  testCVStdThreadPred(false);  // signal cancellation
  std::cout << "\n\n**************************\n";
  testCVStdThreadPred(true);   // call notify()

  std::cout << "\n\n**************************\n";
  testTimedCV(true, false, 200ms);  // call notify(), don't call interrupt()
  std::cout << "\n\n**************************\n";
  testTimedCV(false, true, 200ms);   // don't call notify, call interrupt()
  std::cout << "\n\n**************************\n";
  testTimedCV(false, false, 200ms);   // don't call notify, don't call interrupt()
  std::cout << "\n\n**************************\n";
  testTimedCV(true, false, 60s);  // call notify(), don't call interrupt()
  std::cout << "\n\n**************************\n";
  testTimedCV(false, true, 60s);   // don't call notify, call interrupt()
  std::cout << "\n\n**************************\n";
  testTimedCV(false, false, 60s);   // don't call notify, don't call interrupt()

  std::cout << "\n\n**************************\n";
  testTimedIWait(true, false, 200ms);  // call notify(), don't call interrupt()
  std::cout << "\n\n**************************\n";
  testTimedIWait(false, true, 200ms);   // don't call notify, call interrupt()
  std::cout << "\n\n**************************\n";
  testTimedIWait(false, false, 200ms);   // don't call notify, don't call interrupt()
  std::cout << "\n\n**************************\n";
  testTimedIWait(true, false, 60s);  // call notify(), don't call interrupt()
  std::cout << "\n\n**************************\n";
  testTimedIWait(false, true, 60s);   // don't call notify, call interrupt()
  std::cout << "\n\n**************************\n";
  testTimedIWait(false, false, 60s);   // don't call notify, don't call interrupt()

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

