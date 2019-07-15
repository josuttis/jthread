#include <exception>
#include <iostream>
#include <cassert>
#include <thread>
#include <atomic>
#include <chrono>
#include <optional>
#include <functional>
#include <condition_variable>
#include <mutex>

//#define SAFE
#include "stop_token.hpp"

#include "test.hpp"


//----------------------------------------------------

TEST(DefaultTokenIsNotStoppable)
{
  std::stop_token t;
  CHECK(!t.stop_requested());
  CHECK(!t.stop_possible());
}


//----------------------------------------------------

TEST(RequestingStopOnSourceUpdatesToken)
{
  std::stop_source s;
  std::stop_token t = s.get_token();
  CHECK(t.stop_possible());
  CHECK(!t.stop_requested());
  s.request_stop();
  CHECK(t.stop_requested());
  CHECK(t.stop_possible());
}


//----------------------------------------------------

TEST(TokenCantBeStoppedWhenNoMoreSources)
{
  std::stop_token t;
  {
    std::stop_source s;
    t = s.get_token();
    CHECK(t.stop_possible());
  }

  CHECK(!t.stop_possible());
}


//----------------------------------------------------

TEST(TokenCanBeStoppedWhenNoMoreSourcesIfStopAlreadyRequested)
{
  std::stop_token t;
  {
    std::stop_source s;
    t = s.get_token();
    CHECK(t.stop_possible());
    s.request_stop();
  }

  CHECK(t.stop_possible());
  CHECK(t.stop_requested());
}


//----------------------------------------------------

TEST(CallbackNotExecutedImmediatelyIfStopNotYetRequested)
{
  std::stop_source s;

  bool callbackExecuted = false;
  {
    std::stop_callback cb(s.get_token(), [&] { callbackExecuted = true; });
  }

  CHECK(!callbackExecuted);

  s.request_stop();

  CHECK(!callbackExecuted);
}


//----------------------------------------------------

TEST(CallbackExecutedIfStopRequestedBeforeDestruction)
{
  std::stop_source s;
  bool callbackExecuted = false;
  std::stop_callback cb(
    s.get_token(),
    [&] { callbackExecuted = true; });

  CHECK(!callbackExecuted);

  s.request_stop();

  CHECK(callbackExecuted);
}


//----------------------------------------------------

TEST(CallbackExecutedImmediatelyIfStopAlreadyRequested)
{
  std::stop_source s;
  s.request_stop();

  bool executed = false;
  std::stop_callback r{ s.get_token(), [&] { executed = true; } };
  CHECK(executed);
}


//----------------------------------------------------

TEST(RegisterMultipleCallbacks)
{
  std::stop_source s;
  auto t = s.get_token();

  int callbackExecutionCount = 0;
  auto callback = [&] { ++callbackExecutionCount; };

  std::stop_callback r1{ t, callback };
  std::stop_callback r2{ t, callback };
  std::stop_callback r3{ t, callback };
  std::stop_callback r4{ t, callback };
  std::stop_callback r5{ t, callback };
  std::stop_callback r6{ t, callback };
  std::stop_callback r7{ t, callback };
  std::stop_callback r8{ t, callback };
  std::stop_callback r9{ t, callback };
  std::stop_callback r10{ t, callback };

  s.request_stop();

  CHECK(callbackExecutionCount == 10);
}


//----------------------------------------------------

TEST(ConcurrentCallbackRegistration)
{
  auto threadLoop = [](std::stop_token token)
  {
    std::atomic<bool> cancelled = false;
    while (!cancelled)
    {
      std::stop_callback registration{ token, [&]
      {
        cancelled = true;
      } };

      std::stop_callback cb0{ token, [] {} };
      std::stop_callback cb1{ token, [] {} };
      std::stop_callback cb2{ token, [] {} };
      std::stop_callback cb3{ token, [] {} };
      std::stop_callback cb4{ token, [] {} };
      std::stop_callback cb5{ token, [] {} };
      std::stop_callback cb6{ token, [] {} };
      std::stop_callback cb7{ token, [] {} };
      std::stop_callback cb8{ token, [] {} };
      std::stop_callback cb9{ token, [] {} };
      std::stop_callback cb10{ token, [] {} };
      std::stop_callback cb11{ token, [] {} };
      std::stop_callback cb12{ token, [] {} };
      std::stop_callback cb13{ token, [] {} };
      std::stop_callback cb14{ token, [] {} };
      std::stop_callback cb15{ token, [] {} };
      std::stop_callback cb16{ token, [] {} };

      std::this_thread::yield();
    }
  };

  // Just assert this runs and terminates without crashing.
  for (int i = 0; i < 100; ++i)
  {
    std::stop_source source;

    std::thread waiter1{ threadLoop, source.get_token() };
    std::thread waiter2{ threadLoop, source.get_token() };
    std::thread waiter3{ threadLoop, source.get_token() };

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::thread canceller{ [&source] {
                              source.request_stop();
                            }
                         };

    canceller.join();
    waiter1.join();
    waiter2.join();
    waiter3.join();
  }
}


//----------------------------------------------------

TEST(CallbackDeregisteredFromWithinCallbackDoesNotDeadlock)
{
  std::stop_source src;
  std::optional<std::stop_callback<std::function<void()>>> cb;

  cb.emplace(src.get_token(), [&] {
    cb.reset();
  });

  src.request_stop();

  CHECK(!cb.has_value());
}


