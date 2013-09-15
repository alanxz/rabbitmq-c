
# ==============================================================================================#=
# FILE:
#   Makefile
#
# DESCRIPTION:
#   Builds the rabbitmq-c static library.
# ==============================================================================================#=

# ------------------------------------------------------------------------------+-
# make menuconfig
#
# Support integration with projects that use "make menuconfig" for project level
# customization.
#
# The toplevel project Makefile creates a project-wide ".config" file for use
# in lower level projects like this one.  The ".config" file is typically 
# created via "make menuconfig", "make nconfig", or "make alldefconfig".
#
# ------------------------------------------------------------------------------+-

-include ../$(DOT_CONFIG_FILE)

#
# Define a function to strip quotes from variables defined in .config
#
unquote = $(subst $\",,$1)


ifeq ($(CONFIG_CONFIGURED),y)
    ifeq ($(CONFIG_QUIET_BUILD),y)
        export Q ?= @
    else
        export Q ?= #@
    endif
else
     #
     # When we don't have a command line Q setting
     #   e.g. $ make help Q=@
     # and we don't have a .config file
     #   e.g. $ make nconfig
     #        $ make help
     # we default to a quiet make.
     #
     export Q ?= @
endif


ifeq ($(CONFIG_CONFIGURED),y)

    #
    # CONFIG_CONFIGURED==y when we have a .config file so use it's values
    #
    # Dequote the .config strings before use.  Defines in .config start
    # with CONFIG_
    #
    CROSS_COMPILE ?= $(call unquote,$(CONFIG_CROSS_COMPILE))
    TOOLCHAIN_MCPU_EQ        = $(call unquote,$(CONFIG_TOOLCHAIN_MCPU_EQ))
    TOOLCHAIN_MARCH_EQ       = $(call unquote,$(CONFIG_TOOLCHAIN_MARCH_EQ))
    TOOLCHAIN_MFLOAT_ABI_EQ  = $(call unquote,$(CONFIG_TOOLCHAIN_MFLOAT_ABI_EQ))
    TOOLCHAIN_MFPU_EQ        = $(call unquote,$(CONFIG_TOOLCHAIN_MFPU_EQ))
    TOOLCHAIN_OPTIMISATION   = $(call unquote,$(CONFIG_TOOLCHAIN_OPTIMISATION))

else
    #
    # We don't have .config file so use good defaults
    #
    CROSS_COMPILE           ?= arm-none-eabi-
    TOOLCHAIN_MCPU_EQ        = -mcpu=cortex-m3
    TOOLCHAIN_MARCH_EQ       =
    TOOLCHAIN_MFLOAT_ABI_EQ  =
    TOOLCHAIN_MFPU_EQ        =
    TOOLCHAIN_OPTIMISATION   =

endif


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
    ### @@@ @@@ $(LIB_SRC_DIR)readv.c   \
    ### @@@ @@@ $(LIB_SRC_DIR)writev.c   \



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
CFLAGS-y                              = 
CFLAGS-$(CONFIG_TOOLCHAIN_USE_MTHUMB) += -mthumb
CFLAGS-y                              += $(TOOLCHAIN_MCPU_EQ)
CFLAGS-y                              += $(TOOLCHAIN_MARCH_EQ)
CFLAGS-y                              += $(TOOLCHAIN_MFLOAT_ABI_EQ)
CFLAGS-y                              += $(TOOLCHAIN_MFPU_EQ)
CFLAGS-y                              += $(TOOLCHAIN_OPTIMISATION)
CFLAGS-y                              += -std=c99
CFLAGS-y                              += -pedantic
CFLAGS-$(CONFIG_TOOLCHAIN_WALL)       += -Wall
CFLAGS-y                              += -Wextra
CFLAGS-y                              += -Wstrict-prototypes
CFLAGS-y                              += -Wcast-align
CFLAGS-y                              += -Wno-unused-function
CFLAGS-y                              += -fno-common
CFLAGS-y                              += -fvisibility=hidden
CFLAGS-y                              += -ffunction-sections
CFLAGS-y                              += -fdata-sections
CFLAGS-$(CONFIG_TOOLCHAIN_VERBOSE)    += -v
CFLAGS-$(CONFIG_TOOLCHAIN_PRTHEADERS) += -H
CFLAGS-$(CONFIG_TOOLCHAIN_DEBUG)      += -g

CFLAGS = $(CFLAGS-y)

# ------------------------------------------------------------------------------+-
# GCC Command Line Options
# ------------------------------------------------------------------------------+-
GCC_OPTS = \
    -save-temps=obj \
    $(CPPFLAGS) \
    $(CFLAGS) \
    $(DEBUG) \


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
	@echo "+--$(CC) $<"
	$(Q)$(CC) $(GCC_OPTS) -o $@ -c $<

# --------------------------------------------------------------+-
# Create an object library.
# --------------------------------------------------------------+-
$(TARGET_LIB_DIR)%.a:
	@echo "+--$(AR) $@"
	$(Q)$(AR) rcv $@ $^
	$(Q)$(RANLIB) $@


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

