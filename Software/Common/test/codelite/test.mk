##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=test
ConfigurationName      :=Debug
WorkspaceConfiguration :=Debug
WorkspacePath          :=C:/Users/admin/Documents/git/misc/2022SBC/Software/Common/test/codelite
ProjectPath            :=C:/Users/admin/Documents/git/misc/2022SBC/Software/Common/test/codelite
IntermediateDirectory  :=build-$(WorkspaceConfiguration)
OutDir                 :=$(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=admin
Date                   :=23/09/2022
CodeLitePath           :="C:/Program Files/CodeLite"
MakeDirCommand         :=mkdir
LinkerName             :=C:/msys64/mingw64/bin/g++.exe
SharedObjectLinkerName :=C:/msys64/mingw64/bin/g++.exe -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputDirectory        :=C:/Users/admin/Documents/git/misc/2022SBC/Software/Common/test/codelite/build-$(WorkspaceConfiguration)/bin
OutputFile             :=build-$(WorkspaceConfiguration)\bin\$(ProjectName).exe
Preprocessors          :=$(PreprocessorSwitch)UNITY_INCLUDE_CONFIG_H $(PreprocessorSwitch)TEST 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :=$(IntermediateDirectory)/ObjectsList.txt
PCHCompileFlags        :=
RcCmpOptions           := 
RcCompilerName         :=C:/msys64/mingw64/bin/windres.exe
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch)../.. 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overridden using an environment variable
##
AR       := C:/msys64/mingw64/bin/ar.exe rcu
CXX      := C:/msys64/mingw64/bin/g++.exe
CC       := C:/msys64/mingw64/bin/gcc.exe
CXXFLAGS := -Wundef -Wzero-as-null-pointer-constant -Winit-self -Wshadow -Wunreachable-code -Wall -g -O0 -Og $(Preprocessors)
CFLAGS   := -Wundef -Winit-self -Wshadow -Wunreachable-code -Wall -g -O0 -Og $(Preprocessors)
ASFLAGS  := 
AS       := C:/msys64/mingw64/bin/as.exe


##
## User defined environment variables
##
CodeLiteDir:=C:\Program Files\CodeLite
Objects0=$(IntermediateDirectory)/up_main.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_test_buffer.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_unity.c$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: MakeIntermediateDirs $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@if not exist "$(IntermediateDirectory)" $(MakeDirCommand) "$(IntermediateDirectory)"
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@if not exist "$(IntermediateDirectory)" $(MakeDirCommand) "$(IntermediateDirectory)"
	@if not exist "$(OutputDirectory)" $(MakeDirCommand) "$(OutputDirectory)"

$(IntermediateDirectory)/.d:
	@if not exist "$(IntermediateDirectory)" $(MakeDirCommand) "$(IntermediateDirectory)"

PreBuild:
	@echo Executing Pre Build commands ...
	python ..\grm.py -v -o ..\main.cpp ..\test_buffer.cpp
	@echo Done


##
## Objects
##
$(IntermediateDirectory)/up_main.cpp$(ObjectSuffix): ../main.cpp $(IntermediateDirectory)/up_main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/admin/Documents/git/misc/2022SBC/Software/Common/test/main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_main.cpp$(DependSuffix): ../main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_main.cpp$(DependSuffix) -MM ../main.cpp

$(IntermediateDirectory)/up_main.cpp$(PreprocessSuffix): ../main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_main.cpp$(PreprocessSuffix) ../main.cpp

$(IntermediateDirectory)/up_test_buffer.cpp$(ObjectSuffix): ../test_buffer.cpp $(IntermediateDirectory)/up_test_buffer.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/admin/Documents/git/misc/2022SBC/Software/Common/test/test_buffer.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_test_buffer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_test_buffer.cpp$(DependSuffix): ../test_buffer.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_test_buffer.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_test_buffer.cpp$(DependSuffix) -MM ../test_buffer.cpp

$(IntermediateDirectory)/up_test_buffer.cpp$(PreprocessSuffix): ../test_buffer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_test_buffer.cpp$(PreprocessSuffix) ../test_buffer.cpp

$(IntermediateDirectory)/up_unity.c$(ObjectSuffix): ../unity.c $(IntermediateDirectory)/up_unity.c$(DependSuffix)
	$(CC) $(SourceSwitch) "C:/Users/admin/Documents/git/misc/2022SBC/Software/Common/test/unity.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_unity.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_unity.c$(DependSuffix): ../unity.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_unity.c$(ObjectSuffix) -MF$(IntermediateDirectory)/up_unity.c$(DependSuffix) -MM ../unity.c

$(IntermediateDirectory)/up_unity.c$(PreprocessSuffix): ../unity.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_unity.c$(PreprocessSuffix) ../unity.c


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r $(IntermediateDirectory)


