CC=g++

INCLUDE_DIR=inc
EXTERNAL_DIR=ext
SOURCE_DIR=src
OBJ_DIR=obj
PROJECT_DIR=agentsim
LIBRARY_DIR=lib
BUILD_DIR=bin

SOURCE:=$(shell find $(SOURCE_DIR)/$(PROJECT_DIR) -name *.cpp)
OBJ_FILES:=$(patsubst $(SOURCE_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCE))

TARGET=agentsim
TEST_TARGET=agentsim_tests

CFLAGS=-Wall -g
INC= -I $(INCLUDE_DIR)

all: prep agentsim_prog agentsim_lib agentsim_tests

prep:
	mkdir -p ./$(OBJ_DIR)/$(PROJECT_DIR)
	mkdir -p ./$(BUILD_DIR)

agentsim_prog: $(OBJ_FILES)
	$(CC) $(SOURCE_DIR)/agentsim.cpp \
	$(CFLAGS) \
	-I $(INCLUDE_DIR) -I $(EXTERNAL_DIR) \
	-o $(BUILD_DIR)/$(TARGET) \
	$(OBJ_FILES)

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CC) $(CFLAGS) -c $< -I $(INCLUDE_DIR) -I $(EXTERNAL_DIR) -I $(EXTERNAL_DIR)/openmm -o $@

agentsim_lib:
	ar rvs $(LIBRARY_DIR)/$(TARGET).a $(OBJ_DIR)/$(PROJECT_DIR)/*.o

agentsim_tests: $(SOURCE_DIR)/agentsim_tests.cpp
	$(CC) $(SOURCE_DIR)/agentsim_tests.cpp \
	$(CFLAGS) \
	-I $(INCLUDE_DIR) -I $(EXTERNAL_DIR) \
	-o $(BUILD_DIR)/$(TEST_TARGET) \
	$(LIBRARY_DIR)/gtest_main.a -pthread \
	$(LIBRARY_DIR)/$(TARGET).a

clean:
	rm -f ./bin/*
	rm -f ./$(LIBRARY_DIR)/$(TARGET).a
	rm -f ./$(OBJ_DIR)/$(PROJECT_DIR)/*.o
