test:
	mkdir -p ./build
	./tests/cxxtest/bin/cxxtestgen --error-printer -o ./build/tests.cpp ./tests/*.h
	g++ -o ./build/test-runner -I ./tests/cxxtest ./build/tests.cpp
	./build/test-runner