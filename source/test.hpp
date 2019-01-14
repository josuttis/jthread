#ifndef TEST_HPP
#define TEST_HPP

#include <iostream>
#include <cassert>
#include <atomic>

class test_entry
{
    static std::atomic<bool> any_failures;
    static test_entry* first;
    static test_entry* last;

    test_entry* next;
    test_entry* prev;
    const char* name;
    void(*testFunc)();

public:

    explicit test_entry(const char* name, void(*testFunc)()) noexcept
        : next(nullptr)
        , prev(last)
        , name(name)
        , testFunc(testFunc)
    {
        if (last != nullptr) {
            last->next = this;
        } else {
            first = this;
        }
        last = this;
    }

    ~test_entry()
    {
        if (prev == nullptr) {
            first = next;
        } else {
            prev->next = next;
        }

        if (next == nullptr) {
            last = prev;
        } else {
            next->prev = prev;
        }
    }

    test_entry(const test_entry&) = delete;
    test_entry(test_entry&&) = delete;
    const test_entry& operator=(const test_entry&) = delete;
    const test_entry& operator=(test_entry&&) = delete;

    bool run()
    {
        any_failures = false;
        std::cout << "Test " << name << std::endl;
        try {
            testFunc();
        }
        catch (const std::exception& ex) {
            assert(false);
            std::cout << "  FAIL: unhandled exception: " << ex.what() << std::endl;
            any_failures = true;
        }
        catch (...) {
            assert(false);
            std::cout << "  FAIL: unhandled exception" << std::endl;
            any_failures = true;
        }
        return !any_failures;
    }

    static int run_all()
    {
        int failureCount = 0;
        for (auto* entry = first; entry != nullptr; entry = entry->next) {
            if (!entry->run()) {
                ++failureCount;
            }
        }
        return failureCount;
    }

    static void check_failed(const char* msg)
    {
        assert(false);
        std::cout << "  FAIL: " << msg << std::endl;
        any_failures = true;
    }
};

std::atomic<bool> test_entry::any_failures = false;
test_entry* test_entry::first = nullptr;
test_entry* test_entry::last = nullptr;

// Macro for auto-registering a test-case to be run when you call test_entry::run_all().
//
// Usage:
//  TEST(SomeTest)
//  {
//    CHECK(someCond);
//  }
#define TEST(NAME) \
  static void NAME(); \
  static ::test_entry test_entry_##NAME{#NAME, &NAME}; \
  static void NAME()

// Alternative to assert() macro.
// If parameter evaluates to false then causes current test to be reported as failed.
// Does not stop the program from continuing.
#define CHECK(X) \
    do { \
        if ((X)) {} else { ::test_entry::check_failed("CHECK(" #X ")"); } \
    } while (false)

#endif
