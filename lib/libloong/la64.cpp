#include "machine.hpp"
#include "la_instr.hpp"
#include "la_instr_impl.hpp"
#include "la_instr_printers.hpp"
#include "la_instr_atomic.hpp"

namespace loongarch
{
	// LA64 instruction table using template implementations

	using Impl = InstrImpl<LA64>;
	using Printers = InstrPrinters<LA64>;
	using AtomicI = AtomicImpl<LA64>;
	using AtomicP = AtomicPrinters<LA64>;

	// Helper macro to create instruction descriptors
	// All instructions must have printers for debugging
	#define INSTRUCTION(name) \
		static constexpr CPU<LA64>::instruction_t instr64_##name { Impl::name, Printers::name }

	// Macro to create instruction with custom printer
	#define INSTRUCTION_P(name, printer) \
		static constexpr CPU<LA64>::instruction_t instr64_##name { Impl::name, Printers::printer }

	#define DECODED_INSTR(name) instr64_##name

	// Create all instruction descriptors
	INSTRUCTION(INVALID);
	INSTRUCTION(UNIMPLEMENTED);
	INSTRUCTION(NOP);

	// Arithmetic
	INSTRUCTION(ADD_W);
	INSTRUCTION(ADD_D);
	INSTRUCTION(SUB_W);
	INSTRUCTION(SUB_D);
	INSTRUCTION(SLT);
	INSTRUCTION(SLTU);
	INSTRUCTION(ADDI_W);
	INSTRUCTION(ADDI_D);

	// Division/Modulo
	INSTRUCTION(DIV_W);
	INSTRUCTION(MOD_W);
	INSTRUCTION(DIV_WU);
	INSTRUCTION(MOD_WU);
	INSTRUCTION(DIV_D);
	INSTRUCTION(MOD_D);
	INSTRUCTION(DIV_DU);
	INSTRUCTION(MOD_DU);

	// Logical
	INSTRUCTION(AND);
	INSTRUCTION(OR);
	INSTRUCTION(XOR);
	INSTRUCTION(NOR);
	INSTRUCTION(ORN);
	INSTRUCTION(ANDN);
	INSTRUCTION(MASKEQZ);
	INSTRUCTION(MASKNEZ);
	INSTRUCTION(ANDI);
	INSTRUCTION(ORI);
	INSTRUCTION(XORI);
	INSTRUCTION(BYTEPICK_D);

	// Shift
	INSTRUCTION(SLL_W);
	INSTRUCTION(SRL_W);
	INSTRUCTION(SRA_W);
	INSTRUCTION(SLL_D);
	INSTRUCTION(SRL_D);
	INSTRUCTION(SRA_D);
	INSTRUCTION(ALSL_D);
	INSTRUCTION(SLLI_W);
	INSTRUCTION(SLLI_D);
	INSTRUCTION(SRLI_W);
	INSTRUCTION(SRLI_D);
	INSTRUCTION(SRAI_W);
	INSTRUCTION(SRAI_D);
	INSTRUCTION(ROTRI_D);

	// Load/Store
	INSTRUCTION(LD_B);
	INSTRUCTION(LD_H);
	INSTRUCTION(LD_W);
	INSTRUCTION(LD_D);
	INSTRUCTION(LD_BU);
	INSTRUCTION(LD_HU);
	INSTRUCTION(LD_WU);
	INSTRUCTION(ST_B);
	INSTRUCTION(ST_H);
	INSTRUCTION(ST_W);
	INSTRUCTION(ST_D);
	INSTRUCTION(LDPTR_W);
	INSTRUCTION(STPTR_W);
	INSTRUCTION(LDPTR_D);
	INSTRUCTION(STPTR_D);
	INSTRUCTION(STX_B);
	INSTRUCTION(STX_H);
	INSTRUCTION(STX_W);
	INSTRUCTION(STX_D);

	// Floating-point load/store
	INSTRUCTION(FLD_S);
	INSTRUCTION(FST_S);
	INSTRUCTION(FLD_D);
	INSTRUCTION(FST_D);

	// Atomic operations (using separate template class)
	static constexpr CPU<LA64>::instruction_t instr64_AMSWAP_W { AtomicI::AMSWAP_W, AtomicP::AMSWAP_W };
	static constexpr CPU<LA64>::instruction_t instr64_AMSWAP_D { AtomicI::AMSWAP_D, AtomicP::AMSWAP_D };
	static constexpr CPU<LA64>::instruction_t instr64_AMADD_W { AtomicI::AMADD_W, AtomicP::AMADD_W };
	static constexpr CPU<LA64>::instruction_t instr64_AMADD_D { AtomicI::AMADD_D, AtomicP::AMADD_D };
	static constexpr CPU<LA64>::instruction_t instr64_AMAND_W { AtomicI::AMAND_W, AtomicP::AMAND_W };
	static constexpr CPU<LA64>::instruction_t instr64_AMAND_D { AtomicI::AMAND_D, AtomicP::AMAND_D };
	static constexpr CPU<LA64>::instruction_t instr64_AMOR_W { AtomicI::AMOR_W, AtomicP::AMOR_W };
	static constexpr CPU<LA64>::instruction_t instr64_AMOR_D { AtomicI::AMOR_D, AtomicP::AMOR_D };
	static constexpr CPU<LA64>::instruction_t instr64_AMXOR_W { AtomicI::AMXOR_W, AtomicP::AMXOR_W };
	static constexpr CPU<LA64>::instruction_t instr64_AMXOR_D { AtomicI::AMXOR_D, AtomicP::AMXOR_D };

	// Branches
	INSTRUCTION(BEQZ);
	INSTRUCTION(BNEZ);
	INSTRUCTION(BEQ);
	INSTRUCTION(BNE);
	INSTRUCTION(BLT);
	INSTRUCTION(BGE);
	INSTRUCTION(BLTU);
	INSTRUCTION(BGEU);
	INSTRUCTION(B);
	INSTRUCTION(BL);
	INSTRUCTION(JIRL);

	// Upper Immediate
	INSTRUCTION(LU12I_W);
	INSTRUCTION(LU32I_D);
	INSTRUCTION(PCADDI);
	INSTRUCTION(PCADDU12I);
	INSTRUCTION(PCALAU12I);
	INSTRUCTION(PCADDU18I);
	INSTRUCTION(LU52I_D);

	// Bit Manipulation
	INSTRUCTION(BSTRINS_D);
	INSTRUCTION(BSTRPICK_D);
	INSTRUCTION(BSTRPICK_W);

	// System
	INSTRUCTION(SYSCALL);

	// Memory barriers
	INSTRUCTION(DBAR);
	INSTRUCTION(IBAR);

	// LL/SC atomics
	INSTRUCTION(LL_W);
	INSTRUCTION(LL_D);
	INSTRUCTION(SC_W);
	INSTRUCTION(SC_D);

	// Indexed loads
	INSTRUCTION(LDX_B);
	INSTRUCTION(LDX_H);
	INSTRUCTION(LDX_W);
	INSTRUCTION(LDX_D);
	INSTRUCTION(LDX_BU);
	INSTRUCTION(LDX_HU);
	INSTRUCTION(LDX_WU);

	// Multiply
	INSTRUCTION(MUL_W);
	INSTRUCTION(MULH_W);
	INSTRUCTION(MULH_WU);
	INSTRUCTION(MUL_D);
	INSTRUCTION(MULH_D);
	INSTRUCTION(MULH_DU);

	// Comparison immediate
	INSTRUCTION(SLTI);
	INSTRUCTION(SLTUI);

	// Rotate
	INSTRUCTION(ROTR_W);
	INSTRUCTION(ROTR_D);
	INSTRUCTION(ROTRI_W);

	// Bit manipulation
	INSTRUCTION(EXT_W_B);
	INSTRUCTION(EXT_W_H);
	INSTRUCTION(CLO_W);
	INSTRUCTION(CLZ_W);
	INSTRUCTION(CTO_W);
	INSTRUCTION(CTZ_W);
	INSTRUCTION(CLO_D);
	INSTRUCTION(CLZ_D);
	INSTRUCTION(CTO_D);
	INSTRUCTION(CTZ_D);
	INSTRUCTION(REVB_2H);
	INSTRUCTION(REVB_4H);
	INSTRUCTION(REVB_2W);
	INSTRUCTION(REVB_D);
	INSTRUCTION(REVH_2W);
	INSTRUCTION(REVH_D);
	INSTRUCTION(BITREV_4B);
	INSTRUCTION(BITREV_8B);
	INSTRUCTION(BITREV_W);
	INSTRUCTION(BITREV_D);

	// ALSL.W
	INSTRUCTION(ALSL_W);

	// Vector Load/Store (LSX)
	INSTRUCTION(VLD);
	INSTRUCTION(VST);

	// Vector Load/Store (LASX - 256-bit)
	INSTRUCTION(XVLD);
	INSTRUCTION(XVST);

