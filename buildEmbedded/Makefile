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

ifneq ($(DOT_CONFIG_FILE),)
-include ../$(DOT_CONFIG_FILE)
endif


# Define a function to strip quotes from variables defined in .config
#
unquote = $(subst $\",,$1)


ifeq ($(CONFIG_CONFIGURED),y)
    ifeq ($(CONFIG_QUIET_BUILD),y)
        Q ?= @
    else
        Q ?= #@
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
     Q ?= @
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
    CROSS_COMPILE             ?= arm-none-eabi-
    TOOLCHAIN_MCPU_EQ          = -mcpu=cortex-m3
    TOOLCHAIN_MARCH_EQ         =
    TOOLCHAIN_MFLOAT_ABI_EQ    =
    TOOLCHAIN_MFPU_EQ          =
    TOOLCHAIN_OPTIMISATION     =
    CONFIG_TOOLCHAIN_USE_MTHUMB= y
    CONFIG_TOOLCHAIN_WALL      = y
    CONFIG_TOOLCHAIN_VERBOSE   =
    CONFIG_TOOLCHAIN_PRTHEADERS=
    CONFIG_TOOLCHAIN_DEBUG     =

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

BUILD_DIR      := objs/
PROJ_BUILD_DIR := objs/$(notdir $(CURDIR))/$(LIB)/
LIB_ROOT_DIR   := $(dir $(CURDIR))
PROJ_ROOT_DIR  := $(dir $(patsubst %/, %, $(LIB_ROOT_DIR)))
LIB_SRC_DIR    := $(LIB_ROOT_DIR)$(LIB)/
TARGET_LIB_DIR := $(CURDIR)/



# ------------------------------------------------------------------------------+-
# C Source Files
# ------------------------------------------------------------------------------+-
CSOURCE-y =
CSOURCE-y                                           += ../$(LIB)/amqp_framing.c
CSOURCE-y                                           += ../$(LIB)/amqp_api.c
CSOURCE-y                                           += ../$(LIB)/amqp_connection.c
CSOURCE-y                                           += ../$(LIB)/amqp_mem.c
CSOURCE-y                                           += ../$(LIB)/amqp_socket.c
CSOURCE-y                                           += ../$(LIB)/amqp_table.c
CSOURCE-y                                           += ../$(LIB)/amqp_url.c
CSOURCE-y                                           += ../$(LIB)/amqp_tcp_socket.c
CSOURCE-y                                           += ../$(LIB)/amqp_timer.c
CSOURCE-y                                           += ../$(LIB)/amqp_consumer.c
CSOURCE-$(CONFIG_RABBITMQ_USE_CYASSL_ENA)           += ../$(LIB)/amqp_cyassl.c

CSOURCE = $(CSOURCE-y)

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
INCLUDE_DIRS-y                                 =
INCLUDE_DIRS-y                                 += -I$(LIB_SRC_DIR)
INCLUDE_DIRS-$(CONFIG_APP)                     += -I$(PROJ_ROOT_DIR)$(APP_DIR)/include
INCLUDE_DIRS-$(CONFIG_APP)                     += -I$(PROJ_ROOT_DIR)$(APP_DIR)/include/generated
INCLUDE_DIRS-$(CONFIG_APP)                     += -I$(PROJ_ROOT_DIR)$(APP_DIR)/source
INCLUDE_DIRS-$(CONFIG_QUICKSTART)              += -I$(PROJ_ROOT_DIR)$(QUICKSTART_DIR)/include
INCLUDE_DIRS-$(CONFIG_QUICKSTART)              += -I$(PROJ_ROOT_DIR)$(QUICKSTART_DIR)/include/generated
INCLUDE_DIRS-$(CONFIG_QUICKSTART)              += -I$(PROJ_ROOT_DIR)$(QUICKSTART_DIR)/source
INCLUDE_DIRS-$(CONFIG_QUICKSTART)              += -I$(PROJ_ROOT_DIR)chaney.aculink.bridge.freertos/FreeRTOS/Source/include
INCLUDE_DIRS-$(CONFIG_FREERTOS_PORT_ARM_CM3)   += -I$(PROJ_ROOT_DIR)chaney.aculink.bridge.freertos/FreeRTOS/Source/portable/GCC/ARM_CM3
INCLUDE_DIRS-$(CONFIG_FREERTOS_PORT_ARM_CM4F)  += -I$(PROJ_ROOT_DIR)chaney.aculink.bridge.freertos/FreeRTOS/Source/portable/GCC/ARM_CM4F
INCLUDE_DIRS-y                                 += -I$(PROJ_ROOT_DIR)lwip-contrib/ports/cross/src/include/LM3S
INCLUDE_DIRS-y                                 += -I$(PROJ_ROOT_DIR)lwip/src/include
INCLUDE_DIRS-y                                 += -I$(PROJ_ROOT_DIR)lwip/src/include/ipv4
INCLUDE_DIRS-$(CONFIG_RABBITMQ_USE_CYASSL_ENA) += -I$(PROJ_ROOT_DIR)CyaSSL

INCLUDE_DIRS = $(INCLUDE_DIRS-y)



# ------------------------------------------------------------------------------+-
# C Preprocessor Options
# ------------------------------------------------------------------------------+-
COMPILER = GCC
SUBARCH  = ARM_CM3

CPPFLAGS                                             =
CPPFLAGS-y                                           += $(CPPFLAGS_CMD_LINE)
CPPFLAGS-y                                           += -DAMQP_BUILD
CPPFLAGS-y                                           += -DAMQP_STATIC
CPPFLAGS-y                                           += -DENABLE_THREAD_SAFETY
CPPFLAGS-y                                           += -DWITH_SSL=1
CPPFLAGS-y                                           += -DLWIP_TIMEVAL_PRIVATE=0
CPPFLAGS-y                                           += -DNDEBUG
CPPFLAGS-y                                           += -DVERSION=\"0.3.0\"
CPPFLAGS-y                                           += $(INCLUDE_DIRS)
CPPFLAGS-y                                           += -D $(COMPILER)_$(SUBARCH)
CPPFLAGS-$(CONFIG_RABBITMQ_HAVE_CONFIG_H_ENA)        += -D HAVE_CONFIG_H
### CPPFLAGS-y                                           += -D DEBUG_RABBIT
CPPFLAGS-$(CONFIG_AMQP_USE_UNTESTED_SSL_BACKEND_ENA) += -DAMQP_USE_UNTESTED_SSL_BACKEND

CPPFLAGS = $(CPPFLAGS-y)

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
##CFLAGS-$(CONFIG_TOOLCHAIN_WALL)       += -Wall
CFLAGS-y                              += -Wextra
CFLAGS-y                              += -Wstrict-prototypes
CFLAGS-y                              += -Wcast-align
CFLAGS-y                              += -Wno-unused-function
CFLAGS-$(CONFIG_TOOLCHAIN_WERROR)     += -Werror
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
$(PROJ_BUILD_DIR)%.o : %.c
	@echo "+--$(CC) $<"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(GCC_OPTS) -o $@ -c $<

# --------------------------------------------------------------+-
# Create an object library.
# --------------------------------------------------------------+-
$(TARGET_LIB_DIR)%.a:
	@echo "+--$(AR) $@"
	$(Q)$(AR) rcv $@ $^
	$(Q)$(RANLIB) $@

# --------------------------------------------------------------+-
# automatically generate dependency rules
# --------------------------------------------------------------+-

$(PROJ_BUILD_DIR)%.d : %.c
	@echo "+--Generating Dependency $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(GCC_OPTS) -MF"$@" -MG -MM -MT"$@" -MT"$(@:.d=.o)" "$<"

#
# -MF  write the generated dependency rule to a file
# -MG  assume missing headers will be generated and don't stop with an error
# -MM  generate dependency rule for prerequisite, skipping system headers
# -MP  add phony target for each header to prevent errors when header is missing
# -MT  add a target to the generated dependency
# 
# "$@" is the target (the thing on the left side of the : )
# "$<" is the prerequisite (the thing on the right side of the : ). 
# The expression "$(<:.c=.o)" replaces the .c extension with .o.
#
# The trick here is to generate the rule with two targets by adding -MT twice; 
# this makes both the .o file and the .d file depend on the source file 
# and its headers; that way the dependency file gets automatically regenerated 
# whenever any of the corresponding .c or .h files are changed.
#
# The -MG and -MP options keep make from freaking out if a header file is missing.
#
# We've removed the -MP option to keep make from infinite looping within the
# dependency generation.
#

# ==============================================================================#=
# TARGETS
# ==============================================================================#=

# --------------------------------------------------------------+-
# Target: all
# --------------------------------------------------------------+-
.PHONY: all
all : $(TARGET_LIB_DIR)$(LIB).a
	@# This line prevents warning when nothing to be done for all.


# ----------------------------------------------+-
# target library
# ----------------------------------------------+-
OBJS = $(addprefix $(PROJ_BUILD_DIR), $(CSOURCE:.c=.o))
DEPS = $(addprefix $(PROJ_BUILD_DIR), $(CSOURCE:.c=.d))


$(TARGET_LIB_DIR)$(LIB).a : $(OBJS)


ifndef MAKECMDGOALS
-include $(DEPS)
else
ifneq ($(MAKECMDGOALS),$(filter $(MAKECMDGOALS),"help clean printincludedirs "))
#
# Include the dependencies if they are available
# and we are not invoked with one of the targets:
#
# clean help
#
# .e.g.
#
# make clean
#
-include $(DEPS)
endif
endif


# --------------------------------------------------------------+-
# Target: clean
# --------------------------------------------------------------+-
RM=/bin/rm

.PHONY: clean
clean :
ifneq ($(strip $(BUILD_DIR)),)
	@echo $(SEP)
	@echo "+--AMQP Library clean"
	@echo $(SEP)
	$(RM) -fr $(BUILD_DIR)*
	$(RM) -f $(TARGET_LIB_DIR)*.a
else
	$(Q)echo "BUILD_DIR is not defined or empty, can't clean."
endif	

export INCLUDE_DIRS
.PHONY: printincludedirs
printincludedirs :
	$(Q)for i in $${INCLUDE_DIRS}; do echo $${i}; done

