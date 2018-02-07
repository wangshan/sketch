#!/bin/sh
g++ -Wall -fPIC -std=c++14 -c badlad.cpp
g++ -Wall -std=c++14 -shared -o libbadlad.so badlad.o

g++ -Wall -fPIC -std=c++14 -c plugin.cpp
g++ -Wall -std=c++14 -shared -o libplugin.so plugin.o -L. -lbadlad

g++ -Wall -std=c++14 client.cpp -o client -L. -lbadlad
