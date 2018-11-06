#ifndef INTERRUPT_TOKEN_HPP
#define INTERRUPT_TOKEN_HPP

//*****************************************************************************
#include <memory>
#include <atomic>
#include <mutex>
#include <list>
#include <thread>
#include <cassert>

// for debugging
#include <iostream> // in case we enable the debug output


namespace std {

  //***************************************** 
//* new class for interrupt tokens
//* - very cheap to create
//* - cheap to copy
//* - for both interrupter and interruptee
//***************************************** 

// callbacks objects have different types => type erase them
struct interrupt_callback_base {
  std::atomic<bool> finishedExecuting{false};
  bool* destructorHasRunInsideCallback{nullptr};
  virtual void run() noexcept = 0;
  virtual ~interrupt_callback_base() = default;
};


struct SharedData {
    ::std::atomic<bool> interrupted;  // true if interrupt signaled
    ::std::atomic<std::size_t> numSources;

    ::std::list<interrupt_callback_base*> cbData{};     // currently waiting CVs and its lock
    ::std::mutex cbDataMutex{};       // we have multistep concurrent access to cvPtrs

    std::thread::id interruptingThreadID{};
    
    // TODO: NO LONGER 
    // make polymorphic class for future binary-compatible interrupt_token extensions:
    SharedData(bool initial_state)
     : interrupted{initial_state}, numSources{1} {
    }
    virtual ~SharedData() = default;  // make polymorphic
    SharedData(const SharedData&) = delete;
    SharedData(SharedData&&) = delete;
    SharedData& operator= (const SharedData&) = delete;
    SharedData& operator= (SharedData&&) = delete;
};

template <typename Callback>
class interrupt_callback;

class interrupt_source;

class interrupt_token {
 private:
  ::std::shared_ptr<SharedData> _ip{nullptr};

 public:
  // default constructor is cheap:
  explicit interrupt_token() noexcept = default;

  // special members (default OK):
  //interrupt_token(const interrupt_token&) noexcept;
  //interrupt_token(interrupt_token&&) noexcept;
  //interrupt_token& operator=(const interrupt_token&) noexcept;
  //interrupt_token& operator=(interrupt_token&&) noexcept;
  void swap(interrupt_token& it) noexcept {
    _ip.swap(it._ip);
  }

  // interrupt handling:
  bool is_interruptible() const {
    return _ip != nullptr;
  }
  bool is_interrupted() const noexcept {
    return _ip && _ip->interrupted.load();
  }

  friend bool operator== (const interrupt_token& lhs, const interrupt_token& rhs);
 
 private:
  // stuff to registered condition variables for notofication: 
  template <typename T>
  friend class ::std::interrupt_callback;
  void registerCB(interrupt_callback_base* cvPtr);
  void unregisterCB(interrupt_callback_base* cvPtr);

  friend class interrupt_source;
  explicit interrupt_token(::std::shared_ptr<SharedData> ip)
   : _ip{std::move(ip)} {
  }
};

bool operator== (const interrupt_token& lhs, const interrupt_token& rhs) {
  // TODO: just comparing _ip is enough?
  return (!lhs.is_interruptible() && !rhs.is_interruptible())
         || (lhs._ip.get() == rhs._ip.get());
}
bool operator!= (const interrupt_token& lhs, const interrupt_token& rhs) {
  return !(lhs==rhs);
}


class interrupt_source {
 private:
  ::std::shared_ptr<SharedData> _ip{nullptr};

 public:
  // default constructor is cheap:
  explicit interrupt_source() noexcept 
   : _ip{new SharedData{false}} {
  }

  interrupt_token get_token() const noexcept {
    return interrupt_token(_ip);
  }
  
  // special members (default OK):
  interrupt_source(const interrupt_source& is) noexcept
   : _ip{is._ip} {
       if (_ip) {
         ++(_ip->numSources);
       }
  }
  interrupt_source(interrupt_source&&) noexcept = default;
  // pass by value to do the increments/decrements right:
  interrupt_source& operator=(interrupt_source is) noexcept {
    swap(is);
    return *this;
  }
  ~interrupt_source () {
     if (_ip) {
       --(_ip->numSources);
     }
  }

  void swap(interrupt_source& it) noexcept {
    _ip.swap(it._ip);
  }

  // interrupt handling:
  bool is_interruptible() const {
    return _ip != nullptr;
  }
  bool is_interrupted() const noexcept {
    return _ip && _ip->interrupted.load();
  }

  bool interrupt();
  
