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
SERVER_TARGET=agentsim_server
TEST_TARGET=agentsim_tests

CFLAGS=-Wall -g
INCLUDES= -I $(INCLUDE_DIR) -I $(EXTERNAL_DIR)

EXT_INCLUDES=-isystem $(EXTERNAL_DIR)/openmm -isystem $(EXTERNAL_DIR)/readdy
EXT_CFLAGS=-Wl,-rpath $(LIBRARY_DIR)
EXT_DLLS:= -L$(LIBRARY_DIR) -lreaddy -lreaddy_model -ldl -pthread -lhdf5
SERVER_INCLUDES=-I $(EXTERNAL_DIR)/raknet
SERVER_DLLS=-Llib -lRakNetDLL

.SILENT:

build: begin all end
rebuild: begin clean all end

all: prep agentsim_lib agentsim_prog agentsim_tests
server: begin prep agentsim_lib agentsim_server end

begin:
	@echo begin build:
	@date

end:
	@echo end build:
	@date

prep:
	@echo creating build folders ...
	@mkdir -p ./$(OBJ_DIR)/$(PROJECT_DIR)
	@mkdir -p ./$(BUILD_DIR)

agentsim_lib: $(OBJ_FILES)
	@echo $(LIBRARY_DIR)/$(TARGET).a
	$(CC) $(SOURCE_DIR)/$(DUMMY_TARGET).cpp \
	$(CFLAGS) \
	-o $(BUILD_DIR)/$(DUMMY_TARGET) \
	$(OBJ_FILES)
	@ar rvs $(LIBRARY_DIR)/$(TARGET).a $(OBJ_DIR)/$(PROJECT_DIR)/*.o

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@echo $@
	$(CC) $(CFLAGS) -c $< $(INCLUDES) -o $@

agentsim_prog: $(SOURCE_DIR)/$(TARGET).cpp
	@echo $(BUILD_DIR)/$(TARGET)
	$(CC) $(SOURCE_DIR)/$(TARGET).cpp \
	$(CFLAGS) $(INCLUDES) \
	-o $(BUILD_DIR)/$(TARGET) \
	$(LIBRARY_DIR)/$(TARGET).a \

agentsim_server: $(SOURCE_DIR)/$(SERVER_TARGET).cpp
	@echo $(BUILD_DIR)/$(SERVER_TARGET)
	$(CC) $(SOURCE_DIR)/$(SERVER_TARGET).cpp \
	$(CFLAGS) $(EXT_CFLAGS) $(INCLUDES) $(SERVER_INCLUDES) \
	-o $(BUILD_DIR)/$(SERVER_TARGET) \
	$(LIBRARY_DIR)/$(TARGET).a $(SERVER_DLLS) \

agentsim_tests: $(SOURCE_DIR)/$(TEST_TARGET).cpp
	@echo $(BUILD_DIR)/$(TEST_TARGET)
	$(CC) $(SOURCE_DIR)/$(TEST_TARGET).cpp \
	$(CFLAGS) $(INCLUDES) \
	-o $(BUILD_DIR)/$(TEST_TARGET) \
	$(LIBRARY_DIR)/$(TARGET).a \
	$(LIBRARY_DIR)/gtest_main.a -pthread

clean:
	@echo cleaning build dir ...
	@rm -f ./bin/*
	@rm -f ./$(LIBRARY_DIR)/$(TARGET).a
	@rm -f ./$(OBJ_DIR)/$(PROJECT_DIR)/*.o
