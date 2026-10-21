CPPFLAGS += -DNDEBUG -DBUILD_TYPE_SPEED
CFLAGS += -Os -g3
CXXFLAGS += -Os -g3
ASFLAGS += -Os -g3
LDFLAGS += -Os -g3

# The speed flavour (PLAN D12, D73): -Os like release (a whole -O2 library breaks the
# +10 % size budget), with the arch files' speed kernels; a hot module may add
# LIBC_SPEED_CFLAGS to its own CFLAGS
export LIBC_FLAVOUR := speed
LIBC_SPEED_CFLAGS := -O2

ifeq (${ARCH_CROSS},host)
LISTING_STYLE := -adlns
endif
