#ifndef INTERRUPT_TOKEN_HPP
#define INTERRUPT_TOKEN_HPP

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
    // the callbacks to call on interrupt:
    ::std::list<interrupt_callback_base*> cbData{};   // currently registered callbacks
    ::std::mutex cbDataMutex{};                       // which is concurrently used/modified

    // state flags for (tricky) races:
    ::std::atomic<bool> interrupted;          // true if interrupt signaled
    ::std::atomic<std::size_t> numSources;    // number of interrupt_sources (for is_interruptible())
    std::thread::id interruptingThreadID{};   // temporary ID of interrupting thread
    
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


// forward declarations:
template <typename Callback> class interrupt_callback;
class interrupt_source;


//***************************************** 
// interrupt_token
//***************************************** 
class interrupt_token {
 private:
  ::std::shared_ptr<SharedData> _sp{nullptr};

 public:
  // default constructor is cheap:
  explicit interrupt_token() noexcept = default;

  // special members (all defaults are OK):
  //interrupt_token(const interrupt_token&) noexcept = default;
  //interrupt_token(interrupt_token&&) noexcept = default;
  //interrupt_token& operator=(const interrupt_token&) noexcept = default;
  //interrupt_token& operator=(interrupt_token&&) noexcept = default;
  //~interrupt_token() noexcept = default;
  void swap(interrupt_token& it) noexcept {
    _sp.swap(it._sp);
  }

  // interrupt handling:
  bool is_interrupted() const noexcept {
    return _sp && _sp->interrupted.load();
  }
  bool is_interruptible() const {
    // signals if callback could be called (immediately or later).
    // if not interruptible we remain to be so
    // if interruptible we can only become not interruptible if
    //  not interrupted yet
    return _sp && (is_interrupted() || _sp->numSources > 0);
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
   : _sp{std::move(ip)} {
  }
};

bool operator== (const interrupt_token& lhs, const interrupt_token& rhs) {
  // TODO: just comparing _sp is enough?
  return (!lhs.is_interruptible() && !rhs.is_interruptible())
         || (lhs._sp.get() == rhs._sp.get());
}
bool operator!= (const interrupt_token& lhs, const interrupt_token& rhs) {
  return !(lhs==rhs);
}


//***************************************** 
// interrupt_source
//***************************************** 
class interrupt_source {
 private:
  ::std::shared_ptr<SharedData> _sp{nullptr};

 public:
  // default constructor is expensive:
  explicit interrupt_source() noexcept 
   : _sp{new SharedData{false}} {
  }
  // cheap constructor in case we need it:
  // - see e.g. class jthread
  explicit interrupt_source(std::nullptr_t) noexcept 
   : _sp{nullptr} {
  }

  // special members (default OK):
  interrupt_source(const interrupt_source& is) noexcept
   : _sp{is._sp} {
       if (_sp) {
         ++(_sp->numSources);
       }
  }
  interrupt_source(interrupt_source&&) noexcept = default;
  // pass by value to do the increments/decrements right:
  interrupt_source& operator=(interrupt_source is) noexcept {
    swap(is);
    return *this;
  }
  ~interrupt_source () {
     if (_sp) {
       --(_sp->numSources);
     }
  }

  void swap(interrupt_source& it) noexcept {
    _sp.swap(it._sp);
  }

  // get_token()
  interrupt_token get_token() const noexcept {
    return interrupt_token(_sp);
  }
  
  // interrupt handling:
  bool is_valid() const {
    return _sp != nullptr;
  }
  bool is_interrupted() const noexcept {
    return _sp && _sp->interrupted.load();
  }

  bool interrupt();
  
  friend bool operator== (const interrupt_source& lhs, const interrupt_source& rhs);
};

bool operator== (const interrupt_source& lhs, const interrupt_source& rhs) {
  // TODO: just comparing _sp is enough?
  return (!lhs.is_valid() && !rhs.is_valid())
         || (lhs._sp.get() == rhs._sp.get());
}
bool operator!= (const interrupt_source& lhs, const interrupt_source& rhs) {
  return !(lhs==rhs);
}


//***************************************** 
// interrupt_callback
//***************************************** 
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



//***************************************** 
// key member function for registering and calling callbacks
//***************************************** 

bool interrupt_source::interrupt()
{
  std::cout<<std::this_thread::get_id()<<": Interrupting "<<std::endl;
  //std::cout.put('I').flush();
  if (!is_valid()) return false;
  auto wasInterrupted = _sp->interrupted.exchange(true);
  if (!wasInterrupted) {
      _sp->interruptingThreadID = std::this_thread::get_id();
      ::std::unique_lock lg{_sp->cbDataMutex};  // might throw
      // note, we no longer accept new callbacks here in the list
      // but other callbacks might be unregistered
      while(!_sp->cbData.empty()) {
        std::cout<<std::this_thread::get_id()<<": Notifying "<<std::endl;
        auto elem = _sp->cbData.front();
        _sp->cbData.pop_front();
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

  std::unique_lock lg{_sp->cbDataMutex};
  if (_sp->interrupted.load()) {
    // if already interrupted, make sure the callback is durectly called
    // - but not blocking others
    lg.unlock();
    cbPtr->run();
  }
  else {
    _sp->cbData.emplace_front(cbPtr);  // might throw
  }
  std::cout.put('r').flush();
}

void interrupt_token::unregisterCB(interrupt_callback_base* cbPtr) {
  //std::cout.put('U').flush();
  if (!is_interruptible()) return;

  {
    std::scoped_lock lg{_sp->cbDataMutex};
    // remove the matching CB
    for (auto pos = _sp->cbData.begin(); pos != _sp->cbData.end(); ++pos) {
      if (*pos == cbPtr) {
        _sp->cbData.erase(pos);
        return;
      }
    }
  }
  // If we come here, the callback either has executed or is being executed
  // So we have to find out whether we are inside the callback
  // - for that we compare thread IDs 
  if (std::this_thread::get_id() == _sp->interruptingThreadID) {
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
