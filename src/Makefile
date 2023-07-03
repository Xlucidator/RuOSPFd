include ../config.mk

# Source files
SRCS = $(wildcard *.cpp)

# Object files
OBJS = $(SRCS:.cpp=.o)	# replace all '.cpp' with '.o' in SRCS

.PHONY: all clean

# Final Target
all : $(TARGET)

# Compile object files from source files
%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# TODO: need more relationship like 'lsa_manage.o : ospf_packet.o' and so on

# Link object files to executable
$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $(TARGET)
# sudo ./$(TARGET)

clean:
	rm -rf $(TARGET) *.o
