##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=test
ConfigurationName      :=Debug
WorkspacePath          :=/home/tom/git/misc/2022SBC/Software/Shared/Common/test
ProjectPath            :=/home/tom/git/misc/2022SBC/Software/Shared/Common/test
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Tom
Date                   :=18/10/22
CodeLitePath           :=/home/tom/.codelite
LinkerName             :=/usr/bin/g++
SharedObjectLinkerName :=/usr/bin/g++ -shared -fPIC
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
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=$(PreprocessorSwitch)UNITY_INCLUDE_CONFIG_H $(PreprocessorSwitch)TEST $(PreprocessorSwitch)NO_CRITICAL_SECTIONS 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="test.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch)../include 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -g -O0 -Wall -Wextra $(Preprocessors)
CFLAGS   :=  -g -O0 -Wall -Wextra $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/unity.c$(ObjectSuffix) $(IntermediateDirectory)/test_utils.cpp$(ObjectSuffix) $(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IntermediateDirectory)/test_event.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_src_event.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_src_utils.cpp$(ObjectSuffix) $(IntermediateDirectory)/support_test.cpp$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@test -d ./Debug || $(MakeDirCommand) ./Debug


$(IntermediateDirectory)/.d:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:
	@echo Executing Pre Build commands ...
	../../../Tools/mk-event-defs.py
	./grm.py -v -o main.cpp test*.cpp
	@echo Done


##
## Objects
##
$(IntermediateDirectory)/unity.c$(ObjectSuffix): unity.c $(IntermediateDirectory)/unity.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/unity.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unity.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unity.c$(DependSuffix): unity.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/unity.c$(ObjectSuffix) -MF$(IntermediateDirectory)/unity.c$(DependSuffix) -MM unity.c

$(IntermediateDirectory)/unity.c$(PreprocessSuffix): unity.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unity.c$(PreprocessSuffix) unity.c

$(IntermediateDirectory)/test_utils.cpp$(ObjectSuffix): test_utils.cpp $(IntermediateDirectory)/test_utils.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/test_utils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/test_utils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/test_utils.cpp$(DependSuffix): test_utils.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/test_utils.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/test_utils.cpp$(DependSuffix) -MM test_utils.cpp

$(IntermediateDirectory)/test_utils.cpp$(PreprocessSuffix): test_utils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/test_utils.cpp$(PreprocessSuffix) test_utils.cpp

$(IntermediateDirectory)/main.cpp$(ObjectSuffix): main.cpp $(IntermediateDirectory)/main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.cpp$(DependSuffix): main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/main.cpp$(DependSuffix) -MM main.cpp

$(IntermediateDirectory)/main.cpp$(PreprocessSuffix): main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.cpp$(PreprocessSuffix) main.cpp

$(IntermediateDirectory)/test_event.cpp$(ObjectSuffix): test_event.cpp $(IntermediateDirectory)/test_event.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/test_event.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/test_event.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/test_event.cpp$(DependSuffix): test_event.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/test_event.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/test_event.cpp$(DependSuffix) -MM test_event.cpp

$(IntermediateDirectory)/test_event.cpp$(PreprocessSuffix): test_event.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/test_event.cpp$(PreprocessSuffix) test_event.cpp

$(IntermediateDirectory)/up_src_event.cpp$(ObjectSuffix): ../src/event.cpp $(IntermediateDirectory)/up_src_event.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/src/event.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_src_event.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_src_event.cpp$(DependSuffix): ../src/event.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_src_event.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_src_event.cpp$(DependSuffix) -MM ../src/event.cpp

$(IntermediateDirectory)/up_src_event.cpp$(PreprocessSuffix): ../src/event.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_src_event.cpp$(PreprocessSuffix) ../src/event.cpp

$(IntermediateDirectory)/up_src_utils.cpp$(ObjectSuffix): ../src/utils.cpp $(IntermediateDirectory)/up_src_utils.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/src/utils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_src_utils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_src_utils.cpp$(DependSuffix): ../src/utils.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_src_utils.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_src_utils.cpp$(DependSuffix) -MM ../src/utils.cpp

$(IntermediateDirectory)/up_src_utils.cpp$(PreprocessSuffix): ../src/utils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_src_utils.cpp$(PreprocessSuffix) ../src/utils.cpp

$(IntermediateDirectory)/support_test.cpp$(ObjectSuffix): support_test.cpp $(IntermediateDirectory)/support_test.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/support_test.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/support_test.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/support_test.cpp$(DependSuffix): support_test.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/support_test.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/support_test.cpp$(DependSuffix) -MM support_test.cpp

$(IntermediateDirectory)/support_test.cpp$(PreprocessSuffix): support_test.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/support_test.cpp$(PreprocessSuffix) support_test.cpp


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/

