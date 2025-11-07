# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

# Automatically collect all .cpp files
SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)

# Executable name
TARGET = interrupts

# Build rule
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

# Compile .cpp to .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJ) $(TARGET)
