/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2005  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// recompiler reworked to add dynamic linking Jan06
// and added reg caching, const propagation, block analysis Jun06
// zerofrog(@gmail.com)

#if 1

#ifdef __MSCW32__
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <malloc.h>

#if defined(__WIN32__)
#include <windows.h>
#endif

#include "PS2Etypes.h"
#include "System.h"
#include "zlib.h"
#include "Memory.h"
#include "Misc.h"
#include "Vif.h"
#include "VU.h"

#include "R3000A.h"
#include "PsxMem.h"

#include "ix86/ix86.h"

#include "iCore.h"
#include "ir3000A.h"
#include "PsxCounters.h"

extern u32 psxNextCounter, psxNextsCounter;
u32 g_psxMaxRecMem = 0;
extern char *disRNameGPR[];
extern char* disR3000Fasm(u32 code, u32 pc);
}

#define PSX_NUMBLOCKS (1<<12)
#define MAPBASE			0x48000000
#define RECMEM_SIZE		(8*1024*1024)

// R3000A statics
uptr *psxRecLUT;
int psxreclog = 0;

static u32 s_BranchCount = 0;
static char *recMem;	// the recompiled blocks will be here
static BASEBLOCK *recRAM;	// and the ptr to the blocks here
static BASEBLOCK *recROM;	// and here
static BASEBLOCK *recROM1;	// also here
static BASEBLOCKEX *recBlocks = NULL;
static char *recPtr;
u32 psxpc;			// recompiler psxpc
int psxbranch;		// set for branch
static EEINST* s_pInstCache = NULL;
static u32 s_nInstCacheSize = 0;

u32 g_psxNextBranchCycle = 0;

static BASEBLOCK* s_pCurBlock = NULL;
static BASEBLOCKEX* s_pCurBlockEx = NULL;
static BASEBLOCK* s_pDispatchBlock = NULL;
static u32 s_nEndBlock = 0; // what psxpc the current block ends	

static u32 s_nNextBlock = 0; // next free block in recBlocks

static u32 s_ConstGPRreg;
static u32 s_saveConstGPRreg = 0, s_saveHasConstReg = 0, s_saveFlushedConstReg = 0;
static EEINST* s_psaveInstInfo = NULL;

u32 s_psxBlockCycles = 0; // cycles of current block recompiling
static u32 s_savenBlockCycles = 0;

static void iPsxBranchTest(u32 newpc, u32 cpuBranch);
void psxRecompileNextInstruction(int delayslot);
static void recRecompile(u32 startpc);

extern void (*rpsxBSC[64])();
extern void (*rpsxBSC_co[64])();
void rpsxpropBSC(EEINST* prev, EEINST* pinst);

#ifdef _DEBUG
extern "C" u32 psxdump = 0;
#else
#define psxdump 0
#endif

#define PSX_GETBLOCK(x) PC_GETBLOCK_(x, psxRecLUT)

#define PSXREC_CLEARM(mem) { \
	if ((mem) < g_psxMaxRecMem && psxRecLUT[(mem) >> 16]) { \
		BASEBLOCK* p = PSX_GETBLOCK(mem); \
		if( *(u32*)p ) psxRecClearMem(p); \
	} \
} \

BASEBLOCKEX* PSX_GETBLOCKEX(BASEBLOCK* p)
{
//	BASEBLOCKEX* pex = *(BASEBLOCKEX**)(p+1);
//	if( pex >= recBlocks && pex < recBlocks+PSX_NUMBLOCKS )
//		return pex;

	// otherwise, use the sorted list
	return GetBaseBlockEx(p->startpc, 1);
}

////////////////////////////////////////////////////
static void iDumpBlock( int startpc, char * ptr )
{
	FILE *f;
	char filename[ 256 ];
	u32 i, j;
	EEINST* pcur;
	u8 used[34];
	int numused, count;

	SysPrintf( "dump1 %x:%x, %x\n", startpc, psxpc, psxRegs.cycle );
#ifdef __WIN32__
	CreateDirectory("dumps", NULL);
sprintf( filename, "dumps\\psxdump%.8X.txt", startpc);
#else
	mkdir("dumps", 0755);
	sprintf( filename, "dumps/psxdump%.8X.txt", startpc);
#endif

	fflush( stdout );

	f = fopen( filename, "w" );
	assert( f != NULL );
	for ( i = startpc; i < s_nEndBlock; i += 4 ) {
		fprintf( f, "%s\n", disR3000Fasm( *(u32*)PSXM( i ), i ) );
	}

	// write the instruction info
	fprintf(f, "\n\nlive0 - %x, lastuse - %x used - %x\n", EEINST_LIVE0, EEINST_LASTUSE, EEINST_USED);

	memset(used, 0, sizeof(used));
	numused = 0;
	for(i = 0; i < ARRAYSIZE(s_pInstCache->regs); ++i) {
		if( s_pInstCache->regs[i] & EEINST_USED ) {
			used[i] = 1;
			numused++;
		}
	}

	fprintf(f, "       ");
	for(i = 0; i < ARRAYSIZE(s_pInstCache->regs); ++i) {
		if( used[i] ) fprintf(f, "%2d ", i);
	}
	fprintf(f, "\n");

	fprintf(f, "       ");
	for(i = 0; i < ARRAYSIZE(s_pInstCache->regs); ++i) {
		if( used[i] ) fprintf(f, "%s ", disRNameGPR[i]);
	}
	fprintf(f, "\n");

	pcur = s_pInstCache+1;
	for( i = 0; i < (s_nEndBlock-startpc)/4; ++i, ++pcur) {
		fprintf(f, "%2d: %2.2x ", i+1, pcur->info);
		
		count = 1;
		for(j = 0; j < ARRAYSIZE(s_pInstCache->regs); j++) {
			if( used[j] ) {
				fprintf(f, "%2.2x%s", pcur->regs[j], ((count%8)&&count<numused)?"_":" ");
				++count;
			}
		}
		fprintf(f, "\n");
	}
	fclose( f );
}

