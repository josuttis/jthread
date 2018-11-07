#include "jthread.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <cassert>
#include <atomic>
using namespace::std::literals;

//------------------------------------------------------

void testJThreadWithout()
{
  // test the basic jthread API (not taking interrupt_token arg)
  std::cout << "*** start testJThreadWithout()" << std::endl;

  assert(std::jthread::hardware_concurrency() == std::thread::hardware_concurrency()); 
  std::interrupt_token itoken;
  assert(!itoken.is_interruptible());
  {
    std::jthread::id t1ID{std::this_thread::get_id()};
    std::atomic<bool> t1AllSet{false};
    std::jthread t1([&t1ID, &t1AllSet] {
                   // check some values of the started thread:
                   t1ID = std::this_thread::get_id();
                   t1AllSet.store(true);
                   // wait until loop is done (no interrupt checked)
                   for (int c='9'; c>='0'; --c) {
                      std::this_thread::sleep_for(222ms);
                      std::cout.put(static_cast<char>(c)).flush();
                   }
                   std::cout << "END t1" << std::endl;
                 });
    // wait until t1 has set all initial values:
    for (int i=0; !t1AllSet.load(); ++i) {
      std::this_thread::sleep_for(10ms);
      std::cout.put('o').flush();
    }
    // and check all values:
    assert(t1.joinable());
    assert(t1ID == t1.get_id());
    itoken = t1.get_original_interrupt_source().get_token();
    assert(!itoken.is_interrupted());
  } // leave scope of t1 without join() or detach() (signals cancellation)
  assert(itoken.is_interrupted());
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testThreadWithToken()
{
  // test the basic thread API (taking interrupt_token arg)
  std::cout << "*** start testThreadWithToken()" << std::endl;

  std::interrupt_source isource;
  std::interrupt_source origsource;
  assert(isource.is_interruptible());
  assert(!isource.is_interrupted());
  {
    std::jthread::id t1ID{std::this_thread::get_id()};
    std::atomic<bool> t1AllSet{false};
    std::atomic<bool> t1done{false};
    std::jthread t1([&t1ID, &t1AllSet, &t1done] (std::interrupt_token it) {
                       // check some values of the started thread:
                       t1ID = std::this_thread::get_id();
                       t1AllSet.store(true);
                       // wait until interrupt is signaled (due to destructor of t1):
                       for (int i=0; !it.is_interrupted(); ++i) {
                          std::this_thread::sleep_for(100ms);
                          std::cout.put('.').flush();
                       }
                       t1done.store(true);
                       std::cout << "END t1" << std::endl;
                     },
                     isource.get_token());
    // wait until t1 has set all initial values:
    for (int i=0; !t1AllSet.load(); ++i) {
      std::this_thread::sleep_for(10ms);
      std::cout.put('o').flush();
    }
    // and check all values:
    assert(t1.joinable());
    assert(t1ID == t1.get_id());

    std::this_thread::sleep_for(470ms);
    origsource = std::move(isource);
    isource = t1.get_original_interrupt_source();
    assert(!isource.is_interrupted());
    auto ret = isource.interrupt();
    assert(!ret);
    ret = isource.interrupt();
    assert(ret);
    assert(isource.is_interrupted());
    assert(!t1done.load());
    assert(!origsource.is_interrupted());

    std::this_thread::sleep_for(470ms);
    origsource.interrupt();
  } // leave scope of t1 without join() or detach() (signals cancellation)
  assert(origsource.is_interrupted());
  assert(isource.is_interrupted());
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testJoin()
{
  // test jthread join()
  std::cout << "\n*** start testJoin()" << std::endl;

  std::interrupt_source isource;
  assert(isource.is_interruptible());
  {
    std::jthread t1([](std::interrupt_token itoken) {
                      // wait until interrupt is signaled (due to calling interrupt() for the token):
                      for (int i=0; !itoken.is_interrupted(); ++i) {
                         std::this_thread::sleep_for(100ms);
                         std::cout.put('.').flush();
                      }
                      std::cout << "END t1" << std::endl;
                 });
    isource = t1.get_original_interrupt_source();
    // let nother thread signal cancellation after some time:
    std::jthread t2([isource] () mutable {
                     for (int i=0; i < 10; ++i) {
                       std::this_thread::sleep_for(70ms);
                       std::cout.put('x').flush();
                     }
                     // signal interrupt to other thread:
                     isource.interrupt();
                     std::cout << "END t2" << std::endl;
                   });
    // wait for all thread to finish:
    t2.join();
    assert(!t2.joinable());
    assert(t1.joinable());
    t1.join();
    assert(!t1.joinable());
  } // leave scope of t1 without join() or detach() (signals cancellation)
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testDetach()
{
  // test jthread detach()
  std::cout << "\n*** start testDetach()" << std::endl;

  std::interrupt_source isource;
  assert(isource.is_interruptible());
  std::atomic<bool> t1FinallyInterrupted{false};
  {
    std::jthread t0;
    std::jthread::id t1ID{std::this_thread::get_id()};
    bool t1IsInterrupted;
    std::interrupt_token t1InterruptToken;
    std::atomic<bool> t1AllSet{false};
    std::jthread t1([&t1ID, &t1IsInterrupted, &t1InterruptToken, &t1AllSet, &t1FinallyInterrupted]
                    (std::interrupt_token itoken) {
                   // check some values of the started thread:
                   t1ID = std::this_thread::get_id();
                   t1InterruptToken = itoken;
                   t1IsInterrupted = itoken.is_interrupted();
                   assert(itoken.is_interruptible());
                   assert(!itoken.is_interrupted());
                   t1AllSet.store(true);
                   // wait until interrupt is signaled (due to calling interrupt() for the token):
                   for (int i=0; !itoken.is_interrupted(); ++i) {
                      std::this_thread::sleep_for(100ms);
                      std::cout.put('.').flush();
                   }
                   t1FinallyInterrupted.store(true);
                   std::cout << "END t1" << std::endl;
                 });
    // wait until t1 has set all initial values:
    for (int i=0; !t1AllSet.load(); ++i) {
      std::this_thread::sleep_for(10ms);
      std::cout.put('o').flush();
    }
    // and check all values:
    assert(!t0.joinable());
    //assert(std::interrupt_source{} == t0.get_original_interrupt_source());
    assert(t1.joinable());
    assert(t1ID == t1.get_id());
    assert(t1IsInterrupted == false);
    assert(t1InterruptToken == t1.get_original_interrupt_source().get_token());
    isource = t1.get_original_interrupt_source();
    assert(t1InterruptToken.is_interruptible());
    assert(!t1InterruptToken.is_interrupted());
    // test detach():
    t1.detach();
    assert(!t1.joinable());
  } // leave scope of t1 without join() or detach() (signals cancellation)

  // finally signal cancellation:
  assert(!t1FinallyInterrupted.load());
  isource.interrupt();
  // and check consequences:
  assert(isource.is_interrupted());
  for (int i=0; !t1FinallyInterrupted.load() && i < 100; ++i) {
    std::this_thread::sleep_for(100ms);
    std::cout.put('o').flush();
  }
  assert(t1FinallyInterrupted.load());
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testStdThread()
{
  // test the extended std::thread API
  std::cout << "\n*** start testStdThread()" << std::endl;
  std::thread t0;
  std::thread::id t1ID{std::this_thread::get_id()};
  std::atomic<bool> t1AllSet{false};
  std::interrupt_source t1ShallDie;
  std::thread t1([&t1ID, &t1AllSet, t1ShallDie = t1ShallDie.get_token()] {
                   // check some supplementary values of the started thread:
                   t1ID = std::this_thread::get_id();
                   t1AllSet.store(true);
                   // and wait until cancellation is signaled via passed token:
                   try {
                     for (int i=0; ; ++i) {
                        if (t1ShallDie.is_interrupted()) {
                          throw "interrupted";
                        }
                        std::this_thread::sleep_for(100ms);
                        std::cout.put('.').flush();
                     }
                     assert(false);
                   }
                   catch (std::exception&) { // interrupted not derived from std::exception
                     assert(false);
                   }
                   catch (const char* e) {
                     std::cout << e << std::endl;
                   }
                   catch (...) {
                     assert(false);
                   }
                   assert(t1ShallDie.is_interrupted());
                   std::cout << "END t1" << std::endl;
                 });
  // wait until t1 has set all initial values:
  for (int i=0; !t1AllSet.load(); ++i) {
    std::this_thread::sleep_for(10ms);
    std::cout.put('o').flush();
  }
  // and check all values:
  assert(t1ID == t1.get_id());
  // signal cancellation via manually installed interrupt token:
  t1ShallDie.interrupt();
  t1.join();
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testTemporarilyDisableToken()
{
  // test exchanging the token to disable it temporarily
  std::cout << "*** start testTemporarilyDisableToken()" << std::endl;

  enum class State { init, loop, disabled, restored, interrupted };
  std::atomic<State> state{State::init};
  std::interrupt_source t1is;
  {
    std::jthread t1([&state] (std::interrupt_token itoken) {
                   std::cout << "- start t1" << std::endl;
                   auto actToken = itoken;
                   // just loop (no interrupt should occur):
                   state.store(State::loop); 
                   try {
                     for (int i=0; i < 10; ++i) {
                        if (actToken.is_interrupted()) {
                          throw "interrupted";
                        }
                        std::this_thread::sleep_for(100ms);
                        std::cout.put('.').flush();
                     }
                   }
                   catch (...) {
                     assert(false);
                   }
                   // temporarily disable interrupts:
                   std::interrupt_token interruptDisabled;
                   swap(itoken, interruptDisabled);
                   state.store(State::disabled); 
                   // loop again until interrupt signaled to original interrupt token:
                   try {
                     while (!actToken.is_interrupted()) {
                        if (itoken.is_interrupted()) {
                          throw "interrupted";
                        }
                        std::this_thread::sleep_for(100ms);
                        std::cout.put('d').flush();
                     }
                     for (int i=0; i < 10; ++i) {
                        std::this_thread::sleep_for(100ms);
                        std::cout.put('D').flush();
                     }
                   }
                   catch (...) {
                     assert(false);
                   }
                   state.store(State::restored); 
                   // enable interrupts again:
                   swap(itoken, interruptDisabled);
                   // loop again (should immediately throw):
                   assert(!interruptDisabled.is_interrupted());
                   try {
                     if (actToken.is_interrupted()) {
                       throw "interrupted";
                     }
                   }
                   catch (const char* e) {
                     std::cout.put('i').flush();
                     state.store(State::interrupted); 
                   }
                   std::cout << "\n- end t1" << std::endl;
                 });
    while (state.load() != State::disabled) {
      std::this_thread::sleep_for(100ms);
      std::cout.put('m').flush();
    }
    std::this_thread::sleep_for(500ms);
    std::cout << "\n- leave scope (should interrupt started thread)" << std::endl;
    t1is = t1.get_original_interrupt_source();
  } // leave scope of t1 without join() or detach() (signals cancellation)
  assert(t1is.is_interrupted());
  assert(state.load() == State::interrupted);
  std::cout << "\n*** OK" << std::endl;
}


//------------------------------------------------------

void testJThreadAPI()
{
  // test the basic jthread API (taking interrupt_token arg)
  std::cout << "*** start testJThreadAPI()" << std::endl;

  assert(std::jthread::hardware_concurrency() == std::thread::hardware_concurrency()); 
  std::interrupt_source isource;
  assert(isource.is_interruptible());
  assert(isource.get_token().is_interruptible());
  std::interrupt_token itoken;
  assert(!itoken.is_interruptible());

  // thread with no callable and invalid source:
  std::jthread t0;
  std::jthread::native_handle_type nh = t0.native_handle();
  assert((std::is_same_v<decltype(nh), std::thread::native_handle_type>)); 
  assert(!t0.joinable());
  std::interrupt_source isourceStolen{std::move(isource)};
  assert(!isource.is_interruptible());
  assert(isource == t0.get_original_interrupt_source());

  {
    std::jthread::id t1ID{std::this_thread::get_id()};
    std::interrupt_token t1InterruptToken;
    std::atomic<bool> t1AllSet{false};
    std::jthread t1([&t1ID, &t1InterruptToken, &t1AllSet] (std::interrupt_token itoken) {
                   // check some values of the started thread:
                   t1ID = std::this_thread::get_id();
                   t1InterruptToken = itoken;
                   assert(itoken.is_interruptible());
                   assert(!itoken.is_interrupted());
                   t1AllSet.store(true);
                   // wait until interrupt is signaled (due to destructor of t1):
                   for (int i=0; !itoken.is_interrupted(); ++i) {
                      std::this_thread::sleep_for(100ms);
                      std::cout.put('.').flush();
                   }
                   std::cout << "END t1" << std::endl;
                 });
    // wait until t1 has set all initial values:
    for (int i=0; !t1AllSet.load(); ++i) {
      std::this_thread::sleep_for(10ms);
      std::cout.put('o').flush();
    }
    // and check all values:
    assert(t1.joinable());
    assert(t1ID == t1.get_id());
    assert(t1InterruptToken == t1.get_original_interrupt_source().get_token());
    itoken = t1.get_original_interrupt_source().get_token();
    assert(t1InterruptToken.is_interruptible());
    assert(!t1InterruptToken.is_interrupted());
    // test swap():
    std::swap(t0, t1);
    assert(!t1.joinable());
    assert(std::interrupt_token{} == t1.get_original_interrupt_source().get_token());
    assert(t0.joinable());
    assert(t1ID == t0.get_id());
    assert(t1InterruptToken == t0.get_original_interrupt_source().get_token());
    // manual swap with move():
    auto ttmp{std::move(t0)};
    t0 = std::move(t1);
    t1 = std::move(ttmp);
    assert(!t0.joinable());
    assert(std::interrupt_token{} == t0.get_original_interrupt_source().get_token());
    assert(t1.joinable());
    assert(t1ID == t1.get_id());
    assert(t1InterruptToken == t1.get_original_interrupt_source().get_token());
  } // leave scope of t1 without join() or detach() (signals cancellation)
  assert(itoken.is_interrupted());
  std::cout << "\n*** OK" << std::endl;
}


//------------------------------------------------------
//------------------------------------------------------

int main()
{
  std::set_terminate([](){
                       std::cout << "ERROR: terminate() called" << std::endl;
                       assert(false);
                     });

  std::cout << "\n\n**************************\n";
  testJThreadWithout();
  std::cout << "\n\n**************************\n";
  testThreadWithToken();
  std::cout << "\n\n**************************\n";
  testJoin();
  std::cout << "\n\n**************************\n";
  testDetach();
  std::cout << "\n\n**************************\n";
  testStdThread();
  std::cout << "\n\n**************************\n";
  testTemporarilyDisableToken();
  std::cout << "\n\n**************************\n";
  testJThreadAPI();
  std::cout << "\n\n**************************\n";
}

