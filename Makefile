
all:
	g++ labeler.cpp -o label `pkg-config --cflags opencv` `pkg-config --libs opencv` --std=c++14 -Wextra -Wall -Wpedantic

clean:
	rm label

clangformat:
	clang-format --style=Google -i labeler.cpp