u8 _psxLoadWritesRs(u32 tempcode)
{
	switch(tempcode>>26) {
		case 32: case 33: case 34: case 35: case 36: case 37: case 38:
			return ((tempcode>>21)&0x1f)==((tempcode>>16)&0x1f); // rs==rt
	}
	return 0;
}

u8 _psxIsLoadStore(u32 tempcode)
{
	switch(tempcode>>26) {
		case 32: case 33: case 34: case 35: case 36: case 37: case 38:
		// 4 byte stores
		case 40: case 41: case 42: case 43: case 46:
			return 1;
	}
	return 0;
}

void _psxFlushAllUnused()
{
	int i;
	for(i = 0; i < 34; ++i) {
		if( psxpc < s_nEndBlock ) {
			if( (g_pCurInstInfo[1].regs[i]&EEINST_USED) )
				continue;
		}
		else if( (g_pCurInstInfo[0].regs[i]&EEINST_USED) )
			continue;

		if( i < 32 && PSX_IS_CONST1(i) ) _psxFlushConstReg(i);
		else {	
			_deleteX86reg(X86TYPE_PSX, i, 1);
		}
	}
}

int _psxFlushUnusedConstReg()
{
	int i;
	for(i = 1; i < 32; ++i) {
		if( (g_psxHasConstReg & (1<<i)) && !(g_psxFlushedConstReg&(1<<i)) &&
			!_recIsRegWritten(g_pCurInstInfo+1, (s_nEndBlock-psxpc)/4, XMMTYPE_GPRREG, i) ) {

			// check if will be written in the future
			MOV32ItoM((u32)&psxRegs.GPR.r[i], g_psxConstRegs[i]);
			g_psxFlushedConstReg |= 1<<i;
			return 1;
		}
	}

	return 0;
}

void _psxFlushCachedRegs()
{
	_psxFlushConstRegs();
}

void _psxFlushConstReg(int reg)
{
	if( PSX_IS_CONST1( reg ) && !(g_psxFlushedConstReg&(1<<reg)) ) {
		MOV32ItoM((int)&psxRegs.GPR.r[reg], g_psxConstRegs[reg]);
		g_psxFlushedConstReg |= (1<<reg);
	}
}

void _psxFlushConstRegs()
{
	int i;

	// flush constants

	// ignore r0
	for(i = 1; i < 32; ++i) {
		if( g_psxHasConstReg & (1<<i) ) {
			
			if( !(g_psxFlushedConstReg&(1<<i)) ) {
				MOV32ItoM((u32)&psxRegs.GPR.r[i], g_psxConstRegs[i]);
				g_psxFlushedConstReg |= 1<<i;
			}
#if defined(_DEBUG)&&0
			else {
				// make sure the const regs are the same
				u8* ptemp;
				CMP32ItoM((u32)&psxRegs.GPR.r[i], g_psxConstRegs[i]);
				ptemp = JE8(0);
				CALLFunc((u32)checkconstreg);
				x86SetJ8( ptemp );
			}
#else
			if( g_psxHasConstReg == g_psxFlushedConstReg )
				break;
#endif
		}
	}
}

void _psxDeleteReg(int reg, int flush)
{
	if( !reg ) return;
	if( flush && PSX_IS_CONST1(reg) ) {
		_psxFlushConstReg(reg);
		return;
	}
	PSX_DEL_CONST(reg);
	_deleteX86reg(X86TYPE_PSX, reg, flush ? 0 : 2);
}

void _psxFlushCall(int flushtype)
{
	_freeX86regs();

	if( flushtype & FLUSH_CACHED_REGS )
		_psxFlushConstRegs();
}

void psxSaveBranchState()
{
	s_savenBlockCycles = s_psxBlockCycles;
	s_saveConstGPRreg = 0xffffffff; // indicate searching
	s_saveHasConstReg = g_psxHasConstReg;
	s_saveFlushedConstReg = g_psxFlushedConstReg;
	s_psaveInstInfo = g_pCurInstInfo;

	// save all mmx regs
	memcpy(s_saveX86regs, x86regs, sizeof(x86regs));
}

void psxLoadBranchState()
{
	s_psxBlockCycles = s_savenBlockCycles;

	if( s_saveConstGPRreg != 0xffffffff ) {
		assert( s_saveConstGPRreg > 0 );

		// make sure right GPR was saved
		assert( g_psxHasConstReg == s_saveHasConstReg || (g_psxHasConstReg ^ s_saveHasConstReg) == (1<<s_saveConstGPRreg) );

		// restore the GPR reg
		g_psxConstRegs[s_saveConstGPRreg] = s_ConstGPRreg;
		PSX_SET_CONST(s_saveConstGPRreg);
		//s_saveConstGPRreg = 0;
	}

	g_psxHasConstReg = s_saveHasConstReg;
	g_psxFlushedConstReg = s_saveFlushedConstReg;
	g_pCurInstInfo = s_psaveInstInfo;

	// restore all mmx regs
	memcpy(x86regs, s_saveX86regs, sizeof(x86regs));
}

////////////////////
// Code Templates //
////////////////////

void PSX_CHECK_SAVE_REG(int reg)
{
	if( s_saveConstGPRreg == 0xffffffff ) {
		if( PSX_IS_CONST1(reg) ) {
			s_saveConstGPRreg = reg;
			s_ConstGPRreg = g_psxConstRegs[reg];
		}
	}
	else {
		// can be non zero when double loading
		//assert( s_saveConstGPRreg == 0 );
	}
}

