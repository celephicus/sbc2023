# We either target myprintf alone or all files by setting to myprintf or all, note lower case. 
TARGET = all

# Test case files to be run through grm to generate a test runner `main.cpp'. 
TEST_SRCS_myprintf = test_printf.cpp
TEST_SRCS_buffer = test_buffer.cpp 
TEST_SRCS_all = $(wildcard test_*.cpp)

# Other src files.
OTHER_SRCS_myprintf = ../src/myprintf.cpp
OTHER_SRCS_buffer = ../src/buffer.cpp
OTHER_SRCS_all = ../src/buffer.cpp ../src/myprintf.cpp ../src/event.cpp ../src/modbus.cpp \
				../src/utils.cpp support_test.cpp
#console.cpp regs.cpp  sw_scanner.cpp  thread.cpp 

# Select source files, maybe use use local symbols instead. 
TEST_SRCS = $(TEST_SRCS_$(TARGET))
OTHER_SRCS = $(OTHER_SRCS_$(TARGET))

# Disable built-in rules and variables as they always trip me up. 
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables

# All build output including the binary ends up is this dir.
BUILD_PREFIX = build
BUILD_DIR = $(BUILD_PREFIX)-$(TARGET)$(BUILD)

# Extra options for C & C++
EXTRAS =
DEFINES = -DUNITY_INCLUDE_CONFIG_H -DTEST -DNO_CRITICAL_SECTIONS -DUSE_PROJECT_CONFIG_H \
		    -DMYPRINTF_TEST_BINARY=1 
LINK_FLAGS = 
INCLUDES = -I. -I../include 
LIBS = -lgcov
LIB_PATH =

# Where make searches for prerequisites.
VPATH = ../src ../test

# My current set of warnings.
WARN_FLAGS := -Wall -Wextra -Wno-unused-parameter -Werror -Wsign-conversion \
				-Wduplicated-branches -Wduplicated-cond \
				-Wlogical-op -Wshadow -Wundef -Wswitch-default -Wdouble-promotion -fno-common 
CXXFLAGS :=  -g -O0 $(WARN_FLAGS) $(DEFINES) $(EXTRAS) -fprofile-arcs -ftest-coverage
CFLAGS   :=  -g -O0 $(WARN_FLAGS) -Wmissing-prototypes $(DEFINES) $(EXTRAS) -fprofile-arcs -ftest-coverage


TEST_MAIN_SRC = t_$(TARGET)
SRCS = $(TEST_MAIN_SRC).cpp $(OTHER_SRCS) $(TEST_SRCS) unity.c 

# Commands
RM 		:= rm -rf
MKDIR 	:= mkdir -p
CXX     := /usr/bin/g++
CC      := /usr/bin/gcc
LINK	:= /usr/bin/g++
TOUCH	:= touch

# Ensure we have a build dir.
_dummy := $(shell $(MKDIR) $(BUILD_DIR))

EXE = $(BUILD_DIR)/$(TEST_MAIN_SRC)
OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SRCS)))))

.PHONY : clean all clean-all verify test-quiet

# Main target.
all : $(EXE)

verify:
	@echo "SRCS: " $(SRCS)
	@echo "OBJS: " $(OBJS)
	@echo $(wildcard test_*.cpp)
	
$(EXE) : $(OBJS) 
	@echo $(OBJS) > $(BUILD_DIR)/obj_files
	$(LINK) -o $@ @$(BUILD_DIR)/obj_files $(LIB_PATH) $(LIBS) $(LINK_FLAGS)

# Scrape test runner from test files.
$(TEST_MAIN_SRC).cpp : $(TEST_SRCS)
	./grm.py -v -o $@ $^

clean :
	$(RM) $(BUILD_DIR) $(TEST_MAIN_SRC).cpp

clean-all :
	$(RM) $(BUILD_PREFIX)* t_*.cpp
	
$(BUILD_DIR)/%.o : %.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MG -MP -MT$@ -MF$(addsuffix .d, $(basename $@)) -MM $<
	$(CXX) -c $< $(CXXFLAGS) -o $@ $(INCLUDES)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -E -o $(addsuffix .i, $(basename $@)) $<

$(BUILD_DIR)/%.o : %.c
	@$(CC) $(CFLAGS) $(INCLUDES) -MG -MP -MT$@ -MF$(addsuffix .d, $(basename $@)) -MM $<
	$(CC) -c $< $(CFLAGS) -o $@ $(INCLUDES)
	@$(CC) $(CFLAGS) $(INCLUDES) -E -o $(addsuffix .i, $(basename $@)) $<

-include $(BUILD_DIR)/*.d

test-quiet : $(EXE)
	$(EXE) -f
	
# Coverage
coverage : test-quiet
	lcov --capture --directory . --output-file $(BUILD_DIR)/coverage.info
	genhtml $(BUILD_DIR)/coverage.info --output-directory $(BUILD_DIR)