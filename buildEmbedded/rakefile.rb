
# ==============================================================================================#=
# FILE:
#   rakefile.rb
#
# DESCRIPTION:
#   Ruby make for building a static library for the rabbitmq C client.
#   We expect this file to be in:
#     <ProjectRootPath>/rabbitmq-c-lwip-freertos/buildEmbedded
#
# FUTURE CONSIDERATIONS:
#   Move all puts from :object_build_definitions to a "debug" task.
# ==============================================================================================#=
require 'rake'
require 'rake/clean'


# ------------------------------------------------------------------------------+-
# Global Vars
# ------------------------------------------------------------------------------+-
$LIB_TARGET = :librabbitmq
$STATIC_LIB = "#{$LIB_TARGET}.a"

# ------------------------------------------------------+-
# Tool selection.
# ------------------------------------------------------+-
$COMPILER_EXEC = '/opt/ARM/bin/arm-none-eabi-gcc'
$ARCHIVER_EXEC = '/opt/ARM/bin/arm-none-eabi-ar'
$GENIDX_EXEC   = '/opt/ARM/bin/arm-none-eabi-ranlib'


$SEP = '--------------------------------------------------------------------------------+-'

$PROJ_ROOT_PATH = File.expand_path('../../..', __FILE__)
$LIB_DIR_PATH   = File.expand_path('rabbitmq-c-lwip-freertos', $PROJ_ROOT_PATH)
$SRC_DIR_PATH   = File.expand_path('librabbitmq',              $LIB_DIR_PATH)
$OBJ_DIR_PATH   = File.expand_path('buildEmbedded/objs',       $LIB_DIR_PATH)

# --------------------------------------------------------------+-
# The following are the .c files that comprise the library.
# They are all located in the librabbitmq directory.
#
### Leave this one out for now: amqp_openssl.c
#
# --------------------------------------------------------------+-
$C_FILE_NAMES = %w(
    amqp_framing.c
    amqp_api.c
    amqp_connection.c
    amqp_mem.c
    amqp_socket.c
    amqp_table.c
    amqp_url.c
    amqp_tcp_socket.c
    amqp_timer.c
    amqp_consumer.c
)

$SRC_FILES = FileList.new($C_FILE_NAMES.map {|file| File.expand_path(file,$SRC_DIR_PATH)})
$OBJ_FILES = FileList.new()


# --------------------------------------------------------------+-
# Include paths (relative to project root directory).
# --------------------------------------------------------------+-
$INCLUDE_PATHS = %w(
    rabbitmq-c-lwip-freertos/librabbitmq/unix
    rabbitmq-c-lwip-freertos/librabbitmq
    lwip-contrib/ports/cross/src/include/LM3S
    lwip/src/include/ipv4
    lwip/src/include/lwip
    lwip/src/include
    quickstart.stm32f4xx/src/quick-opts
    quickstart.stm32f4xx/src/quick
    gap/include
)

# --------------------------------------------------------------+-
# Options
# These were determined by running the librabbitmq cmake
# build # and observing the compiler commands it uses.
# Note: the VERSION is from the librabbitmq.spec file.
# We use this in lieu of the config.h file and then
# don't include -DHAVE_CONFIG_H in the list of options.
# Others were added as needed, e.g. LWIP...
# --------------------------------------------------------------+-
$COMPILER_OPTIONS = %w(
    -v
    -H
    -save-temps=obj
    -DAMQP_BUILD
    -DAMQP_STATIC
    -DENABLE_THREAD_SAFETY
    -DWITH_SSL=1
    -DLWIP_TIMEVAL_PRIVATE=0
    -Wall
    -Wextra
    -pedantic
    -Wstrict-prototypes
    -Wcast-align
    -Wno-unused-function
    -fno-common
    -fvisibility=hidden
    -O3
    -DNDEBUG
    -DVERSION=\"0.3.0\"
)


