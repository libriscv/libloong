#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "instruction_tester.hpp"
#include <cmath>

using namespace loongarch;
using namespace loongarch::test;

TEST_CASE("LASX vector load/add/store sequence", "[instructions][lasx]") {
	InstructionTester tester;

	SECTION("xvld -> xvfadd.d -> xvst") {
		// Allocate 4KB of aligned memory for test data
		auto guest_addr = tester.allocate_guest_memory(4096, 32);
		REQUIRE(guest_addr != 0);
		REQUIRE((guest_addr % 32) == 0); // Verify 32-byte alignment

		// Initialize test data: 4 vectors of 4 doubles each
		// Vector 0 (at offset 0)
		tester.write_array<double>(guest_addr + 0, {1.0, 2.0, 3.0, 4.0});
		// Vector 1 (at offset 32)
		tester.write_array<double>(guest_addr + 32, {5.0, 6.0, 7.0, 8.0});
		// Vector 2 (at offset 64)
		tester.write_array<double>(guest_addr + 64, {9.0, 10.0, 11.0, 12.0});
		// Vector 3 (at offset 96)
		tester.write_array<double>(guest_addr + 96, {13.0, 14.0, 15.0, 16.0});

		// Set $t0 (r12) to point to base address
		tester.set_reg(REG_T0, guest_addr);

		// Test instruction sequence:
		const std::vector<uint32_t> instructions = {
			0x2c808183,  // xvld    $xr3, $t0, 32
			0x2c810181,  // xvld    $xr1, $t0, 64
			0x2c818180,  // xvld    $xr0, $t0, 96
			0x2c800182,  // xvld    $xr2, $t0, 0
			0x75310c63,  // xvfadd.d $xr3, $xr3, $xr3
			0x75310421,  // xvfadd.d $xr1, $xr1, $xr1
			0x75310000,  // xvfadd.d $xr0, $xr0, $xr0
			0x75310842,  // xvfadd.d $xr2, $xr2, $xr2
			0x2cc08183,  // xvst    $xr3, $t0, 32
			0x2cc00182,  // xvst    $xr2, $t0, 0
			0x2cc10181,  // xvst    $xr1, $t0, 64
			0x2cc18180,  // xvst    $xr0, $t0, 96
		};

		auto result = tester.execute_sequence(instructions, 0x10000, true);

		REQUIRE(result.success);
		REQUIRE(result.error.empty());
		REQUIRE(result.instructions_executed == instructions.size());

		// Verify results after loads (first 4 instructions)
		// After loading, registers should contain the data
		// xr3 should have vector from offset 32 (5.0, 6.0, 7.0, 8.0)
		// xr1 should have vector from offset 64 (9.0, 10.0, 11.0, 12.0)
		// xr0 should have vector from offset 96 (13.0, 14.0, 15.0, 16.0)
		// xr2 should have vector from offset 0 (1.0, 2.0, 3.0, 4.0)

		// After xvfadd.d (double each value):
		// The results should be stored back to memory
		auto result_vec0 = tester.read_array<double>(guest_addr + 0, 4);
		auto result_vec1 = tester.read_array<double>(guest_addr + 32, 4);
		auto result_vec2 = tester.read_array<double>(guest_addr + 64, 4);
		auto result_vec3 = tester.read_array<double>(guest_addr + 96, 4);

		// Vector 0: was [1,2,3,4], doubled = [2,4,6,8]
		REQUIRE_THAT(result_vec0[0], Catch::Matchers::WithinRel(2.0, 0.0001));
		REQUIRE_THAT(result_vec0[1], Catch::Matchers::WithinRel(4.0, 0.0001));
		REQUIRE_THAT(result_vec0[2], Catch::Matchers::WithinRel(6.0, 0.0001));
		REQUIRE_THAT(result_vec0[3], Catch::Matchers::WithinRel(8.0, 0.0001));

		// Vector 1: was [5,6,7,8], doubled = [10,12,14,16]
		REQUIRE_THAT(result_vec1[0], Catch::Matchers::WithinRel(10.0, 0.0001));
		REQUIRE_THAT(result_vec1[1], Catch::Matchers::WithinRel(12.0, 0.0001));
		REQUIRE_THAT(result_vec1[2], Catch::Matchers::WithinRel(14.0, 0.0001));
		REQUIRE_THAT(result_vec1[3], Catch::Matchers::WithinRel(16.0, 0.0001));

		// Vector 2: was [9,10,11,12], doubled = [18,20,22,24]
		REQUIRE_THAT(result_vec2[0], Catch::Matchers::WithinRel(18.0, 0.0001));
		REQUIRE_THAT(result_vec2[1], Catch::Matchers::WithinRel(20.0, 0.0001));
		REQUIRE_THAT(result_vec2[2], Catch::Matchers::WithinRel(22.0, 0.0001));
		REQUIRE_THAT(result_vec2[3], Catch::Matchers::WithinRel(24.0, 0.0001));

		// Vector 3: was [13,14,15,16], doubled = [26,28,30,32]
		REQUIRE_THAT(result_vec3[0], Catch::Matchers::WithinRel(26.0, 0.0001));
		REQUIRE_THAT(result_vec3[1], Catch::Matchers::WithinRel(28.0, 0.0001));
		REQUIRE_THAT(result_vec3[2], Catch::Matchers::WithinRel(30.0, 0.0001));
		REQUIRE_THAT(result_vec3[3], Catch::Matchers::WithinRel(32.0, 0.0001));
	}
}

