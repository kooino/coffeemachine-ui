CXX = g++
CXXFLAGS = -Wall -std=c++17 -pthread
TARGET = server
SOURCES = main.cpp
LIBS = -lnfc

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

clean:
	rm -f $(TARGET)