void _psxOnWriteReg(int reg)
{
	PSX_CHECK_SAVE_REG(reg);
	PSX_DEL_CONST(reg);
}

// rd = rs op rt
void psxRecompileCodeConst0(R3000AFNPTR constcode, R3000AFNPTR_INFO constscode, R3000AFNPTR_INFO consttcode, R3000AFNPTR_INFO noconstcode)
{
	if ( ! _Rd_ ) return;

	// for now, don't support xmm
	PSX_CHECK_SAVE_REG(_Rd_);

	_deleteX86reg(X86TYPE_PSX, _Rs_, 1);
	_deleteX86reg(X86TYPE_PSX, _Rt_, 1);
	_deleteX86reg(X86TYPE_PSX, _Rd_, 0);

	if( PSX_IS_CONST2(_Rs_, _Rt_) ) {
		PSX_SET_CONST(_Rd_);
		constcode();
		return;
	}

	if( PSX_IS_CONST1(_Rs_) ) {
		constscode(0);
		PSX_DEL_CONST(_Rd_);
		return;
	}

	if( PSX_IS_CONST1(_Rt_) ) {
		consttcode(0);
		PSX_DEL_CONST(_Rd_);
		return;
	}

	noconstcode(0);
	PSX_DEL_CONST(_Rd_);
}

// rt = rs op imm16
void psxRecompileCodeConst1(R3000AFNPTR constcode, R3000AFNPTR_INFO noconstcode)
{
	if ( ! _Rt_ ) return;

	// for now, don't support xmm
	PSX_CHECK_SAVE_REG(_Rt_);

	_deleteX86reg(X86TYPE_PSX, _Rs_, 1);
	_deleteX86reg(X86TYPE_PSX, _Rt_, 0);

	if( PSX_IS_CONST1(_Rs_) ) {
		PSX_SET_CONST(_Rt_);
		constcode();
		return;
	}

	noconstcode(0);
	PSX_DEL_CONST(_Rt_);
}

// rd = rt op sa
void psxRecompileCodeConst2(R3000AFNPTR constcode, R3000AFNPTR_INFO noconstcode)
{
	if ( ! _Rd_ ) return;

	// for now, don't support xmm
	PSX_CHECK_SAVE_REG(_Rd_);

	_deleteX86reg(X86TYPE_PSX, _Rt_, 1);
	_deleteX86reg(X86TYPE_PSX, _Rd_, 0);

	if( PSX_IS_CONST1(_Rt_) ) {
		PSX_SET_CONST(_Rd_);
		constcode();
		return;
	}

	noconstcode(0);
	PSX_DEL_CONST(_Rd_);
}

// rd = rt MULT rs  (SPECIAL)
void psxRecompileCodeConst3(R3000AFNPTR constcode, R3000AFNPTR_INFO constscode, R3000AFNPTR_INFO consttcode, R3000AFNPTR_INFO noconstcode, int LOHI)
{
	_deleteX86reg(X86TYPE_PSX, _Rs_, 1);
	_deleteX86reg(X86TYPE_PSX, _Rt_, 1);

	if( LOHI ) {
		_deleteX86reg(X86TYPE_PSX, PSX_HI, 1);
		_deleteX86reg(X86TYPE_PSX, PSX_LO, 1);
	}

	if( PSX_IS_CONST2(_Rs_, _Rt_) ) {
		constcode();
		return;
	}

	if( PSX_IS_CONST1(_Rs_) ) {
		constscode(0);
		return;
	}

	if( PSX_IS_CONST1(_Rt_) ) {
		consttcode(0);
		return;
	}

	noconstcode(0);
}

static int recInit() {
	int i;

	psxRecLUT = (uptr*) malloc(0x010000 * sizeof(uptr));
	memset(psxRecLUT, 0, 0x010000 * sizeof(uptr));

	recMem = (char*)SysMmap(0, RECMEM_SIZE);
	recRAM = (BASEBLOCK*) _aligned_malloc(sizeof(BASEBLOCK)/4*0x200000, 16);
	recROM = (BASEBLOCK*) _aligned_malloc(sizeof(BASEBLOCK)/4*0x400000, 16);
	recROM1= (BASEBLOCK*) _aligned_malloc(sizeof(BASEBLOCK)/4*0x040000, 16);
	recBlocks = (BASEBLOCKEX*) _aligned_malloc( sizeof(BASEBLOCKEX)*PSX_NUMBLOCKS, 16);
	if (recRAM == NULL || recROM == NULL || recROM1 == NULL ||
		recMem == NULL || psxRecLUT == NULL) {
		SysMessage("Error allocating memory"); return -1;
	}

	s_nInstCacheSize = 128;
	s_pInstCache = (EEINST*)malloc( sizeof(EEINST) * s_nInstCacheSize );

	for (i=0; i<0x80; i++) psxRecLUT[i + 0x0000] = (uptr)&recRAM[(i & 0x1f) << 14];
	for (i=0; i<0x80; i++) psxRecLUT[i + 0x8000] = (uptr)&recRAM[(i & 0x1f) << 14];
	for (i=0; i<0x80; i++) psxRecLUT[i + 0xa000] = (uptr)&recRAM[(i & 0x1f) << 14];

	for (i=0; i<0x40; i++) psxRecLUT[i + 0x1fc0] = (uptr)&recROM[i << 14];
	for (i=0; i<0x40; i++) psxRecLUT[i + 0x9fc0] = (uptr)&recROM[i << 14];
	for (i=0; i<0x40; i++) psxRecLUT[i + 0xbfc0] = (uptr)&recROM[i << 14];

	for (i=0; i<0x40; i++) psxRecLUT[i + 0x1e00] = (uptr)&recROM1[i << 14];
	for (i=0; i<0x40; i++) psxRecLUT[i + 0x9e00] = (uptr)&recROM1[i << 14];
	for (i=0; i<0x40; i++) psxRecLUT[i + 0xbe00] = (uptr)&recROM1[i << 14];

	memset(recMem, 0xcd, RECMEM_SIZE);

	return 0;
}