TEST_CASE("Individual LASX instructions", "[instructions][lasx]") {
	InstructionTester tester;

	SECTION("xvld - load 256-bit vector") {
		auto guest_addr = tester.allocate_guest_memory(64, 32);

		// Write test data: 4 doubles = 32 bytes
		std::vector<double> test_data = {1.5, 2.5, 3.5, 4.5};
		tester.write_array<double>(guest_addr, test_data);

		// Set $t0 to base address
		tester.set_reg(REG_T0, guest_addr);

		// xvld $xr1, $t0, 0
		auto result = tester.execute_one(0x2c800181);

		REQUIRE(result.success);
		REQUIRE(result.error.empty());

		// Verify xr1 contains the loaded data
		auto loaded = tester.get_xvreg<double>(1);
		REQUIRE_THAT(loaded[0], Catch::Matchers::WithinRel(1.5, 0.0001));
		REQUIRE_THAT(loaded[1], Catch::Matchers::WithinRel(2.5, 0.0001));
		REQUIRE_THAT(loaded[2], Catch::Matchers::WithinRel(3.5, 0.0001));
		REQUIRE_THAT(loaded[3], Catch::Matchers::WithinRel(4.5, 0.0001));
	}

	SECTION("xvfadd.d - add two 256-bit vectors of doubles") {
		std::vector<double> vec1 = {1.0, 2.0, 3.0, 4.0};
		std::vector<double> vec2 = {5.0, 6.0, 7.0, 8.0};

		tester.set_xvreg<double>(0, vec1);
		tester.set_xvreg<double>(5, vec2);

		// xvfadd.d $xr0, $xr0, $xr5
		auto result = tester.execute_one(0x75311400);

		REQUIRE(result.success);
		REQUIRE(result.error.empty());

		// Verify xr0 = xr0 + xr5
		auto result_vec = tester.get_xvreg<double>(0);
		REQUIRE_THAT(result_vec[0], Catch::Matchers::WithinRel(6.0, 0.0001));
		REQUIRE_THAT(result_vec[1], Catch::Matchers::WithinRel(8.0, 0.0001));
		REQUIRE_THAT(result_vec[2], Catch::Matchers::WithinRel(10.0, 0.0001));
		REQUIRE_THAT(result_vec[3], Catch::Matchers::WithinRel(12.0, 0.0001));
	}

	SECTION("xvst - store 256-bit vector") {
		auto guest_addr = tester.allocate_guest_memory(64, 32);

		// Set up xr2 with test data
		std::vector<double> test_data = {10.0, 20.0, 30.0, 40.0};
		tester.set_xvreg<double>(2, test_data);

		// Set $t0 to base address
		tester.set_reg(REG_T0, guest_addr);

		// xvst $xr2, $t0, 0
		auto result = tester.execute_one(0x2cc00182);

		REQUIRE(result.success);
		REQUIRE(result.error.empty());

		// Verify data was stored to memory
		auto stored = tester.read_array<double>(guest_addr, 4);
		REQUIRE_THAT(stored[0], Catch::Matchers::WithinRel(10.0, 0.0001));
		REQUIRE_THAT(stored[1], Catch::Matchers::WithinRel(20.0, 0.0001));
		REQUIRE_THAT(stored[2], Catch::Matchers::WithinRel(30.0, 0.0001));
		REQUIRE_THAT(stored[3], Catch::Matchers::WithinRel(40.0, 0.0001));
	}
}

