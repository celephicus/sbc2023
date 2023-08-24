##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=test
ConfigurationName      :=Debug
WorkspacePath          :=/home/tom/git/misc/2022SBC/Software/Shared/Common/test
ProjectPath            :=/home/tom/git/misc/2022SBC/Software/Shared/Common/test/test
IntermediateDirectory  :=$(ConfigurationName)
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Tom
Date                   :=25/08/23
CodeLitePath           :=/home/tom/.codelite
LinkerName             :=/usr/bin/g++-11
SharedObjectLinkerName :=/usr/bin/g++-11 -shared -fPIC
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
Preprocessors          :=$(PreprocessorSwitch)UNITY_INCLUDE_CONFIG_H $(PreprocessorSwitch)TEST $(PreprocessorSwitch)NO_CRITICAL_SECTIONS $(PreprocessorSwitch)USE_PROJECT_CONFIG_H 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="test.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch).. $(IncludeSwitch)../../include 
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
CXX      := /usr/bin/g++-11
CC       := /usr/bin/gcc-11
CXXFLAGS :=  -g -O0 -Wall -Wextra -Werror -Wsign-conversion -Wduplicated-branches -Wduplicated-cond -Wlogical-op -Wshadow -Wundef -Wswitch-default -Wdouble-promotion -fno-common $(Preprocessors)
CFLAGS   :=  -g -O0 -Wall $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/up_up_src_modbus.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_support_test.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_test_modbus.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_t_main.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_test_buffer_dynamic.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_unity.c$(ObjectSuffix) $(IntermediateDirectory)/up_test_buffer_static.cpp$(ObjectSuffix) 



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
	@test -d $(ConfigurationName) || $(MakeDirCommand) $(ConfigurationName)


$(IntermediateDirectory)/.d:
	@test -d $(ConfigurationName) || $(MakeDirCommand) $(ConfigurationName)

PreBuild:
	@echo Executing Pre Build commands ...
	
	../grm.py -v -o ../t_main.cpp ../test_buffer_dynamic.cpp ../test_modbus.cpp ../test_buffer_static.cpp
	@echo Done


##
## Objects
##
$(IntermediateDirectory)/up_up_src_modbus.cpp$(ObjectSuffix): ../../src/modbus.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_up_src_modbus.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_up_src_modbus.cpp$(DependSuffix) -MM ../../src/modbus.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/src/modbus.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_up_src_modbus.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_up_src_modbus.cpp$(PreprocessSuffix): ../../src/modbus.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_up_src_modbus.cpp$(PreprocessSuffix) ../../src/modbus.cpp

$(IntermediateDirectory)/up_support_test.cpp$(ObjectSuffix): ../support_test.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_support_test.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_support_test.cpp$(DependSuffix) -MM ../support_test.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/support_test.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_support_test.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_support_test.cpp$(PreprocessSuffix): ../support_test.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_support_test.cpp$(PreprocessSuffix) ../support_test.cpp

$(IntermediateDirectory)/up_test_modbus.cpp$(ObjectSuffix): ../test_modbus.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_test_modbus.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_test_modbus.cpp$(DependSuffix) -MM ../test_modbus.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/test_modbus.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_test_modbus.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_test_modbus.cpp$(PreprocessSuffix): ../test_modbus.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_test_modbus.cpp$(PreprocessSuffix) ../test_modbus.cpp

$(IntermediateDirectory)/up_t_main.cpp$(ObjectSuffix): ../t_main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_t_main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_t_main.cpp$(DependSuffix) -MM ../t_main.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/t_main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_t_main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_t_main.cpp$(PreprocessSuffix): ../t_main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_t_main.cpp$(PreprocessSuffix) ../t_main.cpp

$(IntermediateDirectory)/up_test_buffer_dynamic.cpp$(ObjectSuffix): ../test_buffer_dynamic.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_test_buffer_dynamic.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_test_buffer_dynamic.cpp$(DependSuffix) -MM ../test_buffer_dynamic.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/test_buffer_dynamic.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_test_buffer_dynamic.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_test_buffer_dynamic.cpp$(PreprocessSuffix): ../test_buffer_dynamic.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_test_buffer_dynamic.cpp$(PreprocessSuffix) ../test_buffer_dynamic.cpp

$(IntermediateDirectory)/up_unity.c$(ObjectSuffix): ../unity.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_unity.c$(ObjectSuffix) -MF$(IntermediateDirectory)/up_unity.c$(DependSuffix) -MM ../unity.c
	$(CC) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/unity.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_unity.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_unity.c$(PreprocessSuffix): ../unity.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_unity.c$(PreprocessSuffix) ../unity.c

$(IntermediateDirectory)/up_test_buffer_static.cpp$(ObjectSuffix): ../test_buffer_static.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_test_buffer_static.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_test_buffer_static.cpp$(DependSuffix) -MM ../test_buffer_static.cpp
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/tom/git/misc/2022SBC/Software/Shared/Common/test/test_buffer_static.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_test_buffer_static.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_test_buffer_static.cpp$(PreprocessSuffix): ../test_buffer_static.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_test_buffer_static.cpp$(PreprocessSuffix) ../test_buffer_static.cpp


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r $(ConfigurationName)/


