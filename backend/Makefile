# Makefile til kaffeautomat backend

CXX = g++
CXXFLAGS = -Wall -std=c++17 -pthread
TARGET = server
SOURCES = main.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)
