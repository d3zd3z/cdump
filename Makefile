# Not quite right, but close.

# all: build/Makefile
# 	$(MAKE) -C build

all: build/rules.ninja
	ninja-build -C build

# Ideally, this would rebuild based on files added and removed, but
# that is hard to determine.
build/Makefile: CMakeLists.txt
	mkdir -p build
	(cd build; sh ../build-cmake.sh)

build/rules.ninja: CMakeLists.txt
	mkdir -p build
	(cd build; sh ../build-cmake.sh)
