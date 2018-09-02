#include "interrupt_token.hpp"
#include <iostream>
#include <chrono>
#include <cassert>

void testInterruptToken()
{
  std::cout << "\n============= testInterruptToken\n";
  // create, copy, assign and destroy:
  std::interrupt_token i1;
  std::interrupt_token i2{i1};
  std::interrupt_token i3 = i1;
  std::interrupt_token i4{std::move(i1)};
  i1 = i2;
  i1 = std::move(i2);
  std::swap(i1,i2);
  i1.swap(i2);

  std::interrupt_token inotvalid;
  std::interrupt_token ivalidNotInterrupted{false};
  std::interrupt_token ivalidInterrupted{true};

  // valid():
  assert(!inotvalid.valid());
  assert(ivalidNotInterrupted.valid());
  assert(ivalidInterrupted.valid());
  assert(!ivalidNotInterrupted.is_interrupted());
  assert(ivalidInterrupted.is_interrupted());

  // interrupt():
  assert(ivalidNotInterrupted.interrupt() == false);
  assert(ivalidNotInterrupted.interrupt() == true);
  assert(ivalidInterrupted.interrupt() == true);

  // assignments:
  ivalidInterrupted = std::interrupt_token{false};
  assert(ivalidInterrupted.valid());
  assert(!ivalidInterrupted.is_interrupted());
  std::swap(ivalidInterrupted, inotvalid);
  assert(!ivalidInterrupted.valid());
  assert(inotvalid.valid());
  assert(!inotvalid.is_interrupted());
  std::interrupt_token itnew = std::move(inotvalid);
  assert(!inotvalid.valid());

  // shared ownership semantics:
  std::interrupt_token it1{false};
  std::interrupt_token it2{it1};
  assert(it1.valid() && !it1.is_interrupted());
  assert(it2.valid() && !it2.is_interrupted());
  it1.interrupt();
  assert(it1.valid() && it1.is_interrupted());
  assert(it2.valid() && it2.is_interrupted());

  // throw if_interrupted():
  {
    std::interrupt_token inotvalid;
    std::interrupt_token ivalidNotInterrupted{false};
    std::interrupt_token ivalidInterrupted{true};
    try {
      inotvalid.throw_if_interrupted();
    }
    catch (...) {
      assert(false);
    }
    try {
      ivalidNotInterrupted.throw_if_interrupted();
    }
    catch (...) {
      assert(false);
    }
    try {
      ivalidInterrupted.throw_if_interrupted();
      assert(false);
    }
    catch (...) {
    }
  }
  std::cout << "**** all OK\n";

  // == and !=:
  {
    std::interrupt_token it0a;
    std::interrupt_token it1a{false};
    std::interrupt_token it2a{true};
    std::interrupt_token it0b;
    std::interrupt_token it1b{it1a};
    std::interrupt_token it2b{it2a};
    std::interrupt_token it3a{false};
    std::interrupt_token it4a{true};
    assert(it0a == it0b && !(it0a != it0b));
    assert(it1a == it1b && !(it1a != it1b));
    assert(it2a == it2b && !(it2a != it2b));
    assert(!(it1a == it3a) && (it1a != it3a));
    assert(!(it2a == it4a) && (it2a != it4a));
  }
}


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
    std::interrupt_token it_interruptor{false};
    std::interrupt_token it_interruptee{it_interruptor};
    ++okSteps; sleep(dur);  // 1
    assert(!it_interruptor.is_interrupted());
    assert(!it_interruptee.is_interrupted());

    std::cout << "---- interruptee.throw_if_interrupted();   (should not throw)\n";
    try {
      it_interruptee.throw_if_interrupted();
      ++okSteps; sleep(dur); // 2
    }
    catch (...) {
      assert(false);
    }
    assert(!it_interruptor.is_interrupted());
    assert(!it_interruptee.is_interrupted());

    std::cout << "---- call interruptor.interrupt(): \n";
    it_interruptor.interrupt();  // INTERRUPT !!!
    ++okSteps; sleep(dur);  // 3
    assert(it_interruptor.is_interrupted());
    assert(it_interruptee.is_interrupted());

    std::cout << "---- call interruptor.interrupt() again:  (should have no effect)\n";
    it_interruptor.interrupt();
    ++okSteps; sleep(dur);  // 4
    assert(it_interruptor.is_interrupted());
    assert(it_interruptee.is_interrupted());

    std::cout << "---- interruptee.throw_if_interrupted();   (should throw std::interrupted)\n";
    try {
      it_interruptee.throw_if_interrupted();
      assert(false);
    }
    catch (const std::exception& e) {
      assert(false);
    }
    catch (const std::interrupted& e) {
      //std::cout << "OK: EXCEPTION (std::interrupted): " << e.what() << '\n';
      ++okSteps; sleep(dur);  // 5
    }
    catch (...) {
      assert(false);
    }
    assert(it_interruptor.is_interrupted());
    assert(it_interruptee.is_interrupted());

    std::cout << "---- simulate reset\n";
    it_interruptor = std::interrupt_token{false};
    it_interruptee = it_interruptor;
    ++okSteps; sleep(dur);  // 6
    assert(!it_interruptor.is_interrupted());
    assert(!it_interruptee.is_interrupted());

    std::cout << "---- interruptee.throw_if_interrupted();   (should not throw)\n";
    try {
      it_interruptee.throw_if_interrupted();
      ++okSteps; sleep(dur); // 7
    }
    catch (...) {
      assert(false);
    }
    assert(!it_interruptor.is_interrupted());
    assert(!it_interruptee.is_interrupted());

    std::cout << "---- call interruptor.interrupt(): \n";
    it_interruptor.interrupt();  // INTERRUPT !!!
    ++okSteps; sleep(dur);  // 8
    assert(it_interruptor.is_interrupted());
    assert(it_interruptee.is_interrupted());

    std::cout << "---- call interruptor.interrupt() again:  (should have no effect)\n";
    it_interruptor.interrupt();
    ++okSteps; sleep(dur);  // 9
    assert(it_interruptor.is_interrupted());
    assert(it_interruptee.is_interrupted());

    std::cout << "---- interruptee.throw_if_interrupted();   (should throw std::interrupted)\n";
    try {
      it_interruptee.throw_if_interrupted();
      assert(false);
    }
    catch (const std::exception& e) {
      assert(false);
    }
    catch (const std::interrupted& e) {
      //std::cout << "OK: EXCEPTION (std::interrupted): " << e.what() << '\n';
      ++okSteps; sleep(dur);  // 10
    }
    catch (...) {
      assert(false);
    }
    assert(it_interruptor.is_interrupted());
    assert(it_interruptee.is_interrupted());
  }
  catch (...) {
    assert(false);
  }
  assert(okSteps == 10);
  std::cout << "**** all OK\n";
}

int main()
{
  testInterruptToken();
  testIToken(::std::chrono::seconds{0});
  testIToken(::std::chrono::milliseconds{500});
}