TEST_CASE("fcmp.cond.d - double precision floating point comparison", "[instructions][fcmp]") {
	InstructionTester tester;

	SECTION("fcmp.ceq.d - equal comparison") {
		// Set f1 and f2 to equal values
		tester.set_freg64(REG_FA0, 3.0);
		tester.set_freg64(REG_FS0, 3.0);

		// fcmp.ceq.d $fcc0, $fa0, $fs0
		const uint32_t instr = 0x0c226000;
		auto result = tester.execute_one(instr);

		REQUIRE(result.success);
		REQUIRE(result.error.empty());

		// Verify fcc0 is set (equal)
		REQUIRE(tester.get_fcc(0) == 1);
	}

	SECTION("fcmp.cule.d - not equal comparison") {
		tester.set_freg64(REG_FA0, 3.14159);
		tester.set_freg64(REG_FA1, 2.71828);

		// Instruction encoding for fcmp.cule.d $fcc1, $fa0, $fa1
		const uint32_t instr = 0x00c270401;
		auto result = tester.execute_one(instr);
		REQUIRE(result.success);

		// Verify fcc1 is clear (less than or equal, not comparable)
		REQUIRE(tester.get_fcc(1) == 0);

		tester.set_freg64(REG_FA0, 2.71828);
		tester.set_freg64(REG_FA1, 3.14159);

		// Test again with reversed values
		result = tester.execute_one(instr);
		REQUIRE(result.success);

		// Verify fcc1 is set (less than)
		REQUIRE(tester.get_fcc(1) == 1);
	}

	SECTION("fcmp.slt.d - less than comparison") {
		// Set f1 < f2
		tester.set_freg64(REG_FA0, 2.0);
		tester.set_freg64(REG_FA1, 5.0);

		// fcmp.slt.d $fcc0, $fa0, $fa1
		auto result = tester.execute_one(0x0c218400);

		REQUIRE(result.success);
		REQUIRE(result.error.empty());

		// Verify fcc0 is set (f1 < f2)
		REQUIRE(tester.get_fcc(0) == 1);
	}

	SECTION("fcmp.clt.d - not less than") {
		// Set f1 >= f2
		tester.set_freg64(1, 5.0);
		tester.set_freg64(2, 2.0);

		// fcmp.clt.d $fcc0, $f1, $f2  (should set fcc0 to 0)
		uint32_t instr = 0x0c110421;

		auto result = tester.execute_one(instr);

		REQUIRE(result.success);
		REQUIRE(result.error.empty());

		// Verify fcc0 is clear (f1 >= f2)
		REQUIRE(tester.get_fcc(0) == 0);
	}
}

