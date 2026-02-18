# ===== config =====
CMAKE ?= cmake
GENERATOR ?=

BUILD_DIR        ?= build
BUILD_DIR_DEBUG  ?= build-debug
BUILD_DIR_ASAN   ?= build-asan

TARGET ?= driver_facade_impl

# valgrind options
VALGRIND ?= valgrind
VG_OPTS  ?= --leak-check=full --show-leak-kinds=all --track-origins=yes

# ===== default =====
.PHONY: all
all: release

# ===== run =====
.PHONY: run
run: all
	./$(BUILD_DIR)/$(TARGET)

.PHONY: r
r: run 

# ===== test =====
.PHONY: test
test: all
	cd $(BUILD_DIR) && ctest --rerun-failed --output-on-failure

.PHONY: t
t: test

# ===== docs =====
.PHONY: docs
docs:
	doxygen Doxyfile

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

.PHONY: d
d: debug

# ===== asan =====
.PHONY: asan
asan:
	$(CMAKE) -S . -B $(BUILD_DIR_ASAN) \
		-DCMAKE_BUILD_TYPE=Debug \
		-DSANITIZE_ADDRESS=ON \
		$(GENERATOR)
	$(CMAKE) --build $(BUILD_DIR_ASAN)
	./$(BUILD_DIR_ASAN)/$(TARGET)
.PHONY: as
as: asan

# ===== valgrind =====
.PHONY: valgrind
valgrind: debug
	$(VALGRIND) $(VG_OPTS) ./$(BUILD_DIR_DEBUG)/$(TARGET)

.PHONY: v
v: valgrind 

# ===== clean =====
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BUILD_DIR_DEBUG) $(BUILD_DIR_ASAN)
