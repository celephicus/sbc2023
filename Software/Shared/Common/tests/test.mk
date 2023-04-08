##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## D0
ProjectName            :=test
ConfigurationName      :=D0
WorkspacePath          :=/home/tom/git/misc/2022SBC/Software/Shared/Common/test
ProjectPath            :=/home/tom/git/misc/2022SBC/Software/Shared/Common/test
IntermediateDirectory  :=./D0
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Tom
Date                   :=05/04/23
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
Preprocessors          :=$(PreprocessorSwitch)UNITY_INCLUDE_CONFIG_H $(PreprocessorSwitch)TEST $(PreprocessorSwitch)NO_CRITICAL_SECTIONS $(PreprocessorSwitch)USE_PROJECT_CONFIG_H $(PreprocessorSwitch)MYPRINTF_TEST_BINARY=1 
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
CXXFLAGS :=  -g -O0 -Wall -Wextra -Wundef $(Preprocessors)
CFLAGS   :=  -g -O0 -Wall -Wextra -Wundef $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/test_utils.cpp$(ObjectSuffix) $(IntermediateDirectory)/test_event.cpp$(ObjectSuffix) $(IntermediateDirectory)/support_test.cpp$(ObjectSuffix) $(IntermediateDirectory)/test_modbus.cpp$(ObjectSuffix) $(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_src_myprintf.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_src_modbus.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_src_event.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_src_utils.cpp$(ObjectSuffix) $(IntermediateDirectory)/test_buffer.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/test_printf.cpp$(ObjectSuffix) $(IntermediateDirectory)/unity.c$(ObjectSuffix) 



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
	@test -d ./D0 || $(MakeDirCommand) ./D0


$(IntermediateDirectory)/.d:
	@test -d ./D0 || $(MakeDirCommand) ./D0

PreBuild:
	@echo Executing Pre Build commands ...
	python3 ../../../Tools/events_mk.py
	python3 ./grm.py -v -o main.cpp test*.cpp
	@echo Done


##
## Objects
##
$(IntermediateDirectory)/test_utils.cpp$(ObjectSuffix): test_utils.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/test_utils.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/test_utils.cpp$(DependSuffix) -MM test_utils.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/test_utils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/test_utils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/test_utils.cpp$(PreprocessSuffix): test_utils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/test_utils.cpp$(PreprocessSuffix) test_utils.cpp

$(IntermediateDirectory)/test_event.cpp$(ObjectSuffix): test_event.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/test_event.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/test_event.cpp$(DependSuffix) -MM test_event.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/test_event.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/test_event.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/test_event.cpp$(PreprocessSuffix): test_event.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/test_event.cpp$(PreprocessSuffix) test_event.cpp

$(IntermediateDirectory)/support_test.cpp$(ObjectSuffix): support_test.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/support_test.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/support_test.cpp$(DependSuffix) -MM support_test.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/support_test.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/support_test.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/support_test.cpp$(PreprocessSuffix): support_test.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/support_test.cpp$(PreprocessSuffix) support_test.cpp

$(IntermediateDirectory)/test_modbus.cpp$(ObjectSuffix): test_modbus.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/test_modbus.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/test_modbus.cpp$(DependSuffix) -MM test_modbus.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/test_modbus.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/test_modbus.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/test_modbus.cpp$(PreprocessSuffix): test_modbus.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/test_modbus.cpp$(PreprocessSuffix) test_modbus.cpp

$(IntermediateDirectory)/main.cpp$(ObjectSuffix): main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/main.cpp$(DependSuffix) -MM main.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.cpp$(PreprocessSuffix): main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.cpp$(PreprocessSuffix) main.cpp

$(IntermediateDirectory)/up_src_myprintf.cpp$(ObjectSuffix): ../src/myprintf.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_src_myprintf.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_src_myprintf.cpp$(DependSuffix) -MM ../src/myprintf.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/src/myprintf.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_src_myprintf.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_src_myprintf.cpp$(PreprocessSuffix): ../src/myprintf.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_src_myprintf.cpp$(PreprocessSuffix) ../src/myprintf.cpp

$(IntermediateDirectory)/up_src_modbus.cpp$(ObjectSuffix): ../src/modbus.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_src_modbus.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_src_modbus.cpp$(DependSuffix) -MM ../src/modbus.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/src/modbus.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_src_modbus.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_src_modbus.cpp$(PreprocessSuffix): ../src/modbus.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_src_modbus.cpp$(PreprocessSuffix) ../src/modbus.cpp

$(IntermediateDirectory)/up_src_event.cpp$(ObjectSuffix): ../src/event.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_src_event.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_src_event.cpp$(DependSuffix) -MM ../src/event.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/src/event.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_src_event.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_src_event.cpp$(PreprocessSuffix): ../src/event.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_src_event.cpp$(PreprocessSuffix) ../src/event.cpp

$(IntermediateDirectory)/up_src_utils.cpp$(ObjectSuffix): ../src/utils.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_src_utils.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_src_utils.cpp$(DependSuffix) -MM ../src/utils.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/src/utils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_src_utils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_src_utils.cpp$(PreprocessSuffix): ../src/utils.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_src_utils.cpp$(PreprocessSuffix) ../src/utils.cpp

$(IntermediateDirectory)/test_buffer.cpp$(ObjectSuffix): test_buffer.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/test_buffer.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/test_buffer.cpp$(DependSuffix) -MM test_buffer.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/test_buffer.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/test_buffer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/test_buffer.cpp$(PreprocessSuffix): test_buffer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/test_buffer.cpp$(PreprocessSuffix) test_buffer.cpp

$(IntermediateDirectory)/test_printf.cpp$(ObjectSuffix): test_printf.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/test_printf.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/test_printf.cpp$(DependSuffix) -MM test_printf.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/test_printf.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/test_printf.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/test_printf.cpp$(PreprocessSuffix): test_printf.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/test_printf.cpp$(PreprocessSuffix) test_printf.cpp

$(IntermediateDirectory)/unity.c$(ObjectSuffix): unity.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/unity.c$(ObjectSuffix) -MF$(IntermediateDirectory)/unity.c$(DependSuffix) -MM unity.c
	$(CC) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/unity.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/unity.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unity.c$(PreprocessSuffix): unity.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/unity.c$(PreprocessSuffix) unity.c


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./D0/