static void recReset() {
	memset(recRAM, 0, sizeof(BASEBLOCK)/4*0x200000);
	memset(recROM, 0, sizeof(BASEBLOCK)/4*0x400000);
	memset(recROM1,0, sizeof(BASEBLOCK)/4*0x040000);

	memset( recBlocks, 0, sizeof(BASEBLOCKEX)*PSX_NUMBLOCKS );
	if( s_pInstCache ) memset( s_pInstCache, 0, sizeof(EEINST)*s_nInstCacheSize );
	ResetBaseBlockEx(1);
	g_psxMaxRecMem = 0;

	__asm emms

	recPtr = recMem;
	psxbranch = 0;
}

static void recShutdown() {
	if (recMem == NULL) return;
	free(psxRecLUT);
	SysMunmap((uptr)recMem, RECMEM_SIZE);
	_aligned_free(recRAM);
	_aligned_free(recROM);
	_aligned_free(recROM1);
	_aligned_free( recBlocks ); recBlocks = NULL;
	free( s_pInstCache ); s_pInstCache = NULL; s_nInstCacheSize = 0;

	x86Shutdown();
}

#pragma warning(disable:4731) // frame pointer register 'ebp' modified by inline assembly code
static u32 s_uSaveESP = 0;

static void execute()
{
#ifdef _DEBUG
	u8* fnptr;
	u32 oldesi;
#else
	R3000AFNPTR pfn;
#endif

	BASEBLOCK* pblock;

	while (EEsCycle > 0) {
		pblock = PSX_GETBLOCK(psxRegs.pc);

		if ( !pblock->pFnptr || pblock->startpc != psxRegs.pc ) {
			recRecompile(psxRegs.pc);
		}

		assert( pblock->pFnptr != 0 );

#ifdef _DEBUG

		fnptr = (u8*)pblock->pFnptr;
		__asm {
			// save data
			mov oldesi, esi
			mov s_uSaveESP, esp
			sub s_uSaveESP, 8
			push ebp

			call fnptr // jump into function
			// restore data
			pop ebp
			mov esi, oldesi
		}
#else
		pfn = ((R3000AFNPTR)pblock->pFnptr);
		// use call instead of pfn()
		__asm call pfn
#endif
	}
}

extern u32 g_psxNextBranchCycle;
u32 g_psxlastpc = 0;
static u32 g_temp;

// jumped to when invalid psxpc address
__declspec(naked,noreturn) void psxDispatcher()
{
	// EDX contains the current psxpc to jump to, stack contains the jump addr to modify
	__asm push edx

	// calc PSX_GETBLOCK
	s_pDispatchBlock = PSX_GETBLOCK(psxRegs.pc);
	
	__asm {
		mov eax, s_pDispatchBlock

		// check if startpc == psxRegs.pc
		mov ecx, psxRegs.pc
		//and ecx, 0x5fffffff // remove higher bits
		cmp ecx, dword ptr [eax+BLOCKTYPE_STARTPC]
		je CheckPtr

		// recompile
		push psxRegs.pc // psxpc
		call recRecompile
		add esp, 4 // pop old param
		mov eax, s_pDispatchBlock
CheckPtr:
		mov eax, dword ptr [eax]
	}

#ifdef _DEBUG
	__asm mov g_temp, eax
	assert( g_temp );
#endif

//	__asm {
//		test eax, 0x40000000 // BLOCKTYPE_NEEDCLEAR
//		jz Done
//		// move new psxpc
//		and eax, 0x0fffffff
//		mov ecx, psxRegs.pc
//		mov dword ptr [eax+1], ecx
//	}
	__asm {
		and eax, 0x0fffffff
		mov edx, eax
		pop ecx // x86Ptr to mod
		sub edx, ecx
		sub edx, 4
		mov dword ptr [ecx], edx

		jmp eax
	}
}

__declspec(naked,noreturn) void psxDispatcherClear()
{
	// EDX contains the current psxpc
	__asm mov psxRegs.pc, edx
	__asm push edx

	// calc PSX_GETBLOCK
	s_pDispatchBlock = PSX_GETBLOCK(psxRegs.pc);

	if( s_pDispatchBlock->startpc == psxRegs.pc ) {
		assert( s_pDispatchBlock->pFnptr != 0 );

		// already modded the code, jump to the new place
		__asm {
			pop edx
			add esp, 4 // ignore stack
			mov eax, s_pDispatchBlock
			mov eax, dword ptr [eax]
			and eax, 0x0fffffff
			jmp eax
		}
	}

	__asm {
		call recRecompile
		add esp, 4 // pop old param
		mov eax, s_pDispatchBlock
		mov eax, dword ptr [eax]

		pop ecx // old fnptr

		and eax, 0x0fffffff
		mov byte ptr [ecx], 0xe9 // jmp32
		mov edx, eax
		sub edx, ecx
		sub edx, 5
		mov dword ptr [ecx+1], edx

		jmp eax
	}
}

