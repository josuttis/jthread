#ifndef INTERRUPTED_HPP
#define INTERRUPTED_HPP

#include <sstream>
#include <thread>

namespace std {

//***************************************** 
//* new exception for interrupts
//* - intentionally NOT derived from std::exception
//***************************************** 
class interrupted
{
  public:
    explicit interrupted() noexcept = default;
    // defaults are OK:
    //explicit interrupted(const interrupted&) noexcept;
    //interrupted& operator=(const interrupted&) noexcept;
    const char* what() const noexcept;
};


//**********************************************************************

//***************************************** 
//* implementation of new exception for interrupts
//***************************************** 
inline const char* interrupted::what() const noexcept {
  ::std::ostringstream os;
  os << "thread " << ::std::this_thread::get_id() << " interrupted";
  static std::string s = os.str();
  return s.c_str();
}

} // namespace std

#endif // INTERRUPTED_HPP