  friend bool operator== (const interrupt_source& lhs, const interrupt_source& rhs);
};

bool operator== (const interrupt_source& lhs, const interrupt_source& rhs) {
  // TODO: just comparing _ip is enough?
  return (!lhs.is_interruptible() && !rhs.is_interruptible())
         || (lhs._ip.get() == rhs._ip.get());
}
bool operator!= (const interrupt_source& lhs, const interrupt_source& rhs) {
  return !(lhs==rhs);
}

template <typename Callback>
struct interrupt_callback : interrupt_callback_base
{
  private:
    interrupt_token itoken;
    Callback callback;
  public:
    interrupt_callback(interrupt_token it, Callback&& cb)
     : itoken{std::move(it)}, callback{std::forward<Callback>(cb)} {
        if (itoken.is_interruptible()) {
          itoken.registerCB(this);
        }
    }
    ~interrupt_callback() {
        if (itoken.is_interruptible()) {
          itoken.unregisterCB(this);
        }
    }
    interrupt_callback(const interrupt_callback&) = delete;
    interrupt_callback(interrupt_callback&&) = delete;
    interrupt_callback& operator=(const interrupt_callback&) = delete;
    interrupt_callback& operator=(interrupt_callback&&) = delete;
  private:
    virtual void run() noexcept override {
      callback();  //or: std::invoke(callback);
      finishedExecuting.store(true);
    }    
};


bool interrupt_source::interrupt()
{
  std::cout<<std::this_thread::get_id()<<": Interrupting "<<std::endl;
  //std::cout.put('I').flush();
  if (!is_interruptible()) return false;
  auto wasInterrupted = _ip->interrupted.exchange(true);
  if (!wasInterrupted) {
      _ip->interruptingThreadID = std::this_thread::get_id();
      ::std::unique_lock lg{_ip->cbDataMutex};  // might throw
      // note, we no longer accept new callbacks here in the list
      // but other callbacks might be unregistered
      while(!_ip->cbData.empty()) {
        std::cout<<std::this_thread::get_id()<<": Notifying "<<std::endl;
        auto elem = _ip->cbData.front();
        _ip->cbData.pop_front();
        lg.unlock();
        bool destructorHasRunInsideCallback{false};
        elem->destructorHasRunInsideCallback = &destructorHasRunInsideCallback;
        std::cout<<std::this_thread::get_id()<<": call callback "<<std::endl;
        elem->run();  // don't call the callback locked
        if (!destructorHasRunInsideCallback) {
          elem->destructorHasRunInsideCallback = nullptr;
          elem->finishedExecuting.store(true);
        }
        lg.lock();
      }
  }
  //std::cout.put('i').flush();
  return wasInterrupted;
}

void interrupt_token::registerCB(interrupt_callback_base* cbPtr) {
  std::cout.put('R').flush();
  if (!is_interruptible()) return;

  std::unique_lock lg{_ip->cbDataMutex};
  if (_ip->interrupted.load()) {
    // if already interrupted, make sure the callback is durectly called
    // - but not blocking others
    lg.unlock();
    cbPtr->run();
  }
  else {
    _ip->cbData.emplace_front(cbPtr);  // might throw
  }
  std::cout.put('r').flush();
}

void interrupt_token::unregisterCB(interrupt_callback_base* cbPtr) {
  //std::cout.put('U').flush();
  if (!is_interruptible()) return;

  {
    std::scoped_lock lg{_ip->cbDataMutex};
    // remove the matching CB
    for (auto pos = _ip->cbData.begin(); pos != _ip->cbData.end(); ++pos) {
      if (*pos == cbPtr) {
        _ip->cbData.erase(pos);
        return;
      }
    }
  }
  // If we come here, the callback either has executed or is being executed
  // So we have to find out whether we are inside the callback
  // - for that we compare thread IDs 
  if (std::this_thread::get_id() == _ip->interruptingThreadID) {
    if (cbPtr->destructorHasRunInsideCallback) {
      // we are inside callback of cpPtr
      *(cbPtr->destructorHasRunInsideCallback) = true;
    }
    else {
      // callback had run before, so:
      assert(cbPtr->finishedExecuting.load());
    }
  }
  else {
    // different thread than where interrupt was called:
    // => wait for the end:
    // busy waiting, better use a binary semaphore or a futex
    while (!cbPtr->finishedExecuting.load()) {
      std::this_thread::yield();
    }
  }
  //std::cout.put('u').flush();
}


} // namespace std

#endif // INTERRUPT_TOKEN_HPP
