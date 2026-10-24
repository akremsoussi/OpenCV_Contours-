CXX = g++
PKG_CONFIG ?= pkg-config
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags opencv4)
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
LDLIBS += $(shell $(PKG_CONFIG) --libs opencv4)

TARGET = contours
IMAGES ?= images
REFERENCES ?= images/gt
OUTPUT ?= resultats

.PHONY: all run clean

all: $(TARGET)

$(TARGET): contours.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $< -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET) "$(IMAGES)" "$(REFERENCES)" "$(OUTPUT)"


clean:
	$(RM) $(TARGET)
