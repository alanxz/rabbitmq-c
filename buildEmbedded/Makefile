
# ==============================================================================================#=
# FILE:
#   Makefile
#
# DESCRIPTION:
#   Builds the rabbitmq-c static library.
# ==============================================================================================#=


# ------------------------------------------------------------------------------+-
# Project Directories
#
# It is assumed that 'make' will be invoked in
# the directory where this makefile is located and
# that this makefile is located in...
#     <project root>/<amqp library root>/buildEmbedded
#
# All .o files are written to...
#     <project root>/<amqp library root>/buildEmbedded/objs
#
# The target lib .a file is written to...
#     <project root>/<amqp library root>/buildEmbedded
# ------------------------------------------------------------------------------+-
LIB := librabbitmq

BUILD_DIR      := $(CURDIR)/objs/
LIB_ROOT_DIR   := $(dir $(CURDIR))
PROJ_ROOT_DIR  := $(dir $(patsubst %/, %, $(LIB_ROOT_DIR)))
LIB_SRC_DIR    := $(LIB_ROOT_DIR)$(LIB)/
TARGET_LIB_DIR := $(CURDIR)/



# ------------------------------------------------------------------------------+-
# C Source Files
# ------------------------------------------------------------------------------+-
CSOURCE := \
    $(LIB_SRC_DIR)amqp_framing.c  \
    $(LIB_SRC_DIR)amqp_api.c   \
    $(LIB_SRC_DIR)amqp_connection.c   \
    $(LIB_SRC_DIR)amqp_mem.c   \
    $(LIB_SRC_DIR)amqp_socket.c   \
    $(LIB_SRC_DIR)amqp_table.c   \
    $(LIB_SRC_DIR)amqp_url.c   \
    $(LIB_SRC_DIR)amqp_tcp_socket.c   \
    $(LIB_SRC_DIR)amqp_timer.c   \
    $(LIB_SRC_DIR)amqp_consumer.c   \
    $(LIB_SRC_DIR)posix_time.c  \
    ### $(LIB_SRC_DIR)readv.c   \
    ### $(LIB_SRC_DIR)writev.c   \



# ------------------------------------------------------------------------------+-
# Include Directories
#
# @@@@@@@@@@@@@@@@
# The include of lwip-contrib/ports/cross/include is not so DRY.
# It is shared across lwipdemo build, lwip build, and here.
# Needs to be refactored.
# Also, the LM3S looks suspicious.
# @@@@@@@@@@@@@@@@
#
# ------------------------------------------------------------------------------+-
INCLUDE_DIRS = \
    -I$(LIB_SRC_DIR) \
    -I$(PROJ_ROOT_DIR)lwipdemo/include \
    -I$(PROJ_ROOT_DIR)lwipdemo/source \
    -I$(PROJ_ROOT_DIR)lwip-contrib/ports/cross/src/include/LM3S \
    -I$(PROJ_ROOT_DIR)lwip/src/include \
    -I$(PROJ_ROOT_DIR)lwip/src/include/ipv4 \



# ------------------------------------------------------------------------------+-
# C Preprocessor Options
# ------------------------------------------------------------------------------+-
COMPILER = GCC
SUBARCH  = ARM_CM3

CPPFLAGS = \
    -DAMQP_BUILD \
    -DAMQP_STATIC \
    -DENABLE_THREAD_SAFETY \
    -DWITH_SSL=1 \
    -DLWIP_TIMEVAL_PRIVATE=0 \
    -DNDEBUG \
    -DVERSION=\"0.3.0\"

CPPFLAGS += \
    $(INCLUDE_DIRS) \
    -D $(COMPILER)_$(SUBARCH)

# ------------------------------------------------------------------------------+-
# Compilation Options
# ------------------------------------------------------------------------------+-
CFLAGS = \
    -mthumb \
    -mcpu=cortex-m3 \
    -std=c99 \
    -Wall \
    -Wextra \
    -pedantic \
    -Wstrict-prototypes \
    -Wcast-align \
    -Wno-unused-function \
    -fno-common \
    -fvisibility=hidden \
    -ffunction-sections \
    -fdata-sections \

DEBUG=-g

OPTIM=-Os

# ------------------------------------------------------------------------------+-
# GCC Command Line Options
# ------------------------------------------------------------------------------+-
GCC_OPTS = \
    -v \
    -H \
    -save-temps=obj \
    $(CPPFLAGS) \
    $(CFLAGS) \
    $(DEBUG) \
    $(OPTIM) \


# ======================================================================#=
# makedefs
# ======================================================================#=
SEP = '-----------------------------------------------------------------------+-'

ifndef CROSS_COMPILE
CROSS_COMPILE = arm-none-eabi-
endif

CC     = $(CROSS_COMPILE)gcc
AR     = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib


# --------------------------------------------------------------+-
# Compile.
# --------------------------------------------------------------+-
$(BUILD_DIR)%.o : $(LIB_SRC_DIR)%.c
	@echo
	@echo $(SEP)
	@echo "Building: $@"
	@echo "From:     $<"
	@echo $(SEP)
	$(CC) $(GCC_OPTS) -o $@ -c $<
	@echo


# --------------------------------------------------------------+-
# Create an object library.
# --------------------------------------------------------------+-
$(TARGET_LIB_DIR)%.a:
	@echo
	@echo $(SEP)
	@echo "Creating Library: $@"
	@echo $(SEP)
	$(AR) rcv $@ $^
	$(RANLIB) $@
	@echo



# ==============================================================================#=
# TARGETS
# ==============================================================================#=

# --------------------------------------------------------------+-
# Target: all
# --------------------------------------------------------------+-
.PHONY: all
all : clean $(TARGET_LIB_DIR)$(LIB).a

# ----------------------------------------------+-
# target library
# ----------------------------------------------+-
OBJS = $(addprefix $(BUILD_DIR), $(notdir $(CSOURCE:.c=.o)))
$(TARGET_LIB_DIR)$(LIB).a : $(OBJS)


# --------------------------------------------------------------+-
# Target: clean
# --------------------------------------------------------------+-
RM=/bin/rm

.PHONY: clean
clean :
	@echo
	@echo $(SEP)
	@echo "AMQP Library Clean Build"
	@echo $(SEP)
	$(RM) -f $(BUILD_DIR)*.i
	$(RM) -f $(BUILD_DIR)*.s
	$(RM) -f $(BUILD_DIR)*.o
	$(RM) -f $(TARGET_LIB_DIR)*.a
	@echo

