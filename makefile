CC=g++

INCLUDE_DIR=inc
EXTERNAL_DIR=ext
SOURCE_DIR=src
PROJECT_DIR=agentsim
LIBRARY_DIR=lib
BUILD_DIR=bin

SOURCE:=$(shell find $(SOURCE_DIR)/$(PROJECT_DIR) -name *.cpp)

TARGET=agentsim
TEST_TARGET=agentsim_tests

VPATH= $(SOURCE_DIR):$(INCLUDE_DIR)
CFLAGS=-Wall -g
INC= -I $(INCLUDE_DIR)

all: agentsim.a agentsim.o agentsim_tests.o

agentsim.a: $(SOURCE)
	$(CC) \
	-c $(SOURCE) \
	$(CFLAGS) \
	-I $(INCLUDE_DIR) -I $(EXTERNAL_DIR) \
	-o $(BUILD_DIR)/$(TARGET).o

	ar rvs $(LIBRARY_DIR)/$(TARGET).a $(BUILD_DIR)/$(TARGET).o

agentsim.o: $(SOURCE_DIR)/agentsim.cpp
	$(CC) $(SOURCE_DIR)/agentsim.cpp \
	$(CFLAGS) \
	-I $(INCLUDE_DIR) -I $(EXTERNAL_DIR) \
	-o $(BUILD_DIR)/$(TARGET) \
	$(LIBRARY_DIR)/$(TARGET).a

agentsim_tests.o: $(SOURCE_DIR)/agentsim_tests.cpp
	$(CC) $(SOURCE_DIR)/agentsim_tests.cpp \
	$(CFLAGS) \
	-I $(INCLUDE_DIR) -I $(EXTERNAL_DIR) \
	-o $(BUILD_DIR)/$(TEST_TARGET) \
	$(LIBRARY_DIR)/gtest_main.a -pthread \
	$(LIBRARY_DIR)/$(TARGET).a

clean:
	rm -f ./bin/*
	rm -f ./$(LIBRARY_DIR)/$(TARGET).a
