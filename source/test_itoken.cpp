#include "interrupt_token.hpp"
#include <iostream>
#include <chrono>
#include <cassert>


//------------------------------------------------------

void testInterruptTokenAPI()
{
  std::cout << "\n============= testInterruptToken\n";

  //***** interrupt_source:

  // create, copy, assign and destroy:
  {
    std::interrupt_source is1;
    std::interrupt_source is2{is1};
    std::interrupt_source is3 = is1;
    std::interrupt_source is4{std::move(is1)};
    assert(!is1.is_valid());
    assert(is2.is_valid());
    assert(is3.is_valid());
    assert(is4.is_valid());
    is1 = is2;
    assert(is1.is_valid());
    is1 = std::move(is2);
    assert(!is2.is_valid());
    std::swap(is1,is2);
    assert(!is1.is_valid());
    assert(is2.is_valid());
    is1.swap(is2);
    assert(is1.is_valid());
    assert(!is2.is_valid());
  }

  //***** interrupt_token:

  // create, copy, assign and destroy:
  {
    std::interrupt_token it1;
    std::interrupt_token it2{it1};
    std::interrupt_token it3 = it1;
    std::interrupt_token it4{std::move(it1)};
    it1 = it2;
    it1 = std::move(it2);
    std::swap(it1,it2);
    it1.swap(it2);
    assert(!it1.is_interruptible());
    assert(!it2.is_interruptible());
    assert(!it3.is_interruptible());
    assert(!it4.is_interruptible());
  }

  //***** source and token:

  // tokens without an source are no longer interruptible:
  {
    std::interrupt_source* isp = new std::interrupt_source;
    std::interrupt_source& isr = *isp;
    std::interrupt_token it{isr.get_token()};
    assert(isr.is_valid());
    assert(it.is_interruptible());
    delete isp;  // not interrupted and losing last source
    assert(!it.is_interruptible());
  }
  {
    std::interrupt_source* isp = new std::interrupt_source;
    std::interrupt_source& isr = *isp;
    std::interrupt_token it{isr.get_token()};
    assert(isr.is_valid());
    assert(it.is_interruptible());
    isr.interrupt();
    //delete isp;  // interrupted and losing last source
    //assert(it.is_interruptible());
  }

  //***** is_valid()/is_interruptible, is_interrupted(), and interrupt():
  {
    std::interrupt_source isNotValid;
    std::interrupt_source isNotInterrupted{std::move(isNotValid)};
    std::interrupt_source isInterrupted;
    isInterrupted.interrupt();
    std::interrupt_token itNotValid{isNotValid.get_token()};
    std::interrupt_token itNotInterrupted{isNotInterrupted.get_token()};
    std::interrupt_token itInterrupted{isInterrupted.get_token()};

    // is_valid() and is_interrupted():
    assert(!isNotValid.is_valid());
    assert(isNotInterrupted.is_valid());
    assert(isInterrupted.is_valid());
    assert(!isNotValid.is_interrupted());
    assert(!isNotInterrupted.is_interrupted());
    assert(isInterrupted.is_interrupted());

    // is_interruptible() ("valid") and is_interrupted():
    assert(!itNotValid.is_interruptible());
    assert(itNotInterrupted.is_interruptible());
    assert(itInterrupted.is_interruptible());
    assert(!itNotInterrupted.is_interrupted());
    assert(itInterrupted.is_interrupted());

    // interrupt():
    assert(isNotInterrupted.interrupt() == false);
    assert(isNotInterrupted.interrupt() == true);
    assert(isInterrupted.interrupt() == true);
    assert(isNotInterrupted.is_interrupted());
    assert(isInterrupted.is_interrupted());
    assert(itNotInterrupted.is_interrupted());
    assert(itInterrupted.is_interrupted());
  }

  //***** assignment and swap():
  {
    std::interrupt_source isNotValid;
    std::interrupt_source isNotInterrupted{std::move(isNotValid)};
    std::interrupt_source isInterrupted;
    isInterrupted.interrupt();
    std::interrupt_token itNotValid{isNotValid.get_token()};
    std::interrupt_token itNotInterrupted{isNotInterrupted.get_token()};
    std::interrupt_token itInterrupted{isInterrupted.get_token()};

    // assignments and swap():
    assert(!std::interrupt_token{}.is_interrupted());
    itInterrupted = std::interrupt_token{};
    assert(!itInterrupted.is_interruptible());
    assert(!itInterrupted.is_interrupted());
    isInterrupted = std::interrupt_source{};
    assert(isInterrupted.is_valid());
    assert(!isInterrupted.is_interrupted());

    std::swap(itInterrupted, itNotValid);
    assert(!itInterrupted.is_interruptible());
    assert(!itNotValid.is_interruptible());
    assert(!itNotValid.is_interrupted());
    std::interrupt_token itnew = std::move(itNotValid);
    assert(!itNotValid.is_interruptible());

    std::swap(isInterrupted, isNotValid);
    assert(!isInterrupted.is_valid());
    assert(isNotValid.is_valid());
    assert(!isNotValid.is_interrupted());
    std::interrupt_source isnew = std::move(isNotValid);
    assert(!isNotValid.is_valid());
  }

  // shared ownership semantics:
  std::interrupt_source is;
  std::interrupt_token it1{is.get_token()};
  std::interrupt_token it2{it1};
  assert(is.is_valid() && !is.is_interrupted());
  assert(it1.is_interruptible() && !it1.is_interrupted());
  assert(it2.is_interruptible() && !it2.is_interrupted());
  is.interrupt();
  assert(is.is_valid() && is.is_interrupted());
  assert(it1.is_interruptible() && it1.is_interrupted());
  assert(it2.is_interruptible() && it2.is_interrupted());

  // == and !=:
  {
    std::interrupt_source isNotValid1;
    std::interrupt_source isNotValid2;
    std::interrupt_source isNotInterrupted1{std::move(isNotValid1)};
    std::interrupt_source isNotInterrupted2{isNotInterrupted1};
    std::interrupt_source isInterrupted1{std::move(isNotValid2)};
    std::interrupt_source isInterrupted2{isInterrupted1};
    isInterrupted1.interrupt();
    std::interrupt_token itNotValid1{isNotValid1.get_token()};
    std::interrupt_token itNotValid2{isNotValid2.get_token()};
    std::interrupt_token itNotValid3;
    std::interrupt_token itNotInterrupted1{isNotInterrupted1.get_token()};
    std::interrupt_token itNotInterrupted2{isNotInterrupted2.get_token()};
    std::interrupt_token itNotInterrupted3{itNotInterrupted1};
    std::interrupt_token itInterrupted1{isInterrupted1.get_token()};
    std::interrupt_token itInterrupted2{isInterrupted2.get_token()};
    std::interrupt_token itInterrupted3{itInterrupted2};

    assert(isNotValid1 == isNotValid2);
    assert(isNotInterrupted1 == isNotInterrupted2);
    assert(isInterrupted1 == isInterrupted2);
    assert(isNotValid1 != isNotInterrupted1);
    assert(isNotValid1 != isInterrupted1);
    assert(isNotInterrupted1 != isInterrupted1);

    assert(itNotValid1 == itNotValid2);
    assert(itNotValid2 == itNotValid3);
    assert(itNotInterrupted1 == itNotInterrupted2);
    assert(itNotInterrupted2 == itNotInterrupted3);
    assert(itInterrupted1 == itInterrupted2);
    assert(itInterrupted2 == itInterrupted3);
    assert(itNotValid1 != itNotInterrupted1);
    assert(itNotValid1 != itInterrupted1);
    assert(itNotInterrupted1 != itInterrupted1);

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
    std::interrupt_token it0;    // should not allocate anything

    std::cout << "---- create interruptor and interruptee\n";
    std::interrupt_source it_interruptor;
    std::interrupt_token it_interruptee{it_interruptor.get_token()};
    ++okSteps; sleep(dur);  // 1
    assert(!it_interruptor.is_interrupted());
    assert(!it_interruptee.is_interrupted());

    std::cout << "---- call interruptor.interrupt(): \n";
    it_interruptor.interrupt();  // INTERRUPT !!!
    ++okSteps; sleep(dur);  // 2
    assert(it_interruptor.is_interrupted());
    assert(it_interruptee.is_interrupted());

    std::cout << "---- call interruptor.interrupt() again:  (should have no effect)\n";
    it_interruptor.interrupt();
    ++okSteps; sleep(dur);  // 3
    assert(it_interruptor.is_interrupted());
    assert(it_interruptee.is_interrupted());

    std::cout << "---- simulate reset\n";
    it_interruptor = std::interrupt_source{};
    it_interruptee = it_interruptor.get_token();
    ++okSteps; sleep(dur);  // 4
    assert(!it_interruptor.is_interrupted());
    assert(!it_interruptee.is_interrupted());

    std::cout << "---- call interruptor.interrupt(): \n";
    it_interruptor.interrupt();  // INTERRUPT !!!
    ++okSteps; sleep(dur);  // 5
    assert(it_interruptor.is_interrupted());
    assert(it_interruptee.is_interrupted());

    std::cout << "---- call interruptor.interrupt() again:  (should have no effect)\n";
    it_interruptor.interrupt();
    ++okSteps; sleep(dur);  // 6
    assert(it_interruptor.is_interrupted());
    assert(it_interruptee.is_interrupted());
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
  testInterruptTokenAPI();
  testIToken(::std::chrono::seconds{0});
  testIToken(::std::chrono::milliseconds{500});
}