// called when jumping to variable psxpc address
__declspec(naked,noreturn) void psxDispatcherReg()
{
	__asm {
		//s_pDispatchBlock = PSX_GETBLOCK(psxRegs.pc);
		mov edx, psxRegs.pc
		mov ecx, edx
	}

	__asm {
		shr edx, 14
		and edx, 0xfffffffc
		add edx, psxRecLUT
		mov edx, dword ptr [edx]

		mov eax, ecx
		and eax, 0xfffc
		// edx += 2*eax
		shl eax, 1
		add edx, eax
		
		// check if startpc == psxRegs.pc
		mov eax, ecx
		//and eax, 0x5fffffff // remove higher bits
		cmp eax, dword ptr [edx+BLOCKTYPE_STARTPC]
		jne recomp

		mov eax, dword ptr [edx]
	}

#ifdef _DEBUG
	__asm mov g_temp, eax
	assert( g_temp );
#endif

	__asm {
		and eax, 0x0fffffff
		jmp eax // fnptr

recomp:
		sub esp, 8
		mov dword ptr [esp+4], edx
		mov dword ptr [esp], ecx
		call recRecompile
		mov edx, dword ptr [esp+4]
		add esp, 8
		
		mov eax, dword ptr [edx]
		and eax, 0x0fffffff
		jmp eax // fnptr
	}
}

static void recClear(u32 Addr, u32 Size)
{
	u32 i;
	for(i = 0; i < Size; ++i, Addr+=4) {
		PSXREC_CLEARM(Addr);
	}
}

#define EE_MIN_BLOCK_BYTES 15

void rpsxMemConstClear(u32 mem)
{
	// NOTE! This assumes recLUT never changes its mapping
	if( !psxRecLUT[mem>>16] )
		return;

	CMP32ItoM((u32)PSX_GETBLOCK(mem), 0);
	j8Ptr[6] = JE8(0);

	PUSH32I((u32)PSX_GETBLOCK(mem));
	CALLFunc((u32)psxRecClearMem);
	ADD32ItoR(ESP, 4);
	x86SetJ8(j8Ptr[6]);
}

void psxRecClearMem(BASEBLOCK* p)
{
	BASEBLOCKEX* pexblock;
	BASEBLOCK* pstart;
	int lastdelay;

	// necessary since recompiler doesn't call femms/emms
	if (cpucaps.has3DNOWInstructionExtensions) __asm femms
	else __asm emms
		
	assert( p != NULL );

	if( p->uType & BLOCKTYPE_DELAYSLOT ) {
		psxRecClearMem(p-1);
		if( p->pFnptr == 0 )
			return;
	}

	assert( p->pFnptr != 0 );
	assert( p->startpc );

	x86Ptr = (s8*)p->pFnptr;

	// there is a small problem: mem can be ored with 0xa<<28 or 0x8<<28, and don't know which
	MOV32ItoR(EDX, p->startpc);
	PUSH32I((u32)x86Ptr); // will be replaced by JMP32
	JMP32((u32)psxDispatcherClear - ( (u32)x86Ptr + 5 ));
	assert( x86Ptr == (s8*)p->pFnptr + EE_MIN_BLOCK_BYTES );

	pstart = PSX_GETBLOCK(p->startpc);
	pexblock = PSX_GETBLOCKEX(pstart);
	assert( pexblock->startpc == pstart->startpc );

//	if( pexblock->pOldFnptr ) {
//		// have to mod oldfnptr too
//		x86Ptr = pexblock->pOldFnptr;
//
//		MOV32ItoR(EDX, p->startpc);
//		JMP32((u32)DispatcherClear - ( (u32)x86Ptr + 5 ));
//	}
//	else
//		pexblock->pOldFnptr = (u8*)p->pFnptr;
	
	// don't delete if last is delay
	lastdelay = pexblock->size;
	if( pstart[pexblock->size-1].uType & BLOCKTYPE_DELAYSLOT ) {
		assert( pstart[pexblock->size-1].pFnptr != pstart->pFnptr );
		if( pstart[pexblock->size-1].pFnptr != 0 ) {
			pstart[pexblock->size-1].uType = 0;
			--lastdelay;
		}
	}

	memset(pstart, 0, lastdelay*sizeof(BASEBLOCK));

	RemoveBaseBlockEx(pexblock, 1);
	pexblock->size = 0;
	pexblock->startpc = 0;
}

void psxSetBranchReg(u32 reg)
{
	psxbranch = 1;

	if( reg != 0xffffffff ) {
		_allocX86reg(ESI, X86TYPE_PCWRITEBACK, 0, MODE_WRITE);
		_psxMoveGPRtoR(ESI, reg);

		psxRecompileNextInstruction(1);
		
		if( x86regs[ESI].inuse ) {
			assert( x86regs[ESI].type == X86TYPE_PCWRITEBACK );
			MOV32RtoM((int)&psxRegs.pc, ESI);
			x86regs[ESI].inuse = 0;
		}
		else {
			MOV32MtoR(EAX, (u32)&g_recWriteback);
			MOV32RtoM((int)&psxRegs.pc, EAX);
		}
	}
	
	_psxFlushCall(FLUSH_EVERYTHING);
	iPsxBranchTest(0xffffffff, 1);

	JMP32((u32)psxDispatcherReg - ( (u32)x86Ptr + 5 ));
}

void psxSetBranchImm( u32 imm )
{
	u32* ptr;
	psxbranch = 1;
	assert( imm );

	// end the current block
	MOV32ItoM( (u32)&psxRegs.pc, imm );
	_psxFlushCall(FLUSH_EVERYTHING);
	iPsxBranchTest(imm, imm <= psxpc);

	MOV32ItoR(EDX, 0);
	ptr = (u32*)(x86Ptr-4);
	*ptr = (u32)JMP32((u32)psxDispatcher - ( (u32)x86Ptr + 5 ));
}

