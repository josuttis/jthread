#include "condition_variable_any2.hpp"
#include "jthread.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <mutex>
#include <atomic>
#include <cassert>
#include <vector>
#include <array>
using namespace::std::literals;

// helper to call iwait() and check some assertions
void cvIWait (std::stop_token sToken, int id,
              bool& ready, std::mutex& readyMutex, std::condition_variable_any2& readyCV,
              bool notifyCalled) {
  std::ostringstream strm;
  strm <<"\ncvIWait(" << std::to_string(id) << ") called in thread "
       << std::this_thread::get_id();
  std::cout << strm.str() << std::endl;

  try {
    {
      std::unique_lock lg{readyMutex};
      //std::cout.put(static_cast<char>('0'+id)).flush();
      readyCV.wait(lg,
                   sToken,
                   [&ready] { return ready; });
      if (sToken.stop_requested()) {
        throw "interrupted";
      }

    }
    if (sToken.stop_requested()) {
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
  std::string msg = "\nEND cvIWait(" + std::to_string(id) + ") ";
  std::cerr << msg << std::endl;
}

//------------------------------------------------------

void testStdCV(bool callNotify)
{
  // test the basic jthread API
  std::cout << "*** start testStdCV(callNotify=" << callNotify << ")" << std::endl;

  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable_any2 readyCV;
  
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, callNotify] (std::stop_token it) {
                      {
                        std::unique_lock lg{readyMutex};
                        while (!it.stop_requested() && !ready) {
                          readyCV.wait_for(lg, 100ms);
                          std::cout.put('.').flush();
                        }
                      }
                      if (it.stop_requested()) {
                        std::cout << "t1: interrupted" << std::endl;
                      }
                      else {
                        std::cout << "t1: ready" << std::endl;
                      }
                      assert(callNotify != it.stop_requested());
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
  std::condition_variable_any2 readyCV;
  
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, callNotify] (std::stop_token st) {
                      try {
                        std::unique_lock lg{readyMutex};
                        readyCV.wait(lg,
                                     st,
                                     [&ready] { return ready; });
                        if (st.stop_requested()) {
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
                      if (st.stop_requested()) {
                        std::cout << "t1: interrupted" << std::endl;
                      }
                      else {
                        std::cout << "t1: ready" << std::endl;
                      }
                      assert(callNotify != st.stop_requested());
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
  std::condition_variable_any2 readyCV;
  
  std::stop_source is;
  {
    std::thread t1([&ready, &readyMutex, &readyCV, st=is.get_token(), callNotify] {
                      {
                        std::unique_lock lg{readyMutex};
                        bool ret = readyCV.wait(lg,
                                                st,
                                                [&ready] { return ready; });
                        if (ret) {
                            std::cout << "t1: ready" << std::endl;
                            assert(!st.stop_requested());
                            assert(callNotify);
                        }
                        else if (st.stop_requested()) {
                            std::cout << "t1: interrupted" << std::endl;
                            assert(!callNotify);
                        }
                      }
                    });
    
    std::this_thread::sleep_for(0.5s);
    assert(!is.stop_requested());
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
      is.request_stop();
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
  std::condition_variable_any2 readyCV;
  
  std::stop_source is;
  {
    std::thread t1([&ready, &readyMutex, &readyCV, st=is.get_token(), callNotify] {
                      bool ret;
                      {
                        std::unique_lock lg{readyMutex};
                        ret = readyCV.wait(lg,
                                           st,
                                           [&ready] { return ready; });
                        if (ret) {
                          std::cout << "t1: ready" << std::endl;
                          assert(!st.stop_requested());
                        }
                        else {
                          std::cout << "t1: interrupted" << std::endl;
                          assert(st.stop_requested());
                        }
                      }
                      assert(callNotify != st.stop_requested());
                    });
    
    std::this_thread::sleep_for(0.5s);
    assert(!is.stop_requested());
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
      is.request_stop();
    }
    t1.join();
  } // leave scope of t1 without join() or detach() (signals cancellation)
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testMinimalWait(int sec)
{
  // test the basic CV wait API
  std::cout << "*** start testMinimalWait(" << sec << "s)" << std::endl;
  auto dur = std::chrono::seconds{sec};   // duration until interrupt is called

  try {
    bool ready = false;
    std::mutex readyMutex;
    std::condition_variable_any2 readyCV;
    {
      std::jthread t1([&ready, &readyMutex, &readyCV, dur] (std::stop_token st) {
                        try {
                          std::cout << "\n- start t1" << std::endl;
                          auto t0 = std::chrono::steady_clock::now();
                          {
                            std::unique_lock lg{readyMutex};
                            readyCV.wait(lg,
                                         st,
                                         [&ready] { return ready; });
                          }
                          assert(std::chrono::steady_clock::now() <  t0 + dur + 1s);
                          std::cout << "\n- t1 done" << std::endl;
                        }
                        catch(const std::exception& e) {
                          std::cout << "THREAD EXCEPTION: " << e.what() << '\n';
                        }
                        catch(...) {
                          std::cout << "THREAD EXCEPTION\n";
                        }
                      });
      
      std::this_thread::sleep_for(dur);
      std::cout << "- leave scope (should signal interrupt and unblock CV wait)" << std::endl;
    } // leave scope of t1 without join() or detach() (signals cancellation)
    std::cout << "\n*** OK" << std::endl;
  }
  catch(const std::exception& e) {
    std::cout << "MAIN EXCEPTION: " << e.what() << '\n';
  }
  catch(...) {
    std::cout << "MAIN EXCEPTION\n";
  }
}

//------------------------------------------------------

void testMinimalWaitFor(int sec1, int sec2) 
{
  // test the basic timed CV wait API
  std::cout << "*** start testMinimalWaitFor(interruptAfter=" << sec1
                                      << "s, waitfor=" << sec2 << "s)" << std::endl;
  auto durInt = std::chrono::seconds{sec1};
  auto durWait = std::chrono::seconds{sec2};

  try {
  bool ready = false;
  std::mutex readyMutex;
  std::condition_variable_any2 readyCV;
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, durInt, durWait] (std::stop_token st) {
                      try {
                      std::cout << "\n- start t1" << std::endl;
                      auto t0 = std::chrono::steady_clock::now();
                      {
                        std::unique_lock lg{readyMutex};
                        readyCV.wait_for(lg,
                                         st,
                                         durWait,
                                         [&ready] {
                                            return ready;
                                         });
                      }
                      assert(std::chrono::steady_clock::now() <  t0 + durInt + 1s);
                      assert(std::chrono::steady_clock::now() <  t0 + durWait + 1s);
                      std::cout << "\n- t1 done" << std::endl;
                      }
                      catch(const std::exception& e) {
                        std::cout << "THREAD EXCEPTION: " << e.what() << '\n';
                      }
                      catch(...) {
                        std::cout << "THREAD EXCEPTION\n";
                      }
                    });
    
    std::this_thread::sleep_for(durInt);
    std::cout << "- leave scope (should signal interrupt and unblock CV wait)" << std::endl;
  } // leave scope of t1 without join() or detach() (signals cancellation)
  std::cout << "\n*** OK" << std::endl;
  }
  catch(const std::exception& e) {
    std::cout << "MAIN EXCEPTION: " << e.what() << '\n';
  }
  catch(...) {
    std::cout << "MAIN EXCEPTION\n";
  }
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
  std::condition_variable_any2 readyCV;
  
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, callNotify, dur] (std::stop_token st) {
                      std::cout << "\n- start t1" << std::endl;
                      auto t0 = std::chrono::steady_clock::now();
                      int timesDone{0};
                      while (timesDone < 3) {
                        {
                          std::unique_lock lg{readyMutex};
                          auto ret = readyCV.wait_for(lg,
                                                      st,
                                                      dur,
                                                      [&ready] { return ready; });
                          if (dur > 5s) {
                            assert(std::chrono::steady_clock::now() < t0 + dur);
                          }
                          if (ret) {
                            //std::cout << "t1: ready" << std::endl;
                            std::cout.put('r').flush();
                            assert(ready);
                            assert(!st.stop_requested());
                            assert(callNotify);
                            ++timesDone;
                          }
                          else if (st.stop_requested()) {
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
    assert(!t1.get_stop_source().stop_requested());
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
      t1.request_stop();
    }
    else {
      std::cout << "-  interrupt" << std::endl;
      t1.request_stop();
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
  std::condition_variable_any2 readyCV;
  
  enum class State { loop, ready, interrupted };
  State t1Feedback{State::loop};
  {
    std::jthread t1([&ready, &readyMutex, &readyCV, callNotify, dur, &t1Feedback] (std::stop_token st) {
                      std::cout << "\n- start t1" << std::endl;
                      auto t0 = std::chrono::steady_clock::now();
                      int timesDone{0};
                      while (timesDone < 3) {
                        try {
                          std::unique_lock lg{readyMutex};
                          auto ret = readyCV.wait_for(lg,
                                                      st,
                                                      dur,
                                                      [&ready] { return ready; });
                          if (st.stop_requested()) {
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
                            assert(!st.stop_requested());
                            assert(callNotify);
                            ++timesDone;
                          }
                          else {
                            // note: might also be interrupted already!
                            //std::cout << "t1: timeout" << std::endl;
                            std::cout.put(st.stop_requested() ? 'T' : 't').flush();
                          }
                        }
                        catch (const char*) {
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
    assert(!t1.get_stop_source().stop_requested());
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
      t1.request_stop();
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

  {
    // thread t0 with CV:
    bool ready = false;
    std::mutex readyMutex;
    std::condition_variable_any2 readyCV;

    std::array<bool,numExtraCV> arrReady{};  // don't forget to initialize with {} here !!!
    std::array<std::mutex,numExtraCV> arrReadyMutex{};
    std::array<std::condition_variable_any2,numExtraCV> arrReadyCV{};
    std::vector<std::jthread> vThreadsDeferred;

    std::jthread t0(cvIWait, 0,
                             std::ref(ready), std::ref(readyMutex), std::ref(readyCV),
                             callNotify);
    {  
      auto t0ssource = t0.get_stop_source();
      std::this_thread::sleep_for(0.5s);

      // starts thread concurrently calling request_stop() for the same token:
      std::cout << "\n- loop to start " << numExtraCV << " threads sharing the token and waiting concurently" << std::endl;

      std::vector<std::jthread> vThreads;
      for (int idx = 0; idx < numExtraCV; ++idx) {
        std::this_thread::sleep_for(0.1ms);
        std::jthread t([idx, t0stoken=t0ssource.get_token(), &arrReady, &arrReadyMutex, &arrReadyCV, callNotify] {
                         // use interrupt token of t0 instead
                         // NOTE: disables signaling interrupts directly to the thread
                         cvIWait(t0stoken, idx+1,
                                 arrReady[idx], arrReadyMutex[idx], arrReadyCV[idx],
                                 callNotify);
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
        t0.request_stop();
      }
      else {
        // Move ownership of the threads to a scope that will
        // destruct after thread t0 has destructed (and thus
        // signalled cancellation and joined) but before the
        // condition_variable/mutex/ready-flag objects that
        // they reference have been destroyed.
        vThreadsDeferred = std::move(vThreads);
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
  testMinimalWait(0);
  std::cout << "\n\n**************************\n";
  testMinimalWait(1);
  std::cout << "\n\n**************************\n";
  testMinimalWaitFor(0, 0);
  std::cout << "\n\n**************************\n";
  testMinimalWaitFor(0, 2);  // 0s for interrupt, 2s for wait
  std::cout << "\n\n**************************\n";
  testMinimalWaitFor(2, 0);  // 2s for interrupt, 0s for wait
  std::cout << "\n\n**************************\n";
  testMinimalWaitFor(1, 3);  // 1s for interrupt, 3s for wait
  std::cout << "\n\n**************************\n";
  testMinimalWaitFor(3, 1);  // 3s for interrupt, 1s for wait

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
  testTimedCV(true, false, 200ms);  // call notify(), don't call request_stop()
  std::cout << "\n\n**************************\n";
  testTimedCV(false, true, 200ms);   // don't call notify, call request_stop()
  std::cout << "\n\n**************************\n";
  testTimedCV(false, false, 200ms);   // don't call notify, don't call request_stop()
  std::cout << "\n\n**************************\n";
  testTimedCV(true, false, 60s);  // call notify(), don't call request_stop()
  std::cout << "\n\n**************************\n";
  testTimedCV(false, true, 60s);   // don't call notify, call request_stop()
  std::cout << "\n\n**************************\n";
  testTimedCV(false, false, 60s);   // don't call notify, don't call request_stop()

  std::cout << "\n\n**************************\n";
  testTimedIWait(true, false, 200ms);  // call notify(), don't call request_stop()
  std::cout << "\n\n**************************\n";
  testTimedIWait(false, true, 200ms);   // don't call notify, call request_stop()
  std::cout << "\n\n**************************\n";
  testTimedIWait(false, false, 200ms);   // don't call notify, don't call request_stop()
  std::cout << "\n\n**************************\n";
  testTimedIWait(true, false, 60s);  // call notify(), don't call request_stop()
  std::cout << "\n\n**************************\n";
  testTimedIWait(false, true, 60s);   // don't call notify, call request_stop()
  std::cout << "\n\n**************************\n";
  testTimedIWait(false, false, 60s);   // don't call notify, don't call request_stop()

  std::cout << "\n\n**************************\n";
  testManyCV<9>(true, false);  // call notify(), don't call request_stop()
  std::cout << "\n\n**************************\n";
  testManyCV<9>(false, true);  // don't call notify, call request_stop()
  std::cout << "\n\n**************************\n";
  testManyCV<9>(false, false); // don't call notify, don't call request_stop() (implicit interrupt)
 }
 catch (const std::exception& e) {
   std::cerr << "EXCEPTION: " << e.what() << std::endl;
 }
 catch (...) {
   std::cerr << "EXCEPTION" << std::endl;
 }
}

