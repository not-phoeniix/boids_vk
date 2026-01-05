# compiler and flags (for compiling and linking)
CXX := g++
PRE_FLAGS := -m64 -g -Wall -std=c++20
POST_FLAGS := -lglfw -lvulkan -ldl
SHADER_CXX := glslc
SHADER_FLAGS :=

# directories
SRC_DIR := src
SHADER_SRC_DIR := shaders
RES_DIR := res
LIB_DIR := include
BIN_DIR := bin
OBJ_DIR := $(BIN_DIR)/obj
SHADER_BIN_DIR := $(BIN_DIR)/shaders
RES_COPY_DIR := $(BIN_DIR)/res

# files
SRC := $(shell find $(SRC_DIR)/ -type f -iname "*.cpp")
OBJ := $(subst $(SRC_DIR),$(OBJ_DIR),$(foreach file,$(basename $(SRC)),$(file).o))
BIN_NAME := build
BIN := $(BIN_DIR)/$(BIN_NAME)
SHADER_SRC := $(shell find $(SHADER_SRC_DIR)/ -type f)
SHADER_BIN := $(subst $(SHADER_SRC_DIR),$(SHADER_BIN_DIR),$(foreach file,$(basename $(SHADER_SRC)),$(file).spv))
RES_SRC := $(shell find $(RES_DIR)/ -type f)
RES_OUT := $(subst $(RES_DIR),$(RES_COPY_DIR),$(RES_SRC))

# === build tasks =========================================

all: $(RES_OUT) $(SHADER_BIN) $(BIN)

$(BIN): $(OBJ)
	@echo "linking..."
	@$(CXX) $(OBJ) $(POST_FLAGS) -o $(BIN)
	@echo "done :D"

.SECONDEXPANSION:

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $$(dir $$@)
	@echo "compiling $<..."
	@$(CXX) -c $< $(PRE_FLAGS) -I $(LIB_DIR) -o $@

$(SHADER_BIN_DIR)/%.spv: $(SHADER_SRC_DIR)/%.* | $$(dir $$@)
	@echo "compiling $<..."
	@$(SHADER_CXX) $< $(SHADER_FLAGS) -o $@

$(RES_COPY_DIR)/%: $(RES_DIR)/% | $$(dir $$@)
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
	rm -rf $(BIN) 
	rm -rf $(SHADER_BIN_DIR)/*
	rm -rf $(OBJ_DIR)/*
	rm -rf $(RES_COPY_DIR)/*
	@echo "project cleaned!"

run: $(RES_OUT) $(SHADER_BIN) $(BIN)
	@echo "running $(BIN)..."
	@cd $(BIN_DIR) && ./$(BIN_NAME)

# make dirs and create main file
setup:
	@echo "setting up project..."

	@echo "creating directories..."
	mkdir -p $(SRC_DIR) $(BIN_DIR) $(OBJ_DIR) $(LIB_DIR) $(SHADER_SRC_DIR) $(RES_DIR)

	@echo "creating main.cpp..."
	@echo "#include <iostream>" >> $(SRC_DIR)/main.cpp
	@echo "" >> $(SRC_DIR)/main.cpp
	@echo "int main() {" >> $(SRC_DIR)/main.cpp
	@echo "	std::cout << \"hello world!!\n\";">> $(SRC_DIR)/main.cpp
	@echo "" >> $(SRC_DIR)/main.cpp
	@echo "	return 0;" >> $(SRC_DIR)/main.cpp
	@echo "}" >> $(SRC_DIR)/main.cpp

	@echo "set up project!!"
