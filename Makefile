# kwz-restoration Makefile

## flags
common_flags = -std=c++11 -march=native -Wall -Wextra -Wpedantic
optimization = -O3
infile = kwz-restoration.cpp
outfile = -o kwz-restoration

kwz-restoration: kwz-restoration.cpp
	g++ $(common_flags) $(optimization) $(infile) $(outfile)
