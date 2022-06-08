CFLAGS = -std=c++17 -O2
INC=-I./src/headers
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

all: src/car_simulator.cpp
	g++ $(CFLAGS) $(INC) -o src/car_simulator src/car_simulator.cpp src/car_simulator.hpp $(LDFLAGS)

.PHONY: test clean

test: src/car_simulator
	cd src/; \
	./car_simulator

clean:
	rm -f src/car_simulator
