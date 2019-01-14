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
// Test-Case by Howard Hinnant
// - emails 8.-9.11.18
// Original problem:
//  There's a bug in condition_variable_any2 that I don't think impacts the implementation of jthread.
//  However this is such a complex subject that no stone should be left unturned.
// 
//     ~condition_variable_any();
// 
//     Requires: There shall be no thread blocked on *this.
//     [Note: That is, all threads shall have been notified; they may subsequently block on the lock specified in the wait.
//            This relaxes the usual rules, which would have required all wait calls to happen before destruction.
//            Only the notification to unblock the wait needs to happen before destruction.
//            The user should take care to ensure that no threads wait on *this once the destructor has been started,
//            especially when the waiting threads are calling the wait functions in a loop or using the overloads of
//            wait, wait_for, or wait_until that take a predicate.
//      end note]
//
// That big long note means ~condition_variable_any() can execute before a signaled thread returns from a wait.
// If this happens with condition_variable_any2, that waiting thread will attempt to lock the destructed mutex mut.
// To fix this, there must be shared ownership of the data member mut between the condition_variable_any object
// and the member functions wait (wait_for, etc.).
// 
// libc++'s implementation gets this right: https://github.com/llvm-mirror/libcxx/blob/master/include/condition_variable
// 
// It holds the data member mutex with a shared_ptr<mutex> instead of mutex directly, and the wait functions
// create a local shared_ptr<mutex> copy on entry so that if *this destructs out from under the thread executing
// the wait function, the mutex stays alive until the wait function returns.
//
// Nico, after fixed by Anthony:
//  Thanks, but if I now use the cvany implementation, fixed by Anthony,
//  I still get a core dump:
//    https://wandbox.org/permlink/VvG1UKubY69yAK7g
//  (#ifdef for both CV implementations)
//  So, either the test or the fix seems to be wrong.
//
// HH:
//  I'm guessing that to reliably test this, one is going to have to rebuild your condition_variable_any2
//  with an internal mutex that checks for unlock-after-destruction.
//  And the problem with that is now you no no longer have a std::mutex to put into your internal std::condition_variable...
//
//  _Maybe_ you could test it by making your internal std::condition_variable a std::condition_variable_any,
//  then you could put a debugging mutex into it.
//  But I'm not sure, because this is getting pretty weird and I have not actually tried this.
//------------------------------------------------------

#ifndef ORIG_CVANY_RACE_TEST
std::condition_variable_any2* cv = nullptr;
std::mutex m;
bool f_ready = false;
bool g_ready = false;

void f() {
    m.lock();
    f_ready = true;
    cv->notify_one();
    cv->~condition_variable_any2();
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
  std::cout << "*** start testCVAnyMUtex() ORIG" << std::endl;
    // AW 9.11.18: 
    // Writing over the deleted memory is undefined behaviour. In particular,
    // it can destroy the heap data structure, and cause other problems.
    // If you replace new/delete with malloc and free, then it's OK
    //cv = new std::condition_variable_any2;
    void* raw=malloc(sizeof(std::condition_variable_any2));
    cv = new(raw) std::condition_variable_any2;
    std::thread th2(g);
    m.lock();
    while (!g_ready)
        cv->wait(m);
    m.unlock();
    std::thread th1(f);
    th1.join();
    th2.join();
    free(raw);
  std::cout << "\n*** OK" << std::endl;
}

#else // ORIG_CVANY_RACE_TEST

void testCVAnyMutex() 
{
  // test the basic jthread API
  std::cout << "*** start testCVAnyMUtex()" << std::endl;

  // AW 9.11.18: 
  // Writing over the deleted memory is undefined behaviour. In particular,
  // it can destroy the heap data structure, and cause other problems.
  // If you replace new/delete with malloc and free, then it's OK
  //std::condition_variable_any2* cv = new std::condition_variable_any2;
  void* raw=malloc(sizeof(std::condition_variable_any2));
  std::condition_variable_any2* cv = new(raw) std::condition_variable_any2;
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
                    cv->~condition_variable_any2();
                    std::memset(cv, 0x55, sizeof(*cv)); // UB but OK to ensure the check
                 });
  t1.join();
  t2.join();
  free(raw);
  std::cout << "\n*** OK" << std::endl;
}
#endif // ORIG_CVANY_RACE_TEST


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

