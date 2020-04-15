AUTHOR  = JDPolanco/jdanypa@gmail.com
VERSION = 1.0

# Functions helpers
ADD_COMMAS = $(addprefix ", $(addsuffix ", ${1}))
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

# Microcontroler for the Waspmote board 
MMCU = atmega1281
MCU_PORT ?= COM20
PROGRAMMER = stk500v1
PROGRAMMER_BAUDRATE = 115200

# Sketch libraries dependencies
WASPMOTE_LIBRARIES_DEP = Wasp4G.h smartWaterIons.h ArduinoJson.h
WSP_LIB_FOLDER_DEP_PATH = $(foreach lib,${WASPMOTE_LIBRARIES_DEP},${WASPMOTE_LIBRARIES_PATH}/$(patsubst %.h,%,${lib}))
CC_LIBH_INC = $(foreach lib,${WSP_LIB_FOLDER_DEP_PATH},$(call ADD_COMMAS, -I${lib}))
CXX_INCLUDE_WASPMOTE_CORE = $(call ADD_COMMAS, -I${WASPMOTE_CORE_PATH})

LIB_FOLD_INC = ${CC_LIBH_INC} ${CXX_INCLUDE_WASPMOTE_CORE}

# Paths
WASPMOTE_LIBRARIES_PATH = ./lib
WASPMOTE_CORE_PATH = ./core
WASPMOTE_AVRDUDE_CONF = ./avrdude.conf
ifeq ($(OS),Windows_NT)
AVR_COMPILTER_PATH = C:/Users/lmora/.platformio/packages/toolchain-atmelavr/bin
else
AVR_COMPILTER_PATH = /usr/bin
endif

# Defines helpers
ifeq ($(OS),Windows_NT)
AVRDUDE = $(call ADD_COMMAS, C:/Users/lmora/.platformio/packages/tool-avrdude/avrdude.exe)
else
AVRDUDE = avrdude
endif
ifeq ($(OS),Windows_NT)
OBJ_COPY =  $(call ADD_COMMAS, ${AVR_COMPILTER_PATH}/avr-objcopy.exe)
AVR_LINKER = $(call ADD_COMMAS, ${AVR_COMPILTER_PATH}/avr-ar.exe)
CPP_COMPILER = $(call ADD_COMMAS, ${AVR_COMPILTER_PATH}/avr-g++.exe)
C_COMPILER = $(call ADD_COMMAS, ${AVR_COMPILTER_PATH}/avr-gcc.exe)
AVR_SIZE = $(call ADD_COMMAS, ${AVR_COMPILTER_PATH}/avr-size.exe)
else
OBJ_COPY =  avr-objcopy
AVR_LINKER = avr-ar
CPP_COMPILER = avr-g++
C_COMPILER = avr-gcc
AVR_SIZE = avr-size
endif

# Flags for compilation
CXX_FLAGS = -c -g -Os -w -ffunction-sections -fdata-sections -MMD -mmcu=atmega1281 
BUILD_DEFINES = -DF_CPU=14745600L -DARDUINO=10613 -DARDUINO_AVR_WASP -DARDUINO_ARCH_AVR
INCLUDE_HEADER_COMPILE = ${LIB_FOLD_INC}
CPP_FLAGS = -std=gnu++11 -fno-exceptions -fno-threadsafe-statics -felide-constructors
C_FLAGS = -std=gnu11
LINKER_FLAGS = rcs
AVR_SIZE_FLAGS = -C --mcu=${MMCU} 

C_BUILD = ${C_COMPILER} ${CXX_FLAGS} ${C_FLAGS} ${BUILD_DEFINES} ${INCLUDE_HEADER_COMPILE}
CPP_BUILD = ${CPP_COMPILER} ${CXX_FLAGS} ${CPP_FLAGS} ${BUILD_DEFINES} ${INCLUDE_HEADER_COMPILE}
LINKER = ${AVR_LINKER} ${LINKER_FLAGS}

# Folder defines
SRC_FOLDER = ./src
OBJ_FOLDER = ./obj
BIN_FOLDER = ./bin
MAIN_FILENAME = firmware.cpp

MAIN_FILE = ${SRC_FOLDER}/${MAIN_FILENAME}

