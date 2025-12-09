build_program() {
	local src_file="$1"
	local output_file="$2"
	LINKER_ARGS="-Wl,-Ttext-segment=0x200000"
	EXTRA_ARGS="-mlsx -mmax-inline-memcpy-size=16"
	loongarch64-linux-gnu-gcc-14 -O2 -gdwarf-4 -static  "$src_file" $LINKER_ARGS $EXTRA_ARGS -o "$output_file"
}
build_freestanding_program() {
	local src_file="$1"
	local output_file="$2"
	LINKER_ARGS="-Wl,-Ttext-segment=0x200000"
	EXTRA_ARGS="-ffreestanding -nostdlib -mmax-inline-memcpy-size=16"
	loongarch64-linux-gnu-gcc-14 -O2 -gdwarf-4 -static  "$src_file" $LINKER_ARGS $EXTRA_ARGS -o "$output_file"
}

build_program hello_world.c hello_world.elf
build_program printf_test.c printf_test.elf
build_program simple_add.c simple_add.elf
build_freestanding_program fib.c fib.elf
build_freestanding_program return_42_bare.c return_42_bare.elf
build_program stream.c stream.elf
build_program dhrystone.c dhrystone.elf
build_program signal_test.c signal_test.elf

loongarch64-linux-gnu-g++-14 -O2 -gdwarf-4 -static -Wl,-Ttext-segment=0x200000 cxx_test.cpp -o cxx_test.elf

pushd gotest
GOOS=linux GOARCH=loong64 go build -o gotest.elf -ldflags "-T 0x401000" gotest.go
popd
