PROJECT = test

# All build output including the binary ends up is this dir.
BUILD_DIR = build$(BUILD)

# Test files to be run through grm to generate a test runner `main.cpp'. 
TEST_FILES = test_printf.cpp

# Extra options for C & C++
EXTRAS =
DEFINES = -DUNITY_INCLUDE_CONFIG_H -DTEST -DNO_CRITICAL_SECTIONS -DUSE_PROJECT_CONFIG_H \
		    -DMYPRINTF_TEST_BINARY=1 
LINK_FLAGS = 
INCLUDES = -I. -I../include -I../test
LIBS =
LIB_PATH =

# Where make searches for prerequisites.
VPATH = ../src ../test

# My current set of warnings.
WARN_FLAGS := -Wall -Wextra -Wno-unused-parameter -Werror -Wsign-conversion \
				-Wduplicated-branches -Wduplicated-cond \
				-Wlogical-op -Wshadow -Wundef -Wswitch-default -Wdouble-promotion -fno-common 
CXXFLAGS :=  -g -O0 $(WARN_FLAGS) $(DEFINES) $(EXTRAS)
CFLAGS   :=  -g -O0 $(WARN_FLAGS) -Wmissing-prototypes $(DEFINES) $(EXTRAS)

EXE = $(BUILD_DIR)/$(PROJECT)

#SRCS = main.cpp ../src/myprintf.cpp test_printf.cpp unity.c 
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/myprintf.o $(BUILD_DIR)/test_printf.o $(BUILD_DIR)/unity.o 

# Commands
RM 		:= rm -rf
MKDIR 	:= mkdir -p
CXX     := /usr/bin/g++
CC      := /usr/bin/gcc
LINK	:= /usr/bin/g++
TOUCH	:= touch

# Ensure we have a build dir.
_dummy := $(shell $(MKDIR) $(BUILD_DIR))

.PHONY : clean all clean-all

# Main target.
all : $(EXE)
$(EXE) : $(OBJS) 
	@echo $(OBJS) > $(BUILD_DIR)/obj_files
	$(LINK) -o $@ @$(BUILD_DIR)/obj_files $(LIB_PATH) $(LIBS) $(LINK_FLAGS)

# Scrape test runner from test files.
main.cpp : $(TEST_FILES)
	./grm.py -v -o $@ $^

clean :
	$(RM) $(BUILD_DIR) main.cpp

clean-all :
	$(RM) build*
	
$(BUILD_DIR)/%.o : %.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MG -MP -MT$@ -MF$(addsuffix .d, $(basename $@)) -MM $<
	$(CXX) -c $< $(CXXFLAGS) -o $@ $(INCLUDES)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -E -o $(addsuffix .i, $(basename $@)) $<

$(BUILD_DIR)/%.o : %.c
	@$(CC) $(CFLAGS) $(INCLUDES) -MG -MP -MT$@ -MF$(addsuffix .d, $(basename $@)) -MM $<
	$(CC) -c $< $(CFLAGS) -o $@ $(INCLUDES)
	@$(CC) $(CFLAGS) $(INCLUDES) -E -o $(addsuffix .i, $(basename $@)) $<

-include $(BUILD_DIR)/*.d

