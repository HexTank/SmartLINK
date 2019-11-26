#!/bin/bash
clang++ -std=c++11 -I ../3rdparty/asio/include Client/main.cpp -o Client/smartlink -lpthread
clang++ -std=c++11 VirtualSDImage/main.cpp -o VirtualSDImage/smartsd