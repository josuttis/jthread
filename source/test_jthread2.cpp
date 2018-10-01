#include "jthread.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <cassert>
#include <atomic>
using namespace::std::literals;

//------------------------------------------------------

// general helpers:

void printID(const std::string& prefix = "", const std::string& postfix = "")
{
   std::ostringstream os;
   os << '\n' << prefix << ' ' << std::this_thread::get_id() << ' ' << postfix;
   std::cout << os.str() << std::endl;
}

template<typename Duration>
std::string asString(const Duration& dur)
{
  auto durMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
  if (durMilliseconds > 100ms) {
    auto value = std::chrono::duration<double, std::ratio<1>>(dur);
    return std::to_string(value.count()) + "s";
  }
  else {
    auto value = std::chrono::duration<double, std::milli>(dur);
    return std::to_string(value.count()) + "ms";
  }
}

//------------------------------------------------------

void interruptByDestructor()
{
  std::cout << "\n*** start interruptByDestructor(): " << std::endl;
  auto interval = 0.2s;

  bool t1WasInterrupted = false;
  {
   std::cout << "\n- start jthread t1" << std::endl;
   std::jthread t1([interval, &t1WasInterrupted] {
                     printID("t1 STARTED with interval " + asString(interval) + " with id");
                     assert(!std::this_thread::is_interrupted());
                     assert(!std::this_thread::get_interrupt_token().is_interrupted());
                     try {
                       // loop until interrupted (at most 40 times the interval)
                       for (int i=0; i < 40; ++i) {
                          if (std::this_thread::is_interrupted()) {
                            throw "interrupted";
                          }
                          std::this_thread::sleep_for(interval);
                          std::cout.put('.').flush();
                       }
                       assert(false);
                     }
                     catch (std::exception&) { // interrupted not derived from std::exception
                       assert(false);
                     }
                     catch (const char* e) {
                       assert(std::this_thread::is_interrupted());
                       assert(std::this_thread::get_interrupt_token().is_interrupted());
                       t1WasInterrupted = true;
                       //throw;
                     }
                     catch (...) {
                       assert(false);
                     }
                   });
   assert(!t1.get_original_interrupt_token().is_interrupted());

   // call destructor after 4 times the interval (should signal the interrupt)
   std::this_thread::sleep_for(4 * interval);
   assert(!t1.get_original_interrupt_token().is_interrupted());
   std::cout << "\n- destruct jthread t1 (should signal interrupt)" << std::endl;
  }

  // key assertion: signaled interrupt was processed
  assert(t1WasInterrupted);
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void interruptStartedThread()
{
  std::cout << "\n*** start interruptStartedThread(): " << std::endl;
  auto interval = 0.2s;

  {
   std::cout << "\n- start jthread t1" << std::endl;
   bool interrupted = false;
   std::jthread t1([interval, &interrupted] {
                     printID("t1 STARTED with interval " + asString(interval) + " with id");
                     try {
                       // loop until interrupted (at most 40 times the interval)
                       for (int i=0; i < 40; ++i) {
                          if (std::this_thread::is_interrupted()) {
                            throw "interrupted";
                          }
                          std::this_thread::sleep_for(interval);
                          std::cout.put('.').flush();
                       }
                       assert(false);
                     }
                     catch (...) {
                       interrupted = true;
                       //throw;
                     }
                   });

   std::this_thread::sleep_for(4 * interval);
   std::cout << "\n- interrupt jthread t1" << std::endl;
   t1.interrupt();
   assert(t1.get_original_interrupt_token().is_interrupted());
   std::cout << "\n- join jthread t1" << std::endl;
   t1.join();
   assert(interrupted);
   std::cout << "\n- destruct jthread t1" << std::endl;
  }
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void interruptStartedThreadWithSubthread()
{
  std::cout << "\n*** start interruptStartedThreadWithSubthread(): " << std::endl;
  auto interval = 0.2s;
  {
   std::cout << "\n- start jthread t1 with nested jthread t2" << std::endl;
   std::jthread t1([interval] {
                     printID("t1 STARTED with id");
                     std::jthread t2([interval] {
                                       printID("t2 STARTED with id");
                                       while(!std::this_thread::is_interrupted()) {
                                         std::cout.put('2').flush();
                                         std::this_thread::sleep_for(interval/2.3);
                                       }
                                       std::cout << "\nt2 INTERRUPTED" << std::endl;
                                     });
                     while(!std::this_thread::is_interrupted()) {
                        std::cout.put('1').flush();
                        std::this_thread::sleep_for(interval);
                     }
                     std::cout << "\nt1 INTERRUPTED" << std::endl;
                   });

   std::this_thread::sleep_for(4 * interval);
   std::cout << "\n- interrupt jthread t1 (should signal interrupt to t2)" << std::endl;
   t1.interrupt();
   assert(t1.get_original_interrupt_token().is_interrupted());
   std::cout << "\n- join jthread t1" << std::endl;
   t1.join();
   std::cout << "\n- destruct jthread t1" << std::endl;
  }
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void foo(const std::string& msg)
{
    printID(msg);
}

void basicAPIWithFunc()
{
  std::cout << "\n*** start basicAPIWithFunc(): " << std::endl;
  std::interrupt_token it;
  assert(!it.valid());
  {
    std::cout << "\n- start jthread t1" << std::endl;
    std::jthread t(&foo, "foo() called in thread with id: ");
    it = t.get_original_interrupt_token();
    std::cout << it.is_interrupted() << std::endl;
    assert(it.valid());
    std::this_thread::sleep_for(0.5s);
    std::cout << "\n- destruct jthread it" << std::endl;
  }
  assert(it.valid());
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testExchangeToken()
{
  std::cout << "\n*** start testExchangeToken()" << std::endl;
  auto interval = 0.5s;

  // - start thread
  // - interrupt
  // - reuse: assign new token
  // - started thread    
  {
    std::cout << "\n- start jthread t1" << std::endl;
    std::atomic<std::interrupt_token*> itPtr = nullptr;
    std::jthread t1([&itPtr](){
              printID("t1 STARTED (id: ", ") printing . or - or i");
              int numInterrupts = 0;
              try {
                char c = ' ';
                for (int i=0; numInterrupts < 2 && i < 500; ++i) {
		    // if we get a new interrupt token from the caller, take it:
                    if (itPtr.load()) {
                      std::this_thread::exchange_interrupt_token(*itPtr);
                      itPtr.store(nullptr);
                    }
		    // print character according to current interrupt token state:
		    // - '.' if valid and not interrupted,
		    // - 'i' if valid and interrupted
		    // - '-' if no valid
		    if (std::this_thread::is_interrupted()) {
                      if (c != 'i') {
                        c = 'i';
                        ++numInterrupts;
                      }
		    }
		    else {
		      c = std::this_thread::get_interrupt_token().valid() ?
		           '.' : '-';
                    }
                    std::cout.put(c).flush();
                    std::this_thread::sleep_for(0.1ms);
                }
              }
              catch (...) {
                assert(false);
              }
              std::cout << "\nt1 END" << std::endl;
           });

   std::this_thread::sleep_for(interval);
   std::cout << "\n- signal interrupt" << std::endl;
   t1.interrupt();

   std::this_thread::sleep_for(interval);
   std::cout << "\n- replace by invalid token" << std::endl;
   std::interrupt_token it;
   itPtr.store(&it);

   std::this_thread::sleep_for(interval);
   std::cout << "\n- replace by valid token" << std::endl;
   it = std::interrupt_token{false};
   itPtr.store(&it);

   std::this_thread::sleep_for(interval);
   std::cout << "\n- signal interrupt again" << std::endl;
   it.interrupt();

   std::this_thread::sleep_for(interval);
   std::cout << "\n- destruct jthread t1" << std::endl;
  }
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testConcurrentInterrupt()
{
  std::cout << "\n*** start testConcurrentInterrupt()" << std::endl;
  int numThreads = 30;
  std::interrupt_token it{false};
  {
    std::atomic<int> numInterrupts{0};
    std::cout << "\n- start jthread t1" << std::endl;
    std::jthread t1([it](){
              printID("t1 STARTED (id: ", ") printing . or - or i");
              try {
                char c = ' ';
                for (int i=0; !it.is_interrupted(); ++i) {
		    // print character according to current interrupt token state:
		    // - '.' if valid and not interrupted,
		    // - 'i' if valid and interrupted
		    // - '-' if no valid
		    if (std::this_thread::is_interrupted()) {
                      c = 'i';
		    }
		    else {
		      // should never switch back to not interrupted:
                      assert(c != 'i');
		      c = std::this_thread::get_interrupt_token().valid() ?
		           '.' : '-';
                    }
                    std::cout.put(c).flush();
                    std::this_thread::sleep_for(0.1ms);
                }
              }
              catch (...) {
                assert(false);
              }
              std::cout << "\nt1 ENDS" << std::endl;
           });

   std::this_thread::sleep_for(0.5s);

   // starts thread concurrently calling interrupt() for the same token:
   std::cout << "\n- loop over " << numThreads << " threads that interrupt() concurrently" << std::endl;
   std::vector<std::jthread> tv;
   for (int i = 0; i < numThreads; ++i) {
       std::this_thread::sleep_for(0.1ms);
       std::jthread t([&t1] {
                        printID("- interrupting thread started with id:");
                        for (int i = 0; i < 13; ++i) {
                          std::cout.put('x').flush();
                          t1.interrupt();
                          assert(t1.interrupt() == true);
                          std::this_thread::sleep_for(0.01ms);
                        }
                      });
       tv.push_back(std::move(t));
   }
   for (auto& t : tv) {
       t.join();
   }
   std::cout << "\n- signal end" << std::endl;
   it.interrupt();
   std::cout << "\n- destruct jthread t1" << std::endl;
  }
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testEnabledIfForCopyConstructor_CompileTimeOnly()
{
  std::cout << "\n*** start testEnableIfForCopyConstructor_CompileTimeOnly()" << std::endl;
  {
    std::jthread t1;
    //std::jthread t2{t1};  // should not compile
    //std::jthread t2{std::move(t1)};  // should not compile
  }
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

int main()
{
  std::set_terminate([](){
                       std::cout << "ERROR: terminate() called" << std::endl;
                       assert(false);
                     });

  std::cout << "\n**************************\n\n";
  interruptByDestructor();
  std::cout << "\n**************************\n\n";
  interruptStartedThread();
  std::cout << "\n**************************\n\n";
  interruptStartedThreadWithSubthread();
  std::cout << "\n**************************\n\n";
  basicAPIWithFunc();
  std::cout << "\n**************************\n\n";
  testExchangeToken();
  std::cout << "\n**************************\n\n";
  testConcurrentInterrupt();
  //std::cout << "\n**************************\n\n";
  //testEnabledIfForCopyConstructor_CompileTimeOnly();
}

