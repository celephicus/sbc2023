// version.h
// This file is edited by version_mk.py before a build, so it records the build timestamp and build number.
// So don't mess with the formatting too much.

// Yes you need to do it TWICE because the first expansion will give you the symbol, the second expands it to the symbol's value.
#define VERSION_STRINGIFY(x) #x
#define VERSION_TO_STRING(x) VERSION_STRINGIFY(x)

// Build number incremented with each build. 
#define VERSION_BUILD_NUMBER 2

#define VERSION_BUILD_NUMBER_STR VERSION_TO_STRING(VERSION_BUILD_NUMBER)

// Timestamp in ISO8601 format.
#define VERSION_BUILD_TIMESTAMP "20220925T112148"

// Version info, set by manual editing. 
#define VERSION_VER_MAJOR 0     // For prototype board. 
#define VERSION_VER_MINOR 0

// Version as an integer.
#define VERSION_VER ((VERSION_VER_MAJOR * 100) + (VERSION_VER_MINOR))

// Version as a string.
#define VERSION_VER_STR VERSION_TO_STRING(VERSION_VER_MAJOR) "." VERSION_TO_STRING(VERSION_VER_MINOR) 

// Product name
#define VERSION_PRODUCT_NAME_STR "TSA SBC2022 Relay Module"

// Banner string combining all the information.
#define VERSION_BANNER_VERBOSE_STR VERSION_PRODUCT_NAME_STR " V" VERSION_VER_STR " (" VERSION_BUILD_NUMBER_STR ") " VERSION_BUILD_TIMESTAMP

// EOF