#define USE_FAST_BRANCHES 0

static void iPsxBranchTest(u32 newpc, u32 cpuBranch)
{
	if( !USE_FAST_BRANCHES || cpuBranch ) {
		MOV32MtoR(ECX, (int)&psxRegs.cycle);
		ADD32ItoR(ECX, s_psxBlockCycles*17/16); // greater mult factor causes nfsmw to crash
		MOV32RtoM((int)&psxRegs.cycle, ECX); // update cycles
	}
	else {
		ADD32ItoM((int)&psxRegs.cycle, s_psxBlockCycles*17/16);
		return;
	}

	SUB32MtoR(ECX, (int)&g_psxNextBranchCycle);

	// check if should branch
	j8Ptr[0] = JS8( 0 );

	CALLFunc((int)psxBranchTest);

	CMP32ItoM((int)&EEsCycle, 0);
	j8Ptr[2] = JG8(0);
	RET2();
	x86SetJ8( j8Ptr[2] );

	if( newpc != 0xffffffff ) {
		CMP32ItoM((int)&psxRegs.pc, newpc);
		JNE32((u32)psxDispatcherReg - ( (u32)x86Ptr + 6 ));
	}

	x86SetJ8( j8Ptr[0] );
}

static int *s_pCode;

static void checkcodefn()
{
	int pctemp;

	__asm mov pctemp, eax
	SysPrintf("iop code changed! %x\n", pctemp);
}

void rpsxSYSCALL()
{
	MOV32ItoM( (u32)&psxRegs.code, psxRegs.code );
	MOV32ItoM((uptr)&psxRegs.pc, psxpc - 4);
	_psxFlushCall(FLUSH_NODESTROY);

	PUSH32I(psxbranch == 1 ? 1 : 0);
	PUSH32I(0x20);
	CALLFunc ((int)psxException);
	ADD32ItoR(ESP, 8);

	CMP32ItoM((int)&psxRegs.pc, psxpc-4);
	j8Ptr[0] = JE8(0);
	ADD32ItoM((u32)&psxRegs.cycle, s_psxBlockCycles);
	JMP32((u32)psxDispatcherReg - ( (u32)x86Ptr + 5 ));
	x86SetJ8(j8Ptr[0]);

	//if (!psxbranch) psxbranch = 2;
}

u32 psxRecompileCodeSafe(u32 temppc)
{
	BASEBLOCK* pblock = PSX_GETBLOCK(temppc);

	if( pblock->pFnptr != 0 && pblock->startpc != s_pCurBlock->startpc ) {
		if( psxpc == pblock->startpc )
			return 0;
	}

	return 1;
}

void psxRecompileNextInstruction(int delayslot)
{
	static u8 s_bFlushReg = 1;

	BASEBLOCK* pblock = PSX_GETBLOCK(psxpc);

	// need *ppblock != s_pCurBlock because of branches
	if( pblock->pFnptr != 0 && pblock->startpc != s_pCurBlock->startpc ) {

		if( !delayslot && psxpc == pblock->startpc ) {
			// code already in place, so jump to it and exit recomp
			assert( PSX_GETBLOCKEX(pblock)->startpc == pblock->startpc );
			
			_psxFlushCall(FLUSH_EVERYTHING);
			MOV32ItoM((u32)&psxRegs.pc, psxpc);
				
//			if( pexblock->pOldFnptr ) {
//				// code already in place, so jump to it and exit recomp
//				JMP32((u32)pexblock->pOldFnptr - ((u32)x86Ptr + 5));
//				branch = 3;
//				return;
//			}
			
			JMP32((u32)pblock->pFnptr - ((u32)x86Ptr + 5));
			psxbranch = 3;
			return;
		}
		else {

			if( !(delayslot && pblock->startpc == psxpc) ) {
				s8* oldX86 = x86Ptr;
				//__Log("clear block %x\n", pblock->startpc);
				psxRecClearMem(pblock);
				x86Ptr = oldX86;
				if( delayslot )
					SysPrintf("delay slot %x\n", psxpc);
			}
		}
	}

	if( delayslot )
		pblock->uType = BLOCKTYPE_DELAYSLOT;

	s_pCode = (int *)PSXM( psxpc );
	assert(s_pCode);

	psxRegs.code = *(int *)s_pCode;
	s_psxBlockCycles++;
	psxpc += 4;

//#ifdef _DEBUG
//	CMP32ItoM((u32)s_pCode, psxRegs.code);
//	j8Ptr[0] = JE8(0);
//	MOV32ItoR(EAX, psxpc);
//	CALLFunc((u32)checkcodefn);
//	x86SetJ8( j8Ptr[ 0 ] );
//#endif

	g_pCurInstInfo++;

	// peephole optimizations
	if( g_pCurInstInfo->info & EEINSTINFO_COREC ) {
		assert(0);
//		recBSC_co[cpuRegs.code>>26]();
//		psxpc += 4;
//		s_psxBlockCycles++;
//		g_pCurInstInfo++;
	}
	else {
	 	assert( !(g_pCurInstInfo->info & EEINSTINFO_NOREC) );
		rpsxBSC[ psxRegs.code >> 26 ]();
	}

	if( !delayslot ) {
		if( s_bFlushReg ) {
			//_psxFlushUnusedConstReg();
		}
		else s_bFlushReg = 1;
	}
	else s_bFlushReg = 1;

	_clearNeededX86regs();
}

static void recExecute() {
	for (;;) execute();
}

static void recExecuteBlock() {
	 
	execute();
}