	// LASX (256-bit) Instructions
	INSTRUCTION(XVREPLGR2VR_B);
	INSTRUCTION(XVXOR_V);
	INSTRUCTION(XVADD_D);
	INSTRUCTION(XVSUB_W);
	INSTRUCTION(XVPICKVE2GR_W);
	INSTRUCTION(XVHADDW_D_W);
	INSTRUCTION(XVHADDW_Q_D);
	INSTRUCTION(XVBITSEL_V);
	INSTRUCTION(XVFCMP_COND_D);
	INSTRUCTION(XVMIN_BU);
	INSTRUCTION(XVMAX_BU);
	INSTRUCTION(XVMSKNZ_B);
	INSTRUCTION(XVPICKVE_W);
	INSTRUCTION(XVSETANYEQZ_B);
	INSTRUCTION(XVSEQ_B);
	INSTRUCTION(XVSETEQZ_V);
	INSTRUCTION(XVPERMI_Q);
	INSTRUCTION(XVLDX);
	INSTRUCTION(XVSTX);
	INSTRUCTION(XVFADD_D);
	INSTRUCTION(XVFMUL_D);
	INSTRUCTION(XVFDIV_D);
	INSTRUCTION(XVFMADD_S);
	INSTRUCTION(XVFMADD_D);
	INSTRUCTION(XVFMSUB_S);
	INSTRUCTION(XVFMSUB_D);
	INSTRUCTION(XVFNMADD_S);
	INSTRUCTION(XVFNMADD_D);
	INSTRUCTION(XVFNMSUB_S);
	INSTRUCTION(XVFNMSUB_D);
	INSTRUCTION(XVORI_B);
	INSTRUCTION(XVXORI_B);
	INSTRUCTION(XVILVL_D);
	INSTRUCTION(XVILVH_D);
	INSTRUCTION(XVPERMI_D);
	INSTRUCTION(XVPACKEV_D);
	INSTRUCTION(XVPACKOD_D);
	INSTRUCTION(XVPICKEV_D);
	INSTRUCTION(XVPICKEV_W);
	INSTRUCTION(XVPICKOD_D);
	INSTRUCTION(XVLDI);

	// Additional LSX Instructions
	INSTRUCTION(VSETANYEQZ_B);
	INSTRUCTION(VSETALLNEZ_B);
	INSTRUCTION(VMSKNZ_B);
	INSTRUCTION(BCNEZ);
	INSTRUCTION(BCEQZ);

	// Vector element extraction
	INSTRUCTION_P(VPICKVE2GR_B, VPICKVE2GR);
	INSTRUCTION_P(VPICKVE2GR_H, VPICKVE2GR);
	INSTRUCTION_P(VPICKVE2GR_W, VPICKVE2GR);
	INSTRUCTION_P(VPICKVE2GR_D, VPICKVE2GR);
	INSTRUCTION_P(VPICKVE2GR_BU, VPICKVE2GR);
	INSTRUCTION_P(VPICKVE2GR_HU, VPICKVE2GR);
	INSTRUCTION_P(VPICKVE2GR_WU, VPICKVE2GR);
	INSTRUCTION_P(VPICKVE2GR_DU, VPICKVE2GR);

	// Vector interleave
	INSTRUCTION_P(VILVL_B, VILVL);
	INSTRUCTION_P(VILVL_H, VILVL);
	INSTRUCTION_P(VILVL_W, VILVL);
	INSTRUCTION_P(VILVL_D, VILVL);
	INSTRUCTION(VILVH_D);
	INSTRUCTION(VPICKEV_W);

	// Vector arithmetic/logic
	INSTRUCTION_P(VSUB_B, VSUB);
	INSTRUCTION_P(VSUB_H, VSUB);
	INSTRUCTION_P(VSUB_W, VSUB);
	INSTRUCTION_P(VSUB_D, VSUB);
	INSTRUCTION_P(VMUL_B, VMUL);
	INSTRUCTION_P(VMUL_H, VMUL);
	INSTRUCTION_P(VMUL_W, VMUL);
	INSTRUCTION_P(VMUL_D, VMUL);
	INSTRUCTION_P(VMADD_B, VMADD);
	INSTRUCTION_P(VMADD_H, VMADD);
	INSTRUCTION_P(VMADD_W, VMADD);
	INSTRUCTION_P(VMADD_D, VMADD);
	INSTRUCTION_P(VADDI_BU, VADDI);
	INSTRUCTION_P(VADDI_HU, VADDI);
	INSTRUCTION_P(VADDI_WU, VADDI);
	INSTRUCTION_P(VADDI_DU, VADDI);
	INSTRUCTION(VHADDW_D_W);
	INSTRUCTION(VSEQ_B);
	INSTRUCTION_P(VSLT_B, VSLT);
	INSTRUCTION_P(VSLT_H, VSLT);
	INSTRUCTION_P(VSLT_W, VSLT);
	INSTRUCTION_P(VSLT_D, VSLT);
	INSTRUCTION(VNOR_V);
	INSTRUCTION(VORN_V);
	INSTRUCTION(VAND_V);
	INSTRUCTION(VFADD_D);
	INSTRUCTION(VFDIV_D);
	INSTRUCTION_P(VFMUL_S, VFMUL);
	INSTRUCTION_P(VFMUL_D, VFMUL);
	INSTRUCTION(VFTINTRZ_W_S);
	INSTRUCTION(VFTINTRZ_L_D);
	INSTRUCTION(VBITREVI_D);
	INSTRUCTION(VORI_B);
	INSTRUCTION(VLDX);
	INSTRUCTION(VSTX);
	INSTRUCTION(VFMADD_D);
	INSTRUCTION(VFNMADD_D);
	INSTRUCTION(VOR_V);
	INSTRUCTION(VXOR_V);

	// Vector replicate
	INSTRUCTION_P(VREPLGR2VR_B, VREPLGR2VR);
	INSTRUCTION_P(VREPLGR2VR_H, VREPLGR2VR);
	INSTRUCTION_P(VREPLGR2VR_W, VREPLGR2VR);
	INSTRUCTION_P(VREPLGR2VR_D, VREPLGR2VR);
	INSTRUCTION_P(VINSGR2VR_B, VINSGR2VR);
	INSTRUCTION_P(VINSGR2VR_H, VINSGR2VR);
	INSTRUCTION_P(VINSGR2VR_W, VINSGR2VR);
	INSTRUCTION_P(VINSGR2VR_D, VINSGR2VR);
	INSTRUCTION(VREPLVEI_D);

	// Vector immediate arithmetic
	INSTRUCTION_P(VADD_B, VADD);
	INSTRUCTION_P(VADD_H, VADD);
	INSTRUCTION_P(VADD_W, VADD);
	INSTRUCTION_P(VADD_D, VADD);
	INSTRUCTION(VSHUF_B);
	INSTRUCTION(VBITSEL_V);
	INSTRUCTION_P(VMAX_B, VMAX);
	INSTRUCTION_P(VMAX_H, VMAX);
	INSTRUCTION_P(VMAX_W, VMAX);
	INSTRUCTION_P(VMAX_D, VMAX);
	INSTRUCTION_P(VMAX_BU, VMAX);
	INSTRUCTION_P(VMAX_HU, VMAX);
	INSTRUCTION_P(VMAX_WU, VMAX);
	INSTRUCTION_P(VMAX_DU, VMAX);
	INSTRUCTION_P(VMIN_B, VMIN);
	INSTRUCTION_P(VMIN_H, VMIN);
	INSTRUCTION_P(VMIN_W, VMIN);
	INSTRUCTION_P(VMIN_D, VMIN);
	INSTRUCTION_P(VMIN_BU, VMIN);
	INSTRUCTION_P(VMIN_HU, VMIN);
	INSTRUCTION_P(VMIN_WU, VMIN);
	INSTRUCTION_P(VMIN_DU, VMIN);
	INSTRUCTION_P(VSEQI_B, VSEQI);
	INSTRUCTION_P(VSEQI_H, VSEQI);
	INSTRUCTION_P(VSEQI_W, VSEQI);
	INSTRUCTION_P(VSEQI_D, VSEQI);
	INSTRUCTION(VFRSTPI_B);
	INSTRUCTION(VLDI);

