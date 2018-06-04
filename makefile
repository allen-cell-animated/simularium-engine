CC=g++

INCLUDE_DIR=inc
EXTERNAL_DIR=ext
SOURCE_DIR=src
LIBRARY_DIR=lib
BUILD_DIR=bin

TARGET=agentsim
TEST_TARGET=agentsim_tests

VPATH= $(SOURCE_DIR):$(INCLUDE_DIR)
CFLAGS=-Wall -g
INC= -I $(INCLUDE_DIR)

all: agentsim.o agentsim_tests.o

agentsim.o: $(SOURCE_DIR)/agentsim.cpp
	$(CC) $(SOURCE_DIR)/agentsim.cpp $(CFLAGS) \
	-I $(INCLUDE_DIR) -I $(EXTERNAL_DIR) \
	-o $(BUILD_DIR)/$(TARGET)

agentsim_tests.o: $(SOURCE_DIR)/agentsim_tests.cpp
	$(CC) $(SOURCE_DIR)/agentsim_tests.cpp $(CFLAGS) \
	-I $(INCLUDE_DIR) -I $(EXTERNAL_DIR) \
	-o $(BUILD_DIR)/$(TEST_TARGET) \
	$(LIBRARY_DIR)/gtest_main.a -pthread

clean:
	rm ./bin/*
