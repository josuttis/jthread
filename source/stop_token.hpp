#ifndef STOP_TOKEN_HPP
#define STOP_TOKEN_HPP

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
//* new classes for signaling stops
//* - stop_source to signal stops
//* - stop_token to receive signaled stops
//* - stop_callback to to register callbacks
//***************************************** 

// type-erased base class for all stop callbacks
struct stop_callback_base {
  std::atomic<bool> finishedExecuting{false};
  bool* destructorHasRunInsideCallback{nullptr};
  virtual void run() noexcept = 0;
  virtual ~stop_callback_base() = default;
};

// shared state to signal stops
struct SharedData {
    // the callbacks to be called on a signaled stop:
    ::std::list<stop_callback_base*> cbData{};  // list of currently registered callbacks
    ::std::mutex cbDataMutex{};                 // which is concurrently used/modified

    // state flags for (tricky) races:
    ::std::atomic<bool> stopSignaled;           // true if stop was signaled
    ::std::atomic<std::size_t> numSources;      // number of stop_sources (for stop_done_or_possile())
    std::thread::id stoppingThreadID{};         // temporary ID of stopping thread
    
    // make polymorphic class for future binary-compatible stop_token extensions:
    // TODO: probably no loner necessary as we ave callbacks now, which
    //       allow any future extensions on what to do an stops
    SharedData(bool initial_state)
     : stopSignaled{initial_state}, numSources{1} {
    }
    virtual ~SharedData() = default;  // make polymorphic
    SharedData(const SharedData&) = delete;
    SharedData(SharedData&&) = delete;
    SharedData& operator= (const SharedData&) = delete;
    SharedData& operator= (SharedData&&) = delete;
};


// forward declarations:
template <typename Callback> class stop_callback;
class stop_source;


//***************************************** 
// stop_token
//***************************************** 
class stop_token {
 private:
  ::std::shared_ptr<SharedData> _sp{nullptr};

 public:
  // default constructor is cheap:
  // TODO: constexpr ??
  explicit stop_token() noexcept = default;

  // special members (all defaults are OK):
  //stop_token(const stop_token&) noexcept = default;
  //stop_token(stop_token&&) noexcept = default;
  //stop_token& operator=(const stop_token&) noexcept = default;
  //stop_token& operator=(stop_token&&) noexcept = default;
  //~stop_token() noexcept = default;
  void swap(stop_token& it) noexcept {
    _sp.swap(it._sp);
  }

  // stop handling:
  bool stop_signaled() const noexcept {
    return _sp && _sp->stopSignaled.load();
  }
  bool stop_done_or_possible() const {
    // yields if new callbacks could be called (immediately or later).
    // - true if already stopped or still sources exist that can stop it 
    // - thus:
    //   - true => false
    //      if not stopped yet and the last source is deleted
    //      if we swap/move/assign away the state 
    //   - false => true
    //      if we swap/assign away the state 
    return _sp && (stop_signaled() || _sp->numSources > 0);
  }

  friend bool operator== (const stop_token& lhs, const stop_token& rhs) {
    // TODO: just comparing _sp is enough?
    //return (!lhs.is_interruptible() && !rhs.is_interruptible())
    //       || (lhs._sp.get() == rhs._sp.get());
    return lhs._sp == rhs._sp;
  }
  friend bool operator!= (const stop_token& lhs, const stop_token& rhs) {
    return !(lhs==rhs);
  }
 
 private:
  // allow stop_source to create and return a token:
  friend class stop_source;
  explicit stop_token(::std::shared_ptr<SharedData> ip)
   : _sp{std::move(ip)} {
  }

  // allow stop_callback to register callbacks:
  // TODO: make only the base class a friend
  template <typename T>
  friend class ::std::stop_callback;
  void registerCB(stop_callback_base* cvPtr);
  void unregisterCB(stop_callback_base* cvPtr);
};



//***************************************** 
// stop_source
//***************************************** 
class stop_source {
 private:
  ::std::shared_ptr<SharedData> _sp{nullptr};