	// FP/Vector to GPR
	INSTRUCTION(MOVFR2GR_S);
	INSTRUCTION(MOVFR2GR_D);
	INSTRUCTION(MOVGR2FR_W);
	INSTRUCTION(MOVGR2FR_D);
	INSTRUCTION(MOVFCSR2GR);
	INSTRUCTION(MOVFR2CF);
	INSTRUCTION(MOVCF2FR);
	INSTRUCTION(MOVGR2CF);
	INSTRUCTION(MOVCF2GR);
	INSTRUCTION(VFCMP_COND_D);
	INSTRUCTION(FSEL);
	INSTRUCTION(FABS_D);
	INSTRUCTION(FNEG_D);
	INSTRUCTION(FMOV_D);
	INSTRUCTION_P(FCLASS_S, FCLASS);
	INSTRUCTION_P(FCLASS_D, FCLASS);
	INSTRUCTION(FFINT_D_L);
	INSTRUCTION(FFINT_D_W);
	INSTRUCTION(FFINT_S_W);
	INSTRUCTION(FFINT_S_L);
	INSTRUCTION(FTINTRZ_W_S);
	INSTRUCTION(FTINTRZ_W_D);
	INSTRUCTION(FTINTRZ_L_S);
	INSTRUCTION(FTINTRZ_L_D);
	INSTRUCTION(FADD_D);
	INSTRUCTION(FMUL_D);
	INSTRUCTION(FSUB_D);
	INSTRUCTION(FDIV_D);
	INSTRUCTION(FMSUB_D);
	INSTRUCTION(FMADD_D);
	INSTRUCTION(FLDX_D);
	INSTRUCTION(FSTX_D);
	INSTRUCTION(FCMP_COND_D);

