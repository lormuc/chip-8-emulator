target = program
lib = -lm -lSDL2 -lSDL2main
cc = g++
c_flags = \
-funsigned-char -Wall -Wextra -Wno-unused-parameter -std=c++14 -O3 # -g
obj = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
hdr = $(wildcard *.hpp)

all: $(target)

%.o: %.cpp $(hdr)
	$(cc) -c $(c_flags) $< -o $@

.PRECIOUS: $(target) $(obj)

$(target): $(obj)
	$(cc) -o $@ $(obj) -Wall $(lib)

clean:
	rm -f *.o
	rm -rf $(target)

.PHONY: all clean
