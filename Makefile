CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2
LIBS = -lGL -lGLU -lglut -lm

TARGET = ball_pivoting
SOURCE = ball_pivoting.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE) $(LIBS)

clean:
	rm -f $(TARGET)

install-deps:
	sudo apt-get update
	sudo apt-get install freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev

.PHONY: all clean install-deps