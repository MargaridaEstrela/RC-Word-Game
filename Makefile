# COMPILER, TOOLS AND OPTIONS
CXX = g++
CXXFLAGS = -g -Wall -Wextra -std=c++11

.PHONY: all clean run
CXXSOURCES = ./playerApp/*.cpp ./gameServer/*.cpp aux_functions.cpp aux_functions.hpp constants.hpp
 
all: player server server_udp


# PLAYER
player: playerApp/player.o aux_functions.o
		g++ $(CXXFLAGS) -o player playerApp/player.o aux_functions.o

playerApp/player.o: playerApp/player.cpp aux_functions.hpp
		g++ $(CXXFLAGS) -o playerApp/player.o -c playerApp/player.cpp


#SERVER
server: gameServer/server.o 
		g++ $(CXXFLAGS) -o server gameServer/server.o

server_udp: gameServer/server_udp.o aux_functions.o gameServer/data.o 
		g++ $(CXXFLAGS) -o server_udp gameServer/server_udp.o aux_functions.o gameServer/data.o

gameServer/server.o: gameServer/server.cpp 
		g++ $(CXXFLAGS) -o gameServer/server.o -c gameServer/server.cpp

gameServer/server_udp.o: gameServer/server_udp.cpp gameServer/data.hpp aux_functions.hpp
		g++ $(CXXFLAGS) -o gameServer/server_udp.o -c gameServer/server_udp.cpp

gameServer/data.o: gameServer/data.cpp aux_functions.hpp 
		g++ $(CXXFLAGS) -o gameServer/data.o -c gameServer/data.cpp


# AUXIALIARY
aux_functions.o: aux_functions.cpp
		g++ $(CXXFLAGS) -o aux_functions.o -c aux_functions.cpp


clang-format: 
	@clang-format -i -style=Webkit $(CXXSOURCES) || true

clean: 
	@echo Cleaning... 
	rm -f ./*.o playerApp/*.o gameServer/*.o player server server_udp