# Makefile metadata
help:
	@echo     This makefile helps to compile waspmote projects
	@echo          make                  - shows this help message
	@echo          make help             - same as make
	@echo          make build            - build the whole project
	@echo          make flash            - upload firmware to board
	@echo          make update           - builds and uploads firmware
	@echo          make check_size       - util: shows program size
	@echo          make clean            - util: clean the obj and bin folder
	@echo     .
	@echo     Actual flags:
	@echo          MMCU      : "${MMCU}"
	@echo          MCU_PORT  : "${MCU_PORT}"
	@echo          PROGRAMMER: "${PROGRAMMER}"
	@echo          WASPMOTE_LIBRARIES_DEP: "${WASPMOTE_LIBRARIES_DEP}"
	@echo     . 
	@echo     Author: ${AUTHOR}
	@echo     Version: ${VERSION}

# Tarjets to build the waspmote core
WASPMOTE_FILES = $(filter %.c %.cpp, $(wildcard ${WASPMOTE_CORE_PATH}/*) $(wildcard ${WASPMOTE_CORE_PATH}/*/*))
WASPMOTE_C_FILES = $(filter %.c, ${WASPMOTE_FILES})
WASPMOTE_C_OBJECTS = ${WASPMOTE_C_FILES:%=%.o}
WASPMOTE_CPP_FILES = $(filter %.cpp, ${WASPMOTE_FILES})
WASPMOTE_CPP_OBJECTS = ${WASPMOTE_CPP_FILES:%=%.o}
WASPMOTE_STATIC_LIBRARIES = ${WASPMOTE_C_OBJECTS:%=%.a} ${WASPMOTE_CPP_OBJECTS:%=%.a}
WASPMOTE_CORE_OUTPUT = waspmote_core.a

${WASPMOTE_C_OBJECTS}:
	@echo Compiling "${patsubst %.o,%,$@}"
	@${C_BUILD} "${patsubst %.o,%,$@}" -o "${OBJ_FOLDER}/$(notdir $@)"

${WASPMOTE_CPP_OBJECTS}:
	@echo Compiling "${patsubst %.o,%,$@}"
	@${CPP_BUILD} "${patsubst %.o,%,$@}" -o "${OBJ_FOLDER}/$(notdir $@)"

${WASPMOTE_STATIC_LIBRARIES}: ${WASPMOTE_C_OBJECTS} ${WASPMOTE_CPP_OBJECTS}
	@echo Linking "${OBJ_FOLDER}/${WASPMOTE_CORE_OUTPUT}" from "${OBJ_FOLDER}/$(basename $(notdir $@))"
	@${LINKER} "${OBJ_FOLDER}/${WASPMOTE_CORE_OUTPUT}" "${OBJ_FOLDER}/$(basename $(notdir $@))"
	
say_compile_waspmote_core:
	@echo ----- Compilando waspmote core...

${WASPMOTE_CORE_OUTPUT}: say_compile_waspmote_core ${WASPMOTE_STATIC_LIBRARIES}

# Libraries targets
LIBRARIES_FILES = $(foreach lib,${WSP_LIB_FOLDER_DEP_PATH},$(call rwildcard,${lib}/,*.cpp) $(call rwildcard,${lib}/,*.c))
LIBRARIES_C_FILES = $(filter %.c, ${LIBRARIES_FILES})
LIBRARIES_C_OBJ_F = ${LIBRARIES_C_FILES:%=%.o}
LIBRARIES_CPP_FILES = $(filter %.cpp, ${LIBRARIES_FILES})
LIBRARIES_CPP_OBJ_F = ${LIBRARIES_CPP_FILES:%=%.o}
LIBRARIES_OBJECT_FILES = $(foreach lib,${LIBRARIES_C_OBJ_F} ${LIBRARIES_CPP_OBJ_F},${OBJ_FOLDER}/$(notdir ${lib}))

${LIBRARIES_C_OBJ_F}:
	@echo Compiling "${patsubst %.o,%,$@}"
	@${C_BUILD} "${patsubst %.o,%,$@}" -o "${OBJ_FOLDER}/$(notdir $@)"