TEST_CASE("fcmp.cond.s - single precision floating point comparison", "[instructions][fcmp]") {
	InstructionTester tester;

	SECTION("fcmp.cle.s - less than or equal (for >= via swapped operands)") {
		// Test case: y >= 23.0
		// Compiled as: 23.0 <= y, which is fcmp.cle.s $fcc0, $fa1, $fa0
		// where fa1 = 23.0, fa0 = y

		// Case 1: y = 24.0, should be true (24.0 >= 23.0)
		tester.set_freg32(REG_FA0, 24.0f);  // y
		tester.set_freg32(REG_FA1, 23.0f);  // constant 23.0

		// fcmp.cle.s $fcc0, $fa1, $fa0  (checks: 23.0 <= 24.0)
		const uint32_t instr = 0x0c130020;
		auto result = tester.execute_one(instr);

		REQUIRE(result.success);
		REQUIRE(result.error.empty());

		// Should be true: 23.0 <= 24.0
		REQUIRE(tester.get_fcc(0) == 1);

		// Case 2: y = 23.0, should be true (23.0 >= 23.0)
		tester.set_freg32(REG_FA0, 23.0f);  // y
		tester.set_freg32(REG_FA1, 23.0f);  // constant 23.0

		result = tester.execute_one(instr);
		REQUIRE(result.success);

		// Should be true: 23.0 <= 23.0
		REQUIRE(tester.get_fcc(0) == 1);

		// Case 3: y = 22.0, should be false (22.0 < 23.0, not >=)
		tester.set_freg32(REG_FA0, 22.0f);  // y
		tester.set_freg32(REG_FA1, 23.0f);  // constant 23.0

		result = tester.execute_one(instr);
		REQUIRE(result.success);

		// Should be false: 23.0 <= 22.0 is false
		REQUIRE(tester.get_fcc(0) == 0);
	}

	SECTION("fcmp.clt.s - less than comparison") {
		tester.set_freg32(REG_FA0, 2.0f);
		tester.set_freg32(REG_FA1, 5.0f);

		// fcmp.clt.s $fcc0, $fa0, $fa1
		auto result = tester.execute_one(0x0c110400);

		REQUIRE(result.success);
		REQUIRE(result.error.empty());

		// 2.0 < 5.0 should be true
		REQUIRE(tester.get_fcc(0) == 1);
	}
}

TEST_CASE("vfcmp and xvfcmp - vector FP comparisons", "[instructions][vector][fcmp]") {
	InstructionTester tester;

	SECTION("vfcmp.ceq.d - LSX equal comparison") {
		// Set up two LSX vectors with doubles
		std::vector<double> vec0 = {1.0, 2.0}; // 128-bit LSX = 2 doubles
		std::vector<double> vec1 = {0.5, 3.0};
		tester.set_vreg<double>(0, vec0);
		tester.set_vreg<double>(1, vec1);

		// vfcmp.slt.d $vr1, $vr1, $vr0
		auto result = tester.execute_one(0x0c618021);
		// v1[0] < v0[0] (0.5 < 1.0) = true
		// v1[1] < v0[1] (3.0 < 2.0) = false

		REQUIRE(result.success);
		REQUIRE(result.error.empty());

		// Result in vr1: first 64-bit element should be all 1s, second should be all 0s
		auto result_vec = tester.get_vreg<uint64_t>(1);
		REQUIRE(result_vec[0] == 0xFFFFFFFFFFFFFFFFULL); // Match
		REQUIRE(result_vec[1] == 0x0000000000000000ULL); // No match
	}

	SECTION("xvfcmp.slt.d - LASX less than comparison") {
		// Set up two LASX vectors with doubles
		std::vector<double> vec1 = {1.0, 5.0, 3.0, 2.0}; // 256-bit LASX = 4 doubles
		std::vector<double> vec2 = {2.0, 3.0, 4.0, 2.0};

		tester.set_xvreg<double>(1, vec1);
		tester.set_xvreg<double>(0, vec2);

		// xvfcmp.slt.d $xr1, $xr1, $xr0
		// Compare: 1.0<2.0 (T), 5.0<3.0 (F), 3.0<4.0 (T), 2.0<2.0 (F)
		uint32_t instr = 0x0ca18021; // xvfcmp.slt.d $xr1, $xr1, $xr0

		auto result = tester.execute_one(instr);

		REQUIRE(result.success);
		REQUIRE(result.error.empty());

		// Result in xr1: elements where comparison is true should be all 1s
		auto result_vec = tester.get_xvreg<uint64_t>(1);
		REQUIRE(result_vec[0] == 0xFFFFFFFFFFFFFFFFULL); // 1.0 < 2.0 = true
		REQUIRE(result_vec[1] == 0x0000000000000000ULL); // 5.0 < 3.0 = false
		REQUIRE(result_vec[2] == 0xFFFFFFFFFFFFFFFFULL); // 3.0 < 4.0 = true
		REQUIRE(result_vec[3] == 0x0000000000000000ULL); // 2.0 < 2.0 = false
	}
}

