#include "stop_token.hpp"
#include <iostream>
#include <chrono>
#include <cassert>


void testStopTokenBasicAPI()
{
  std::cout << "\n============= testStopTokenBasicAPI()\n";

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
  auto cb1 = [&]{
               cb1called = true;
             };
  {
  std::stop_callback scb1{stok, cb1}; // copies cb1
  assert(ssrc.stop_possible());
  assert(!ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(!stok.stop_requested());
  assert(!cb1called);
  } // unregister callback

  // register another callback
  bool cb2called{false};
  auto cb2 = [&]{
               assert(stok.stop_requested());
               cb2called = true;
             };
  //OK: std::stop_callback scb2{stok, cb2};
  //OK: std::stop_callback<std::decay_t<decltype(cb2)>> scb2{stok, cb2};
  //OK: std::stop_callback<decltype(cb2)&> scb2{stok, cb2};
  //OK:
  std::stop_callback<decltype(cb2)> scb2a{stok, cb2}; // copies cb2
  std::stop_callback<decltype(cb2)> scb2b{stok, std::move(cb2)};
  assert(ssrc.stop_possible());
  assert(!ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(!stok.stop_requested());
  assert(!cb1called);
  assert(!cb2called);

  // request stop
  auto b = ssrc.request_stop();
  assert(b);
  assert(ssrc.stop_possible());
  assert(ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(stok.stop_requested());
  assert(!cb1called);
  assert(cb2called);

  b = ssrc.request_stop();
  assert(!b);

  // register another callback
  bool cb3called{false};
  //auto cb3 = [&]{
  //             cb3called = true;
  //           };
  std::stop_callback scb3{stok, 
                          [&]{
                            cb3called = true;
                          }};
  assert(ssrc.stop_possible());
  assert(ssrc.stop_requested());
  assert(stok.stop_possible());
  assert(stok.stop_requested());
  assert(!cb1called);
  assert(cb2called);
  assert(cb3called);

  std::cout << "**** all OK\n";
}

//------------------------------------------------------

void testStopTokenAPI()
{
  std::cout << "\n============= testStopTokenAPI()\n";

  //***** stop_source:

  // create, copy, assign and destroy:
  {
    std::stop_source is1;
    std::stop_source is2{is1};
    std::stop_source is3 = is1;
    std::stop_source is4{std::move(is1)};
    assert(!is1.stop_possible());
    assert(is2.stop_possible());
    assert(is3.stop_possible());
    assert(is4.stop_possible());
    is1 = is2;
    assert(is1.stop_possible());
    is1 = std::move(is2);
    assert(!is2.stop_possible());
    std::swap(is1,is2);
    assert(!is1.stop_possible());
    assert(is2.stop_possible());
    is1.swap(is2);
    assert(is1.stop_possible());
    assert(!is2.stop_possible());

    // stop_source without shared stop state:
    std::stop_source is0{std::nostopstate};
    assert(!is0.stop_requested());
    assert(!is0.stop_possible());
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
    assert(!it1.stop_possible());
    assert(!it2.stop_possible());
    assert(!it3.stop_possible());
    assert(!it4.stop_possible());
  }

  //***** source and token:

  // tokens without an source are no longer interruptible:
  {
    std::stop_source* isp = new std::stop_source;
    std::stop_source& isr = *isp;
    std::stop_token it{isr.get_token()};
    assert(isr.stop_possible());
    assert(it.stop_possible());
    delete isp;  // not interrupted and losing last source
    assert(!it.stop_possible());
  }
  {
    std::stop_source* isp = new std::stop_source;
    std::stop_source& isr = *isp;
    std::stop_token it{isr.get_token()};
    assert(isr.stop_possible());
    assert(it.stop_possible());
    isr.request_stop();
    //delete isp;  // interrupted and losing last source
    //assert(it.stop_possible());
  }

  //***** stop_possible(), stop_requested(), and request_stop():
  {
    std::stop_source isNotValid;
    std::stop_source isNotStopped{std::move(isNotValid)};
    std::stop_source isStopped;
    isStopped.request_stop();
    std::stop_token itNotValid{isNotValid.get_token()};
    std::stop_token itNotStopped{isNotStopped.get_token()};
    std::stop_token itStopped{isStopped.get_token()};

    // stop_possible() and stop_requested():
    assert(!isNotValid.stop_possible());
    assert(isNotStopped.stop_possible());
    assert(isStopped.stop_possible());
    assert(!isNotValid.stop_requested());
    assert(!isNotStopped.stop_requested());
    assert(isStopped.stop_requested());

    // stop_possible() and stop_requested():
    assert(!itNotValid.stop_possible());
    assert(itNotStopped.stop_possible());
    assert(itStopped.stop_possible());
    assert(!itNotStopped.stop_requested());
    assert(itStopped.stop_requested());

    // request_stop():
    assert(isNotStopped.request_stop() == true);
    assert(isNotStopped.request_stop() == false);
    assert(isStopped.request_stop() == false);
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
    assert(!itStopped.stop_possible());
    assert(!itStopped.stop_requested());
    isStopped = std::stop_source{};
    assert(isStopped.stop_possible());
    assert(!isStopped.stop_requested());

    std::swap(itStopped, itNotValid);
    assert(!itStopped.stop_possible());
    assert(!itNotValid.stop_possible());
    assert(!itNotValid.stop_requested());
    std::stop_token itnew = std::move(itNotValid);
    assert(!itNotValid.stop_possible());

    std::swap(isStopped, isNotValid);
    assert(!isStopped.stop_possible());
    assert(isNotValid.stop_possible());
    assert(!isNotValid.stop_requested());
    std::stop_source isnew = std::move(isNotValid);
    assert(!isNotValid.stop_possible());
  }

  // shared ownership semantics:
  std::stop_source is;
  std::stop_token it1{is.get_token()};
  std::stop_token it2{it1};
  assert(is.stop_possible() && !is.stop_requested());
  assert(it1.stop_possible() && !it1.stop_requested());
  assert(it2.stop_possible() && !it2.stop_requested());
  is.request_stop();
  assert(is.stop_possible() && is.stop_requested());
  assert(it1.stop_possible() && it1.stop_requested());
  assert(it2.stop_possible() && it2.stop_requested());

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
void testSToken(D dur)
{
  std::cout << "\n============= testSToken(" << std::chrono::duration<double,std::milli>(dur).count() << "ms)\n";
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
  testStopTokenBasicAPI();
  testStopTokenAPI();
  testSToken(::std::chrono::seconds{0});
  testSToken(::std::chrono::milliseconds{500});
}