${LIBRARIES_CPP_OBJ_F}:
	@echo Compiling "${patsubst %.o,%,$@}"
	@${CPP_BUILD} "${patsubst %.o,%,$@}" -o "${OBJ_FOLDER}/$(notdir $@)"

say_libraries:
	@echo ----- Librerias: ${WASPMOTE_LIBRARIES_DEP}

${LIBRARIES_OBJECT_FILES}: say_libraries ${LIBRARIES_C_OBJ_F} ${LIBRARIES_CPP_OBJ_F}

# Targets to build the hex file
MAIN_FILE_OBJ_OUTPUT = ${patsubst %.o,%,${MAIN_FILE}}
MAIN_FILE_BASENAME = ${basename ${MAIN_FILENAME}}
OUTPUT_ASSEMBLY = ${MAIN_FILE_BASENAME}.elf
OUTPUT_EEPROM = ${MAIN_FILE_BASENAME}.eep
OUTPUT_FLASH = ${MAIN_FILE_BASENAME}.hex

ASSEMBLER_LINK_FLAGS = -w -Os -Wl,--gc-sections -mmcu=${MMCU}
EEPROM_LINK_FLAGS = -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 
HEX_LINK_FLAGS = -O ihex -R .eeprom

say_source:
	@echo ----- Compilando source

${MAIN_FILENAME}.o: say_source
	@echo Compiling "${MAIN_FILE_OBJ_OUTPUT}"
	@${CPP_BUILD} "${MAIN_FILE_OBJ_OUTPUT}" -o "${OBJ_FOLDER}/$(notdir $@)"

${OUTPUT_ASSEMBLY}: ${WASPMOTE_CORE_OUTPUT} ${LIBRARIES_OBJECT_FILES} ${MAIN_FILENAME}.o
	@echo Linking all together... "${OUTPUT_ASSEMBLY}"
	@${C_COMPILER} ${ASSEMBLER_LINK_FLAGS} -o ${BIN_FOLDER}/${OUTPUT_ASSEMBLY} ${OBJ_FOLDER}/${WASPMOTE_CORE_OUTPUT} ${LIBRARIES_OBJECT_FILES} ${OBJ_FOLDER}/${MAIN_FILENAME}.o -L./obj/

${OUTPUT_EEPROM}:	${OUTPUT_ASSEMBLY}
	@echo Linking eeprom... "${OUTPUT_EEPROM}"
	@${OBJ_COPY} ${EEPROM_LINK_FLAGS} ${BIN_FOLDER}/${OUTPUT_ASSEMBLY} ${BIN_FOLDER}/${OUTPUT_EEPROM}

${OUTPUT_FLASH}: ${OUTPUT_EEPROM}
	@echo Linking flash... "${OUTPUT_FLASH}"
	@${OBJ_COPY} ${HEX_LINK_FLAGS} ${BIN_FOLDER}/${OUTPUT_ASSEMBLY} ${BIN_FOLDER}/${OUTPUT_FLASH}

# Util targets
check_size:
	@echo ----- Mostrando uso de memoria del firmware
	@${AVR_SIZE} ${AVR_SIZE_FLAGS} ${BIN_FOLDER}/${OUTPUT_ASSEMBLY}

# Main tarjets
build: ${OUTPUT_FLASH} check_size

flash:
	@echo ----- Subiendo firmware a la placa
	@${AVRDUDE} -C${WASPMOTE_AVRDUDE_CONF} -v -V -p${MMCU} -c${PROGRAMMER} -P${MCU_PORT} -b${PROGRAMMER_BAUDRATE}  -D -F  -Uflash:w:${BIN_FOLDER}/${OUTPUT_FLASH}:i

update: build flash

clean:
	@echo ----- Borrando archivos temporales
ifeq ($(OS),Windows_NT)
	@del obj\*.o obj\*.d obj\*.a bin\*.eep bin\*.elf bin\*.hex
else
	@rm obj/*.o obj/*.d obj/*.a bin/*.eep bin/*.elf bin/*.hex
endif

monitor:
	@pio device monitor -b 115200 -p ${MCU_PORT} 
	
csv:
	@python ./PlotSeries.py --port ${MCU_PORT}
