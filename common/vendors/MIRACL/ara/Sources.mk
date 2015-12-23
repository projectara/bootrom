# Miracl Crypto Library
LIBCORE_SRC := $(LIB_DIR)/mcl_aes.c
LIBCORE_SRC += $(LIB_DIR)/mcl_gcm.c
LIBCORE_SRC += $(LIB_DIR)/mcl_hash.c
LIBCORE_SRC += $(LIB_DIR)/mcl_oct.c
LIBCORE_SRC += $(LIB_DIR)/mcl_rand.c
LIBCORE_SRC += $(LIB_DIR)/mcl_x509.c

LIBCURVE_SRC := $(LIB_DIR)/mcl_rom.c
LIBCURVE_SRC += $(LIB_DIR)/mcl_big.c
LIBCURVE_SRC += $(LIB_DIR)/mcl_ecp.c
LIBCURVE_SRC += $(LIB_DIR)/mcl_ff.c
LIBCURVE_SRC += $(LIB_DIR)/mcl_fp.c

# Smoke tests
STEST_SRC := $(TEST_DIR)/test_ecdh.c
STEST_SRC += $(TEST_DIR)/test_rsa.c
STEST_SRC += $(TEST_DIR)/test_hash.c

# Unit tests
TEST_SRC := $(TEST_DIR)/test_gcm_encrypt.c
ifeq ($(MCL_CHOICE),$(MCL_NIST256))
TEST_SRC += $(TEST_DIR)/test_x509.c
endif
ifeq ($(MCL_CHOICE),$(MCL_NIST384))
TEST_SRC += $(TEST_DIR)/test_x509.c
endif
ifeq ($(MCL_CHOICE),$(MCL_NIST521))
TEST_SRC += $(TEST_DIR)/test_x509.c
endif

# Benchmark tests
BENCH_SRC := $(BENCH_DIR)/time_ecdh.c
BENCH_SRC += $(BENCH_DIR)/time_rsa.c

# Tests with three curves
RTEST_SRC := $(TEST_DIR)/test_runtime.c
RTEST_SRC += $(TEST_DIR)/test_runtime_dev.c
