REM Copy shared source files to a subdirectory for the use of this project.
attrib -R src\common\*.*
copy /Y ..\..\Common\*.* src\common
attrib +R src\common\*.*
