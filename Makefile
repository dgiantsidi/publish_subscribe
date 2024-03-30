export CXX = g++
export FLAGS = -Wall -O3 -g -std=c++17 -fsanitize=address
BIN=main

target: main

main: clean
	$(CXX) $(FLAGS) $(BIN).cpp -lfmt -o $(BIN)

clean: 
	rm -f $(BIN)
