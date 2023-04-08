
# Test files to be run through grm to generate a test runner `main.cpp'. 
TEST_FILES = test_printf.cpp

RM = rm -rf

# Scrape test runner from test files.
main.cpp : $(TEST_FILES)
	./grm.py -v -o $@ $^

.PHONY : clean
clean :
	$(RM) main.cpp

