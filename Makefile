
labeler : labeler.cpp
	$(CXX) labeler.cpp -o labeler `pkg-config --cflags opencv` `pkg-config --libs opencv` -lboost_program_options --std=c++14 -Wextra -Wall -Wpedantic

clean:
	rm -f labeler

clangformat:
	clang-format --style=Google -i labeler.cpp
