# C++ Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -std=c++11

# Linker flags
LDFLAGS = -pthread

# Source files
SRCS = $(wildcard *.cpp)

# Object files
OBJS = $(SRCS:.cpp=.o)	# replace all '.cpp' with '.o' in SRCS

# Executable file
TARGET = my_ospf

.PHONY: all clean

# Final Target
all : $(TARGET)

# Compile object files from source files
%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link object files to executable
$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $(TARGET)
	sudo ./$(TARGET)

clean:
	rm -rf $(TARGET) *.o
