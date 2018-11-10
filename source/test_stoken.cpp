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
    assert(!is1.valid());
    assert(is2.valid());
    assert(is3.valid());
    assert(is4.valid());
    is1 = is2;
    assert(is1.valid());
    is1 = std::move(is2);
    assert(!is2.valid());
    std::swap(is1,is2);
    assert(!is1.valid());
    assert(is2.valid());
    is1.swap(is2);
    assert(is1.valid());
    assert(!is2.valid());
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
    assert(!it1.stop_done_or_possible());
    assert(!it2.stop_done_or_possible());
    assert(!it3.stop_done_or_possible());
    assert(!it4.stop_done_or_possible());
  }

  //***** source and token:

  // tokens without an source are no longer interruptible:
  {
    std::stop_source* isp = new std::stop_source;
    std::stop_source& isr = *isp;
    std::stop_token it{isr.get_token()};
    assert(isr.valid());
    assert(it.stop_done_or_possible());
    delete isp;  // not interrupted and losing last source
    assert(!it.stop_done_or_possible());
  }
  {
    std::stop_source* isp = new std::stop_source;
    std::stop_source& isr = *isp;
    std::stop_token it{isr.get_token()};
    assert(isr.valid());
    assert(it.stop_done_or_possible());
    isr.signal_stop();
    //delete isp;  // interrupted and losing last source
    //assert(it.stop_done_or_possible());
  }

  //***** valid()/stop_done_or_possible(), stop_signaled(), and signal_stop():
  {
    std::stop_source isNotValid;
    std::stop_source isNotStopped{std::move(isNotValid)};
    std::stop_source isStopped;
    isStopped.signal_stop();
    std::stop_token itNotValid{isNotValid.get_token()};
    std::stop_token itNotStopped{isNotStopped.get_token()};
    std::stop_token itStopped{isStopped.get_token()};

    // valid() and stop_signaled():
    assert(!isNotValid.valid());
    assert(isNotStopped.valid());
    assert(isStopped.valid());
    assert(!isNotValid.stop_signaled());
    assert(!isNotStopped.stop_signaled());
    assert(isStopped.stop_signaled());

    // stop_done_or_possible() ("valid") and stop_signaled():
    assert(!itNotValid.stop_done_or_possible());
    assert(itNotStopped.stop_done_or_possible());
    assert(itStopped.stop_done_or_possible());
    assert(!itNotStopped.stop_signaled());
    assert(itStopped.stop_signaled());

    // signal_stop():
    assert(isNotStopped.signal_stop() == false);
    assert(isNotStopped.signal_stop() == true);
    assert(isStopped.signal_stop() == true);
    assert(isNotStopped.stop_signaled());
    assert(isStopped.stop_signaled());
    assert(itNotStopped.stop_signaled());
    assert(itStopped.stop_signaled());
  }

  //***** assignment and swap():
  {
    std::stop_source isNotValid;
    std::stop_source isNotStopped{std::move(isNotValid)};
    std::stop_source isStopped;
    isStopped.signal_stop();
    std::stop_token itNotValid{isNotValid.get_token()};
    std::stop_token itNotStopped{isNotStopped.get_token()};
    std::stop_token itStopped{isStopped.get_token()};

    // assignments and swap():
    assert(!std::stop_token{}.stop_signaled());
    itStopped = std::stop_token{};
    assert(!itStopped.stop_done_or_possible());
    assert(!itStopped.stop_signaled());
    isStopped = std::stop_source{};
    assert(isStopped.valid());
    assert(!isStopped.stop_signaled());

    std::swap(itStopped, itNotValid);
    assert(!itStopped.stop_done_or_possible());
    assert(!itNotValid.stop_done_or_possible());
    assert(!itNotValid.stop_signaled());
    std::stop_token itnew = std::move(itNotValid);
    assert(!itNotValid.stop_done_or_possible());

    std::swap(isStopped, isNotValid);
    assert(!isStopped.valid());
    assert(isNotValid.valid());
    assert(!isNotValid.stop_signaled());
    std::stop_source isnew = std::move(isNotValid);
    assert(!isNotValid.valid());
  }

  // shared ownership semantics:
  std::stop_source is;
  std::stop_token it1{is.get_token()};
  std::stop_token it2{it1};
  assert(is.valid() && !is.stop_signaled());
  assert(it1.stop_done_or_possible() && !it1.stop_signaled());
  assert(it2.stop_done_or_possible() && !it2.stop_signaled());
  is.signal_stop();
  assert(is.valid() && is.stop_signaled());
  assert(it1.stop_done_or_possible() && it1.stop_signaled());
  assert(it2.stop_done_or_possible() && it2.stop_signaled());

  // == and !=:
  {
    std::stop_source isNotValid1;
    std::stop_source isNotValid2;
    std::stop_source isNotStopped1{std::move(isNotValid1)};
    std::stop_source isNotStopped2{isNotStopped1};
    std::stop_source isStopped1{std::move(isNotValid2)};
    std::stop_source isStopped2{isStopped1};
    isStopped1.signal_stop();
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
    assert(!interruptor.stop_signaled());
    assert(!interruptee.stop_signaled());

    std::cout << "---- call interruptor.signal_stop(): \n";
    interruptor.signal_stop();  // INTERRUPT !!!
    ++okSteps; sleep(dur);  // 2
    assert(interruptor.stop_signaled());
    assert(interruptee.stop_signaled());

    std::cout << "---- call interruptor.signal_stop() again:  (should have no effect)\n";
    interruptor.signal_stop();
    ++okSteps; sleep(dur);  // 3
    assert(interruptor.stop_signaled());
    assert(interruptee.stop_signaled());

    std::cout << "---- simulate reset\n";
    interruptor = std::stop_source{};
    interruptee = interruptor.get_token();
    ++okSteps; sleep(dur);  // 4
    assert(!interruptor.stop_signaled());
    assert(!interruptee.stop_signaled());

    std::cout << "---- call interruptor.signal_stop(): \n";
    interruptor.signal_stop();  // INTERRUPT !!!
    ++okSteps; sleep(dur);  // 5
    assert(interruptor.stop_signaled());
    assert(interruptee.stop_signaled());

    std::cout << "---- call interruptor.signal_stop() again:  (should have no effect)\n";
    interruptor.signal_stop();
    ++okSteps; sleep(dur);  // 6
    assert(interruptor.stop_signaled());
    assert(interruptee.stop_signaled());
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

