all:
	g++ labeler.cpp -o label `pkg-config --cflags opencv` `pkg-config --libs opencv` --std=c++11