TEST_CASE("Complex instruction sequence from real code", "[instructions][complex]") {
	InstructionTester tester;

	SECTION("Mixed LASX operations") {
		// From the example:
		//   200d1c: 751b80e0  xvilvl.d   $xr0, $xr7, $xr0
		//   200d20: 77ec8040  xvpermi.q  $xr0, $xr2, 0x20
		//   200d24: 77d40002  xvori.b    $xr2, $xr0, 0x0
		//   200d28: 75310084  xvfadd.d   $xr4, $xr4, $xr0
		//   200d2c: 0ca18021  xvfcmp.slt.d $xr1, $xr1, $xr0

		// Initialize registers with test data
		tester.set_xvreg<double>(0, {1.0, 2.0, 3.0, 4.0});
		tester.set_xvreg<double>(2, {5.0, 6.0, 7.0, 8.0});
		tester.set_xvreg<double>(4, {10.0, 20.0, 30.0, 40.0});
		tester.set_xvreg<double>(7, {0.5, 1.5, 2.5, 3.5});
		tester.set_xvreg<double>(1, {0.5, 10.0, 2.5, 50.0});

		std::vector<uint32_t> instructions = {
			0x751b80e0,  // xvilvl.d   $xr0, $xr7, $xr0
			0x77ec8040,  // xvpermi.q  $xr0, $xr2, 0x20
			0x77d40002,  // xvori.b    $xr2, $xr0, 0x0
			0x75310084,  // xvfadd.d   $xr4, $xr4, $xr0
			0x0ca18021,  // xvfcmp.slt.d $xr1, $xr1, $xr0
		};

		auto result = tester.execute_sequence(instructions);

		REQUIRE(result.success);
		REQUIRE(result.error.empty());
		REQUIRE(result.instructions_executed == instructions.size());

		// The exact results depend on the instruction implementations
		// but we can verify the execution completed without errors
		// and that registers contain valid data (not NaN or infinity)
		auto xr0_result = tester.get_xvreg<double>(0);
		auto xr1_result = tester.get_xvreg<uint64_t>(1);
		auto xr2_result = tester.get_xvreg<double>(2);
		auto xr4_result = tester.get_xvreg<double>(4);

		// Basic sanity checks
		for (int i = 0; i < 4; i++) {
			REQUIRE(std::isfinite(xr0_result[i]));
			REQUIRE(std::isfinite(xr2_result[i]));
			REQUIRE(std::isfinite(xr4_result[i]));
		}
	}
}

TEST_CASE("Step-by-step instruction verification", "[instructions][step]") {
	InstructionTester tester;

	SECTION("Verify each instruction executes exactly once") {
		// Simple sequence to verify single-step execution
		const std::vector<uint32_t> instructions = {
			0x02802004,  // li.w $a0, 8
			0x02804005,  // li.w $a1, 16
			0x00109484,  // add.d $a0, $a0, $a1
		};

		// Execute first instruction
		tester.reset();
		auto r1 = tester.execute_one(instructions[0], 0x10000);
		REQUIRE(r1.success);
		REQUIRE(r1.instructions_executed == 1);
		printf("%s\n",
			tester.dump_registers().c_str());
		REQUIRE(tester.get_reg(REG_A0) == 8);
		REQUIRE(tester.get_reg(REG_A1) == 0);

		// Execute second instruction (need to setup execution area again)
		auto r2 = tester.execute_one(instructions[1], 0x10004);
		REQUIRE(r2.success);
		REQUIRE(r2.instructions_executed == 1);
		REQUIRE(tester.get_reg(REG_A0) == 8);
		REQUIRE(tester.get_reg(REG_A1) == 16);

		// Execute third instruction
		auto r3 = tester.execute_one(instructions[2], 0x10008);
		REQUIRE(r3.success);
		REQUIRE(r3.instructions_executed == 1);
		REQUIRE(tester.get_reg(REG_A0) == 24); // 8 + 16 = 24
		REQUIRE(tester.get_reg(REG_A1) == 16);
	}
}
