### copyright 2021 David Anderson
### This Makefile is hereby placed in the Public Domain
### for anyone to use in any way.

all: dicheck trimtrailing

dicheck: src/dicheck.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) src/dicheck.cc -o dicheck

trimtrailing: src/trimtrailing.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) src/trimtrailing.cc -o trimtrailing

clean:
	-rm -f dicheck
	-rm -f trimtrailing
	-rm -f junkta 
	-rm -f junktb
	-rm -f junktc
	-rm -f junktd
	-rm -f junkte
	-rm -f junktf
	-rm -f test/testt-a
	-rm -f test/junk*
	-rm -f junk.difference
	-rm -f junktx
	-rm -f test/testt-a

installlocal: dicheck trimtrailing
	cp dicheck      ~/bin
	cp trimtrailing ~/bin

check: dicheck trimtrailing
	sh test/runtest.sh "./dicheck -h" "test/testcase"  test/baseta di
	sh test/runtest.sh "./dicheck"    "test/testcase"  test/basetb di
	sh test/runtest.sh "./dicheck -t" "test/testcase"  test/basetc di
	sh test/runtest.sh "./dicheck -l" "src/dicheck.cc" test/basetd di
	sh test/runtest.sh "./dicheck"    "test/testcase2" test/basete di
	sh test/runtest.sh "./dicheck"    "test/test.c"    test/basetf di
	sh test/runtest.sh "./dicheck -l" "src/trimtrailing.cc" test/basetg di
	cp test/testcase2  test/testt-a
	sh test/runtest.sh "./trimtrailing" "test/testt-a" test/basett-a tt
	rm -f test/testt-a
	@echo "PASS dicheck tests"
