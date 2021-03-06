TARGET = libnwidget.a

# 除当前目录外其余源文件所在文件夹
DIR_SRC = base multiple

# 头文件路径
DIR_INC = include

# 项目根目录，一般和 Makefile 同一目录
DIR_ROOT = .

# 可执行文件输出目录
DIR_BIN = .

# 编译中间文件输出目录
DIR_BUILD = _build

# 其他依赖工程
DIR_OTHER = 

# flags =====================================
CFLAGS    := -Wall -O2 -std=gnu99
CXXFLAGS  := -Wall -O2 -std=c++11
LDFLAGS    = $(addprefix -l,${LIBRARY})
LDFLAGS   += $(addprefix -L,${DIR_LIB})

# command ==================================
CC  = gcc
CXX = g++

# make host=linaro/aarch64 :
HOST = 
ifdef host
	ifeq ($(host), linaro)
		HOST = arm-linux-gnueabihf-
	else ifeq ($(host), aarch64)
		HOST = aarch64-linux-gnu-
	endif
endif

ifdef DEBUG
	CFLAGS += -DDEBUG
	CXXFLAGS += -DDEBUG
else
	CFLAGS += -s
	CXXFLAGS += -s
endif

TARGET   := $(DIR_BIN)/$(TARGET)
DIR_INC  := $(addprefix -I$(DIR_ROOT)/,${DIR_INC})
DIR_SRC  := $(addprefix $(DIR_ROOT)/,${DIR_SRC})
DIR_SRC  += .

C_SRCS   = $(foreach n,$(DIR_SRC),$(wildcard $(n)/*.c))
CPP_SRCS = $(foreach n,$(DIR_SRC),$(wildcard $(n)/*.cpp))

# 生成的 .o 文件存于 DIR_BUILD 相关路径
OBJS = $(patsubst $(DIR_ROOT)/%.cpp,$(DIR_BUILD)/%.o, $(CPP_SRCS))
OBJS += $(patsubst $(DIR_ROOT)/%.c, $(DIR_BUILD)/%.o, $(C_SRCS))
DEP  := $(OBJS:%.o=%.d)

# 生成输出目录
BUILD_DIR = $(foreach n,$(DIR_SRC),$(DIR_BUILD)/$(n))

ifeq ($(OS),Windows_NT)
	TARGET    := $(subst /,\,$(TARGET))
	BUILD_DIR := $(subst /,\,$(BUILD_DIR))
	DIR_BUILD := $(subst /,\,$(DIR_BUILD))
	MKDIR      = -mkdir >nul 2>nul
	RMDIR      = -rmdir /S /Q
	RM         = -del /f
else
	RM       = rm -f
	MKDIR    = mkdir -p
	RMDIR    = rm -rf
endif

.PHONY : all clean cleanall rebuild others
all  :makedir $(TARGET)
	@echo Built target:$(TARGET)

makedir:
	@$(MKDIR) ${BUILD_DIR}


-include $(DEP)
$(DIR_BUILD)/%.o:$(DIR_ROOT)/%.cpp
	@$(HOST)$(CXX) $(CXXFLAGS) $(DIR_INC) -MM -MT $@ -MF $(patsubst %.o, %.d, $@) $<
	$(HOST)$(CXX) $(CXXFLAGS) -c $< -o $@ $(DIR_INC)

#-include $(DEP)
$(DIR_BUILD)/%.o:$(DIR_ROOT)/%.c
	@$(HOST)$(CC) $(CFLAGS) $(DIR_INC) -MM -MT $@ -MF $(patsubst %.o, %.d, $@) $<
	$(HOST)$(CC) $(CFLAGS) -c $< -o $@ $(DIR_INC)

$(TARGET) : $(OBJS)
	$(HOST)ar rc $(TARGET) $^

rebuild: clean all
clean :
	$(foreach subdir,$(DIR_OTHER),$(MAKE) -C $(subdir) clean)
	@$(RMDIR) $(DIR_BUILD)

cleanall:
	$(foreach subdir,$(DIR_OTHER),$(MAKE) -C $(subdir) cleanall)
	@$(RMDIR) $(DIR_BUILD)
	@$(RM) $(TARGET)
