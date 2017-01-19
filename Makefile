TARGET=release/main
CFLAGS=-Wall -Wextra -O3 -flto -std=c++11
LFLAGS=-static-libgcc -O3 -flto \
	-lsfml-network -lsfml-system  \
	-lws2_32 -lwinmm
	
FILES=$(wildcard src/*.cpp) $(wildcard src/*.c)
FILES+=$(wildcard src/*/*.cpp) $(wildcard src/*/*.c)
FILES+=$(wildcard src/*/*/*.cpp) $(wildcard src/*/*/*.c)
FILES+=$(wildcard src/*/*/*/*.cpp) $(wildcard src/*/*/*/*.c)
FILES+=$(wildcard src/*/*/*/*/*.cpp) $(wildcard src/*/*/*/*/*.c)

OBJS=$(patsubst %,build/%.o,$(basename $(FILES:src/%=%)))
.PHONY: all clean
all: $(TARGET)
$(TARGET): $(OBJS)
	@echo "Linking..."
	@mkdir -p $(@D)
	@g++ $(OBJS) $(LFLAGS) -o $@
	@echo "Done."
build/%.o: src/%.cpp
	@echo "Compiling $<"
	@mkdir -p $(@D)
	@g++ -c $< $(CFLAGS) -o $@
	@g++ $< $(CFLAGS) -MT $@ -MM -MF build/$*.d
build/%.o: src/%.c
	@echo "Compiling $<"
	@mkdir -p $(@D)
	@g++ -c $< $(CFLAGS) -o $@
	@g++ $< $(CFLAGS) -MT $@ -MM -MF build/$*.d
-include $(OBJS:.o=.d)
%.hpp %.h %.cpp %.c:
clean:
	@rm -rf *.o $(TARGET) $(TARGET).exe build/
	@echo "Cleaned."
