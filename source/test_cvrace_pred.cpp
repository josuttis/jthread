#include "condition_variable_any2.hpp"
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>

using namespace std::literals::chrono_literals;

std::condition_variable_any2 cv;
std::mutex mut;
int sharedStateA = 0;
int sharedStateB = 0;

void threadA()
{
  std::this_thread::sleep_for(100ms);
  std::unique_lock lock{mut};
  sharedStateA = 1;
  cv.notify_all();
  std::this_thread::sleep_for(100ms);
  sharedStateB = 1;
}

void threadB()
{
  std::unique_lock lock{mut};
  cv.wait(lock, [&] {
    // Since pred() should only be called while holding lock on 'mut'
    // we should be able to assume invariants protected by the lock.
    if (sharedStateA != sharedStateB) {
      std::cout << "ERROR: Invariants maintained by lock not preserved." << std::endl;
      std::abort();
    }
    return sharedStateA == 1;
  });
}

int main() {
  std::thread a{&threadA};
  threadB();
  a.join();
  return 0;
}
