test:
	mkdir -p ./build
	./tests/cxxtest/bin/cxxtestgen --error-printer -o ./build/tests.cpp ./tests/*.h
	g++ -o ./build/test-runner -I ./ -I ./tests/cxxtest -I ./tests/stubs ./tests/stubs/*.cpp ./build/tests.cpp ./Nightlight.cpp
	./build/test-runner