# --------------------------------------------------------------+-
# Task: object_build_definitions
#   This task defines $OBJ_FILES and
#   a file task for building each object file.
# --------------------------------------------------------------+-
task :object_build_definitions do
    ### puts
    ### puts 'SRC_DIR_PATH: ' + $SRC_DIR_PATH

    ### puts
    ### puts "Source Files are: \n"
    ### $SRC_FILES.each do |srcFilePath|
        ### puts "  #{srcFilePath} \n"
    ### end

    opts = ''
    $COMPILER_OPTIONS.each do |optstr|
        opts += optstr + ' '
    end

    includePaths = ''
    $INCLUDE_PATHS.each do |inclPath|
        includePaths += "-I" + File.join($PROJ_ROOT_PATH, inclPath) + ' '
    end

    # --------------------------------------------------------------+-
    # Create one file task for creating each object file.
    # --------------------------------------------------------------+-
    $SRC_FILES.each do |srcFilePath|
        objFilePath = File.join($OBJ_DIR_PATH, File.basename(srcFilePath) + '.o')
        $OBJ_FILES.add(objFilePath)

        file objFilePath => [$OBJ_DIR_PATH, srcFilePath] do
            cmd  = "#{$COMPILER_EXEC} "
            cmd += opts
            cmd += includePaths
            cmd += "-o #{objFilePath} "
            cmd += "-c #{srcFilePath} "
            puts
            puts $SEP
            puts "Building C Object: " + File.basename(objFilePath)
            puts $SEP
            sh cmd
        end
    end

    ### puts
    ### puts "Object Files are: \n"
    ### $OBJ_FILES.each do |objFilePath|
        ### puts "  #{objFilePath} \n"
    ### end
end

# --------------------------------------------------------------+-
# These steps must always get done first
# because other task definitions depend on them.
# --------------------------------------------------------------+-
Rake::Task['object_build_definitions'].invoke
directory $OBJ_DIR_PATH

# --------------------------------------------------------------+-
# usage
# --------------------------------------------------------------+-
task :default do
    puts
    puts "Usage/targets for this rakefile: "
    puts "  rake clean"
    puts "  rake clobber"
    puts "  rake #{$LIB_TARGET}"
    puts
    puts "  rake -T"
end


# --------------------------------------------------------------+-
# Target: rabbitmq static library
# --------------------------------------------------------------+-
task :build_library => $OBJ_FILES.to_a() do
    puts
    puts $SEP
    puts "Creating Static Library... "
    puts $SEP
    objs = ''
    $OBJ_FILES.each do |filepath|
        objs += filepath + ' '
    end

    cmd  = "#{$ARCHIVER_EXEC} rcv #{$STATIC_LIB} "
    cmd += objs
    puts
    puts cmd
    puts
    sh cmd

    cmd  = "#{$GENIDX_EXEC} #{$STATIC_LIB} "
    puts "  "
    sh cmd
end
$GENIDX_EXEC   = '/opt/ARM/bin/arm-none-eabi-ranlib'

desc "Build the rabbitmq client (static) library."
task $LIB_TARGET do
    puts
    puts 'We only support building from clean.'
    Rake::Task[:clean].invoke
    Rake::Task[:build_library].invoke
end


# --------------------------------------------------------------+-
# Targets: Clean/Clobber
# --------------------------------------------------------------+-
CLEAN.include("#{$OBJ_DIR_PATH}/*")
CLOBBER.include("#{$LIB_DIR_PATH}/buildEmbedded/#{$STATIC_LIB}")




__END__
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|@
source material
not currently used.

$LINKER_OPTIONS = %w(
    -v
    -fPIC
    -Wall
    -Wextra
    -pedantic
    -Wstrict-prototypes
    -Wcast-align
    -Wno-unused-function
    -fno-common
    -fvisibility=hidden 
    -O3
    -DNDEBUG  
    -shared
    -Wl,-soname,librabbitmq.so.1
    -o librabbitmq.so.1.0.1
)

# --------------------------------------------------------------+-
# The libraries with which we need to link.
#
# Leave the following out for now:
#     -lssl
#     -lcrypto
#     -lrt    -- real time extensions (required for pthreads)
#     -lpthread
# --------------------------------------------------------------+-
$LIBRARIES = %w(
)

# --------------------------------------------------------------+-
# Target: rabbitmq static library
# --------------------------------------------------------------+-
task :build_library => $OBJ_FILES.to_a() do
    lopts = ''
    $LINKER_OPTIONS.each do |optstr|
        lopts += optstr + ' '
    end
    objs = ''
    $OBJ_FILES.each do |filepath|
        objs += filepath + ' '
    end
    libs = ''
    $LIBRARIES.each do |libstr|
        libs += libstr + ' '
    end
    cmd  = "#{$COMPILER_EXEC} "
    cmd += lopts
    cmd += objs
    cmd += libs
    puts
    puts $SEP
    puts "Linking... "
    puts $SEP
    sh cmd
end




