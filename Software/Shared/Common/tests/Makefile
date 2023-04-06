PROJECT = test

# Test files to be run through grm to generate a test runner `main.cpp'. 
TEST_FILES = test_printf.cpp

DEFINES = -DUNITY_INCLUDE_CONFIG_H -DTEST -DNO_CRITICAL_SECTIONS -DUSE_PROJECT_CONFIG_H -DMYPRINTF_TEST_BINARY=1 
LINK_FLAGS = 
INCLUDES = -I. -I../include 
LIBS =
LIB_PATH =

CXXFLAGS :=  -g -O0 -Wall -Wextra -Wundef $(DEFINES)
CFLAGS   :=  -g -O0 -Wall -Wextra -Wundef $(DEFINES)

BUILD_DIR = build
EXE = $(BUILD_DIR)/$(PROJECT)

#SRCS = $(BUILD_DIR)/main.cpp ../src/myprintf.cpp test_printf.cpp unity.c 
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/myprintf.o $(BUILD_DIR)/test_printf.o $(BUILD_DIR)/unity.o 

# Commands
RM 		:= rm -rf
MKDIR 	:= mkdir -p
CXX     := /usr/bin/g++
CC      := /usr/bin/gcc
LINK	:= /usr/bin/g++
TOUCH	:= touch

.PHONY : clean all 

# Main target.
all : $(EXE)
$(EXE) : $(OBJS) 
	@$(MKDIR) $(@D)
	@echo $(OBJS) > $(BUILD_DIR)/obj_files
	$(LINK) -o $@ @$(BUILD_DIR)/obj_files $(LIB_PATH) $(LIBS) $(LINK_FLAGS)

# Scrape test runner from test files.
$(BUILD_DIR)/main.cpp : $(TEST_FILES)
	@$(MKDIR) $(@D)
	./grm.py -v -o $@ $^

clean :
	$(RM) $(BUILD_DIR)

#$(BUILD_DIR)/%.o : ../src/%.cpp
VPATH = ../src

$(BUILD_DIR)/%.o : %.cpp
	@$(MKDIR) $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MG -MP -MT$@ -MF$(addsuffix .d, $(basename $@)) -MM $<
	$(CXX) -c $< $(CXXFLAGS) -o $@ $(INCLUDES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -E -o $(addsuffix .i, $(basename $@)) $<

$(BUILD_DIR)/%.o : %.c
	@$(MKDIR) $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -MG -MP -MT$@ -MF$(addsuffix .d, $(basename $@)) -MM $<
	$(CC) -c $< $(CFLAGS) -o $@ $(INCLUDES)
	$(CC) $(CFLAGS) $(INCLUDES) -E -o $(addsuffix .i, $(basename $@)) $<

-include $(BUILD_DIR)/*.d

