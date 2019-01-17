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

std::condition_variable_any2* cv = nullptr;
std::mutex m;
std::stop_source ss;
std::stop_token st=ss.get_token();

bool done = false;
bool waiting=false;

std::condition_variable deleter_cv;

void deleter_thread() {
    std::unique_lock<std::mutex> lk(m);
    deleter_cv.wait(lk,[]{return waiting;});
    done = true;
    cv->notify_all();
    cv->~condition_variable_any2();
    std::memset(cv, 0x55, sizeof(*cv)); // UB but OK to ensure the check
    ss.request_stop();
}

void testCVAnyMutex() 
{
  std::cout << "*** start testCVAnyMUtex() ORIG" << std::endl;
    void* raw=malloc(sizeof(std::condition_variable_any2));
    cv = new(raw) std::condition_variable_any2;
    std::thread th(deleter_thread);
    m.lock();
    waiting=true;
    deleter_cv.notify_all();
    cv->wait_until(m,[]{return done;},st);
    m.unlock();
    th.join();
    free(raw);
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

