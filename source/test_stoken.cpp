#include "stop_token.hpp"
#include <iostream>
#include <chrono>
#include <cassert>


//------------------------------------------------------

void testStopTokenAPI()
{
  std::cout << "\n============= testStopToken\n";

  //***** stop_source:

  // create, copy, assign and destroy:
  {
    std::stop_source is1;
    std::stop_source is2{is1};
    std::stop_source is3 = is1;
    std::stop_source is4{std::move(is1)};
    assert(!is1.stoppable());
    assert(is2.stoppable());
    assert(is3.stoppable());
    assert(is4.stoppable());
    is1 = is2;
    assert(is1.stoppable());
    is1 = std::move(is2);
    assert(!is2.stoppable());
    std::swap(is1,is2);
    assert(!is1.stoppable());
    assert(is2.stoppable());
    is1.swap(is2);
    assert(is1.stoppable());
    assert(!is2.stoppable());
  }

  //***** stop_token:

  // create, copy, assign and destroy:
  {
    std::stop_token it1;
    std::stop_token it2{it1};
    std::stop_token it3 = it1;
    std::stop_token it4{std::move(it1)};
    it1 = it2;
    it1 = std::move(it2);
    std::swap(it1,it2);
    it1.swap(it2);
    assert(it1.callbacks_ignored());
    assert(it2.callbacks_ignored());
    assert(it3.callbacks_ignored());
    assert(it4.callbacks_ignored());
  }

  //***** source and token:

  // tokens without an source are no longer interruptible:
  {
    std::stop_source* isp = new std::stop_source;
    std::stop_source& isr = *isp;
    std::stop_token it{isr.get_token()};
    assert(isr.stoppable());
    assert(!it.callbacks_ignored());
    delete isp;  // not interrupted and losing last source
    assert(it.callbacks_ignored());
  }
  {
    std::stop_source* isp = new std::stop_source;
    std::stop_source& isr = *isp;
    std::stop_token it{isr.get_token()};
    assert(isr.stoppable());
    assert(!it.callbacks_ignored());
    isr.request_stop();
    //delete isp;  // interrupted and losing last source
    //assert(!it.callbacks_ignored());
  }

  //***** stoppable()/callbacks_ignored(), stop_requested(), and request_stop():
  {
    std::stop_source isNotValid;
    std::stop_source isNotStopped{std::move(isNotValid)};
    std::stop_source isStopped;
    isStopped.request_stop();
    std::stop_token itNotValid{isNotValid.get_token()};
    std::stop_token itNotStopped{isNotStopped.get_token()};
    std::stop_token itStopped{isStopped.get_token()};

    // stoppable() and stop_requested():
    assert(!isNotValid.stoppable());
    assert(isNotStopped.stoppable());
    assert(isStopped.stoppable());
    assert(!isNotValid.stop_requested());
    assert(!isNotStopped.stop_requested());
    assert(isStopped.stop_requested());

    // callbacks_ignored() and stop_requested():
    assert(itNotValid.callbacks_ignored());
    assert(!itNotStopped.callbacks_ignored());
    assert(!itStopped.callbacks_ignored());
    assert(!itNotStopped.stop_requested());
    assert(itStopped.stop_requested());

    // request_stop():
    assert(isNotStopped.request_stop() == false);
    assert(isNotStopped.request_stop() == true);
    assert(isStopped.request_stop() == true);
    assert(isNotStopped.stop_requested());
    assert(isStopped.stop_requested());
    assert(itNotStopped.stop_requested());
    assert(itStopped.stop_requested());
  }

  //***** assignment and swap():
  {
    std::stop_source isNotValid;
    std::stop_source isNotStopped{std::move(isNotValid)};
    std::stop_source isStopped;
    isStopped.request_stop();
    std::stop_token itNotValid{isNotValid.get_token()};
    std::stop_token itNotStopped{isNotStopped.get_token()};
    std::stop_token itStopped{isStopped.get_token()};

    // assignments and swap():
    assert(!std::stop_token{}.stop_requested());
    itStopped = std::stop_token{};
    assert(itStopped.callbacks_ignored());
    assert(!itStopped.stop_requested());
    isStopped = std::stop_source{};
    assert(isStopped.stoppable());
    assert(!isStopped.stop_requested());

    std::swap(itStopped, itNotValid);
    assert(itStopped.callbacks_ignored());
    assert(itNotValid.callbacks_ignored());
    assert(!itNotValid.stop_requested());
    std::stop_token itnew = std::move(itNotValid);
    assert(itNotValid.callbacks_ignored());

    std::swap(isStopped, isNotValid);
    assert(!isStopped.stoppable());
    assert(isNotValid.stoppable());
    assert(!isNotValid.stop_requested());
    std::stop_source isnew = std::move(isNotValid);
    assert(!isNotValid.stoppable());
  }

  // shared ownership semantics:
  std::stop_source is;
  std::stop_token it1{is.get_token()};
  std::stop_token it2{it1};
  assert(is.stoppable() && !is.stop_requested());
  assert(!it1.callbacks_ignored() && !it1.stop_requested());
  assert(!it2.callbacks_ignored() && !it2.stop_requested());
  is.request_stop();
  assert(is.stoppable() && is.stop_requested());
  assert(!it1.callbacks_ignored() && it1.stop_requested());
  assert(!it2.callbacks_ignored() && it2.stop_requested());

  // == and !=:
  {
    std::stop_source isNotValid1;
    std::stop_source isNotValid2;
    std::stop_source isNotStopped1{std::move(isNotValid1)};
    std::stop_source isNotStopped2{isNotStopped1};
    std::stop_source isStopped1{std::move(isNotValid2)};
    std::stop_source isStopped2{isStopped1};
    isStopped1.request_stop();
    std::stop_token itNotValid1{isNotValid1.get_token()};
    std::stop_token itNotValid2{isNotValid2.get_token()};
    std::stop_token itNotValid3;
    std::stop_token itNotStopped1{isNotStopped1.get_token()};
    std::stop_token itNotStopped2{isNotStopped2.get_token()};
    std::stop_token itNotStopped3{itNotStopped1};
    std::stop_token itStopped1{isStopped1.get_token()};
    std::stop_token itStopped2{isStopped2.get_token()};
    std::stop_token itStopped3{itStopped2};

    assert(isNotValid1 == isNotValid2);
    assert(isNotStopped1 == isNotStopped2);
    assert(isStopped1 == isStopped2);
    assert(isNotValid1 != isNotStopped1);
    assert(isNotValid1 != isStopped1);
    assert(isNotStopped1 != isStopped1);

    assert(itNotValid1 == itNotValid2);
    assert(itNotValid2 == itNotValid3);
    assert(itNotStopped1 == itNotStopped2);
    assert(itNotStopped2 == itNotStopped3);
    assert(itStopped1 == itStopped2);
    assert(itStopped2 == itStopped3);
    assert(itNotValid1 != itNotStopped1);
    assert(itNotValid1 != itStopped1);
    assert(itNotStopped1 != itStopped1);

    assert(!(isNotValid1 != isNotValid2));
    assert(!(itNotValid1 != itNotValid2));
  }

  std::cout << "**** all OK\n";
}


