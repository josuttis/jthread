include Makefile.h

default: all
all:: test_stoken test_stokencb test_stokenrace test_stopcb test_jthread1 test_jthread2
all:: test_cv test_cvcb test_cvrace test_cvrace_hh test_cvrace_stop test_cvrace_pred test_cvprodcons
all::
	@echo ""
	@echo "Testcases:"
	@echo "  test_stoken"
	@echo "  test_stokencb"
	@echo "  test_stokenrace"
	@echo "  test_stopcb"
	@echo "  test_jthread1"
	@echo "  test_jthread2"
	@echo "  test_cv"
	@echo "  test_cvcb"
	@echo "  test_cvrace"
	@echo "  test_cvrace_hh"
	@echo "  test_cvrace_stop"
	@echo "  test_cvrace_pred"
	@echo "  test_cvprodcons"

test_stoken: stop_token.hpp condition_variable_any2.hpp test_stoken.cpp test.hpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_stoken.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_stoken: test_stoken
	./test_stoken17raw.exe

test_stokencb: stop_token.hpp condition_variable_any2.hpp test_stokencb.cpp test.hpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_stokencb.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_stokencb: test_stokencb
	./test_stokencb17raw.exe

test_stokenrace: stop_token.hpp condition_variable_any2.hpp test_stokenrace.cpp test.hpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_stokenrace.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_stokenrace: test_stokenrace
	./test_stokenrace17raw.exe

test_stopcb: stop_token.hpp test_stopcb.cpp test.hpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_stopcb.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_stopcb: test_stopcb
	./test_stopcb17raw.exe

test_jthread1: stop_token.hpp condition_variable_any2.hpp jthread.hpp test_jthread1.cpp test.hpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_jthread1.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_jthread1: test_jthread1
	./test_jthread117raw.exe

test_jthread2: stop_token.hpp condition_variable_any2.hpp jthread.hpp test_jthread2.cpp test.hpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_jthread2.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_jthread2: test_jthread2
	./test_jthread217raw.exe

test_cv: stop_token.hpp jthread.hpp condition_variable_any2.hpp test_cv.cpp test.hpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_cv.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_cv: test_cv
	./test_cv17raw.exe

test_cvcb: stop_token.hpp jthread.hpp condition_variable_any2.hpp test_cvcb.cpp test.hpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_cvcb.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_cvcb: test_cvcb
	./test_cvcb17raw.exe

test_cvrace: stop_token.hpp jthread.hpp condition_variable_any2.hpp test_cvrace.cpp test.hpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_cvrace.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_cvrace: test_cvrace
	./test_cvrace17raw.exe

test_cvrace_hh: stop_token.hpp jthread.hpp condition_variable_any2.hpp test_cvrace_hh.cpp test.hpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_cvrace_hh.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_cvrace_hh: test_cvrace_hh
	./test_cvracehh17raw.exe

test_cvrace_stop: stop_token.hpp jthread.hpp condition_variable_any2.hpp test_cvrace_stop.cpp test.hpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_cvrace_stop.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_cvrace_stop: test_cvrace_stop
	./test_cvrace_stop17raw.exe

test_cvrace_pred: condition_variable_any2.hpp test_cvrace_pred.cpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_cvrace_pred.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_cvrace_pred: test_cvrace_pred
	./test_cvrace_pred17raw.exe

test_cvprodcons: stop_token.hpp jthread.hpp condition_variable_any2.hpp test_cvprodcons.cpp test.hpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_cvprodcons.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_cvprodcons: test_cvprodcons
	./test_cvprodcons17raw.exe

jthread.clang: jthread.hpp jthread.cpp stop_token.hpp iwait.hpp test.hpp Makefile
	$(CXXCLANG) $(CXXFLAGSCLANG) -std=c++1z $(INCLUDES) jthread.cpp $(LDFLAGSCLANG) -o $@clangraw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@clangraw.exe '$$*' > $@clang.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@clangraw.exe '$$*' > $@.exe
	@chmod +x $@.exe
	echo "- OK:  $@ and $@17  call  $@clangraw.exe"

run_tests: run_cvrace_stop run_cvrace_pred run_cvcb run_cvrace run_cvprodcons run_cv run_jthread2 run_jthread1 run_stokencb run_stoken