	// Decode function
	template <>
	const CPU<LA64>::instruction_t& CPU<LA64>::decode(format_t instr)
	{
		uint32_t opcode = instr.whole & 0xFC000000;
		uint32_t op6 = opcode >> 26;
		uint32_t op22 = instr.whole & 0xFFC00000;
		uint32_t op17 = instr.whole & 0xFFFF8000;

		// System instructions (checked first for exact match)
		if (instr.whole == Opcode::SYSCALL) return DECODED_INSTR(SYSCALL);
		if (instr.whole == Opcode::BREAK) return DECODED_INSTR(UNIMPLEMENTED);
		if (instr.whole == 0) return DECODED_INSTR(NOP);

		// Decode based on opcode
		switch (op6) {
		case 0x00: // 3R-type ALU operations + bit manipulation + immediates
			// NOTE: LSX vector instructions should NOT be in case 0x00 (they use op6=0x1C)

			// Byte manipulation (check bits[31:18] for bytepick.d)
			if ((instr.whole & 0xFFFC0000) == 0x000C0000) return DECODED_INSTR(BYTEPICK_D);

			// 3R-type instructions (need exact op17 match)
			if (op17 == Opcode::ADD_W) return DECODED_INSTR(ADD_W);
			if (op17 == Opcode::ADD_D) return DECODED_INSTR(ADD_D);
			if (op17 == Opcode::SUB_W) return DECODED_INSTR(SUB_W);
			if (op17 == Opcode::SUB_D) return DECODED_INSTR(SUB_D);
			if (op17 == Opcode::SLT) return DECODED_INSTR(SLT);
			if (op17 == Opcode::SLTU) return DECODED_INSTR(SLTU);
			if (op17 == Opcode::AND) return DECODED_INSTR(AND);
			if (op17 == Opcode::OR) return DECODED_INSTR(OR);
			if (op17 == Opcode::XOR) return DECODED_INSTR(XOR);
			if (op17 == Opcode::NOR) return DECODED_INSTR(NOR);
			if (op17 == 0x00160000) return DECODED_INSTR(ORN);
			if (op17 == 0x00168000) return DECODED_INSTR(ANDN);
			if (op17 == Opcode::MASKEQZ) return DECODED_INSTR(MASKEQZ);
			if (op17 == Opcode::MASKNEZ) return DECODED_INSTR(MASKNEZ);
			if (op17 == Opcode::SLL_W) return DECODED_INSTR(SLL_W);
			if (op17 == Opcode::SRL_W) return DECODED_INSTR(SRL_W);
			if (op17 == Opcode::SRA_W) return DECODED_INSTR(SRA_W);
			if (op17 == Opcode::SLL_D) return DECODED_INSTR(SLL_D);
			if (op17 == Opcode::SRL_D) return DECODED_INSTR(SRL_D);
			if (op17 == Opcode::SRA_D) return DECODED_INSTR(SRA_D);

			// Multiply instructions
			if (op17 == Opcode::MUL_W) return DECODED_INSTR(MUL_W);
			if (op17 == Opcode::MULH_W) return DECODED_INSTR(MULH_W);
			if (op17 == Opcode::MULH_WU) return DECODED_INSTR(MULH_WU);
			if (op17 == Opcode::MUL_D) return DECODED_INSTR(MUL_D);
			if (op17 == Opcode::MULH_D) return DECODED_INSTR(MULH_D);
			if (op17 == Opcode::MULH_DU) return DECODED_INSTR(MULH_DU);

			// Rotate instructions
			if (op17 == Opcode::ROTR_W) return DECODED_INSTR(ROTR_W);
			if (op17 == Opcode::ROTR_D) return DECODED_INSTR(ROTR_D);

			// FADD.D: op17 = 0x01010000
			if (op17 == 0x01010000) return DECODED_INSTR(FADD_D);
			// FMUL.D: op17 = 0x01050000
			if (op17 == 0x01050000) return DECODED_INSTR(FMUL_D);
			// FSUB.D: op17 = 0x01030000
			if (op17 == 0x01030000) return DECODED_INSTR(FSUB_D);
			// FDIV.D: op17 = 0x01070000
			if (op17 == 0x01070000) return DECODED_INSTR(FDIV_D);

			// Division/Modulo instructions
			if (op17 == Opcode::DIV_W) return DECODED_INSTR(DIV_W);
			if (op17 == Opcode::MOD_W) return DECODED_INSTR(MOD_W);
			if (op17 == Opcode::DIV_WU) return DECODED_INSTR(DIV_WU);
			if (op17 == Opcode::MOD_WU) return DECODED_INSTR(MOD_WU);
			if (op17 == Opcode::DIV_D) return DECODED_INSTR(DIV_D);
			if (op17 == Opcode::MOD_D) return DECODED_INSTR(MOD_D);
			if (op17 == Opcode::DIV_DU) return DECODED_INSTR(DIV_DU);
			if (op17 == Opcode::MOD_DU) return DECODED_INSTR(MOD_DU);

			// Shift immediate instructions (use bits[31:16])
			{
				uint32_t op16 = (instr.whole >> 16) & 0xFFFF;
				if (op16 == 0x0040) return DECODED_INSTR(SLLI_W);
				if (op16 == 0x0041) return DECODED_INSTR(SLLI_D);
				if (op16 == 0x0044) return DECODED_INSTR(SRLI_W);
				if (op16 == 0x0045) return DECODED_INSTR(SRLI_D);
				if (op16 == 0x0048) return DECODED_INSTR(SRAI_W);
				if (op16 == 0x0049) return DECODED_INSTR(SRAI_D);
				// ROTRI.W: bits[31:16] = 0x0049, but with different ui5 position than SRAI.D
				// Need to check more bits: ROTRI.W has bit pattern 0000 0000 0100 1001 (ui5 in bits[14:10])
				// SRAI.D has bit pattern 0000 0000 0100 1001 (ui6 in bits[15:10])
				// They overlap, so we distinguish by checking if bit 15 is used for shift amount
				// Actually ROTRI.W is bits[31:21] = 0x00049 with ui5 in [14:10]
				uint32_t op11 = (instr.whole >> 21) & 0x7FF;
				if (op11 == 0x009) {  // 0x009 << 10 bits = 0x2400 for top bits check
					// Check if this is ROTRI.W (ui5 < 32) vs SRAI.D (ui6 can be up to 63)
					uint32_t shift_amt = (instr.whole >> 10) & 0x3F;
					if (shift_amt < 32 && ((instr.whole >> 15) & 1) == 0)
						return DECODED_INSTR(ROTRI_W);
				}
				// ROTRI.D uses bits[31:16] for opcode (16-bit opcode = 0x0024)
				if (op16 == 0x0024) return DECODED_INSTR(ROTRI_D);
			}

			// ALSL.W and ALSL.D use bits [31:18] with sa2 in bits [16:15]
			if ((instr.whole & 0xFFFC0000) == Opcode::ALSL_W) return DECODED_INSTR(ALSL_W);
			if ((instr.whole & 0xFFFC0000) == Opcode::ALSL_D) return DECODED_INSTR(ALSL_D);

			// Immediate instructions (op22 match)
			if (op22 == (Opcode::ADDI_W & 0xFFC00000)) return DECODED_INSTR(ADDI_W);
			if (op22 == (Opcode::ADDI_D & 0xFFC00000)) return DECODED_INSTR(ADDI_D);
			if (op22 == (Opcode::SLTI & 0xFFC00000)) return DECODED_INSTR(SLTI);
			if (op22 == (Opcode::SLTUI & 0xFFC00000)) return DECODED_INSTR(SLTUI);
			if (op22 == (Opcode::ANDI & 0xFFC00000)) return DECODED_INSTR(ANDI);
			if (op22 == (Opcode::ORI & 0xFFC00000)) return DECODED_INSTR(ORI);
			if (op22 == (Opcode::XORI & 0xFFC00000)) return DECODED_INSTR(XORI);
			if (op22 == (Opcode::LU52I_D & 0xFFC00000)) return DECODED_INSTR(LU52I_D);

			// Bit manipulation 2R instructions: CLO/CLZ/CTO/CTZ
			{
				uint32_t op22_val = (instr.whole >> 10) & 0x3FFFFF;
				if (op22_val == 0x000004) return DECODED_INSTR(CLO_W);
				if (op22_val == 0x000005) return DECODED_INSTR(CLZ_W);
				if (op22_val == 0x000006) return DECODED_INSTR(CTO_W);
				if (op22_val == 0x000007) return DECODED_INSTR(CTZ_W);
				if (op22_val == 0x000008) return DECODED_INSTR(CLO_D);
				if (op22_val == 0x000009) return DECODED_INSTR(CLZ_D);
				if (op22_val == 0x00000A) return DECODED_INSTR(CTO_D);
				if (op22_val == 0x00000B) return DECODED_INSTR(CTZ_D);
				// Byte reversal instructions
				if (op22_val == 0x00000C) return DECODED_INSTR(REVB_2H);
				if (op22_val == 0x00000D) return DECODED_INSTR(REVB_4H);
				if (op22_val == 0x00000F) return DECODED_INSTR(REVB_D);
				// EXT.W.B and EXT.W.H
				if (op22_val == 0x000017) return DECODED_INSTR(EXT_W_B);
				if (op22_val == 0x000016) return DECODED_INSTR(EXT_W_H);
				// MOVFR2GR.S: bits[31:10] = 0x452D
				if (op22_val == 0x452D) return DECODED_INSTR(MOVFR2GR_S);
				// MOVFR2GR.D: bits[31:10] = 0x452E
				if (op22_val == 0x452E) return DECODED_INSTR(MOVFR2GR_D);
				// MOVGR2FR.W: bits[31:10] = 0x4529
				if (op22_val == 0x4529) return DECODED_INSTR(MOVGR2FR_W);
				// MOVGR2FR.D: bits[31:10] = 0x452A
				if (op22_val == 0x452A) return DECODED_INSTR(MOVGR2FR_D);
				// MOVFCSR2GR: bits[31:10] = 0x4532
				if (op22_val == 0x4532) return DECODED_INSTR(MOVFCSR2GR);
				// MOVFR2CF: bits[31:10] = 0x4534
				if (op22_val == 0x4534) return DECODED_INSTR(MOVFR2CF);
				// MOVCF2FR: bits[31:10] = 0x4535
				if (op22_val == 0x4535) return DECODED_INSTR(MOVCF2FR);
				// MOVGR2CF: bits[31:10] = 0x4536
				if (op22_val == 0x4536) return DECODED_INSTR(MOVGR2CF);
				// MOVCF2GR: bits[31:10] = 0x4537
				if (op22_val == 0x4537) return DECODED_INSTR(MOVCF2GR);
				// FABS.D: bits[31:10] = 0x4502
				if (op22_val == 0x4502) return DECODED_INSTR(FABS_D);
				// FNEG.D: bits[31:10] = 0x4506
				if (op22_val == 0x4506) return DECODED_INSTR(FNEG_D);
				// FMOV.D: bits[31:10] = 0x4526
				if (op22_val == 0x4526) return DECODED_INSTR(FMOV_D);
				// FCLASS.S: bits[31:10] = 0x450D
				if (op22_val == 0x450D) return DECODED_INSTR(FCLASS_S);
				// FCLASS.D: bits[31:10] = 0x450E
				if (op22_val == 0x450E) return DECODED_INSTR(FCLASS_D);
				// FFINT.D.L: bits[31:10] = 0x474A (convert 64-bit int to double)
				if (op22_val == 0x474A) return DECODED_INSTR(FFINT_D_L);
				// FFINT.D.W: bits[31:10] = 0x4748 (convert 32-bit int to double)
				if (op22_val == 0x4748) return DECODED_INSTR(FFINT_D_W);
				// FFINT.S.W: bits[31:10] = 0x4744 (convert 32-bit int to single)
				if (op22_val == 0x4744) return DECODED_INSTR(FFINT_S_W);
				// FFINT.S.L: bits[31:10] = 0x4746 (convert 64-bit int to single)
				if (op22_val == 0x4746) return DECODED_INSTR(FFINT_S_L);
				// FTINTRZ.W.S: bits[31:10] = 0x46A1 (truncate single to int32)
				if (op22_val == 0x46A1) return DECODED_INSTR(FTINTRZ_W_S);
				// FTINTRZ.W.D: bits[31:10] = 0x46A2 (truncate double to int32)
				if (op22_val == 0x46A2) return DECODED_INSTR(FTINTRZ_W_D);
				// FTINTRZ.L.S: bits[31:10] = 0x46A9 (truncate single to int64)
				if (op22_val == 0x46A9) return DECODED_INSTR(FTINTRZ_L_S);
				// FTINTRZ.L.D: bits[31:10] = 0x46AA (truncate double to int64)
				if (op22_val == 0x46AA) return DECODED_INSTR(FTINTRZ_L_D);
			}
			// FCMP.COR.D: bits[31:22] = 0x0C (opcode), bits[19:15] = 0x14 (COR condition)
			{
				uint32_t op10 = (instr.whole >> 22) & 0x3FF;
				if (op10 == 0x00C) {
					uint32_t cond = (instr.whole >> 15) & 0x1F;
					if (cond == 0x14) return DECODED_INSTR(FCMP_COND_D);
				}
			}
			// BSTRINS.D and BSTRPICK.D: bits [31:22] (op10)
			{
				uint32_t op10 = (instr.whole >> 22) & 0x3FF;
				if (op10 == 0x002) return DECODED_INSTR(BSTRINS_D);
				if (op10 == 0x003) return DECODED_INSTR(BSTRPICK_D);
			}
			// BSTRPICK.W: bits [31:21] (op11)
			// Encoding: 0000 0000 011x xxxx - top 11 bits = 0x003
			{
				uint32_t op11 = (instr.whole >> 21) & 0x7FF;
				if (op11 == 0x003) return DECODED_INSTR(BSTRPICK_W);
			}
			break;

		case 0x02: // Fused multiply-add/sub instructions (4R-type)
			{
				// For 4R-type instructions, check bits[31:20] for the opcode
				uint32_t op12_4r = (instr.whole >> 20) & 0xFFF;
				// FMADD.D: bits[31:20] = 0x082
				if (op12_4r == 0x082) {
					return DECODED_INSTR(FMADD_D);
				}
				// FMSUB.D: bits[31:20] = 0x086
				if (op12_4r == 0x086) return DECODED_INSTR(FMSUB_D);
				// VFMADD.D: Vector FMA (bits[31:20] = 0x092)
				if (op12_4r == 0x092) return DECODED_INSTR(VFMADD_D);
				// NOTE: VFNMADD.D is handled in case 0x03 or has a different encoding
				// LASX vector FMA instructions (XVFMADD, XVFMSUB, XVFNMADD, XVFNMSUB)
				// XVFMADD.S: bits[31:20] = 0x0A1
				if (op12_4r == 0x0A1) return DECODED_INSTR(XVFMADD_S);
				// XVFMADD.D: bits[31:20] = 0x0A2
				if (op12_4r == 0x0A2) return DECODED_INSTR(XVFMADD_D);
				// XVFMSUB.S: bits[31:20] = 0x0A5
				if (op12_4r == 0x0A5) return DECODED_INSTR(XVFMSUB_S);
				// XVFMSUB.D: bits[31:20] = 0x0A6
				if (op12_4r == 0x0A6) return DECODED_INSTR(XVFMSUB_D);
				// XVFNMADD.S: bits[31:20] = 0x0A9
				if (op12_4r == 0x0A9) return DECODED_INSTR(XVFNMADD_S);
				// XVFNMADD.D: bits[31:20] = 0x0AA
				if (op12_4r == 0x0AA) return DECODED_INSTR(XVFNMADD_D);
				// XVFNMSUB.S: bits[31:20] = 0x0AD
				if (op12_4r == 0x0AD) return DECODED_INSTR(XVFNMSUB_S);
				// XVFNMSUB.D: bits[31:20] = 0x0AE
				if (op12_4r == 0x0AE) return DECODED_INSTR(XVFNMSUB_D);
			}
			break;

		case 0x03: // VSHUF.B (4R-type vector shuffle) and FCMP instructions
		// XVBITSEL.V: LASX bit select (bits[31:20] = 0x0D2)
		if ((instr.whole >> 20) == 0x0D2) return DECODED_INSTR(XVBITSEL_V);
		// VSHUF.B: bits[31:20] = 0x00D5
		if ((instr.whole >> 20) == 0x00D5) return DECODED_INSTR(VSHUF_B);
		// VBITSEL.V: bits[31:20] = 0x0D1
		if ((instr.whole >> 20) == 0x0D1) return DECODED_INSTR(VBITSEL_V);
		// FSEL: FP conditional select - bits[31:18] = 0x0340
		if (((instr.whole >> 18) & 0x3FFF) == 0x0340) return DECODED_INSTR(FSEL);

		// FCMP instructions: bits[31:22] = 0x030, bits[19:15] = condition code
		{
			uint32_t op10 = (instr.whole >> 22) & 0x3FF;
			if (op10 == 0x030) return DECODED_INSTR(FCMP_COND_D);
		}
		// VFCMP instructions: bits[31:21] = 0x063, bits[20:15] = condition code
		{
			uint32_t op11 = (instr.whole >> 21) & 0x7FF;
			if (op11 == 0x063) {
				return DECODED_INSTR(VFCMP_COND_D);
			}
		}
		// XVFCMP instructions: bits[31:21] = 0x065, bits[20:15] = condition code (LASX)
		{
			uint32_t op11 = (instr.whole >> 21) & 0x7FF;
			if (op11 == 0x065) {
				return DECODED_INSTR(XVFCMP_COND_D);
			}
		}
			break;
		case 0x05: // LU12I.W (0x14000000) / LU32I.D (0x16000000)
			// Need to check bits[31:25] to distinguish:
			// LU12I.W: 0001010 (0x0A), LU32I.D: 0001011 (0x0B)
			{
				uint32_t op7 = (instr.whole >> 25) & 0x7F;
				if (op7 == 0x0A) return DECODED_INSTR(LU12I_W);
				if (op7 == 0x0B) return DECODED_INSTR(LU32I_D);
			}
			break;

		case 0x06: // PCADDI (0x18) / PCALAU12I (0x1A)
			{
				uint32_t op7 = (instr.whole >> 25) & 0x7F;
				if (op7 == 0x0C) return DECODED_INSTR(PCADDI);
				if (op7 == 0x0D) return DECODED_INSTR(PCALAU12I);
			}
			break;

		case 0x07: // PCADDU12I (0x1C000000) / PCADDU18I (0x1E000000)
			{
				uint32_t op7 = (instr.whole >> 25) & 0x7F;
				if (op7 == 0x0E) return DECODED_INSTR(PCADDU12I);
				if (op7 == 0x0F) return DECODED_INSTR(PCADDU18I);
			}
			break;

		case 0x09: // LDPTR.W / STPTR.W / LDPTR.D / STPTR.D (mask is 0xFF000000)
			{
				uint32_t op8 = instr.whole & 0xFF000000;
				if (op8 == 0x24000000) return DECODED_INSTR(LDPTR_W);
				if (op8 == 0x25000000) return DECODED_INSTR(STPTR_W);
				if (op8 == 0x26000000) return DECODED_INSTR(LDPTR_D);
				if (op8 == 0x27000000) return DECODED_INSTR(STPTR_D);
			}
			break;

		case 0x0A: // All load/store instructions have op6=0x0A
			// Load instructions
			if (op22 == (Opcode::LD_B & 0xFFC00000)) return DECODED_INSTR(LD_B);
			if (op22 == (Opcode::LD_H & 0xFFC00000)) return DECODED_INSTR(LD_H);
			if (op22 == (Opcode::LD_W & 0xFFC00000)) return DECODED_INSTR(LD_W);
			if (op22 == (Opcode::LD_D & 0xFFC00000)) return DECODED_INSTR(LD_D);
			if (op22 == (Opcode::LD_BU & 0xFFC00000)) return DECODED_INSTR(LD_BU);
			if (op22 == (Opcode::LD_HU & 0xFFC00000)) return DECODED_INSTR(LD_HU);
			if (op22 == (Opcode::LD_WU & 0xFFC00000)) return DECODED_INSTR(LD_WU);
			// Store instructions
			if (op22 == (Opcode::ST_B & 0xFFC00000)) return DECODED_INSTR(ST_B);
			if (op22 == (Opcode::ST_H & 0xFFC00000)) return DECODED_INSTR(ST_H);
			if (op22 == (Opcode::ST_W & 0xFFC00000)) return DECODED_INSTR(ST_W);
			if (op22 == (Opcode::ST_D & 0xFFC00000)) return DECODED_INSTR(ST_D);
			// Floating-point load/store
			if (op22 == (Opcode::FLD_S & 0xFFC00000)) return DECODED_INSTR(FLD_S);
			if (op22 == (Opcode::FST_S & 0xFFC00000)) return DECODED_INSTR(FST_S);
			if (op22 == (Opcode::FLD_D & 0xFFC00000)) return DECODED_INSTR(FLD_D);
			if (op22 == (Opcode::FST_D & 0xFFC00000)) return DECODED_INSTR(FST_D);
			break;

		case 0x10: // BEQZ
			return DECODED_INSTR(BEQZ);
		case 0x11: // BNEZ
			return DECODED_INSTR(BNEZ);

		case 0x13: // JIRL
			return DECODED_INSTR(JIRL);

		case 0x14: // B
			return DECODED_INSTR(B);
		case 0x15: // BL
			return DECODED_INSTR(BL);

		case 0x16: // BEQ
			return DECODED_INSTR(BEQ);
		case 0x17: // BNE
			return DECODED_INSTR(BNE);
		case 0x18: // BLT
			return DECODED_INSTR(BLT);
		case 0x19: // BGE
			return DECODED_INSTR(BGE);
		case 0x1A: // BLTU
			return DECODED_INSTR(BLTU);
		case 0x1B: // BGEU
			return DECODED_INSTR(BGEU);

		case 0x08: // LL/SC instructions
			{
				uint32_t op8 = (instr.whole >> 24) & 0xFF;
				if (op8 == 0x20) return DECODED_INSTR(LL_W);
				if (op8 == 0x21) return DECODED_INSTR(SC_W);
				if (op8 == 0x22) return DECODED_INSTR(LL_D);
				if (op8 == 0x23) return DECODED_INSTR(SC_D);
			}
			break;

		case 0x0E: // Indexed load/store, atomics, barriers (op8 = 0x38)
			// DBAR/IBAR: 0x38720000 / 0x38728000
			if ((instr.whole & 0xFFFF8000) == 0x38720000) return DECODED_INSTR(DBAR);
			if ((instr.whole & 0xFFFF8000) == 0x38728000) return DECODED_INSTR(IBAR);

			// LDX instructions (indexed loads)
			if ((instr.whole & 0xFFFF8000) == 0x38000000) return DECODED_INSTR(LDX_B);
			if ((instr.whole & 0xFFFF8000) == 0x38040000) return DECODED_INSTR(LDX_H);
			if ((instr.whole & 0xFFFF8000) == 0x38080000) return DECODED_INSTR(LDX_W);
			if ((instr.whole & 0xFFFF8000) == 0x380C0000) return DECODED_INSTR(LDX_D);
			if ((instr.whole & 0xFFFF8000) == 0x38200000) return DECODED_INSTR(LDX_BU);
			if ((instr.whole & 0xFFFF8000) == 0x38240000) return DECODED_INSTR(LDX_HU);
			if ((instr.whole & 0xFFFF8000) == 0x38280000) return DECODED_INSTR(LDX_WU);

			// STX instructions: bits [31:18] identify the operation
			if ((instr.whole & 0xFFFC0000) == Opcode::STX_B) return DECODED_INSTR(STX_B);
			if ((instr.whole & 0xFFFC0000) == Opcode::STX_H) return DECODED_INSTR(STX_H);
			if ((instr.whole & 0xFFFC0000) == Opcode::STX_W) return DECODED_INSTR(STX_W);
			if ((instr.whole & 0xFFFC0000) == Opcode::STX_D) return DECODED_INSTR(STX_D);
			// FLDX_D: Floating-point indexed load (double)
			if ((instr.whole & 0xFFFC0000) == 0x38340000) return DECODED_INSTR(FLDX_D);
			// FSTX_D: Floating-point indexed store
			if ((instr.whole & 0xFFFC0000) == Opcode::FSTX_D) return DECODED_INSTR(FSTX_D);
			// VLDX: Vector indexed load (LSX 128-bit)
			if ((instr.whole & 0xFFFC0000) == 0x38400000) return DECODED_INSTR(VLDX);
			// VSTX: Vector indexed store (LSX 128-bit)
			if ((instr.whole & 0xFFFC0000) == 0x38440000) return DECODED_INSTR(VSTX);
			// XVLDX: Vector indexed load (LASX 256-bit)
			if ((instr.whole & 0xFFFC0000) == 0x38480000) return DECODED_INSTR(XVLDX);
			// XVSTX: Vector indexed store (LASX 256-bit)
			if ((instr.whole & 0xFFFC0000) == 0x384C0000) return DECODED_INSTR(XVSTX);

			// Atomic instructions: bits [31:20] = 0x386
			// Empirical encoding: bits[19:16] encode both operation and ordering
			// Bit [15] = size (0=W, 1=D)
			// Pattern: bits[19:16] = base_operation + (ordering_bits)
			// Examples:
			//   amswap.w:    bits[19:16]=0, bit[15]=0
			//   amswap_db.w: bits[19:16]=9, bit[15]=0
			//   amadd.w:     bits[19:16]=1, bit[15]=0
			//   amadd_db.w:  bits[19:16]=10, bit[15]=0
			if ((instr.whole & 0xFFF00000) == 0x38600000) {
				uint32_t op_ord = (instr.whole >> 16) & 0xF;
				bool is_64bit = ((instr.whole >> 15) & 1);

				// Decode by bits[19:16] value
				// Based on empirical observation:
				// AMSWAP: 0 (no ord), 9 (_db)
				// AMADD: 1 (no ord), 10 (_db)
				// Assuming pattern continues: AMAND: 2, AMOR: 3, etc.
				switch (op_ord) {
					case 0: case 9: // AMSWAP
						return is_64bit ? DECODED_INSTR(AMSWAP_D) : DECODED_INSTR(AMSWAP_W);
					case 1: case 10: // AMADD
						return is_64bit ? DECODED_INSTR(AMADD_D) : DECODED_INSTR(AMADD_W);
					case 2: case 11: // AMAND (guessed)
						return is_64bit ? DECODED_INSTR(AMAND_D) : DECODED_INSTR(AMAND_W);
					case 3: case 12: // AMOR (guessed)
						return is_64bit ? DECODED_INSTR(AMOR_D) : DECODED_INSTR(AMOR_W);
					// TODO: Verify AMXOR, AMMAX, AMMIN encodings when encountered
				}
			}
			// AMXOR: bits [31:20] = 0x387
			if ((instr.whole & 0xFFF00000) == 0x38700000) {
				bool is_64bit = ((instr.whole >> 15) & 1);
				return is_64bit ? DECODED_INSTR(AMXOR_D) : DECODED_INSTR(AMXOR_W);
			}
			break;
		case 0x0B: // VLD / VST / XVLD / XVST instructions (op6 = 0x0B for 0x2Cxxxxxx)
			// Example VLD: 2c0041a2
			// Example VST: 2c404202
			// VLD: bits[31:22] = 0x2C0 (0xB0 << 2)
			// VST: bits[31:22] = 0x2C1 (0xB04 >> 2)
			// XVLD: bits[31:22] = 0x2C8 (256-bit LASX load)
			// XVST: bits[31:22] = 0x2CC (256-bit LASX store)
			if (op22 == 0x2C000000) return DECODED_INSTR(VLD); // VLD
			if (op22 == 0x2C400000) return DECODED_INSTR(VST); // VST
			if (op22 == 0x2C800000) return DECODED_INSTR(XVLD); // XVLD
			if (op22 == 0x2CC00000) return DECODED_INSTR(XVST); // XVST
			break;

		case 0x12: // BCEQZ/BCNEZ (FP condition branches)
			// BCEQZ: 0x48000000, BCNEZ: 0x48000100
			if ((instr.whole & 0x00000300) == 0x00000000) return DECODED_INSTR(BCEQZ);
			if ((instr.whole & 0x00000300) == 0x00000100) return DECODED_INSTR(BCNEZ);
			break;

		case 0x1C: // LSX vector compare/test/extract instructions
			{
				uint32_t top16 = (instr.whole >> 16) & 0xFFFF;
				uint32_t op10  = (instr.whole >> 22) & 0x3FF;

				// VLDI: Vector load immediate - bits[31:18] = 0x1CF8
				if ((instr.whole >> 18) == 0x1CF8) return DECODED_INSTR(VLDI);
				// VORI.B: Vector OR immediate (byte) - bits[31:18] = 0x1CF5
				if ((instr.whole >> 18) == 0x1CF5) return DECODED_INSTR(VORI_B);
				// VBITREVI.D: Vector Bit Reverse Immediate (double) - op10 = 0x1CC
				if (op10 == 0x1CC) return DECODED_INSTR(VBITREVI_D);
				// VMAX/VMIN instructions - bits[31:15] identify variants
				// Signed: 0xE0E0-E0E3 (b/h/w/d), Unsigned: 0xE0E8-E0EB (bu/hu/wu/du)
				uint32_t bits15 = instr.whole >> 15;
				if (bits15 == 0xE0E0) return DECODED_INSTR(VMAX_B);
				if (bits15 == 0xE0E1) return DECODED_INSTR(VMAX_H);
				if (bits15 == 0xE0E2) return DECODED_INSTR(VMAX_W);
				if (bits15 == 0xE0E3) return DECODED_INSTR(VMAX_D);
				if (bits15 == 0xE0E4) return DECODED_INSTR(VMIN_B);
				if (bits15 == 0xE0E5) return DECODED_INSTR(VMIN_H);
				if (bits15 == 0xE0E6) return DECODED_INSTR(VMIN_W);
				if (bits15 == 0xE0E7) return DECODED_INSTR(VMIN_D);
				if (bits15 == 0xE0E8) return DECODED_INSTR(VMAX_BU);
				if (bits15 == 0xE0E9) return DECODED_INSTR(VMAX_HU);
				if (bits15 == 0xE0EA) return DECODED_INSTR(VMAX_WU);
				if (bits15 == 0xE0EB) return DECODED_INSTR(VMAX_DU);
				if (bits15 == 0xE0EC) return DECODED_INSTR(VMIN_BU);
				if (bits15 == 0xE0ED) return DECODED_INSTR(VMIN_HU);
				if (bits15 == 0xE0EE) return DECODED_INSTR(VMIN_WU);
				if (bits15 == 0xE0EF) return DECODED_INSTR(VMIN_DU);
				// VHADDW.D.W: Vector horizontal add with widening - bits[31:15] = 0xE0AA
				if (bits15 == 0xE0AA) return DECODED_INSTR(VHADDW_D_W);
				// VFADD.D: Vector floating-point add (double) - op17 = 0x71310000
				if ((instr.whole & 0xFFFF8000) == 0x71310000) return DECODED_INSTR(VFADD_D);
				// VFDIV.D: Vector floating-point divide (double) - bits[31:15] = 0xE276
				if ((instr.whole >> 15) == 0xE276) return DECODED_INSTR(VFDIV_D);
				// VFMUL.S: Vector floating-point multiply (single) - bits[31:15] = 0xE271
				if ((instr.whole >> 15) == 0xE271) return DECODED_INSTR(VFMUL_S);
				// VFMUL.D: Vector floating-point multiply (double) - bits[31:15] = 0xE272
				if ((instr.whole >> 15) == 0xE272) return DECODED_INSTR(VFMUL_D);
				// VILVL: Vector Interleave Low (B/H/W/D)
				// VILVL.B: bits[31:15] = 0xE234
				if ((instr.whole >> 15) == 0xE234) {
					return DECODED_INSTR(VILVL_B);
				}
				// VILVL.H: bits[31:15] = 0xE235
				if ((instr.whole >> 15) == 0xE235) {
					return DECODED_INSTR(VILVL_H);
				}
				// VILVL.W: bits[31:15] = 0xE236
				if ((instr.whole >> 15) == 0xE236) {
					return DECODED_INSTR(VILVL_W);
				}
				// VILVL.D: bits[31:15] = 0xE237
				if ((instr.whole >> 15) == 0xE237) {
					return DECODED_INSTR(VILVL_D);
				}
				// VILVH.D: Vector Interleave High Double-word
				// bits[31:15] = 0xE23F
				if ((instr.whole >> 15) == 0xE23F) {
					return DECODED_INSTR(VILVH_D);
				}
				// VPICKEV.W: Vector Pick Even Word
				// bits[31:15] = 0xE23E
				if ((instr.whole >> 15) == 0xE23E) {
					return DECODED_INSTR(VPICKEV_W);
				}
				// VPICKVE2GR instructions: signed (0x72EF) and unsigned (0x72F3)
				// bits[15:12] determine size: 0x8=B, 0xC=H, 0xE=W, 0xF=D
				if (top16 == 0x72EF) {
					uint32_t subop = (instr.whole >> 12) & 0xF;
					if (subop == 0x8) return DECODED_INSTR(VPICKVE2GR_B);
					if (subop == 0xC) return DECODED_INSTR(VPICKVE2GR_H);
					if (subop == 0xE) return DECODED_INSTR(VPICKVE2GR_W);
					if (subop == 0xF) return DECODED_INSTR(VPICKVE2GR_D);
				}
				if (top16 == 0x72F3) {
					uint32_t subop = (instr.whole >> 12) & 0xF;
					if (subop == 0x8) return DECODED_INSTR(VPICKVE2GR_BU);
					if (subop == 0xC) return DECODED_INSTR(VPICKVE2GR_HU);
					if (subop == 0xE) return DECODED_INSTR(VPICKVE2GR_WU);
					if (subop == 0xF) return DECODED_INSTR(VPICKVE2GR_DU);
				}
				// VREPLVEI.D: 0x72F7xxxx (Vector Replicate Vector Element Immediate - double)
				if (top16 == 0x72F7) {
					return DECODED_INSTR(VREPLVEI_D);
				}
				if (top16 == 0x729C) {
					uint32_t subop = (instr.whole >> 10) & 0x3F;
					if (subop == 0x28) return DECODED_INSTR(VSETANYEQZ_B);
					if (subop == 0x18) return DECODED_INSTR(VMSKNZ_B);
				}
				if (top16 == 0x729D) {
					uint32_t subop = (instr.whole >> 10) & 0x3F;
					if (subop == 0x18) return DECODED_INSTR(VSETALLNEZ_B);
				}
				// VREPLGR2VR: 0x729F0xxx
				// .B (subop=0x00), .H (subop=0x01), .W (subop=0x02), .D (subop=0x03)
				if (top16 == 0x729F) {
					uint32_t subop = (instr.whole >> 10) & 0x3F;
					if (subop == 0x00) return DECODED_INSTR(VREPLGR2VR_B);
					if (subop == 0x01) return DECODED_INSTR(VREPLGR2VR_H);
					if (subop == 0x02) return DECODED_INSTR(VREPLGR2VR_W);
					if (subop == 0x03) return DECODED_INSTR(VREPLGR2VR_D);
				}
				// VINSGR2VR: 0x72EBxxxx with different bit patterns
				// .b: bits[15:14]=10, .h: bits[15:13]=110, .w: bits[15:12]=1110, .d: bits[15:11]=11110
				if (top16 == 0x72EB) {
					uint32_t bits15_14 = (instr.whole >> 14) & 0x3;
					uint32_t bits15_13 = (instr.whole >> 13) & 0x7;
					uint32_t bits15_12 = (instr.whole >> 12) & 0xF;
					uint32_t bits15_11 = (instr.whole >> 11) & 0x1F;

					if (bits15_14 == 0x2) return DECODED_INSTR(VINSGR2VR_B);  // 10
					if (bits15_13 == 0x6) return DECODED_INSTR(VINSGR2VR_H);  // 110
					if (bits15_12 == 0xE) return DECODED_INSTR(VINSGR2VR_W);  // 1110
					if (bits15_11 == 0x1E) return DECODED_INSTR(VINSGR2VR_D); // 11110
				}
				// VSUB.B: bits[31:15] = 0xE018
				if ((instr.whole >> 15) == 0xE018) {
					return DECODED_INSTR(VSUB_B);
				}
				// VSUB.H: bits[31:15] = 0xE019
				if ((instr.whole >> 15) == 0xE019) {
					return DECODED_INSTR(VSUB_H);
				}
				// VSUB.W: bits[31:15] = 0xE01A
				if ((instr.whole >> 15) == 0xE01A) {
					return DECODED_INSTR(VSUB_W);
				}
				// VSUB.D: bits[31:15] = 0xE01B
				if ((instr.whole >> 15) == 0xE01B) {
					return DECODED_INSTR(VSUB_D);
				}
				// VMUL.B: bits[31:15] = 0xE108
				if ((instr.whole >> 15) == 0xE108) {
					return DECODED_INSTR(VMUL_B);
				}
				// VMUL.H: bits[31:15] = 0xE109
				if ((instr.whole >> 15) == 0xE109) {
					return DECODED_INSTR(VMUL_H);
				}
				// VMUL.W: bits[31:15] = 0xE10A
				if ((instr.whole >> 15) == 0xE10A) {
					return DECODED_INSTR(VMUL_W);
				}
				// VMUL.D: bits[31:15] = 0xE10B
				if ((instr.whole >> 15) == 0xE10B) {
					return DECODED_INSTR(VMUL_D);
				}
				// VMADD.B: bits[31:15] = 0xE150
				if ((instr.whole >> 15) == 0xE150) {
					return DECODED_INSTR(VMADD_B);
				}
				// VMADD.H: bits[31:15] = 0xE151
				if ((instr.whole >> 15) == 0xE151) {
					return DECODED_INSTR(VMADD_H);
				}
				// VMADD.W: bits[31:15] = 0xE152
				if ((instr.whole >> 15) == 0xE152) {
					return DECODED_INSTR(VMADD_W);
				}
				// VMADD.D: bits[31:15] = 0xE153
				if ((instr.whole >> 15) == 0xE153) {
					return DECODED_INSTR(VMADD_D);
				}
				// VADDI.BU: bits[31:15] = 0xE514
				if ((instr.whole >> 15) == 0xE514) {
					return DECODED_INSTR(VADDI_BU);
				}
				// VADDI.HU: bits[31:15] = 0xE515
				if ((instr.whole >> 15) == 0xE515) {
					return DECODED_INSTR(VADDI_HU);
				}
				// VADDI.WU: bits[31:15] = 0xE516
				if ((instr.whole >> 15) == 0xE516) {
					return DECODED_INSTR(VADDI_WU);
				}
				// VADDI.DU: bits[31:15] = 0xE517
				if ((instr.whole >> 15) == 0xE517) {
					return DECODED_INSTR(VADDI_DU);
				}
				// VADD.B: bits[31:15] = 0xE014
				if ((instr.whole >> 15) == 0xE014) {
					return DECODED_INSTR(VADD_B);
				}
				// VADD.H: bits[31:15] = 0xE015
				if ((instr.whole >> 15) == 0xE015) {
					return DECODED_INSTR(VADD_H);
				}
				// VADD.W: bits[31:15] = 0xE016
				if ((instr.whole >> 15) == 0xE016) {
					return DECODED_INSTR(VADD_W);
				}
				// VADD.D: bits[31:15] = 0xE017
				if ((instr.whole >> 15) == 0xE017) {
					return DECODED_INSTR(VADD_D);
				}
				// VSEQ.B: bits[31:15] = 0xE000
				if ((instr.whole >> 15) == 0xE000) {
					return DECODED_INSTR(VSEQ_B);
				}
				// VSLT.B: bits[31:15] = 0xE00C
				if ((instr.whole >> 15) == 0xE00C) {
					return DECODED_INSTR(VSLT_B);
				}
				// VSLT.H: bits[31:15] = 0xE00D
				if ((instr.whole >> 15) == 0xE00D) {
					return DECODED_INSTR(VSLT_H);
				}
				// VSLT.W: bits[31:15] = 0xE00E
				if ((instr.whole >> 15) == 0xE00E) {
					return DECODED_INSTR(VSLT_W);
				}
				// VSLT.D: bits[31:15] = 0xE00F
				if ((instr.whole >> 15) == 0xE00F) {
					return DECODED_INSTR(VSLT_D);
				}
				// VSEQI.B: bits[31:15] = 0xE500
				if ((instr.whole >> 15) == 0xE500) {
					return DECODED_INSTR(VSEQI_B);
				}
				// VSEQI.H: bits[31:15] = 0xE501
				if ((instr.whole >> 15) == 0xE501) {
					return DECODED_INSTR(VSEQI_H);
				}
				// VSEQI.W: bits[31:15] = 0xE502
				if ((instr.whole >> 15) == 0xE502) {
					return DECODED_INSTR(VSEQI_W);
				}
				// VSEQI.D: bits[31:15] = 0xE503
				if ((instr.whole >> 15) == 0xE503) {
					return DECODED_INSTR(VSEQI_D);
				}
				// VFRSTPI.B: bits[31:15] = 0xE534
				if ((instr.whole >> 15) == 0xE534) {
					return DECODED_INSTR(VFRSTPI_B);
				}
				// VFTINTRZ.W.S / VFTINTRZ.L.D: bits[31:15] = 0xE53C
				if ((instr.whole >> 15) == 0xE53C) {
					// Differentiate by bit 10
					if ((instr.whole >> 10) & 1) {
						return DECODED_INSTR(VFTINTRZ_L_D);
					} else {
						return DECODED_INSTR(VFTINTRZ_W_S);
					}
				}
				// VXOR.V: bits[31:15] = 0xE24E
				if ((instr.whole >> 15) == 0xE24E) {
					return DECODED_INSTR(VXOR_V);
				}
				// VAND.V: bits[31:15] = 0xE24C (need to add for case 0x1C)
				if ((instr.whole >> 15) == 0xE24C) {
					return DECODED_INSTR(VAND_V);
				}
				// VOR.V: bits[31:15] = 0xE24D
				if ((instr.whole >> 15) == 0xE24D) {
					return DECODED_INSTR(VOR_V);
				}
				// VNOR.V: bits[31:15] = 0xE24F
				if ((instr.whole >> 15) == 0xE24F) {
					return DECODED_INSTR(VNOR_V);
				}
				// VORN.V: bits[31:15] = 0xE251
				if ((instr.whole >> 15) == 0xE251) {
					return DECODED_INSTR(VORN_V);
				}
			}
			break;

		case 0x1D: // LASX (256-bit) instructions
			{
				uint32_t top16 = (instr.whole >> 16) & 0xFFFF;

				// XVMSKNZ.B: 0x769C6xxx (bits[31:16] = 0x769C, bits[15:10] = 0x18)
				// XVSETANYEQZ.B: 0x769CAxx (bits[31:16] = 0x769C, bits[15:10] = 0x28)
				// XVSETEQZ.V: 0x769CBxxx (bits[31:16] = 0x769C, bits[15:10] = 0x2C)
				if (top16 == 0x769C) {
					uint32_t subop = (instr.whole >> 10) & 0x3F;
					if (subop == 0x18) return DECODED_INSTR(XVMSKNZ_B);
					if (subop == 0x28) return DECODED_INSTR(XVSETANYEQZ_B);
					if (subop == 0x2C) return DECODED_INSTR(XVSETEQZ_V);
				}
				// XVREPLGR2VR.B: 0x769F0xxx (bits[31:16] = 0x769F, bits[15:10] = 0x00)
				if (top16 == 0x769F) {
					uint32_t subop = (instr.whole >> 10) & 0x3F;
					if (subop == 0x00) return DECODED_INSTR(XVREPLGR2VR_B);
				}
				// XVPICKVE.W: 0x7703Dxxx (bits[31:16] = 0x7703, bits[15:10] = 0x34)
				if (top16 == 0x7703) {
					uint32_t subop = (instr.whole >> 10) & 0x3F;
					if (subop == 0x34) return DECODED_INSTR(XVPICKVE_W);
				}
				// XVSEQ.B: bits[31:15] = 0xEE00
				if ((instr.whole >> 15) == 0xEE00) {
					return DECODED_INSTR(XVSEQ_B);
				}
				// XVXOR.V: bits[31:15] = 0xEA4E
				if ((instr.whole >> 15) == 0xEA4E) {
					return DECODED_INSTR(XVXOR_V);
				}
				// XVADD.D: LASX vector add doublewords - bits[31:15] = 0xE817
				// Opcode: 0x740b8000 >> 15 = 0xE817
				if ((instr.whole >> 15) == 0xE817) {
					return DECODED_INSTR(XVADD_D);
				}
				// XVSUB.W: LASX vector subtract words - bits[31:15] = 0xE81A
				// Opcode: 0x740d0000 >> 15 = 0xE81A
				if ((instr.whole >> 15) == 0xE81A) {
					return DECODED_INSTR(XVSUB_W);
				}
				// XVHADDW.D.W: LASX vector horizontal add with widening - bits[31:15] = 0xE8AA
				// Opcode: 0x74551000 >> 15 = 0xE8AA
				if ((instr.whole >> 15) == 0xE8AA) {
					return DECODED_INSTR(XVHADDW_D_W);
				}
				// XVHADDW.Q.D: LASX vector horizontal add with widening - bits[31:15] = 0xE8AB
				// Opcode: 0x74559080 >> 15 = 0xE8AB
				if ((instr.whole >> 15) == 0xE8AB) {
					return DECODED_INSTR(XVHADDW_Q_D);
				}
				// XVMIN.BU: bits[31:15] = 0xE8EC
				if ((instr.whole >> 15) == 0xE8EC) {
					return DECODED_INSTR(XVMIN_BU);
				}
				// XVMAX.BU: bits[31:15] = 0xE8ED
				if ((instr.whole >> 15) == 0xE8ED) {
					return DECODED_INSTR(XVMAX_BU);
				}
				// XVPICKVE2GR.W: bits[31:18] = 0x1DBB (0x76EFxxxx)
				// Pick word element from LASX vector to general register
				if ((instr.whole >> 18) == 0x1DBB) {
					return DECODED_INSTR(XVPICKVE2GR_W);
				}
				// XVPERMI.Q: LASX permute quadword - bits[31:18] = 0x1DFB
				// Opcode: 0x77ec0000 >> 18 = 0x1DFB (format: XdXjUk8)
				if ((instr.whole >> 18) == 0x1DFB) {
					return DECODED_INSTR(XVPERMI_Q);
				}
				// XVFADD.D: LASX vector floating-point add (double) - bits[31:15] = 0xEA62
				// Opcode: 0x75310000 >> 15 = 0xEA62
				if ((instr.whole >> 15) == 0xEA62) return DECODED_INSTR(XVFADD_D);
				// XVFMUL.D: LASX vector floating-point multiply (double) - bits[31:15] = 0xEA72
				// Opcode: 0x75390000 >> 15 = 0xEA72
				if ((instr.whole >> 15) == 0xEA72) return DECODED_INSTR(XVFMUL_D);
				// XVFDIV.D: LASX vector floating-point divide (double) - bits[31:15] = 0xEA76
				// Opcode: 0x753b2000 >> 15 = 0xEA76
				if ((instr.whole >> 15) == 0xEA76) return DECODED_INSTR(XVFDIV_D);
				// XVORI.B: LASX vector OR immediate byte - bits[31:15] = 0xEFA8
				// Opcode: 0x77d40000 >> 15 = 0xEFA8
				if ((instr.whole >> 15) == 0xEFA8) return DECODED_INSTR(XVORI_B);
				// XVXORI.B: LASX vector XOR immediate byte - bits[31:15] = 0xEFB0
				// Opcode: 0x77d80000 >> 15 = 0xEFB0
				if ((instr.whole >> 15) == 0xEFB0) return DECODED_INSTR(XVXORI_B);
				// XVILVL.D: LASX vector interleave low double-word - bits[31:15] = 0xEA37
				// Opcode: 0x751b8000 >> 15 = 0xEA37
				if ((instr.whole >> 15) == 0xEA37) return DECODED_INSTR(XVILVL_D);
				// XVILVH.D: LASX vector interleave high double-word - bits[31:15] = 0xEA3F
				// Opcode: 0x751f8000 >> 15 = 0xEA3F
				if ((instr.whole >> 15) == 0xEA3F) return DECODED_INSTR(XVILVH_D);
				// XVPERMI.D: LASX vector permute double-word - bits[31:18] = 0x1dc1
				// Opcode: 0x7707e000 >> 18 = 0x1dc1
				if ((instr.whole >> 18) == 0x1dc1) return DECODED_INSTR(XVPERMI_D);
				// XVPACKEV.D: LASX vector pack even double-word - bits[31:15] = 0xea66
				// Opcode: 0x75330000 >> 15 = 0xea66
				if ((instr.whole >> 15) == 0xea66) return DECODED_INSTR(XVPACKEV_D);
				// XVPACKOD.D: LASX vector pack odd double-word - bits[31:15] = 0xee33
				// Opcode: 0x77198000 >> 15 = 0xee33
				if ((instr.whole >> 15) == 0xee33) return DECODED_INSTR(XVPACKOD_D);
				// XVPICKEV.D: LASX vector pick even double-word - bits[31:15] = 0xee07
				// Opcode: 0x7703c000 >> 15 = 0xee07
				if ((instr.whole >> 15) == 0xee07) return DECODED_INSTR(XVPICKEV_D);
				// XVPICKEV.W: LASX vector pick even word - bits[31:15] = 0xea3e
				// Opcode: 0x751f0000 >> 15 = 0xea3e
				if ((instr.whole >> 15) == 0xea3e) return DECODED_INSTR(XVPICKEV_W);
				// XVPICKOD.D: LASX vector pick odd double-word - bits[31:15] = 0xee0f
				// Opcode: 0x77078000 >> 15 = 0xee0f
				if ((instr.whole >> 15) == 0xee0f) return DECODED_INSTR(XVPICKOD_D);
				// XVLDI: LASX load immediate - bits[31:23] = 0xEF
				// Opcode: 0x77e00000 >> 23 = 0xEF (format: XdSj13 with 13-bit immediate)
				// Note: Checked last as catch-all for 0x77exxxxx / 0x77fxxxxx after specific instructions
				if ((instr.whole >> 23) == 0xEF) {
					return DECODED_INSTR(XVLDI);
				}
			}
			break;
		}

		return DECODED_INSTR(UNIMPLEMENTED);
	}

	template <>
	const CPU<LA64>::instruction_t& CPU<LA64>::get_invalid_instruction() noexcept
	{
		return DECODED_INSTR(INVALID);
	}

	template <>
	const CPU<LA64>::instruction_t& CPU<LA64>::get_unimplemented_instruction() noexcept
	{
		return DECODED_INSTR(UNIMPLEMENTED);
	}

} // loongarch
