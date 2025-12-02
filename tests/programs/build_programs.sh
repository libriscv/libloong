build_program() {
	local src_file="$1"
	local output_file="$2"
	LINKER_ARGS="-Wl,-Ttext-segment=0x200000"
	EXTRA_ARGS="-mlasx -mmax-inline-memcpy-size=16"
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
build_program stream.c stream.elf
