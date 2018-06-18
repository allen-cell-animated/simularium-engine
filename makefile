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

DUMMY_TARGET=main
TARGET=agentsim
TEST_TARGET=agentsim_tests

CFLAGS=-Wall -g -Wl,-rpath $(LIBRARY_DIR)
INCLUDES= -I $(INCLUDE_DIR) -I $(EXTERNAL_DIR) \
-isystem $(EXTERNAL_DIR)/openmm -isystem $(EXTERNAL_DIR)/readdy
LIBRARIES:=$(shell find $(LIBRARY_DIR) -name *.a)
DLLS:= -L$(LIBRARY_DIR) -lreaddy -lreaddy_model -ldl -pthread -lhdf5

all: prep agentsim_lib agentsim_prog agentsim_tests

prep:
	mkdir -p ./$(OBJ_DIR)/$(PROJECT_DIR)
	mkdir -p ./$(BUILD_DIR)

agentsim_lib: $(OBJ_FILES)
	$(CC) $(SOURCE_DIR)/$(DUMMY_TARGET).cpp \
	$(CFLAGS) \
	-o $(BUILD_DIR)/$(DUMMY_TARGET) \
	$(OBJ_FILES)
	ar rvs $(LIBRARY_DIR)/$(TARGET).a $(OBJ_DIR)/$(PROJECT_DIR)/*.o

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CC) $(CFLAGS) -c $< $(INCLUDES) -o $@

agentsim_prog: $(SOURCE_DIR)/$(TARGET).cpp
		$(CC) $(SOURCE_DIR)/$(TARGET).cpp \
		$(CFLAGS) $(INCLUDES) \
		-o $(BUILD_DIR)/$(TARGET) \
		$(LIBRARIES) $(LIBRARY_DIR)/$(TARGET).a $(DLLS)\

agentsim_tests: $(SOURCE_DIR)/$(TEST_TARGET).cpp
	$(CC) $(SOURCE_DIR)/$(TEST_TARGET).cpp \
	$(CFLAGS) $(INCLUDES) \
	-o $(BUILD_DIR)/$(TEST_TARGET) \
	$(LIBRARIES) $(LIBRARY_DIR)/$(TARGET).a $(DLLS)\

clean:
	rm -f ./bin/*
	rm -f ./$(LIBRARY_DIR)/$(TARGET).a
	rm -f ./$(OBJ_DIR)/$(PROJECT_DIR)/*.o