//------------------------------------------------------

template<typename D>
void sleep(D dur)
{
   if (dur > std::chrono::milliseconds{0}) {
     //std::cout << "- sleep " << std::chrono::duration<double,std::milli>(dur).count() << "ms\n";
     std::this_thread::sleep_for(dur);
   }
}

template<typename D>
void testIToken(D dur)
{
  std::cout << "\n============= testIToken(" << std::chrono::duration<double,std::milli>(dur).count() << "ms)\n";
  int okSteps = 0;
  try {

    std::cout << "---- default constructor\n";
    std::stop_token it0;    // should not allocate anything

    std::cout << "---- create interruptor and interruptee\n";
    std::stop_source interruptor;
    std::stop_token interruptee{interruptor.get_token()};
    ++okSteps; sleep(dur);  // 1
    assert(!interruptor.stop_requested());
    assert(!interruptee.stop_requested());

    std::cout << "---- call interruptor.request_stop(): \n";
    interruptor.request_stop();  // INTERRUPT !!!
    ++okSteps; sleep(dur);  // 2
    assert(interruptor.stop_requested());
    assert(interruptee.stop_requested());

    std::cout << "---- call interruptor.request_stop() again:  (should have no effect)\n";
    interruptor.request_stop();
    ++okSteps; sleep(dur);  // 3
    assert(interruptor.stop_requested());
    assert(interruptee.stop_requested());

    std::cout << "---- simulate reset\n";
    interruptor = std::stop_source{};
    interruptee = interruptor.get_token();
    ++okSteps; sleep(dur);  // 4
    assert(!interruptor.stop_requested());
    assert(!interruptee.stop_requested());

    std::cout << "---- call interruptor.request_stop(): \n";
    interruptor.request_stop();  // INTERRUPT !!!
    ++okSteps; sleep(dur);  // 5
    assert(interruptor.stop_requested());
    assert(interruptee.stop_requested());

    std::cout << "---- call interruptor.request_stop() again:  (should have no effect)\n";
    interruptor.request_stop();
    ++okSteps; sleep(dur);  // 6
    assert(interruptor.stop_requested());
    assert(interruptee.stop_requested());
  }
  catch (...) {
    assert(false);
  }
  assert(okSteps == 6);
  std::cout << "**** all OK\n";
}


//------------------------------------------------------

int main()
{
  testStopTokenAPI();
  testIToken(::std::chrono::seconds{0});
  testIToken(::std::chrono::milliseconds{500});
}