//----------------------------------------------------

TEST(CallbackDeregistrationDoesNotWaitForOtherCallbacksToFinishExecuting)
{
  std::stop_source src;

  std::mutex mut;
  std::condition_variable cv;

  bool releaseCallback = false;
  bool callbackExecuting = false;

  auto dummyCallback = []{};

  std::optional<std::stop_callback<decltype(dummyCallback)&>> cb1{std::in_place, src.get_token(), dummyCallback};

  // Register a first callback that will signal when it starts executing
  // and then block until it receives a signal.
  std::stop_callback blockingCb{ src.get_token(), [&] {
    std::unique_lock lock{mut};
    callbackExecuting = true;
    cv.notify_all();
    cv.wait(lock, [&] { return releaseCallback; });
  } };

  std::optional<std::stop_callback<decltype(dummyCallback)&>> cb2{std::in_place, src.get_token(), dummyCallback};

  std::thread signallingThread{ [&] { src.request_stop(); } };

  // Wait until the callback starts executing on the signalling-thread.
  // The signalling thread will remain blocked in this callback until we release it.
  {
    std::unique_lock lock{mut};
    cv.wait(lock, [&] { return callbackExecuting; });
  }

  // Then try and deregister the other callbacks.
  // This operation should not be blocked on other callbacks.
  cb1.reset();
  cb2.reset();

  // Finally, signal the callback to unblock and wait for the signalling thread to finish.
  {
    std::unique_lock lock{mut};
    releaseCallback = true;
    cv.notify_all();
  }

  signallingThread.join();
}


//----------------------------------------------------

TEST(CallbackDeregistrationBlocksUntilCallbackFinishes)
{
  std::stop_source src;

  std::mutex mut;
  std::condition_variable cv;

  bool callbackRegistered = false;

  std::thread callbackRegisteringThread{ [&] {
    bool callbackExecuting = false;
    bool callbackAboutToReturn = false;
    bool callbackDeregistered = false;

    {
      std::stop_callback cb{ src.get_token(), [&] {
        std::unique_lock lock{mut};
        callbackExecuting = true;
        cv.notify_all();
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        CHECK(!callbackDeregistered);
        callbackAboutToReturn = true;
      }};

      {
        std::unique_lock lock{mut};
        callbackRegistered = true;
        cv.notify_all();
        cv.wait(lock, [&] { return callbackExecuting; });
      }

      CHECK(!callbackAboutToReturn);
    }

    callbackDeregistered = true;

    CHECK(callbackExecuting);
    CHECK(callbackAboutToReturn);
  }};

  {
    std::unique_lock lock{mut};
    cv.wait(lock, [&] { return callbackRegistered; });
    // Make sure to release the lock before requesting stop
    // since this will execute the callback which will try to
    // acquire the lock on the mutex.
  }

  src.request_stop();

  callbackRegisteringThread.join();
}


//----------------------------------------------------

template<typename CB>
struct callback_batch
{
  using callback_t = std::stop_callback<CB&>;

  callback_batch(std::stop_token t, CB& callback)
   : r0(t, callback),
     r1(t, callback),
     r2(t, callback),
     r3(t, callback),
     r4(t, callback),
     r5(t, callback),
     r6(t, callback),
     r7(t, callback),
     r8(t, callback),
     r9(t, callback) {
  }

  callback_t r0;
  callback_t r1;
  callback_t r2;
  callback_t r3;
  callback_t r4;
  callback_t r5;
  callback_t r6;
  callback_t r7;
  callback_t r8;
  callback_t r9;
};

TEST(CancellationSingleThreadPerformance)
{
  auto callback = []{};

  std::stop_source s;

  constexpr int iterationCount = 100'000;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterationCount; ++i)
  {
    std::stop_callback r{ s.get_token(), callback };
  }

  auto end = std::chrono::high_resolution_clock::now();

  auto time1 = end - start;

  start = end;

  for (int i = 0; i < iterationCount; ++i)
  {
    callback_batch b{ s.get_token(), callback };
  }

  end = std::chrono::high_resolution_clock::now();

  auto time2 = end - start;

  start = end;

  for (int i = 0; i < iterationCount; ++i)
  {
    callback_batch b0{ s.get_token(), callback };
    callback_batch b1{ s.get_token(), callback };
    callback_batch b2{ s.get_token(), callback };
    callback_batch b3{ s.get_token(), callback };
    callback_batch b4{ s.get_token(), callback };
  }

  end = std::chrono::high_resolution_clock::now();

  auto time3 = end - start;

  auto report = [](const char* label, auto time, std::uint64_t count)
  {
    auto ms = std::chrono::duration<double, std::milli>(time).count();
    auto ns = std::chrono::duration<double, std::nano>(time).count();
    std::cout << label << " took " << ms << "ms (" << (ns / static_cast<double>(count)) << " ns/item)" << std::endl;
  };

  report("Individual", time1, iterationCount);
  report("Batch10", time2, 10 * iterationCount);
  report("Batch50", time3, 50 * iterationCount);
}


//----------------------------------------------------

int main()
{
  auto status = test_entry::run_all();
  if (status == 0) {
    std::cout << "**** all OK\n";
  }
  return status;
}
