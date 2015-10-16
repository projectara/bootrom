##
 # Copyright (c) 2015 Google Inc.
 # All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions are met:
 # 1. Redistributions of source code must retain the above copyright notice,
 # this list of conditions and the following disclaimer.
 # 2. Redistributions in binary form must reproduce the above copyright notice,
 # this list of conditions and the following disclaimer in the documentation
 # and/or other materials provided with the distribution.
 # 3. Neither the name of the copyright holder nor the names of its
 # contributors may be used to endorse or promote products derived from this
 # software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 # AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 # THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 # OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 # WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 # OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 # ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ##

#
# Coordinate all of the test-specific defines from here
#

ifdef _TESTING
# Test-specific enables
# NOTE: These defines are named such that we get a "production" configuration
# by eliminating all of them. As a result, 

#  _NOCRYPTO==1:  Simulate memory clearing and crypto for HW simulation
#  _NOCRYPTO!=1:  Use memory clearing and real crypto
ifeq ($(_NOCRYPTO),1)
  XCFLAGS += -D_NOCRYPTO
  XAFLAGS += -D_NOCRYPTO
endif

#  _DEBUG_MODE==1:  Enable debug messages
#  _DEBUG_MODE!=1:  Suppress debug messages
ifeq ($(_DEBUG_MODE),1)
XCFLAGS += -D_DEBUG
XAFLAGS += -D_DEBUG
CONFIG_DEBUG = y
endif

#  _DEBUGMSGS==1:  Enable debug messages
#  _DEBUGMSGS!=1:  Suppress debug messages
ifeq ($(_DBGPRINT),1)
XCFLAGS += -D_DEBUGMSGS
XAFLAGS += -D_DEBUGMSGS

# If _DME_LOGGING==1, DME writes are logged to dbgserial
ifeq ($(_DME_LOGGING),1)
XCFLAGS += -D_DME_LOGGING
XAFLAGS += -D_DME_LOGGING
endif
endif

ifeq ($(_GBBOOT_SERVER_STANDBY),1)
XCFLAGS += -D_GBBOOT_SERVER_STANDBY
XAFLAGS += -D_GBBOOT_SERVER_STANDBY
# this test depends on the _STANDBY_TEST
_STANDBY_TEST = 1
endif

ifeq ($(_STANDBY_TEST),1)
XCFLAGS += -D_STANDBY_TEST
XAFLAGS += -D_STANDBY_TEST
# this test depends on _HANDSHAKE
_HANDSHAKE = 1
endif

#  _HANDSHAKE==1:  GPIO handshake with simulation/test controller
#  _HANDSHAKE!=1:  No GPIO handshake with simulation/test controller
ifeq ($(_HANDSHAKE),1)
XCFLAGS += -D_HANDSHAKE
XAFLAGS += -D_HANDSHAKE

# HANDSHAKE depends on CONFIG_GPIO
CONFIG_GPIO = y

#  _TEST_HANDSHAKE defined:  Special handshake (via DME variable) with simulation/test controller
#  otherwise:  No Special handshake (via DME variable)  with simulation/test controller
# NOTES:
#    1. Only takes effect if _HANDSHAKE=1
#    2. Ensure all valid _TEST_HANDSHAKES are >= 1
ifdef _SPECIAL_TEST
XCFLAGS += -D_SPECIAL_TEST=$(_SPECIAL_TEST)
XAFLAGS += -D_SPECIAL_TEST=$(_SPECIAL_TEST)
endif
endif

#  _CLEAR_MIN_MEMORY==1:  Clear only the minimum of RAM at startup
#  _CLEAR_MIN_MEMORY!=1:  Clear all of RAM at startup
ifeq ($(_CLEAR_MIN_MEMORY),1)
XCFLAGS += -D_CLEAR_MIN_MEMORY
XAFLAGS += -D_CLEAR_MIN_MEMORY
endif

ifeq ($(_GEAR_CHANGE_TEST),1)
XCFLAGS += -D_GEAR_CHANGE_TEST
XAFLAGS += -D_GEAR_CHANGE_TEST
endif

## deprecate _SIMULATION!
##  _SIMULATION==1:  Simulate memory clearing and crypto for HW simulation
##  _SIMULATION!=1:  Use memory clearing and real crypto
#ifeq ($(_SIMULATION),1)
#XCFLAGS += -D_SIMULATION
#XAFLAGS += -D_SIMULATION
#endif
endif # _TESTING

