#include "condition_variable_any2.hpp"
#include "jthread.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <mutex>
#include <atomic>
#include <cassert>
#include <sstream>
#include <vector>
using namespace::std::literals;



//------------------------------------------------------

void exampleProducerConsumer(double prodSec, double consSec, bool interrupt)
{
  std::cout << "*** start exampleProducerConsumer(prodSleep=" << prodSec << "s, consSleep=" << consSec << "s, "
            << "interrupt=" << interrupt << ")" << std::endl;
  auto prodSleep = std::chrono::duration<double,std::milli>{prodSec};   // duration producer creates new values
  auto consSleep = std::chrono::duration<double,std::milli>{consSec};   // duration producer deals with new values

  std::vector<int> items;
  std::mutex itemsMx;
  std::condition_variable_any2 itemsCV;
  std::stop_source ssource;
  std::stop_token stoken{ssource.get_token()};
  constexpr size_t maxQueueSize = 100;

  std::thread producer{
    [&] {

      auto nextValue = [val=0] () mutable {
                         return ++val;
                       };

      //std::cout << "\nP: lock " << std::endl;
      std::unique_lock lock{itemsMx};
      while (!stoken.stop_requested()) {
        ////std::cout << "\nP: wait " << std::endl;
        if (!itemsCV.wait(lock, stoken, [&] { return items.size() < maxQueueSize; })) {
         //std::cout << "\nP: wait returned false " << std::endl;
         return;
        }
        //// OOPS: NOTE: true does NOT necessarily meant that we have no interrupt !!!
        ////std::cout << "\nP: wait returned true " << std::endl;

        while (items.size() < maxQueueSize && !stoken.stop_requested()) {
          // Ok to produce a value.

          // Don't hold mutex while working to allow consumer to run.
          lock.unlock();

          int item = nextValue();
          //do_work();
          if (prodSec > 0) {
            std::this_thread::sleep_for(prodSleep);
          }

          std::ostringstream strm;
          strm <<"\nP: produce " << item << "\n";
          std::cout << strm.str() << std::flush;

          // Enqueue a value
          lock.lock();
          items.push_back(item);
          itemsCV.notify_all(); // notify that items were added
        }
      }
   
    }};


  std::thread consumer{
    [&]{
      //std::cout << "\nC: lock " << std::endl;
      std::unique_lock lock{itemsMx};
      for(;;) {
        lock.unlock();
        if (consSec > 0) {
          std::this_thread::sleep_for(consSleep);
        }
        lock.lock();

        //std::cout << "\nC: wait " << std::endl;
        if (!itemsCV.wait(lock, stoken, [&] { return !items.empty(); })) {
          // returned false, so it was interrupted
          //std::cout << "\nC: wait returned false " << std::endl;
          return;
        }
        //std::cout << "\nC: wait returned true " << std::endl;
  
        // process current items (note: itemsMx is locked):
        std::ostringstream strm;
        strm <<"\nC: consume ";
        for (int item : items) {
          strm << " " << item;
          if (item == 42) {
            // Found the item I'm looking for. Cancel producer.
            // Whoops, this is being called while holding a lock on mutex 'itemMx'!
            strm << " INTERRUPT";
            assert(ssource.request_stop() == true);
            return;
          }
        }
        std::cout << '\n';
        std::cout << strm.str() << std::flush;

        items.clear();
        itemsCV.notify_all();  // notify that items were removed
      }
    }};

  // Let the producer/consumer run for a little while
  // Interrupt if they don't find a result quickly enough.

  if (interrupt) {
    std::this_thread::sleep_for(prodSleep*10);
    assert(ssource.request_stop() == true);
  }
#ifdef QQQ
  {
    std::condition_variable2 cv2;
    std::mutex m2;
    std::unique_lock lock{ m2 };

    // A hacky way of implementing an interruptible sleep.
    cv2.wait_for(lock, stoken, 5s, [] { std::cout << "S" << std::endl; return false; }, stoken);

    // Ideally this would be:
    // std::this_thread::sleep(5s, stoken);
  }
#endif

  consumer.join();
  producer.join();
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
  exampleProducerConsumer(0,0, false);
  std::cout << "\n\n**************************\n";
  exampleProducerConsumer(0.1,0, false);
  std::cout << "\n\n**************************\n";
  exampleProducerConsumer(0,0.1, false);
  std::cout << "\n\n**************************\n";
  exampleProducerConsumer(0.1,0.9, false);
  std::cout << "\n\n**************************\n";
  exampleProducerConsumer(0,5.0, false);
  std::cout << "\n\n**************************\n";
  exampleProducerConsumer(0.05,5.0, false);
  std::cout << "\n\n**************************\n";

  std::cout << "\n\n**************************\n";
  exampleProducerConsumer(0,0, true);
  std::cout << "\n\n**************************\n";
  exampleProducerConsumer(0.1,0, true);
  std::cout << "\n\n**************************\n";
  exampleProducerConsumer(0,0.1, true);
  std::cout << "\n\n**************************\n";
  exampleProducerConsumer(0.1,0.9, true);
  std::cout << "\n\n**************************\n";
 }
 catch (const std::exception& e) {
   std::cerr << "EXCEPTION: " << e.what() << std::endl;
   return 1;
 }
 catch (...) {
   std::cerr << "EXCEPTION" << std::endl;
   return 1;
 }
 std::cout << "**** all OK\n";
}

