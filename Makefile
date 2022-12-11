# COMPILER, TOOLS AND OPTIONS
CXX = g++
CXXFLAGS = -g -Wall -Wextra -std=c++11

.PHONY: all clean run

all: player server server_udp

# PLAYER
player: playerApp/player.o
		g++ $(CXXFLAGS) -o player playerApp/player.o

playerApp/player.o: playerApp/player.cpp
		g++ $(CXXFLAGS) -o playerApp/player.o -c playerApp/player.cpp

#SERVER
server: gameServer/server.o 
		g++ $(CXXFLAGS) -o server gameServer/server.o

server_udp: gameServer/server_udp.o
		g++ $(CXXFLAGS) -o server_udp gameServer/server_udp.o

gameServer/server.o: gameServer/server.cpp 
		g++ $(CXXFLAGS) -o gameServer/server.o -c gameServer/server.cpp

gameServer/server_udp.o: gameServer/server_udp.cpp
		g++ $(CXXFLAGS) -o gameServer/server_udp.o -c gameServer/server_udp.cpp


clean: 
	@echo Cleaning... 
	rm -f playerApp/*.o gameServer/*.o player server server_udp