 public:
  // default constructor creates shared state
  // TODO: could/should probably be delayed until get_token()
  explicit stop_source() noexcept 
   : _sp{new SharedData{false}} {
  }
  // cheap constructor in case we need it:
  // - see e.g. class jthread
  // - same as moved_from state
  // TODO: not necessary wen get_token creates shared state
  explicit stop_source(std::nullptr_t) noexcept 
   : _sp{nullptr} {
  }

  // special members (default OK):
  stop_source(const stop_source& is) noexcept
   : _sp{is._sp} {
       if (_sp) {
         ++(_sp->numSources);
       }
  }
  stop_source(stop_source&&) noexcept = default;
  // pass by value to do the increments/decrements right:
  stop_source& operator=(stop_source is) noexcept {
    swap(is);
    return *this;
  }
  ~stop_source () {
     if (_sp) {
       --(_sp->numSources);
     }
  }

  void swap(stop_source& it) noexcept {
    _sp.swap(it._sp);
  }

  // get_token()
  // TODO: non-const wen get_token creates shared state
  stop_token get_token() const noexcept {
    return stop_token{_sp};  // using private constructor of stop_token
  }
  
  // stop handling:
  bool is_valid() const {
    return _sp != nullptr;
  }
  bool stop_signaled() const noexcept {
    return _sp && _sp->stopSignaled.load();
  }

  bool signal_stop();
  
  friend bool operator== (const stop_source& lhs, const stop_source& rhs) {
    // TODO: just comparing _sp is enough?
    //return (!lhs.is_valid() && !rhs.is_valid())
    //       || (lhs._sp.get() == rhs._sp.get());
    return lhs._sp.get() == rhs._sp.get();
  }
  friend bool operator!= (const stop_source& lhs, const stop_source& rhs) {
    return !(lhs==rhs);
  }
};


//***************************************** 
// stop_callback
//***************************************** 
// TODO: with concepts: template <Invocable Callback>
template <typename Callback>
struct stop_callback : stop_callback_base
{
  private:
    stop_token stoken;
    Callback callback;
  public:
    stop_callback(stop_token it, Callback&& cb)
     : stoken{std::move(it)}, callback{std::forward<Callback>(cb)} {
        if (stoken.stop_done_or_possible()) {
          stoken.registerCB(this);
        }
    }
    ~stop_callback() {
        if (stoken.stop_done_or_possible()) { // TODO: OK not to unregister if possible when registering?
          stoken.unregisterCB(this);
        }
    }
    stop_callback(const stop_callback&) = delete;
    stop_callback(stop_callback&&) = delete;
    stop_callback& operator=(const stop_callback&) = delete;
    stop_callback& operator=(stop_callback&&) = delete;
  private:
    virtual void run() noexcept override {
      callback();  //or: std::invoke(callback);
      finishedExecuting.store(true);
    }
};



//***************************************** 
// key member function for registering and calling callbacks
//***************************************** 

bool stop_source::signal_stop()
{
  std::cout<<std::this_thread::get_id()<<": signalstop "<<std::endl;
  //std::cout.put('I').flush();
  if (!is_valid()) return false;
  auto wasStopSignaled = _sp->stopSignaled.exchange(true);
  if (!wasStopSignaled) {
      _sp->stoppingThreadID = std::this_thread::get_id();
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
  return wasStopSignaled;
}

void stop_token::registerCB(stop_callback_base* cbPtr) {
  std::cout.put('R').flush();
  if (!stop_done_or_possible()) return;

  std::unique_lock lg{_sp->cbDataMutex};
  if (_sp->stopSignaled.load()) {
    // if stop already signaled, make sure the callback is durectly called
    // - but not blocking others
    lg.unlock();
    cbPtr->run();
  }
  else {
    _sp->cbData.emplace_front(cbPtr);  // might throw
  }
  std::cout.put('r').flush();
}

void stop_token::unregisterCB(stop_callback_base* cbPtr) {
  //std::cout.put('U').flush();
  if (!stop_done_or_possible()) return;

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
  if (std::this_thread::get_id() == _sp->stoppingThreadID) {
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
    // different thread than where stop was called:
    // => wait for the end:
    // TODO: busy waiting, better use a binary semaphore or a futex
    while (!cbPtr->finishedExecuting.load()) {
      std::this_thread::yield();
    }
  }
  //std::cout.put('u').flush();
}


} // namespace std

#endif // STOP_TOKEN_HPP
