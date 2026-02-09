# ===== config =====
CMAKE ?= cmake
GENERATOR ?=

BUILD_DIR        ?= build
BUILD_DIR_DEBUG  ?= build-debug
BUILD_DIR_ASAN   ?= build-asan

# executable name (adjust if needed)
TARGET ?= mymem_impl

# valgrind options
VALGRIND ?= valgrind
VG_OPTS  ?= --leak-check=full --show-leak-kinds=all --track-origins=yes

# ===== default =====
.PHONY: all
all: release

# ===== run =====
r: all
	./build/$(TARGET)

# ===== release =====
.PHONY: release
release:
	$(CMAKE) -S . -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=Release \
		$(GENERATOR)
	$(CMAKE) --build $(BUILD_DIR)

# ===== debug =====
.PHONY: debug
debug:
	$(CMAKE) -S . -B $(BUILD_DIR_DEBUG) \
		-DCMAKE_BUILD_TYPE=Debug \
		$(GENERATOR)
	$(CMAKE) --build $(BUILD_DIR_DEBUG)

# ===== asan =====
.PHONY: asan
asan:
	$(CMAKE) -S . -B $(BUILD_DIR_ASAN) \
		-DCMAKE_BUILD_TYPE=Debug \
		-DSANITIZE_ADDRESS=ON \
		$(GENERATOR)
	$(CMAKE) --build $(BUILD_DIR_ASAN)

# ===== valgrind =====
.PHONY: valgrind
valgrind: debug
	$(VALGRIND) $(VG_OPTS) ./$(BUILD_DIR_DEBUG)/$(TARGET)

# ===== clean =====
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BUILD_DIR_DEBUG) $(BUILD_DIR_ASAN)
