# C++ Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall Wextra -std=c++11

# Linker flags
LDFLAGS = 

# Source files
SRCS = $(wildcard *.cpp)

# Object files
OBJS = $(SRCS:.cpp=.o)	# replace all '.cpp' with '.o' in SRCS

# Executable file
TARGET = my_ospf

# Final Target
all : $(TARGET)

# Compile object files from source files
%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link object files to executable
$(EXEC): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $(TARGET)

clean:
	rm -rf $(TARGET) *.o

my_ospfd: main.c