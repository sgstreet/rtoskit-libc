CC := gcc
CXX := g++
LD := g++
AR := ar
AS := as
OBJCOPY := objcopy
OBJDUMP := objdump
SIZE := size
NM := nm
INSTALL := install
INSTALL_STRIP := install --strip-program=strip -s

CPPFLAGS :=
ARFLAGS := cr
ASFLAGS := 

CFLAGS := -pipe -feliminate-unused-debug-types -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wunused -Wuninitialized -Wmissing-declarations -Werror -std=c23 -funwind-tables
CXXFLAGS := -pipe -feliminate-unused-debug-types -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wunused -Wuninitialized -Wmissing-declarations -Werror -std=gnu++23

LDSCRIPTS :=
LDFLAGS := -Wl,-O1 -Wl,--hash-style=gnu
LOADLIBES := 
LDLIBS :=

SOURCE_INTERLEAVE=0