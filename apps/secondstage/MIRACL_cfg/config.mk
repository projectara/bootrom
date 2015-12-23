# Short Weierstrass form curve
MCL_WEIERSTRASS:=0
# Edwards or Twisted Edwards curve  
MCL_EDWARDS:=1
# Montgomery form curve 
MCL_MONTGOMERY:=2

# NIST 256-bit standard curve - MCL_WEIERSTRASS only 
MCL_NIST256:=0
# Bernstein's Modulus 2^255-19 - MCL_EDWARDS or MCL_MONTGOMERY only 
MCL_C25519:=1
# Bernstein et al Curve41417 2^414-17 - MCL_EDWARDS only
MCL_C41417:=2  
# NIST 384-bit standard curve - MCL_WEIERSTRASS only
MCL_NIST384:=3
# NIST 521-bit standard curve - MCL_WEIERSTRASS only
MCL_NIST521:=4
# Goldilocks curve - MCL_EDWARDS only  
MCL_C448:=5

# Default configuration
# size of mcl_chunk in bits = wordlength of computer = 16, 32 or 64. 
# Note not all curve options are supported on 16-bit processors - see rom.c
MCL_CHUNK:=32
# Current choice of Elliptic Curve (see mcl_arch.h)
MCL_CHOICE:=$(MCL_C25519)
# type of curve  (see mcl_arch.h) 
MCL_CURVETYPE:=$(MCL_EDWARDS)
# 2^n multiplier of MCL_BIGBITS to specify supported Finite Field size, 
# e.g 2048=256*2^3 where MCL_BIGBITS=256  (see mcl_config.h) 
MCL_FFLEN:=8