extern "C"
void iDumpPsxRegisters(u32 startpc, u32 temp)
{
	int i;
	char* pstr = temp ? "t" : "";

	__Log("%spsxreg: %x %x\n", pstr, startpc, psxRegs.interrupt);
	for(i = 0; i < 34; i+=2) __Log("%spsx%d: %x %x\n", pstr, i, psxRegs.GPR.r[i], psxRegs.GPR.r[i+1]);
	__Log("%scycle: %x %x %x %x; counters %x %x\n", pstr, psxRegs.cycle, g_psxNextBranchCycle, EEsCycle, IOPoCycle,
		(u32)psxNextsCounter, (u32)psxNextCounter);

	for(i = 0; i < 6; ++i) __Log("%scounter%d: %x %x %x\n", pstr, i, psxCounters[i].count, psxCounters[i].rate, psxCounters[i].sCycleT);
	for(i = 0; i < 32; ++i) {
		__Log("int%d: %x %x\n", i, psxRegs.sCycle[i], psxRegs.eCycle[i]);
	}
}

void iDumpPsxRegisters(u32 startpc);

static void printfn()
{
	static int lastrec = 0;
	static int curcount = 0;
	const int skip = 0;

	if( (psxdump&2) ) {//&& lastrec != g_psxlastpc ) {
		curcount++;

		if( curcount > skip ) {
			iDumpPsxRegisters(g_psxlastpc, 1);
			curcount = 0;
		}

		lastrec = g_psxlastpc;
	}
}

u32 s_recblocks[] = {0};

