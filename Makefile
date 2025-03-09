# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Include directories (adjust paths if necessary)
INCLUDES = -I./include

# Libraries to link
LDFLAGS = -lglfw -lGL -lGLEW -lglut -ldl -lGLU -pthread

# Source file
SRCS = \
src/main.cpp


# Output executable
TARGET = grapher


# Build rule
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

# Clean up build files
clean:
	rm -f $(TARGET)

.PHONY: all clean 
