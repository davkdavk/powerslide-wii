# Makefile for Wii (devkitPPC) - Powerslide Remake

# Check for devkitPPC
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=/opt/devkitpro/devkitPPC")
endif

# Set DEVKITPRO
export DEVKITPRO	:=	/opt/devkitpro

include $(DEVKITPPC)/wii_rules

.DEFAULT_GOAL := all

# Project settings
TITLE		:=	PowerslideRemake
APP_ID		:=	PWSL0000
VERSION		:=	1.0
TARGET		:=	powerslide
BUILD		:=	wii_build
OBJ_DIR		:=	$(BUILD)/obj

# Compiler flags
CFLAGS		:=	-g -O2 -Wall $(MACHDEP) \
			-I$(CURDIR)/wii_stubs/OGRE \
			-I$(CURDIR)/wii_stubs/OIS \
			-I$(CURDIR)/wii_stubs/AL \
			-I$(CURDIR)/wii_stubs/SFML \
			-I$(CURDIR)/wii_stubs \
			-I$(CURDIR) \
			-I$(CURDIR)/orig_src \
			-I$(CURDIR)/orig_src/includes \
			-I$(CURDIR)/game \
			-I$(CURDIR)/game/lua \
			-I$(LIBOGC_INC) $(INCLUDE) \
			-DNO_OPENAL -DWII -D__wii__ -DGEKKO -DNO_OGRE_PLUGINS -DOGRE_STATIC_LIB -DWII_FULL_BUILD -DWII_NATIVE_ASSET_PIPELINE -DNDEBUG
CXXFLAGS	:=	$(CFLAGS) -fno-exceptions -fno-rtti -std=gnu++17

# Linker
export LD	:=	$(CXX)


# Libraries - order matters!
LIBS		:=	-lfat -lwiiuse -lbte -logc -lm

# Library paths  
LIBPATHS	:=	-L$(LIBOGC_LIB) -L$(DEVKITPPC)/lib

# Source files
ORIG_SOURCES	:=	$(shell find $(CURDIR)/orig_src -name '*.cpp')
STUB_SOURCES	:=	$(shell find $(CURDIR)/wii_stubs -name '*.cpp')
SOURCES		:=	$(ORIG_SOURCES) $(STUB_SOURCES)

# Object files - mirror source tree under build/obj
OBJS		:=	$(patsubst $(CURDIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

# Output files
ELF		:=	$(BUILD)/$(TARGET).elf
DOL		:=	$(BUILD)/$(TARGET).dol
MAP_FILE	:=	$(BUILD)/$(TARGET).map

LDFLAGS		:=	-g $(MACHDEP) -Wl,-Map,$(MAP_FILE) -T $(DEVKITPPC)/powerpc-eabi/lib/rvl.ld

# Ensure build directories exist
$(BUILD):
	@mkdir -p $(BUILD)

$(OBJ_DIR): | $(BUILD)
	@mkdir -p $(OBJ_DIR)

# Compile source files
$(OBJ_DIR)/%.o: $(CURDIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link step
$(ELF): $(OBJS) | $(BUILD)
	@echo "Linking $@"
	$(LD) $^ $(LDFLAGS) $(LIBPATHS) $(LIBS) -o $@

$(DOL): $(ELF)
	@echo "Creating DOL..."
	$(DEVKITPRO)/tools/bin/elf2dol $< $@
	@ls -la $@

.PHONY: clean info all

all:	$(ELF) $(DOL)

clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).dol

info:
	@echo "Title: $(TITLE)"
	@echo "App ID: $(APP_ID)"
	@echo "Version: $(VERSION)"
	@echo "DEVKITPPC: $(DEVKITPPC)"
	@echo "LIBOGC: $(LIBOGC_INC)"
