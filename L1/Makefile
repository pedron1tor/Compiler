CPP_FILES		:= $(wildcard src/*.cpp)
OBJ_FILES		:= $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))
CC_FLAGS		:= --std=c++17 -I./src -I../lib/PEGTL/include -I../lib -I../util -g3 -DDEBUG -pedantic -pedantic-errors -Werror=pedantic
CC				:= g++
PL_CLASS        := L1
COMPILER 		:= bin/$(PL_CLASS)

build: dirs $(COMPILER)

all: dirs $(COMPILER)

dirs: obj bin

obj:
	mkdir -p $@

bin:
	mkdir -p $@

$(COMPILER): $(OBJ_FILES)
	$(CC) $(LD_FLAGS) -o $@ $^

obj/%.o: src/%.cpp
	$(CC) $(CC_FLAGS) -c -o $@ $<

clean:
	rm -fr bin obj *.out *.o core.*

.PHONY: build all dirs clean