static void recRecompile(u32 startpc)
{
	u32 i;
	u32 branchTo;
	u32 willbranch3 = 0;
	u32* ptr;

#ifdef _DEBUG
	//psxdump |= 4;
	if( psxdump & 4 )
		iDumpPsxRegisters(startpc, 0);
#endif

	assert( startpc );

	// if recPtr reached the mem limit reset whole mem
	if (((uptr)recPtr - (uptr)recMem) >= (RECMEM_SIZE - 0x10000))
		recReset();

	s_pCurBlock = PSX_GETBLOCK(startpc);
	
	if( s_pCurBlock->pFnptr ) {
		// clear if already taken
		assert( s_pCurBlock->startpc < startpc );
		psxRecClearMem(s_pCurBlock);	
	}

	if( s_pCurBlock->startpc == startpc ) {
		s_pCurBlockEx = PSX_GETBLOCKEX(s_pCurBlock);
		assert( s_pCurBlockEx->startpc == startpc );
	}
	else {
		s_pCurBlockEx = NULL;
		for(i = 0; i < PSX_NUMBLOCKS; ++i) {
			if( recBlocks[(i+s_nNextBlock)%PSX_NUMBLOCKS].size == 0 ) {
				s_pCurBlockEx = recBlocks+(i+s_nNextBlock)%PSX_NUMBLOCKS;
				s_nNextBlock = (i+s_nNextBlock+1)%PSX_NUMBLOCKS;
				break;
			}
		}

		if( s_pCurBlockEx == NULL ) {
			//SysPrintf("ee reset (blocks)\n");
			recReset();
			s_nNextBlock = 0;
			s_pCurBlockEx = recBlocks;
		}

		s_pCurBlockEx->startpc = startpc;
	}
	
	x86SetPtr( recPtr );
	x86Align(16);
	recPtr = x86Ptr;

	psxbranch = 0;

	s_pCurBlock->startpc = startpc;
	s_pCurBlock->pFnptr = (u32)x86Ptr;
	s_psxBlockCycles = 0;

	// reset recomp state variables
	psxpc = startpc;
	s_saveConstGPRreg = 0;
	g_psxHasConstReg = g_psxFlushedConstReg = 1;

	_initX86regs();

#ifdef _DEBUG
	// for debugging purposes
	MOV32ItoM((u32)&g_psxlastpc, psxpc);
	CALLFunc((u32)printfn);
#endif

	// go until the next branch
	i = startpc;
	s_nEndBlock = 0xffffffff;
	
	while(1) {
		BASEBLOCK* pblock = PSX_GETBLOCK(i);
		if( pblock->pFnptr != 0 && pblock->startpc != s_pCurBlock->startpc ) {

			if( i == pblock->startpc ) {
				// branch = 3
				willbranch3 = 1;
				s_nEndBlock = i;
				break;
			}
		}

		psxRegs.code = *(int *)PSXM(i);

		switch(psxRegs.code >> 26) {
			case 0: // special

				if( _Funct_ == 8 || _Funct_ == 9 ) { // JR, JALR
					s_nEndBlock = i + 8;
					goto StartRecomp;
				}

				break;
			case 1: // regimm
				
				if( _Rt_ == 0 || _Rt_ == 1 || _Rt_ == 16 || _Rt_ == 17 ) {
					
					branchTo = _Imm_ * 4 + i + 4;
					if( branchTo > startpc && branchTo < i ) s_nEndBlock = branchTo;
					else  s_nEndBlock = i+8;

					goto StartRecomp;
				}

				break;

			case 2: // J
			case 3: // JAL
				s_nEndBlock = i + 8;
				goto StartRecomp;

			// branches
			case 4: case 5: case 6: case 7: 

				branchTo = _Imm_ * 4 + i + 4;
				if( branchTo > startpc && branchTo < i ) s_nEndBlock = branchTo;
				else  s_nEndBlock = i+8;
				
				goto StartRecomp;
		}

		i += 4;
	}

StartRecomp:

	// rec info //
	{
		EEINST* pcur;

		if( s_nInstCacheSize < (s_nEndBlock-startpc)/4+1 ) {
			free(s_pInstCache);
			s_nInstCacheSize = (s_nEndBlock-startpc)/4+10;
			s_pInstCache = (EEINST*)malloc(sizeof(EEINST)*s_nInstCacheSize);
			assert( s_pInstCache != NULL );
		}

		pcur = s_pInstCache + (s_nEndBlock-startpc)/4;
		_recClearInst(pcur);
		pcur->info = 0;

		for(i = s_nEndBlock; i > startpc; i -= 4 ) {
			psxRegs.code = *(int *)PSXM(i-4);
			pcur[-1] = pcur[0];
			rpsxpropBSC(pcur-1, pcur);
			pcur--;
		}
	}

	// peephole optimizations //
//	{
//		g_pCurInstInfo = s_pInstCache;
//
//		for(i = startpc; i < s_nEndBlock-4; i += 4) {
//			g_pCurInstInfo++;
//			if( psxRecompileCodeSafe(i) ) {
//				u32 curcode = *(u32*)PSXM(i);
//				u32 nextcode = *(u32*)PSXM(i+4);
//				if( _psxIsLoadStore(curcode) && _psxIsLoadStore(nextcode) && (curcode>>26) == (nextcode>>26) && rpsxBSC_co[curcode>>26] != NULL ) {
//
//					// rs has to be the same, and cannot be just written
//					if( ((curcode >> 21) & 0x1F) == ((nextcode >> 21) & 0x1F) && !_psxLoadWritesRs(curcode) ) {
//
//						// good enough
//						g_pCurInstInfo[0].info |= EEINSTINFO_COREC;
//						g_pCurInstInfo[0].numpeeps = 1;
//						g_pCurInstInfo[1].info |= EEINSTINFO_NOREC;
//						g_pCurInstInfo++;
//						i += 4;
//						continue;
//					}
//				}
//			}
//		}
//	}

#ifdef _DEBUG
	// dump code
	for(i = 0; i < ARRAYSIZE(s_recblocks); ++i) {
		if( startpc == s_recblocks[i] ) {
			iDumpBlock(startpc, recPtr);
		}
	}

	if( (psxdump & 1) )
		iDumpBlock(startpc, recPtr);
#endif

	g_pCurInstInfo = s_pInstCache;
	while (!psxbranch && psxpc < s_nEndBlock) {
		psxRecompileNextInstruction(0);
	}

	assert( (psxpc-startpc)>>2 <= 0xffff );
	s_pCurBlockEx->size = (psxpc-startpc)>>2;

	for(i = 1; i < (u32)s_pCurBlockEx->size-1; ++i) {
		s_pCurBlock[i].pFnptr = s_pCurBlock->pFnptr;
		s_pCurBlock[i].startpc = s_pCurBlock->startpc;
	}

	// don't overwrite if delay slot
	if( i < (u32)s_pCurBlockEx->size && !(s_pCurBlock[i].uType & BLOCKTYPE_DELAYSLOT) ) {
		s_pCurBlock[i].pFnptr = s_pCurBlock->pFnptr;
		s_pCurBlock[i].startpc = s_pCurBlock->startpc;
	}

	// set the block ptr
	AddBaseBlockEx(s_pCurBlockEx, 1);

	if( !(psxpc&0x10000000) )
		g_psxMaxRecMem = max( (psxpc&~0xa0000000), g_psxMaxRecMem );

	if( psxbranch == 2 ) {
		_psxFlushCall(FLUSH_EVERYTHING);

		iPsxBranchTest(0xffffffff, 1);	

		JMP32((u32)psxDispatcherReg - ( (u32)x86Ptr + 5 ));
	}
	else {
		assert( psxbranch != 3 );
		if( psxbranch ) assert( !willbranch3 );
		else ADD32ItoM((int)&psxRegs.cycle, s_psxBlockCycles*17/16);

		if( willbranch3 ) {
			BASEBLOCK* pblock = PSX_GETBLOCK(s_nEndBlock);
			assert( psxpc == s_nEndBlock );
			_psxFlushCall(FLUSH_EVERYTHING);
			MOV32ItoM((u32)&psxRegs.pc, psxpc);			
			JMP32((u32)pblock->pFnptr - ((u32)x86Ptr + 5));
			psxbranch = 3;
		}
		else if( !psxbranch ) {
			// didn't branch, but had to stop
			MOV32ItoM( (u32)&psxRegs.pc, psxpc );

			_psxFlushCall(FLUSH_EVERYTHING);

			ptr = JMP32(0);
			//JMP32((u32)psxDispatcherReg - ( (u32)x86Ptr + 5 ));
		}
	}

	assert( x86Ptr >= (s8*)s_pCurBlock->pFnptr + EE_MIN_BLOCK_BYTES );
	assert( x86Ptr < recMem+RECMEM_SIZE );

	recPtr = x86Ptr;

	assert( (g_psxHasConstReg&g_psxFlushedConstReg) == g_psxHasConstReg );

	if( !psxbranch ) {
		BASEBLOCK* pcurblock = s_pCurBlock;
		u32 nEndBlock = s_nEndBlock;
		s_pCurBlock = PSX_GETBLOCK(psxpc);
		assert( ptr != NULL );
		
		if( s_pCurBlock->startpc != psxpc ) 
 			recRecompile(psxpc);

		// could have reset
		if( pcurblock->startpc == startpc ) {
			assert( pcurblock->pFnptr );
			assert( s_pCurBlock->startpc == nEndBlock );
			*ptr = s_pCurBlock->pFnptr - ( (u32)ptr + 4 );
		}
		else {
			recRecompile(startpc);
			assert( pcurblock->pFnptr != 0 );
		}
	}
}

R3000Acpu psxRec = {
	recInit,
	recReset,
	recExecute,
	recExecuteBlock,
	recClear,
	recShutdown
};

#endif