include Makefile.h

default: all
all:: test_stoken test_stokencb test_jthread1 test_jthread2 test_cv test_cvcb test_cvrace test_cvprodcons
all::
	@echo ""
	@echo "Testcases:"
	@echo "  test_stoken"
	@echo "  test_stokencb"
	@echo "  test_jthread1"
	@echo "  test_jthread2"
	@echo "  test_cv"
	@echo "  test_cvcb"
	@echo "  test_cvrace"
	@echo "  test_cvprodcons"

test_stoken: stop_token.hpp condition_variable_any2.hpp test_stoken.cpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_stoken.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_stoken: test_stoken
	./test_stoken17raw.exe

test_stokencb: stop_token.hpp condition_variable_any2.hpp test_stokencb.cpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_stokencb.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_stokencb: test_stokencb
	./test_stokencb17raw.exe

test_jthread1: stop_token.hpp condition_variable_any2.hpp jthread.hpp test_jthread1.cpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_jthread1.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_jthread1: test_jthread1
	./test_jthread117raw.exe

test_jthread2: stop_token.hpp condition_variable_any2.hpp jthread.hpp test_jthread2.cpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_jthread2.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_jthread2: test_jthread2
	./test_jthread217raw.exe

test_cv: stop_token.hpp jthread.hpp condition_variable_any2.hpp test_cv.cpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_cv.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_cv: test_cv
	./test_cv17raw.exe

test_cvcb: stop_token.hpp jthread.hpp condition_variable_any2.hpp test_cvcb.cpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_cvcb.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_cvcb: test_cvcb
	./test_cvcb17raw.exe

test_cvrace: stop_token.hpp jthread.hpp condition_variable_any2.hpp test_cvrace.cpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_cvrace.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_cvrace: test_cvrace
	./test_cvrace17raw.exe

test_cvprodcons: stop_token.hpp jthread.hpp condition_variable_any2.hpp test_cvprodcons.cpp Makefile
	$(CXX17) $(CXXFLAGS17) $(INCLUDES) test_cvprodcons.cpp $(LDFLAGS17) -o $@17raw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@17.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@17raw.exe '$$*' > $@.exe
	echo "- OK:  $@ and $@17  call  $@17raw.exe"

run_cvprodcons: test_cvprodcons
	./test_cvprodcons17raw.exe

jthread.clang: jthread.hpp jthread.cpp stop_token.hpp iwait.hpp Makefile
	$(CXXCLANG) $(CXXFLAGSCLANG) -std=c++1z $(INCLUDES) jthread.cpp $(LDFLAGSCLANG) -o $@clangraw.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@clangraw.exe '$$*' > $@clang.exe
	echo PATH=\"$(PATH17)/bin:$$PATH\" ./$@clangraw.exe '$$*' > $@.exe
	echo "- OK:  $@ and $@17  call  $@clangraw.exe"

run_tests: run_cvcb run_cvrace run_cvprodcons run_cv run_jthread2 run_jthread1 run_stokencb run_stoken
