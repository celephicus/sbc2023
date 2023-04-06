// Don't forget to define UNITY_INCLUDE_CONFIG_H to include this file.

// Dummy macros read by test runner generator.
#define TT_BEGIN_FIXTURE(...)
#define TT_END_FIXTURE()
#define TT_TEST_CASE(...)
#define TT_IGNORE_FROM_HERE()
#define TT_BEGIN_INCLUDE()
#define TT_END_INCLUDE()
#define TT_BEGIN_SCRIPT()
#define TT_END_SCRIPT()
#define TT_IF(...)
#define TT_ENDIF()

// Function to print test context on failure. Defined in autogenerated file.
void dumpTestContext();

// Make Unity call our dump function for failing tests.
#define UNITY_PRINT_TEST_CONTEXT dumpTestContext

// Pretty...
// #define UNITY_OUTPUT_COLOR


