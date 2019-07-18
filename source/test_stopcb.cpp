#include "stop_token.hpp"
#include <iostream>
#include <type_traits>
#include <functional>
#include <cassert>


void testStopCallbackInits()
{
  std::cout << "\n============= testStopCallbackInits()\n";

  std::stop_token token;

  struct ImplicitArg {
  };
  struct ExplicitArg {
  };
  struct MyCallback {
    MyCallback() {}
    MyCallback(ImplicitArg) {}
    explicit MyCallback(ExplicitArg) {}
    void operator()() const {
      std::cout << "MyCallback operator()\n";
    }
  };

  std::cout << "----- simple lambdas:\n";
  auto stop10 = [] { std::cout << "stop10\n"; };
  std::stop_callback cb10{token, stop10};
  static_assert(std::is_same_v<decltype(cb10)::callback_type,
                               decltype(stop10)>);

  std::cout << "-----\n";
  auto stop11 = [] { std::cout << "stop11\n"; };
  std::stop_callback cb11{token, std::move(stop11)};
  static_assert(std::is_same_v<decltype(cb11)::callback_type,
                               decltype(stop11)>);
  static_assert(!std::is_reference_v<decltype(cb11)::callback_type>);

  std::cout << "-----\n";
  auto stop12 = [] { std::cout << "stop12\n"; };
  std::stop_callback cb12{token, std::ref(stop12)};
  static_assert(std::is_same_v<decltype(cb12)::callback_type,
                               std::reference_wrapper<decltype(stop12)>>);
  static_assert(!std::is_reference_v<decltype(cb12)::callback_type>);

  std::cout << "-----\n";
  std::stop_callback cb13{token,
                          [] { std::cout << "stop13\n"; }
                         };
  static_assert(!std::is_reference_v<decltype(cb13)::callback_type>);

  std::cout << "----- function<void()>:\n";
  std::stop_callback<std::function<void()>> cb14{
    token, [] { std::cout << "stop14\n"; }
  };
  static_assert(std::is_same_v<decltype(cb14)::callback_type,
                               std::function<void()>>);
  static_assert(!std::is_reference_v<decltype(cb14)::callback_type>);

  std::cout << "-----\n";
  std::function<void()> stop15 = [] { std::cout << "stop15\n"; };
  std::stop_callback cb15{token, stop15};
  static_assert(std::is_same_v<decltype(cb15)::callback_type,
                               std::function<void()>>);
  static_assert(!std::is_reference_v<decltype(cb15)::callback_type>);

  std::cout << "-----\n";
  std::function<void()> stop16 = [] { std::cout << "stop16\n"; };
  std::stop_callback<std::function<void()>> cb16{token, stop16};
  static_assert(std::is_same_v<decltype(cb16)::callback_type,
                               std::function<void()>>);
  static_assert(!std::is_reference_v<decltype(cb16)::callback_type>);

  std::cout << "-----\n";
  std::function<void()> stop17;
  auto cb17 = [token] {
                std::function<void()> f;
                if (true) {
                  f = [] { std::cout << "stop17a\n"; };
                } else {
                  f = [] { std::cout << "stop17b\n"; };
                }
                return std::stop_callback{token, f};
              }();
  static_assert(std::is_same_v<decltype(cb17)::callback_type,
                               std::function<void()>>);
  static_assert(!std::is_reference_v<decltype(cb17)::callback_type>);

  std::cout << "----- ImplicitArg i:\n";
  ImplicitArg i;
  std::stop_callback<MyCallback> cb18{token, i};
  static_assert(std::is_same_v<decltype(cb18)::callback_type,
                               MyCallback>);
  static_assert(!std::is_reference_v<decltype(cb18)::callback_type>);

  std::cout << "----- ExplicitArg i:\n";
  ExplicitArg e;
  std::stop_callback<MyCallback> cb19{token, e};
  static_assert(std::is_same_v<decltype(cb19)::callback_type,
                               MyCallback>);
  static_assert(!std::is_reference_v<decltype(cb19)::callback_type>);

  std::cout << "----- the following should not compile:\n";
  // ERROR:
  //std::stop_callback<MyCallback> cb =
  //  []() -> stop_callback<MyCallback> {
  //      ExplicitArg e;
  //      return {token, e};
  //  }();
  // ERROR:
  //std::stop_callback<MyCallback> cb =
  //  []() -> stop_callback<MyCallback> {
  //      ImplicitArg i;
  //      return {token, i};
  //  }();

  std::cout << "**** all OK\n";
}


//------------------------------------------------------

int main()
{
  try {
    testStopCallbackInits();
  }
  catch (...) {
    assert(false);
  }
}

