# Not quite right, but close.

all: build/Makefile
	make -C build

# Ideally, this would rebuild based on files added and removed, but
# that is hard to determine.
build/Makefile: CMakeLists.txt
	mkdir -p build
	(cd build; sh ../build-cmake.sh)
