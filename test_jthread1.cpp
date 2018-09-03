#include "jthread.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <cassert>
#include <atomic>
using namespace::std::literals;

//------------------------------------------------------

void testJThread()
{
  // test the basic jthread API
  std::cout << "\n*** start testJThread()" << std::endl;

  assert(std::jthread::hardware_concurrency() == std::thread::hardware_concurrency()); 
  std::interrupt_token itoken;
  assert(!itoken.valid());
  {
    std::jthread t0;
    std::jthread::native_handle_type nh = t0.native_handle();
    assert((std::is_same_v<decltype(nh), std::thread::native_handle_type>)); 

    std::jthread::id t1ID{std::this_thread::get_id()};
    bool t1IsInterrupted{true};
    std::interrupt_token t1InterruptToken;
    std::atomic<bool> t1AllSet{false};
    std::jthread t1([&t1ID, &t1IsInterrupted, &t1InterruptToken, &t1AllSet] {
                   // check some values of the started thread:
                   t1ID = std::this_thread::get_id();
                   t1IsInterrupted = std::this_thread::is_interrupted();
                   t1InterruptToken = std::this_thread::get_interrupt_token();
                   assert(std::this_thread::get_interrupt_token().valid());
                   assert(!std::this_thread::get_interrupt_token().is_interrupted());
                   try {
                     std::this_thread::throw_if_interrupted();
                   }
                   catch (...) {
                     assert(false);
                   }
                   t1AllSet.store(true);
                   // wait until interrupt is signaled (due to destructor of t1):
                   for (int i=0; !std::this_thread::is_interrupted(); ++i) {
                      std::this_thread::sleep_for(100ms);
                      std::cout.put('.').flush();
                   }
                   // test itoken setter:
                   std::interrupt_token it1;
                   std::this_thread::exchange_interrupt_token(it1);
                   assert(!std::this_thread::get_interrupt_token().valid());
                   assert(!std::this_thread::get_interrupt_token().is_interrupted());
                   std::interrupt_token it2{false};
                   std::this_thread::exchange_interrupt_token(it2);
                   assert(std::this_thread::get_interrupt_token().valid());
                   assert(!std::this_thread::get_interrupt_token().is_interrupted());
                   std::interrupt_token it3{true};
                   std::this_thread::exchange_interrupt_token(it3);
                   assert(std::this_thread::get_interrupt_token().valid());
                   assert(std::this_thread::get_interrupt_token().is_interrupted());
                   std::cout << "END t1" << std::endl;
                 });
    // wait until t1 has set all initial values:
    for (int i=0; !t1AllSet.load(); ++i) {
      std::this_thread::sleep_for(10ms);
      std::cout.put('o').flush();
    }
    // and check all values:
    assert(!t0.joinable());
    assert(std::interrupt_token{} == t0.get_original_interrupt_token());
    assert(t1.joinable());
    assert(t1ID == t1.get_id());
    assert(t1IsInterrupted == false);
    assert(t1InterruptToken == t1.get_original_interrupt_token());
    itoken = t1.get_original_interrupt_token();
    assert(t1InterruptToken.valid());
    assert(!t1InterruptToken.is_interrupted());
    // test swap():
    std::swap(t0, t1);
    assert(!t1.joinable());
    assert(std::interrupt_token{} == t1.get_original_interrupt_token());
    assert(t0.joinable());
    assert(t1ID == t0.get_id());
    assert(t1InterruptToken == t0.get_original_interrupt_token());
    // manual swap with move():
    auto ttmp{std::move(t0)};
    t0 = std::move(t1);
    t1 = std::move(ttmp);
    assert(!t0.joinable());
    assert(std::interrupt_token{} == t0.get_original_interrupt_token());
    assert(t1.joinable());
    assert(t1ID == t1.get_id());
    assert(t1InterruptToken == t1.get_original_interrupt_token());
  } // leave scope of t1 without join() or detach() (signals cancellation)
  assert(itoken.is_interrupted());
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------

void testJoin()
{
  // test jthread join()
  std::cout << "\n*** start testJoin()" << std::endl;

  std::interrupt_token itoken;
  assert(!itoken.valid());
  {
    std::jthread t1([] {
                      // wait until interrupt is signaled (due to calling interrupt() for the token):
                      for (int i=0; !std::this_thread::is_interrupted(); ++i) {
                         std::this_thread::sleep_for(100ms);
                         std::cout.put('.').flush();
                      }
                      std::cout << "END t1" << std::endl;
                 });
    itoken = t1.get_original_interrupt_token();
    // let nother thread signal cancellation after some time:
    std::jthread t2([itoken] () mutable {
                     for (int i=0; i < 10; ++i) {
                       std::this_thread::sleep_for(70ms);
                       std::cout.put('x').flush();
                     }
		     // signal interrupt to other thread:
		     itoken.interrupt();
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

  std::interrupt_token itoken;
  assert(!itoken.valid());
  std::atomic<bool> t1FinallyInterrupted{false};
  {
    std::jthread t0;
    std::jthread::id t1ID{std::this_thread::get_id()};
    bool t1IsInterrupted{true};
    std::interrupt_token t1InterruptToken;
    std::atomic<bool> t1AllSet{false};
    std::jthread t1([&t1ID, &t1IsInterrupted, &t1InterruptToken, &t1AllSet, &t1FinallyInterrupted] {
                   // check some values of the started thread:
                   t1ID = std::this_thread::get_id();
                   t1IsInterrupted = std::this_thread::is_interrupted();
                   t1InterruptToken = std::this_thread::get_interrupt_token();
                   assert(std::this_thread::get_interrupt_token().valid());
                   assert(!std::this_thread::get_interrupt_token().is_interrupted());
                   try {
                     std::this_thread::throw_if_interrupted();
                   }
                   catch (...) {
                     assert(false);
                   }
                   t1AllSet.store(true);
                   // wait until interrupt is signaled (due to calling interrupt() for the token):
                   for (int i=0; !std::this_thread::is_interrupted(); ++i) {
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
    assert(std::interrupt_token{} == t0.get_original_interrupt_token());
    assert(t1.joinable());
    assert(t1ID == t1.get_id());
    assert(t1IsInterrupted == false);
    assert(t1InterruptToken == t1.get_original_interrupt_token());
    itoken = t1.get_original_interrupt_token();
    assert(t1InterruptToken.valid());
    assert(!t1InterruptToken.is_interrupted());
    // test detach():
    t1.detach();
    assert(!t1.joinable());
  } // leave scope of t1 without join() or detach() (signals cancellation)

  // finally signal cancellation:
  assert(!t1FinallyInterrupted.load());
  itoken.interrupt();
  // and check consequences:
  assert(itoken.is_interrupted());
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
  bool t1IsInterrupted{true};
  std::interrupt_token t1InterruptToken;
  std::atomic<bool> t1AllSet{false};
  std::interrupt_token t1ShallDie{false};
  std::thread t1([&t1ID, &t1IsInterrupted, &t1InterruptToken, &t1AllSet, t1ShallDie] {
                   // check some supplementary values of the started thread:
                   t1ID = std::this_thread::get_id();
                   t1IsInterrupted = std::this_thread::is_interrupted();
                   t1InterruptToken = std::this_thread::get_interrupt_token();
                   try {
                     std::this_thread::throw_if_interrupted();
                   }
                   catch (...) {
                     assert(false);
                   }
                   t1AllSet.store(true);
                   // manually establish our own interrupt token:
                   std::this_thread::exchange_interrupt_token(t1ShallDie);
                   // and wait until cancellation is signaled:
                   try {
                     for (int i=0; ; ++i) {
                        std::this_thread::throw_if_interrupted();
                        std::this_thread::sleep_for(100ms);
                        std::cout.put('.').flush();
                     }
                     assert(false);
                   }
		   catch (std::exception&) { // interrupted not derived from std::exception
		     assert(false);
		   }
                   catch (const std::interrupted& e) {
                     std::cout << e.what() << std::endl;
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
  assert(t1IsInterrupted == false);
  assert(!t1InterruptToken.valid());
  assert(!t1InterruptToken.is_interrupted());
  // signal cancellation via manually installed interrupt token:
  t1ShallDie.interrupt();
  t1.join();
  std::cout << "\n*** OK" << std::endl;
}

//------------------------------------------------------


int main()
{
  std::set_terminate([](){
		       std::cout << "ERROR: terminate() called" << std::endl;
		       assert(false);
		     });

  std::cout << "\n\n**************************\n\n";
  testJThread();
  std::cout << "\n\n**************************\n\n";
  testJoin();
  std::cout << "\n\n**************************\n\n";
  testDetach();
  std::cout << "\n\n**************************\n\n";
  testStdThread();
}

