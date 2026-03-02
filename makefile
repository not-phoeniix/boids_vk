# compiler and flags (for compiling and linking)
CXX := g++
PRE_FLAGS := -m64 -g -Wall -std=c++20
POST_FLAGS := -lglfw -lvulkan -ldl -lrender_thing
SHADER_CXX := slangc
SHADER_FLAGS := -target spirv -profile spirv_1_4 -fvk-use-entrypoint-name

# directories
SRC_DIR := src
SHADER_SRC_DIR := shaders
RES_DIR := res
INC_DIR := include
LIB_DIR := lib
BIN_DIR := bin
OBJ_DIR := $(BIN_DIR)/obj
LIB_COPY_DIR := $(BIN_DIR)/lib
SHADER_BIN_DIR := $(BIN_DIR)/shaders
RES_COPY_DIR := $(BIN_DIR)/res

# files
SRC := $(shell find $(SRC_DIR)/ -type f -iname "*.cpp")
OBJ := $(subst $(SRC_DIR),$(OBJ_DIR),$(foreach file,$(basename $(SRC)),$(file).o))
BIN_NAME := build
BIN := $(BIN_DIR)/$(BIN_NAME)
SHADER_SRC := $(shell find $(SHADER_SRC_DIR)/ -type f -iname "*.slang")
SHADER_BIN := $(subst $(SHADER_SRC_DIR),$(SHADER_BIN_DIR),$(foreach file,$(basename $(SHADER_SRC)),$(file).spv))
RES_SRC := $(shell find $(RES_DIR)/ -type f)
LIB_SRC	:= $(shell find $(LIB_DIR)/ -type f)
RES_OUT := $(patsubst $(RES_DIR)%,$(RES_COPY_DIR)%,$(RES_SRC))
LIB_OUT	:= $(patsubst $(LIB_DIR)%,$(LIB_COPY_DIR)%,$(LIB_SRC))

# === build tasks =========================================

all: $(RES_OUT) $(LIB_OUT) $(SHADER_BIN) $(BIN)

$(BIN): $(OBJ)
	@echo "linking..."
	@$(CXX) $(OBJ) -L$(LIB_DIR) $(POST_FLAGS) -o $(BIN)
	@echo "done :D"

.SECONDEXPANSION:

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $$(dir $$@)
	@echo "compiling $<..."
	@$(CXX) -c $< $(PRE_FLAGS) -I$(INC_DIR) -o $@

$(SHADER_BIN_DIR)/%.spv: $(SHADER_SRC_DIR)/%.slang | $$(dir $$@)
	@echo "compiling $<..."
	@$(SHADER_CXX) $< $(SHADER_FLAGS) -o $@

$(RES_COPY_DIR)/%: $(RES_DIR)/% | $$(dir $$@)
	@echo "copying $< to $@"
	@cp $< $@

$(LIB_COPY_DIR)/%: $(LIB_DIR)/% | $$(dir $$@)
	@echo "copying $< to $@"
	@cp $< $@

# ensure directories are created via custom task
%/:
	@mkdir -p $@

.PRECIOUS: %/

# === utility tasks =======================================

.PHONY: clean run setup

clean:
	@echo "cleaning project..."
	rm -rf $(BIN_DIR)/*
	@echo "project cleaned!"

run: $(RES_OUT) $(LIB_OUT) $(SHADER_BIN) $(BIN)
	@echo "running $(BIN)..."
	@cd $(BIN_DIR) && export LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:./$(LIB_DIR) && ./$(BIN_NAME)

# make dirs and create main file
setup:
	@echo "setting up project..."

	@echo "creating directories..."
	mkdir -p $(SRC_DIR) $(BIN_DIR) $(OBJ_DIR) $(INC_DIR) $(SHADER_SRC_DIR) $(RES_DIR) $(LIB_DIR)

	@echo "creating main.cpp..."
	@echo "#include <iostream>" >> $(SRC_DIR)/main.cpp
	@echo "" >> $(SRC_DIR)/main.cpp
	@echo "int main() {" >> $(SRC_DIR)/main.cpp
	@echo "	std::cout << \"hello world!!\n\";">> $(SRC_DIR)/main.cpp
	@echo "" >> $(SRC_DIR)/main.cpp
	@echo "	return 0;" >> $(SRC_DIR)/main.cpp
	@echo "}" >> $(SRC_DIR)/main.cpp

	@echo "set up project!!"
