#define SAFE
#include "stop_token.hpp"
#include <iostream>
#include <chrono>
#include <cassert>
#include <functional>
#include <optional>
#include <chrono>
#include <thread>
#include <cstdlib>

//------------------------------------------------------

void testCallbackRegister()
{
  std::cout << "\n============= testCallbackRegister()\n";

  // create stop_source
  std::stop_source ssrc;
  assert(ssrc.stop_possible());
  assert(!ssrc.stop_requested());

  // create stop_token from stop_source
  std::stop_token stok{ssrc.get_token()};
  assert(ssrc.stop_possible());
  assert(!ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(!stok.stop_requested());

  // register callback
  bool cb1called{false};
  bool cb2called{false};
  std::cout << "register cb1\n";
  std::stop_callback cb1{stok, 
                         [&] {
                           std::cout << "cb1 called\n";
                           cb1called = true;
                           // register another callback
                           std::cout << "register cb2\n";
                           std::stop_callback cb2{stok, 
                                                  [&] {
                                                    std::cout << "cb2 called\n";
                                                    cb2called = true;
                                                    std::cout << "cb2 done\n";
                                                  }};
                           std::cout << "cb1 done\n";
                         }};
  assert(ssrc.stop_possible());
  assert(!ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(!stok.stop_requested());
  assert(!cb1called);
  assert(!cb2called);

  // request stop
  std::cout << "request stop\n";
  auto b = ssrc.request_stop();
  assert(b);
  assert(ssrc.stop_possible());
  assert(ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(stok.stop_requested());
  assert(cb1called);
  assert(cb2called);

  std::cout << "**** all OK\n";
}

//------------------------------------------------------

void testCallbackUnregister()
{
  using namespace std::literals;
  std::cout << "\n============= testCallbackUnregister()\n";

  // create stop_source
  std::stop_source ssrc;
  assert(ssrc.stop_possible());
  assert(!ssrc.stop_requested());

  // create stop_token from stop_source
  std::stop_token stok{ssrc.get_token()};
  assert(ssrc.stop_possible());
  assert(!ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(!stok.stop_requested());

  // register callback that unregisters itself
  bool cb1called{false};
  std::optional<std::stop_callback<std::function<void()>>> cb;
  cb.emplace(stok,
             [&] {
                cb1called = true;
                // remove this lambda in optional while being called
                cb.reset();
                //std::this_thread::sleep_for(5s);
              });
  assert(ssrc.stop_possible());
  assert(!ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(!stok.stop_requested());
  assert(!cb1called);

  // request stop
  auto b = ssrc.request_stop();
  assert(b);
  assert(ssrc.stop_possible());
  assert(ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(stok.stop_requested());
  assert(cb1called);

  std::cout << "**** all OK\n";
}

//------------------------------------------------------

struct RegUnregCB {
  std::optional<std::stop_callback<std::function<void()>>> cb{};
  bool called{false};

  void reg(std::stop_token& stok) {
    cb.emplace(stok,
               [&] {
                 called = true;
               });
  }
  void unreg() {
    cb.reset();
  }
};

void testCallbackConcUnregister()
{
  using namespace std::literals;
  std::cout << "\n============= testCallbackConcUnregister()\n";

  // create stop_source and stop_token:
  std::stop_source ssrc;
  std::stop_token stok{ssrc.get_token()};

  std::atomic<bool> cb1called{false};
  std::atomic<bool> cb1done{false};
  std::atomic<bool> cb1end{false};
  std::optional<std::stop_callback<std::function<void()>>> optCB;
  auto cb1 = [&]{
              std::cout << "start cb1()" << std::endl;
              cb1called = true;   
              std::this_thread::sleep_for(1s);
              std::cout << "reset cb1()" << std::endl;
              optCB.reset();
              std::cout << "reset cb1() done" << std::endl;
              std::this_thread::sleep_for(5s);
              std::cout << "end cb1()" << std::endl;
              // this is criticl, because we use the lambda:
              // => std::ref to ensure that the lambda is still there
              cb1done = true;   
            };
  optCB.emplace(stok, std::ref(cb1));  // ref() necessary because we copy to std::function
  // anabling this thead create a CORE DUMP at the end of cb1 being in request_stop():
  std::thread t1{[&] {
                   while (!cb1end) {
                     if (!cb1called) {
                       std::cout << "t1: cb1 not called" << std::endl;
                     }
                     else if (!cb1done) {
                       std::cout << "t1: in cb1" << std::endl;
                     }
                     else {
                       std::cout << "t1: cb1 done" << std::endl;
                     }
                     std::this_thread::sleep_for(0.3s);
                   }
                   std::cout << "t1: cb1 end" << std::endl;
                 }};

  // request stop
  std::this_thread::sleep_for(2s);
  std::cout << "request_stop()" << std::endl;
  ssrc.request_stop();
  std::cout << "request_stop() done" << std::endl;

  std::this_thread::sleep_for(6s);
  std::cout << "t1.join()" << std::endl;
  cb1end = true;
  t1.join();
  std::cout << "t1.join() done" << std::endl;
  assert(ssrc.stop_possible());
  assert(ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(stok.stop_requested());
  //assert(cb1called);

  std::cout << "**** all OK\n";
}

//------------------------------------------------------

void testCallbackConcUnregisterOtherThread()
{
  using namespace std::literals;
  std::cout << "\n============= testCallbackConcUnregisterOtherThread()\n";

  // create stop_source and stop_token:
  std::stop_source ssrc;
  std::stop_token stok{ssrc.get_token()};

  std::atomic<bool> cb1called{false};
  std::atomic<bool> cb1done{false};
  std::atomic<bool> cb1end{false};
  std::optional<std::stop_callback<std::function<void()>>> optCB;
  auto cb1 = [&]{
              std::cout << "start cb1()" << std::endl;
              cb1called = true;   
              std::this_thread::sleep_for(1s);
              std::cout << "reset cb1()" << std::endl;
              optCB.reset();
              std::cout << "reset cb1() done" << std::endl;
              std::this_thread::sleep_for(5s);
              std::cout << "end cb1()" << std::endl;
              cb1done = true;   
            };
  std::thread t0{[&] {
                   optCB.emplace(stok, std::ref(cb1));
                   std::this_thread::sleep_for(20s);
                 }};
  // enabling this thead create a CORE DUMP at the end of cb1 being in request_stop():
  std::thread t1{[&] {
                   while (!cb1end) {
                     if (!cb1called) {
                       std::cout << "t1: cb1 not called" << std::endl;
                     }
                     else if (!cb1done) {
                       std::cout << "t1: in cb1" << std::endl;
                     }
                     else {
                       std::cout << "t1: cb1 done" << std::endl;
                     }
                     std::this_thread::sleep_for(0.3s);
                   }
                   std::cout << "t1: cb1 end" << std::endl;
                 }};

  // request stop
  std::this_thread::sleep_for(2s);
  std::cout << "request_stop()" << std::endl;
  ssrc.request_stop();
  std::cout << "request_stop() done" << std::endl;

  std::this_thread::sleep_for(6s);
  std::cout << "t1.join()" << std::endl;
  t1.join();
  std::cout << "t1.join() done" << std::endl;
  assert(ssrc.stop_possible());
  assert(ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(stok.stop_requested());
  //assert(cb1called);

  std::cout << "**** all OK\n";
}
//
//------------------------------------------------------

void testCallbackThrow()
{
  std::cout << "\n============= testCallbackThrow()\n";

  // create stop_source
  std::stop_source ssrc;
  assert(ssrc.stop_possible());
  assert(!ssrc.stop_requested());

  // create stop_token from stop_source
  std::stop_token stok{ssrc.get_token()};
  assert(ssrc.stop_possible());
  assert(!ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(!stok.stop_requested());

  // register callback
  bool cb1called{false};
  bool cb2called{false};
  std::stop_callback cb1{stok, 
                         [&] {
                           cb1called = true;
                           // throw
                           throw "callback called";
                         }};
  assert(ssrc.stop_possible());
  assert(!ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(!stok.stop_requested());
  assert(!cb1called);
  assert(!cb2called);

  // catch terminate() call:
  std::set_terminate([]{
                      std::cout << "terminate() called\n";
                      std::cout << "**** all OK\n";
                      std::exit(0);
                     });

  // request stop
  ssrc.request_stop();
  assert(false);
}



//------------------------------------------------------

int main()
{
  testCallbackUnregister();
  testCallbackConcUnregister();
  testCallbackRegister();

  //testCallbackConcUnregisterOtherThread();

  testCallbackThrow();
  // no other test behind this one, because it terminates
}

