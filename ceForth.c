/******************************************************************************/
/* ceForth_33.cpp, Version 3.3 : Forth in C                                   */
/******************************************************************************/

/* Chen-Hanson Ting                                                           */
/* 01jul19cht   version 3.3                                                   */
/* Macro assembler, Visual Studio 2019 Community                              */
/* 13jul17cht   version 2.3                                                   */
/* True byte code machine with bytecode                                       */
/* Change w to WP, pointing to parameter field                                */
/* 08jul17cht  version 2.2                                                    */
/* Stacks are 256 cell circular buffers                                       */
/* Clean up, delete SP@, SP!, RP@, RP!                                        */
/* 13jun17cht  version 2.1                                                    */
/* Compiled as a C++ console project in Visual Studio Community 2017          */
/* Follow the eForth model with 64 primitives                                 */
/* Kernel                                                                     */
/* Use long long int to implement multipy and divide primitives               */
/* Case insensitive interpreter                                               */
/* data[] must be filled with rom_21.h eForth dictionary                      */
/*   from c:/F#/ceforth_21                                                    */
/******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

#define	FALSE	0
#define	TRUE	-1
#define	LOGICAL ? TRUE : FALSE
#define LOWER(x,y) ((uint32_t)(x)<(uint32_t)(y))
#define	pop	top = stack[S--]
#define	push	stack[++S] = top; top =
#define	popR	rack[R--]
#define	pushR	rack[++R]

int32_t rack[256] = { 0 };
int32_t stack[256] = { 0 };
int8_t R = 0; // uint8_t
int8_t S = 0; // uint8_t
int32_t top = 0;
int32_t  P, IP, WP, thread;
int32_t data[16000] = {};
uint8_t* cData = (uint8_t*)data;

// Virtual Forth Machine

void
bye(void)
{
	exit(0);
}

void
qrx(void)
{
	push(long) getchar();
	if (top != 0) {
		push TRUE;
	}
}

void
txsto(void)
{
	putchar(top);
	pop;
}

void
next(void)
{
	P = data[IP >> 2];
	WP = P + 4;
	IP += 4;
}

void
dovar(void)
{
	push WP;
}

void
docon(void)
{
	push data[WP >> 2];
}

void
dolit(void)
{
	push data[IP >> 2];
	IP += 4;
	next();
}

void
dolist(void)
{
	rack[++R] = IP;
	IP = WP;
	next();
}

void
exitt(void)
{
	IP = (long)rack[R--];
	next();
}

void
execu(void)
{
	P = top;
	WP = P + 4;
	pop;
}

void
donext(void)
{
	if (rack[R]) {
		rack[R] -= 1;
		IP = data[IP >> 2];
	} else {
		IP += 4;
		R--;
	}
	next();
}

void
qbran(void)
{
	if (top == 0) {
		IP = data[IP >> 2];
	} else {
		IP += 4;
	}
	pop;
	next();
}

void
bran(void)
{
	IP = data[IP >> 2];
	next();
}

void
store(void)
{
	data[top >> 2] = stack[S--];
	pop;
}

void
at(void)
{
	top = data[top >> 2];
}

void
cstor(void)
{
	cData[top] = stack[S--];
	pop;
}

void
cat(void)
{
	top = (long)cData[top];
}

void
rfrom(void)
{
	push rack[R--];
}

void
rat(void)
{
	push rack[R];
}

void
tor(void)
{
	rack[++R] = top;
	pop;
}

void
drop(void)
{
	pop;
}

void
dup(void)
{
	stack[++S] = top;
}

void
swap(void)
{
	WP = top;
	top = stack[S];
	stack[S] = WP;
}

void
over(void)
{
	push stack[S - 1];
}

void
zless(void)
{
	top = (top < 0) LOGICAL;
}

void
andd(void)
{
	top &= stack[S--];
}

void
orr(void)
{
	top |= stack[S--];
}

void
xorr(void)
{
	top ^= stack[S--];
}

void
uplus(void)
{
	stack[S] += top;
	top = LOWER(stack[S], top);
}

void
nop(void)
{
	next();
}

void
qdup(void)
{
	if (top) {
		stack[++S] = top;
	}
}

void
rot(void)
{
	WP = stack[S - 1];
	stack[S - 1] = stack[S];
	stack[S] = top;
	top = WP;
}

void
ddrop(void)
{
	drop(); drop();
}

void
ddup(void)
{
	over(); over();
}

void
plus(void)
{
	top += stack[S--];
}

void
inver(void)
{
	top = -top - 1;
}

void
negat(void)
{
	top = 0 - top;
}

void
dnega(void)
{
	inver();
	tor();
	inver();
	push 1;
	uplus();
	rfrom();
	plus();
}

void
subb(void)
{
	top = stack[S--] - top;
}

void
abss(void)
{
	if (top < 0) {
		top = -top;
	}
}

void
great(void)
{
	top = (stack[S--] > top) LOGICAL;
}

void
less(void)
{
	top = (stack[S--] < top) LOGICAL;
}

void
equal(void)
{
	top = (stack[S--] == top) LOGICAL;
}

void
uless(void)
{
	top = LOWER(stack[S], top) LOGICAL; S--;
}

void
ummod(void)
{
	int64_t d = (uint32_t)top;
	int64_t m = (uint32_t)stack[S];
	int64_t n = (uint32_t)stack[S - 1];
	n += m << 32;
	pop;
	top = (uint32_t)(n / d);
	stack[S] = (uint32_t)(n % d);
}

void
msmod(void)
{
	int64_t d = top;
	int64_t m = stack[S];
	int64_t n = stack[S - 1];
	n += m << 32;
	pop;
	top = (n / d);
	stack[S] = (n % d);
}

void
slmod(void)
{
	if (top != 0) {
		WP = stack[S] / top;
		stack[S] %= top;
		top = WP;
	}
}

void
mod(void)
{
	top = (top) ? stack[S--] % top : stack[S--];
}

void
slash(void)
{
	top = (top) ? stack[S--] / top : (S--, 0);
}

void
umsta(void)
{
	int64_t d = (uint32_t)top;
	int64_t m = (uint32_t)stack[S];
	m *= d;
	top = (uint32_t)(m >> 32);
	stack[S] = (uint32_t)m;
}

void
star(void)
{
	top *= stack[S--];
}

void
mstar(void)
{
	int64_t d = top;
	int64_t m = stack[S];
	m *= d;
	top = (m >> 32);
	stack[S] = m;
}

void
ssmod(void)
{
	int64_t d = top;
	int64_t m = stack[S];
	int64_t n = stack[S - 1];
	n *= m;
	pop;
	top = (n / d);
	stack[S] = (n % d);
}

void
stasl(void)
{
	int64_t d = top;
	int64_t m = stack[S];
	int64_t n = stack[S - 1];
	n *= m;
	pop; pop;
	top = (n / d);
}

void
pick(void)
{
	top = stack[S - top];
}

void
pstor(void)
{
	data[top >> 2] += stack[S--], pop;
}

void
dstor(void)
{
	data[(top >> 2) + 1] = stack[S--];
	data[top >> 2] = stack[S--];
	pop;
}

void
dat(void)
{
	push data[top >> 2];
	top = data[(top >> 2) + 1];
}

void
count(void)
{
	stack[++S] = top + 1;
	top = cData[top];
}

void
max(void)
{
	if (top < stack[S]) {
		pop;
	} else {
		S--;
	}
}

void
min(void)
{
	if (top < stack[S]) {
		S--;
	} else {
		pop;
	}
}

// Byte Code Assembler
enum  {
	as_nop, as_bye, as_qrx, as_txsto, as_docon, as_dolit, as_dolist, as_exit,
	as_execu, as_donext, as_qbran, as_bran, as_store, as_at, as_cstor, as_cat,
	as_rpat, as_rpsto, as_rfrom, as_rat, as_tor, as_spat, as_spsto, as_drop,
	as_dup, as_swap, as_over, as_zless, as_andd, as_orr, as_xorr, as_uplus,
	as_next, as_qdup, as_rot, as_ddrop, as_ddup, as_plus, as_inver, as_negat,
	as_dnega, as_subb, as_abss, as_equal, as_uless, as_less, as_ummod, as_msmod,
	as_slmod, as_mod, as_slash, as_umsta, as_star, as_mstar, as_ssmod, as_stasl,
	as_pick, as_pstor, as_dstor, as_dat, as_count, as_dovar, as_max, as_min,
};

void(*primitives[64])(void) = {
	[as_nop] = nop,
	[as_bye] = bye,
	[as_qrx] = qrx,
	[as_txsto] = txsto,
	[as_docon] = docon,
	[as_dolit] = dolit,
	[as_dolist] = dolist,
	[as_exit] = exitt,
	[as_execu] = execu,
	[as_donext] = donext,
	[as_qbran] = qbran,
	[as_bran] = bran,
	[as_store] = store,
	[as_at] = at,
	[as_cstor] = cstor,
	[as_cat] = cat,
	[as_rpat] = nop,
	[as_rpsto] = nop,
	[as_rfrom] = rfrom,
	[as_rat] = rat,
	[as_tor] = tor,
	[as_spat] = nop,
	[as_spsto] = nop,
	[as_drop] = drop,
	[as_dup] = dup,
	[as_swap] = swap,
	[as_over] = over,
	[as_zless] = zless,
	[as_andd] = andd,
	[as_orr] = orr,
	[as_xorr] = xorr,
	[as_uplus] = uplus,
	[as_next] = next,
	[as_qdup] = qdup,
	[as_rot] = rot,
	[as_ddrop] = ddrop,
	[as_ddup] = ddup,
	[as_plus] = plus,
	[as_inver] = inver,
	[as_negat] = negat,
	[as_dnega] = dnega,
	[as_subb] = subb,
	[as_abss] = abss,
	[as_equal] = equal,
	[as_uless] = uless,
	[as_less] = less,
	[as_ummod] = ummod,
	[as_msmod] = msmod,
	[as_slmod] = slmod,
	[as_mod] = mod,
	[as_slash] = slash,
	[as_umsta] = umsta,
	[as_star] = star,
	[as_mstar] = mstar,
	[as_ssmod] = ssmod,
	[as_stasl] = stasl,
	[as_pick] = pick,
	[as_pstor] = pstor,
	[as_dstor] = dstor,
	[as_dat] = dat,
	[as_count] = count,
	[as_dovar] = dovar,
	[as_max] = max,
	[as_min] = min,
};

// Macro Assembler

int IMEDD = 0x80;
int COMPO = 0x40;
int BRAN = 0, QBRAN = 0, DONXT = 0, DOTQP = 0, STRQP = 0, TOR = 0, ABORQP = 0;

void
HEADER(int lex, const char seq[])
{
	IP = P >> 2;
	int i;
	int len = lex & 31;
	data[IP++] = thread;
	P = IP << 2;
	thread = P;
	cData[P++] = lex;
	for (i = 0; i < len; i++) {
		cData[P++] = seq[i];
	}
	while (P & 3) {
		cData[P++] = 0;
	}
}

int
CODE(int len, ...)
{
	int addr = P;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		cData[P++] = j;
	}
	va_end(argList);
	return addr;
}

int
COLON(int len, ...)
{
	int addr = P;
	IP = P >> 2;
	data[IP++] = as_dolist; // dolist
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
	return addr;
}

int
LABEL(int len, ...)
{
	int addr = P;
	IP = P >> 2;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
	return addr;
}

void
BEGIN(int len, ...)
{
	IP = P >> 2;
	pushR = IP;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
}

void
AGAIN(int len, ...)
{
	IP = P >> 2;
	data[IP++] = BRAN;
	data[IP++] = popR << 2;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
}

void
UNTIL(int len, ...)
{
	IP = P >> 2;
	data[IP++] = QBRAN;
	data[IP++] = popR << 2;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
}

void
WHILE(int len, ...)
{
	IP = P >> 2;
	int k;
	data[IP++] = QBRAN;
	data[IP++] = 0;
	k = popR;
	pushR = (IP - 1);
	pushR = k;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
}

void
REPEAT(int len, ...)
{
	IP = P >> 2;
	data[IP++] = BRAN;
	data[IP++] = popR << 2;
	data[popR] = IP << 2;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
}

void
IF(int len, ...)
{
	IP = P >> 2;
	data[IP++] = QBRAN;
	pushR = IP;
	data[IP++] = 0;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
}

void
ELSE(int len, ...)
{
	IP = P >> 2;
	data[IP++] = BRAN;
	data[IP++] = 0;
	data[popR] = IP << 2;
	pushR = IP - 1;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
}

void
THEN(int len, ...)
{
	IP = P >> 2;
	data[popR] = IP << 2;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
}

void
FOR(int len, ...)
{
	IP = P >> 2;
	data[IP++] = TOR;
	pushR = IP;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
}

void
NEXT(int len, ...)
{
	IP = P >> 2;
	data[IP++] = DONXT;
	data[IP++] = popR << 2;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
}

void
AFT(int len, ...)
{
	IP = P >> 2;
	int k;
	data[IP++] = BRAN;
	data[IP++] = 0;
	k = popR;
	pushR = IP;
	pushR = IP - 1;
	va_list argList;
	va_start(argList, len);
	for (; len; len--) {
		int j = va_arg(argList, int);
		data[IP++] = j;
	}
	P = IP << 2;
	va_end(argList);
}

void
DOTQ(const char seq[])
{
	IP = P >> 2;
	int i;
	int len = strlen(seq);
	data[IP++] = DOTQP;
	P = IP << 2;
	cData[P++] = len;
	for (i = 0; i < len; i++) {
		cData[P++] = seq[i];
	}
	while (P & 3) {
		cData[P++] = 0;
	}
}

void
STRQ(const char seq[])
{
	IP = P >> 2;
	int i;
	int len = strlen(seq);
	data[IP++] = STRQP;
	P = IP << 2;
	cData[P++] = len;
	for (i = 0; i < len; i++) {
		cData[P++] = seq[i];
	}
	while (P & 3) {
		cData[P++] = 0;
	}
}

void
ABORQ(const char seq[])
{
	IP = P >> 2;
	int i;
	int len = strlen(seq);
	data[IP++] = ABORQP;
	P = IP << 2;
	cData[P++] = len;
	for (i = 0; i < len; i++) {
		cData[P++] = seq[i];
	}
	while (P & 3) {
		cData[P++] = 0;
	}
}

/*
* Main Program
*/

int
main(int ac, char* av[])
{
	P = 512;
	R = 0;

	// Kernel

	HEADER(3, "HLD");
	int HLD = CODE(8, as_docon, as_next, 0, 0, 0x80, 0, 0, 0);
	HEADER(4, "SPAN");
	int SPAN = CODE(8, as_docon, as_next, 0, 0, 0x84, 0, 0, 0);
	HEADER(3, ">IN");
	int INN = CODE(8, as_docon, as_next, 0, 0, 0x88, 0, 0, 0);
	HEADER(4, "#TIB");
	int NTIB = CODE(8, as_docon, as_next, 0, 0, 0x8C, 0, 0, 0);
	HEADER(4, "'TIB");
	int TTIB = CODE(8, as_docon, as_next, 0, 0, 0x90, 0, 0, 0);
	HEADER(4, "BASE");
	int BASE = CODE(8, as_docon, as_next, 0, 0, 0x94, 0, 0, 0);
	HEADER(7, "CONTEXT");
	int CNTXT = CODE(8, as_docon, as_next, 0, 0, 0x98, 0, 0, 0);
	HEADER(2, "CP");
	int CP = CODE(8, as_docon, as_next, 0, 0, 0x9C, 0, 0, 0);
	HEADER(4, "LAST");
	int LAST = CODE(8, as_docon, as_next, 0, 0, 0xA0, 0, 0, 0);
	HEADER(5, "'EVAL");
	int TEVAL = CODE(8, as_docon, as_next, 0, 0, 0xA4, 0, 0, 0);
	HEADER(6, "'ABORT");
	int TABRT = CODE(8, as_docon, as_next, 0, 0, 0xA8, 0, 0, 0);
	HEADER(3, "tmp");
	int TEMP = CODE(8, as_docon, as_next, 0, 0, 0xAC, 0, 0, 0);

	HEADER(3, "NOP");
	int NOP = CODE(4, as_next, 0, 0, 0);
	HEADER(3, "BYE");
	int BYE = CODE(4, as_bye, as_next, 0, 0);
	HEADER(3, "?RX");
	int QRX = CODE(4, as_qrx, as_next, 0, 0);
	HEADER(3, "TX!");
	int TXSTO = CODE(4, as_txsto, as_next, 0, 0);
	HEADER(5, "DOCON");
	int DOCON = CODE(4, as_docon, as_next, 0, 0);
	HEADER(5, "DOLIT");
	int DOLIT = CODE(4, as_dolit, as_next, 0, 0);
	HEADER(6, "DOLIST");
	int DOLST = CODE(4, as_dolist, as_next, 0, 0);
	HEADER(4, "EXIT");
	int EXITT = CODE(4, as_exit, as_next, 0, 0);
	HEADER(7, "EXECUTE");
	int EXECU = CODE(4, as_execu, as_next, 0, 0);
	HEADER(6, "DONEXT");
	DONXT = CODE(4, as_donext, as_next, 0, 0);
	HEADER(7, "QBRANCH");
	QBRAN = CODE(4, as_qbran, as_next, 0, 0);
	HEADER(6, "BRANCH");
	BRAN = CODE(4, as_bran, as_next, 0, 0);
	HEADER(1, "!");
	int STORE = CODE(4, as_store, as_next, 0, 0);
	HEADER(1, "@");
	int AT = CODE(4, as_at, as_next, 0, 0);
	HEADER(2, "C!");
	int CSTOR = CODE(4, as_cstor, as_next, 0, 0);
	HEADER(2, "C@");
	int CAT = CODE(4, as_cat, as_next, 0, 0);
	HEADER(2, "R>");
	int RFROM = CODE(4, as_rfrom, as_next, 0, 0);
	HEADER(2, "R@");
	int RAT = CODE(4, as_rat, as_next, 0, 0);
	HEADER(2, ">R");
	TOR = CODE(4, as_tor, as_next, 0, 0);
	HEADER(4, "DROP");
	int DROP = CODE(4, as_drop, as_next, 0, 0);
	HEADER(3, "DUP");
	int DUPP = CODE(4, as_dup, as_next, 0, 0);
	HEADER(4, "SWAP");
	int SWAP = CODE(4, as_swap, as_next, 0, 0);
	HEADER(4, "OVER");
	int OVER = CODE(4, as_over, as_next, 0, 0);
	HEADER(2, "0<");
	int ZLESS = CODE(4, as_zless, as_next, 0, 0);
	HEADER(3, "AND");
	int ANDD = CODE(4, as_andd, as_next, 0, 0);
	HEADER(2, "OR");
	int ORR = CODE(4, as_orr, as_next, 0, 0);
	HEADER(3, "XOR");
	int XORR = CODE(4, as_xorr, as_next, 0, 0);
	HEADER(3, "UM+");
	int UPLUS = CODE(4, as_uplus, as_next, 0, 0);
	HEADER(4, "NEXT");
	int NEXTT = CODE(4, as_next, as_next, 0, 0);
	HEADER(4, "?DUP");
	int QDUP = CODE(4, as_qdup, as_next, 0, 0);
	HEADER(3, "ROT");
	int ROT = CODE(4, as_rot, as_next, 0, 0);
	HEADER(5, "2DROP");
	int DDROP = CODE(4, as_ddrop, as_next, 0, 0);
	HEADER(4, "2DUP");
	int DDUP = CODE(4, as_ddup, as_next, 0, 0);
	HEADER(1, "+");
	int PLUS = CODE(4, as_plus, as_next, 0, 0);
	HEADER(3, "NOT");
	int INVER = CODE(4, as_inver, as_next, 0, 0);
	HEADER(6, "NEGATE");
	int NEGAT = CODE(4, as_negat, as_next, 0, 0);
	HEADER(7, "DNEGATE");
	int DNEGA = CODE(4, as_dnega, as_next, 0, 0);
	HEADER(1, "-");
	int SUBBB = CODE(4, as_subb, as_next, 0, 0);
	HEADER(3, "ABS");
	int ABSS = CODE(4, as_abss, as_next, 0, 0);
	HEADER(1, "=");
	int EQUAL = CODE(4, as_equal, as_next, 0, 0);
	HEADER(2, "U<");
	int ULESS = CODE(4, as_uless, as_next, 0, 0);
	HEADER(1, "<");
	int LESS = CODE(4, as_less, as_next, 0, 0);
	HEADER(6, "UM/MOD");
	int UMMOD = CODE(4, as_ummod, as_next, 0, 0);
	HEADER(5, "M/MOD");
	int MSMOD = CODE(4, as_msmod, as_next, 0, 0);
	HEADER(4, "/MOD");
	int SLMOD = CODE(4, as_slmod, as_next, 0, 0);
	HEADER(3, "MOD");
	int MODD = CODE(4, as_mod, as_next, 0, 0);
	HEADER(1, "/");
	int SLASH = CODE(4, as_slash, as_next, 0, 0);
	HEADER(3, "UM*");
	int UMSTA = CODE(4, as_umsta, as_next, 0, 0);
	HEADER(1, "*");
	int STAR = CODE(4, as_star, as_next, 0, 0);
	HEADER(2, "M*");
	int MSTAR = CODE(4, as_mstar, as_next, 0, 0);
	HEADER(5, "*/MOD");
	int SSMOD = CODE(4, as_ssmod, as_next, 0, 0);
	HEADER(2, "*/");
	int STASL = CODE(4, as_stasl, as_next, 0, 0);
	HEADER(4, "PICK");
	int PICK = CODE(4, as_pick, as_next, 0, 0);
	HEADER(2, "+!");
	int PSTOR = CODE(4, as_pstor, as_next, 0, 0);
	HEADER(2, "2!");
	int DSTOR = CODE(4, as_dstor, as_next, 0, 0);
	HEADER(2, "2@");
	int DAT = CODE(4, as_dat, as_next, 0, 0);
	HEADER(5, "COUNT");
	int COUNT = CODE(4, as_count, as_next, 0, 0);
	HEADER(3, "MAX");
	int MAX = CODE(4, as_max, as_next, 0, 0);
	HEADER(3, "MIN");
	int MIN = CODE(4, as_min, as_next, 0, 0);
	HEADER(2, "BL");
	int BLANK = CODE(8, as_docon, as_next, 0, 0, 32, 0, 0, 0);
	HEADER(4, "CELL");
	int CELL = CODE(8, as_docon, as_next, 0, 0, 4, 0, 0, 0);
	HEADER(5, "CELL+");
	int CELLP = CODE(8, as_docon, as_plus, as_next, 0, 4, 0, 0, 0);
	HEADER(5, "CELL-");
	int CELLM = CODE(8, as_docon, as_subb, as_next, 0, 4, 0, 0, 0);
	HEADER(5, "CELLS");
	int CELLS = CODE(8, as_docon, as_star, as_next, 0, 4, 0, 0, 0);
	HEADER(5, "CELL/");
	int CELLD = CODE(8, as_docon, as_slash, as_next, 0, 4, 0, 0, 0);
	HEADER(2, "1+");
	int ONEP = CODE(8, as_docon, as_plus, as_next, 0, 1, 0, 0, 0);
	HEADER(2, "1-");
	int ONEM = CODE(8, as_docon, as_subb, as_next, 0, 1, 0, 0, 0);
	HEADER(5, "DOVAR");
	int DOVAR = CODE(4, as_dovar, as_next, 0, 0);

	// Common Colon Words

	HEADER(4, "?KEY");
	int QKEY = COLON(2, QRX, EXITT);
	HEADER(3, "KEY");
	int KEY = COLON(0);
	BEGIN(1, QKEY);
	UNTIL(1, EXITT);
	HEADER(4, "EMIT");
	int EMIT = COLON(2, TXSTO, EXITT);
	HEADER(6, "WITHIN");
	int WITHI = COLON(7, OVER, SUBBB, TOR, SUBBB, RFROM, ULESS, EXITT);
	HEADER(5, ">CHAR");
	int TCHAR = COLON(8, DOLIT, 0x7F, ANDD, DUPP, DOLIT, 0x7F, BLANK, WITHI);
	IF(3, DROP, DOLIT, 0x5F);
	THEN(1, EXITT);
	HEADER(7, "ALIGNED");
	int ALIGN = COLON(7, DOLIT, 3, PLUS, DOLIT, 0xFFFFFFFC, ANDD, EXITT);
	HEADER(4, "HERE");
	int HERE = COLON(3, CP, AT, EXITT);
	HEADER(3, "PAD");
	int PAD = COLON(5, HERE, DOLIT, 0x50, PLUS, EXITT);
	HEADER(3, "TIB");
	int TIB = COLON(3, TTIB, AT, EXITT);
	HEADER(8, "@EXECUTE");
	int ATEXE = COLON(2, AT, QDUP);
	IF(1, EXECU);
	THEN(1, EXITT);
	HEADER(5, "CMOVE");
	int CMOVEE = COLON(0);
	FOR(0);
	AFT(8, OVER, CAT, OVER, CSTOR, TOR, ONEP, RFROM, ONEP);
	THEN(0);
	NEXT(2, DDROP, EXITT);
	HEADER(4, "MOVE");
	int MOVE = COLON(1, CELLD);
	FOR(0);
	AFT(8, OVER, AT, OVER, STORE, TOR, CELLP, RFROM, CELLP);
	THEN(0);
	NEXT(2, DDROP, EXITT);
	HEADER(4, "FILL");
	int FILL = COLON(1, SWAP);
	FOR(1, SWAP);
	AFT(3, DDUP, CSTOR, ONEP);
	THEN(0);
	NEXT(2, DDROP, EXITT);

	// Number Conversions

	HEADER(5, "DIGIT");
	int DIGIT = COLON(12, DOLIT, 9, OVER, LESS, DOLIT, 7, ANDD, PLUS, DOLIT, 0x30, PLUS, EXITT);
	HEADER(7, "EXTRACT");
	int EXTRC = COLON(7, DOLIT, 0, SWAP, UMMOD, SWAP, DIGIT, EXITT);
	HEADER(2, "<#");
	int BDIGS = COLON(4, PAD, HLD, STORE, EXITT);
	HEADER(4, "HOLD");
	int HOLD = COLON(8, HLD, AT, ONEM, DUPP, HLD, STORE, CSTOR, EXITT);
	HEADER(1, "#");
	int DIG = COLON(5, BASE, AT, EXTRC, HOLD, EXITT);
	HEADER(2, "#S");
	int DIGS = COLON(0);
	BEGIN(2, DIG, DUPP);
	WHILE(0);
	REPEAT(1, EXITT);
	HEADER(4, "SIGN");
	int SIGN = COLON(1, ZLESS);
	IF(3, DOLIT, 0x2D, HOLD);
	THEN(1, EXITT);
	HEADER(2, "#>");
	int EDIGS = COLON(7, DROP, HLD, AT, PAD, OVER, SUBBB, EXITT);
	HEADER(3, "str");
	int STRR = COLON(9, DUPP, TOR, ABSS, BDIGS, DIGS, RFROM, SIGN, EDIGS, EXITT);
	HEADER(3, "HEX");
	int HEXX = COLON(5, DOLIT, 16, BASE, STORE, EXITT);
	HEADER(7, "DECIMAL");
	int DECIM = COLON(5, DOLIT, 10, BASE, STORE, EXITT);
	HEADER(6, "wupper");
	int UPPER = COLON(4, DOLIT, 0x5F5F5F5F, ANDD, EXITT);
	HEADER(6, ">upper");
	int TOUPP = COLON(6, DUPP, DOLIT, 0x61, DOLIT, 0x7B, WITHI);
	IF(3, DOLIT, 0x5F, ANDD);
	THEN(1, EXITT);
	HEADER(6, "DIGIT?");
	int DIGTQ = COLON(9, TOR, TOUPP, DOLIT, 0x30, SUBBB, DOLIT, 9, OVER, LESS);
	IF(8, DOLIT, 7, SUBBB, DUPP, DOLIT, 10, LESS, ORR);
	THEN(4, DUPP, RFROM, ULESS, EXITT);
	HEADER(7, "NUMBER?");
	int NUMBQ = COLON(12, BASE, AT, TOR, DOLIT, 0, OVER, COUNT, OVER, CAT, DOLIT, 0x24, EQUAL);
	IF(5, HEXX, SWAP, ONEP, SWAP, ONEM);
	THEN(13, OVER, CAT, DOLIT, 0x2D, EQUAL, TOR, SWAP, RAT, SUBBB, SWAP, RAT, PLUS, QDUP);
	IF(1, ONEM);
	FOR(6, DUPP, TOR, CAT, BASE, AT, DIGTQ);
	WHILE(7, SWAP, BASE, AT, STAR, PLUS, RFROM, ONEP);
	NEXT(2, DROP, RAT);
	IF(1, NEGAT);
	THEN(1, SWAP);
	ELSE(6, RFROM, RFROM, DDROP, DDROP, DOLIT, 0);
	THEN(1, DUPP);
	THEN(6, RFROM, DDROP, RFROM, BASE, STORE, EXITT);

	// Terminal Output

	HEADER(5, "SPACE");
	int SPACE = COLON(3, BLANK, EMIT, EXITT);
	HEADER(5, "CHARS");
	int CHARS = COLON(4, SWAP, DOLIT, 0, MAX);
	FOR(0);
	AFT(2, DUPP, EMIT);
	THEN(0);
	NEXT(2, DROP, EXITT);
	HEADER(6, "SPACES");
	int SPACS = COLON(3, BLANK, CHARS, EXITT);
	HEADER(4, "TYPE");
	int TYPES = COLON(0);
	FOR(0);
	AFT(3, COUNT, TCHAR, EMIT);
	THEN(0);
	NEXT(2, DROP, EXITT);
	HEADER(2, "CR");
	int CR = COLON(7, DOLIT, 10, DOLIT, 13, EMIT, EMIT, EXITT);
	HEADER(3, "do$");
	int DOSTR = COLON(10, RFROM, RAT, RFROM, COUNT, PLUS, ALIGN, TOR, SWAP, TOR, EXITT);
	HEADER(3, "$\"|");
	int STRQP = COLON(2, DOSTR, EXITT);
	HEADER(3, ".\"|");
	DOTQP = COLON(4, DOSTR, COUNT, TYPES, EXITT);
	HEADER(2, ".R");
	int DOTR = COLON(8, TOR, STRR, RFROM, OVER, SUBBB, SPACS, TYPES, EXITT);
	HEADER(3, "U.R");
	int UDOTR = COLON(10, TOR, BDIGS, DIGS, EDIGS, RFROM, OVER, SUBBB, SPACS, TYPES, EXITT);
	HEADER(2, "U.");
	int UDOT = COLON(6, BDIGS, DIGS, EDIGS, SPACE, TYPES, EXITT);
	HEADER(1, ".");
	int DOT = COLON(5, BASE, AT, DOLIT, 0xA, XORR);
	IF(2, UDOT, EXITT);
	THEN(4, STRR, SPACE, TYPES, EXITT);
	HEADER(1, "?");
	int QUEST = COLON(3, AT, DOT, EXITT);

	// Parser

	HEADER(7, "(parse)");
	int PARS = COLON(5, TEMP, CSTOR, OVER, TOR, DUPP);
	IF(5, ONEM, TEMP, CAT, BLANK, EQUAL);
	IF(0);
	FOR(6, BLANK, OVER, CAT, SUBBB, ZLESS, INVER);
	WHILE(1, ONEP);
	NEXT(6, RFROM, DROP, DOLIT, 0, DUPP, EXITT);
	THEN(1, RFROM);
	THEN(2, OVER, SWAP);
	FOR(9, TEMP, CAT, OVER, CAT, SUBBB, TEMP, CAT, BLANK, EQUAL);
	IF(1, ZLESS);
	THEN(0);
	WHILE(1, ONEP);
	NEXT(2, DUPP, TOR);
	ELSE(5, RFROM, DROP, DUPP, ONEP, TOR);
	THEN(6, OVER, SUBBB, RFROM, RFROM, SUBBB, EXITT);
	THEN(4, OVER, RFROM, SUBBB, EXITT);
	HEADER(5, "PACK$");
	int PACKS = COLON(18, DUPP, TOR, DDUP, PLUS, DOLIT, 0xFFFFFFFC, ANDD, DOLIT, 0, SWAP, STORE, DDUP, CSTOR, ONEP, SWAP, CMOVEE, RFROM, EXITT);
	HEADER(5, "PARSE");
	int PARSE = COLON(15, TOR, TIB, INN, AT, PLUS, NTIB, AT, INN, AT, SUBBB, RFROM, PARS, INN, PSTOR, EXITT);
	HEADER(5, "TOKEN");
	int TOKEN = COLON(9, BLANK, PARSE, DOLIT, 0x1F, MIN, HERE, CELLP, PACKS, EXITT);
	HEADER(4, "WORD");
	int WORDD = COLON(5, PARSE, HERE, CELLP, PACKS, EXITT);
	HEADER(5, "NAME>");
	int NAMET = COLON(7, COUNT, DOLIT, 0x1F, ANDD, PLUS, ALIGN, EXITT);
	HEADER(5, "SAME?");
	int SAMEQ = COLON(4, DOLIT, 0x1F, ANDD, CELLD);
	FOR(0);
	AFT(14, OVER, RAT, CELLS, PLUS, AT, UPPER, OVER, RAT, CELLS, PLUS, AT, UPPER, SUBBB, QDUP);
	IF(3, RFROM, DROP, EXITT);
	THEN(0);
	THEN(0);
	NEXT(3, DOLIT, 0, EXITT);
	HEADER(4, "find");
	int FIND = COLON(10, SWAP, DUPP, AT, TEMP, STORE, DUPP, AT, TOR, CELLP, SWAP);
	BEGIN(2, AT, DUPP);
	IF(9, DUPP, AT, DOLIT, 0xFFFFFF3F, ANDD, UPPER, RAT, UPPER, XORR);
	IF(3, CELLP, DOLIT, 0xFFFFFFFF);
	ELSE(4, CELLP, TEMP, AT, SAMEQ);
	THEN(0);
	ELSE(6, RFROM, DROP, SWAP, CELLM, SWAP, EXITT);
	THEN(0);
	WHILE(2, CELLM, CELLM);
	REPEAT(9, RFROM, DROP, SWAP, DROP, CELLM, DUPP, NAMET, SWAP, EXITT);
	HEADER(5, "NAME?");
	int NAMEQ = COLON(3, CNTXT, FIND, EXITT);

	// Terminal Input

	HEADER(2, "^H");
	int HATH = COLON(6, TOR, OVER, RFROM, SWAP, OVER, XORR);
	IF(9, DOLIT, 8, EMIT, ONEM, BLANK, EMIT, DOLIT, 8, EMIT);
	THEN(1, EXITT);
	HEADER(3, "TAP");
	int TAP = COLON(6, DUPP, EMIT, OVER, CSTOR, ONEP, EXITT);
	HEADER(4, "kTAP");
	int KTAP = COLON(9, DUPP, DOLIT, 0xD, XORR, OVER, DOLIT, 0xA, XORR, ANDD);
	IF(3, DOLIT, 8, XORR);
	IF(2, BLANK, TAP);
	ELSE(1, HATH);
	THEN(1, EXITT);
	THEN(5, DROP, SWAP, DROP, DUPP, EXITT);
	HEADER(6, "ACCEPT");
	int ACCEP = COLON(3, OVER, PLUS, OVER);
	BEGIN(2, DDUP, XORR);
	WHILE(7, KEY, DUPP, BLANK, SUBBB, DOLIT, 0x5F, ULESS);
	IF(1, TAP);
	ELSE(1, KTAP);
	THEN(0);
	REPEAT(4, DROP, OVER, SUBBB, EXITT);
	HEADER(6, "EXPECT");
	int EXPEC = COLON(5, ACCEP, SPAN, STORE, DROP, EXITT);
	HEADER(5, "QUERY");
	int QUERY = COLON(12, TIB, DOLIT, 0x50, ACCEP, NTIB, STORE, DROP, DOLIT, 0, INN, STORE, EXITT);

	// Text Interpreter

	HEADER(5, "ABORT");
	int ABORT = COLON(2, TABRT, ATEXE);
	HEADER(6, "abort\"");
	ABORQP = COLON(0);
	IF(4, DOSTR, COUNT, TYPES, ABORT);
	THEN(3, DOSTR, DROP, EXITT);
	HEADER(5, "ERROR");
	int ERRORR = COLON(11, SPACE, COUNT, TYPES, DOLIT, 0x3F, EMIT, DOLIT, 0x1B, EMIT, CR, ABORT);
	HEADER(10, "$INTERPRET");
	int INTER = COLON(2, NAMEQ, QDUP);
	IF(4, CAT, DOLIT, COMPO, ANDD);
	ABORQ(" compile only");
	int INTER0 = LABEL(2, EXECU, EXITT);
	THEN(1, NUMBQ);
	IF(1, EXITT);
	ELSE(1, ERRORR);
	THEN(0);
	HEADER(IMEDD + 1, "[");
	int LBRAC = COLON(5, DOLIT, INTER, TEVAL, STORE, EXITT);
	HEADER(3, ".OK");
	int DOTOK = COLON(6, CR, DOLIT, INTER, TEVAL, AT, EQUAL);
	IF(14, TOR, TOR, TOR, DUPP, DOT, RFROM, DUPP, DOT, RFROM, DUPP, DOT, RFROM, DUPP, DOT);
	DOTQ(" ok>");
	THEN(1, EXITT);
	HEADER(4, "EVAL");
	int EVAL = COLON(0);
	BEGIN(3, TOKEN, DUPP, AT);
	WHILE(2, TEVAL, ATEXE);
	REPEAT(3, DROP, DOTOK, EXITT);
	HEADER(4, "QUIT");
	int QUITT = COLON(5, DOLIT, 0x100, TTIB, STORE, LBRAC);
	BEGIN(2, QUERY, EVAL);
	AGAIN(0);

	// Colon Word Compiler

	HEADER(1, ",");
	int COMMA = COLON(7, HERE, DUPP, CELLP, CP, STORE, STORE, EXITT);
	HEADER(IMEDD + 7, "LITERAL");
	int LITER = COLON(5, DOLIT, DOLIT, COMMA, COMMA, EXITT);
	HEADER(5, "ALLOT");
	int ALLOT = COLON(4, ALIGN, CP, PSTOR, EXITT);
	HEADER(3, "$,\"");
	int STRCQ = COLON(9, DOLIT, 0x22, WORDD, COUNT, PLUS, ALIGN, CP, STORE, EXITT);
	HEADER(7, "?UNIQUE");
	int UNIQU = COLON(3, DUPP, NAMEQ, QDUP);
	IF(6, COUNT, DOLIT, 0x1F, ANDD, SPACE, TYPES);
	DOTQ(" reDef");
	THEN(2, DROP, EXITT);
	HEADER(3, "$,n");
	int SNAME = COLON(2, DUPP, AT);
	IF(14, UNIQU, DUPP, NAMET, CP, STORE, DUPP, LAST, STORE, CELLM, CNTXT, AT, SWAP, STORE, EXITT);
	THEN(1, ERRORR);
	HEADER(1, "'");
	int TICK = COLON(2, TOKEN, NAMEQ);
	IF(1, EXITT);
	THEN(1, ERRORR);
	HEADER(IMEDD + 9, "[COMPILE]");
	int BCOMP = COLON(3, TICK, COMMA, EXITT);
	HEADER(7, "COMPILE");
	int COMPI = COLON(7, RFROM, DUPP, AT, COMMA, CELLP, TOR, EXITT);
	HEADER(8, "$COMPILE");
	int SCOMP = COLON(2, NAMEQ, QDUP);
	IF(4, AT, DOLIT, IMEDD, ANDD);
	IF(1, EXECU);
	ELSE(1, COMMA);
	THEN(1, EXITT);
	THEN(1, NUMBQ);
	IF(2, LITER, EXITT);
	THEN(1, ERRORR);
	HEADER(5, "OVERT");
	int OVERT = COLON(5, LAST, AT, CNTXT, STORE, EXITT);
	HEADER(1, "]");
	int RBRAC = COLON(5, DOLIT, SCOMP, TEVAL, STORE, EXITT);
	HEADER(1, ":");
	int COLN = COLON(7, TOKEN, SNAME, RBRAC, DOLIT, 0x6, COMMA, EXITT);
	HEADER(IMEDD + 1, ";");
	int SEMIS = COLON(6, DOLIT, EXITT, COMMA, LBRAC, OVERT, EXITT);

	// Debugging Tools

	HEADER(3, "dm+");
	int DMP = COLON(4, OVER, DOLIT, 6, UDOTR);
	FOR(0);
	AFT(6, DUPP, AT, DOLIT, 9, UDOTR, CELLP);
	THEN(0);
	NEXT(1, EXITT);
	HEADER(4, "DUMP");
	int DUMP = COLON(10, BASE, AT, TOR, HEXX, DOLIT, 0x1F, PLUS, DOLIT, 0x20, SLASH);
	FOR(0);
	AFT(10, CR, DOLIT, 8, DDUP, DMP, TOR, SPACE, CELLS, TYPES, RFROM);
	THEN(0);
	NEXT(5, DROP, RFROM, BASE, STORE, EXITT);
	HEADER(5, ">NAME");
	int TNAME = COLON(1, CNTXT);
	BEGIN(2, AT, DUPP);
	WHILE(3, DDUP, NAMET, XORR);
	IF(1, ONEM);
	ELSE(3, SWAP, DROP, EXITT);
	THEN(0);
	REPEAT(3, SWAP, DROP, EXITT);
	HEADER(3, ".ID");
	int DOTID = COLON(7, COUNT, DOLIT, 0x1F, ANDD, TYPES, SPACE, EXITT);
	HEADER(5, "WORDS");
	int WORDS = COLON(6, CR, CNTXT, DOLIT, 0, TEMP, STORE);
	BEGIN(2, AT, QDUP);
	WHILE(9, DUPP, SPACE, DOTID, CELLM, TEMP, AT, DOLIT, 0xA, LESS);
	IF(4, DOLIT, 1, TEMP, PSTOR);
	ELSE(5, CR, DOLIT, 0, TEMP, STORE);
	THEN(0);
	REPEAT(1, EXITT);
	HEADER(6, "FORGET");
	int FORGT = COLON(3, TOKEN, NAMEQ, QDUP);
	IF(12, CELLM, DUPP, CP, STORE, AT, DUPP, CNTXT, STORE, LAST, STORE, DROP, EXITT);
	THEN(1, ERRORR);
	HEADER(4, "COLD");
	int COLD = COLON(1, CR);
	DOTQ("eForth in C,Ver 2.3,2017 ");
	int DOTQ1 = LABEL(2, CR, QUITT);

	// Structure Compiler

	HEADER(IMEDD + 4, "THEN");
	int THENN = COLON(4, HERE, SWAP, STORE, EXITT);
	HEADER(IMEDD + 3, "FOR");
	int FORR = COLON(4, COMPI, TOR, HERE, EXITT);
	HEADER(IMEDD + 5, "BEGIN");
	int BEGIN = COLON(2, HERE, EXITT);
	HEADER(IMEDD + 4, "NEXT");
	int NEXT = COLON(4, COMPI, DONXT, COMMA, EXITT);
	HEADER(IMEDD + 5, "UNTIL");
	int UNTIL = COLON(4, COMPI, QBRAN, COMMA, EXITT);
	HEADER(IMEDD + 5, "AGAIN");
	int AGAIN = COLON(4, COMPI, BRAN, COMMA, EXITT);
	HEADER(IMEDD + 2, "IF");
	int IFF = COLON(7, COMPI, QBRAN, HERE, DOLIT, 0, COMMA, EXITT);
	HEADER(IMEDD + 5, "AHEAD");
	int AHEAD = COLON(7, COMPI, BRAN, HERE, DOLIT, 0, COMMA, EXITT);
	HEADER(IMEDD + 6, "REPEAT");
	int REPEA = COLON(3, AGAIN, THENN, EXITT);
	HEADER(IMEDD + 3, "AFT");
	int AFT = COLON(5, DROP, AHEAD, HERE, SWAP, EXITT);
	HEADER(IMEDD + 4, "ELSE");
	int ELSEE = COLON(4, AHEAD, SWAP, THENN, EXITT);
	HEADER(IMEDD + 4, "WHEN");
	int WHEN = COLON(3, IFF, OVER, EXITT);
	HEADER(IMEDD + 5, "WHILE");
	int WHILEE = COLON(3, IFF, SWAP, EXITT);
	HEADER(IMEDD + 6, "ABORT\"");
	int ABRTQ = COLON(6, DOLIT, ABORQP, HERE, STORE, STRCQ, EXITT);
	HEADER(IMEDD + 2, "$\"");
	int STRQ = COLON(6, DOLIT, STRQP, HERE, STORE, STRCQ, EXITT);
	HEADER(IMEDD + 2, ".\"");
	int DOTQQ = COLON(6, DOLIT, DOTQP, HERE, STORE, STRCQ, EXITT);
	HEADER(4, "CODE");
	int CODE = COLON(4, TOKEN, SNAME, OVERT, EXITT);
	HEADER(6, "CREATE");
	int CREAT = COLON(5, CODE, DOLIT, 0x203D, COMMA, EXITT);
	HEADER(8, "VARIABLE");
	int VARIA = COLON(5, CREAT, DOLIT, 0, COMMA, EXITT);
	HEADER(8, "CONSTANT");
	int CONST = COLON(6, CODE, DOLIT, 0x2004, COMMA, COMMA, EXITT);
	HEADER(IMEDD + 2, ".(");
	int DOTPR = COLON(5, DOLIT, 0x29, PARSE, TYPES, EXITT);
	HEADER(IMEDD + 1, "\\");
	int BKSLA = COLON(5, DOLIT, 0xA, WORDD, DROP, EXITT);
	HEADER(IMEDD + 1, "(");
	int PAREN = COLON(5, DOLIT, 0x29, PARSE, DDROP, EXITT);
	HEADER(12, "COMPILE-ONLY");
	int ONLY = COLON(6, DOLIT, 0x40, LAST, AT, PSTOR, EXITT);
	HEADER(9, "IMMEDIATE");
	int IMMED = COLON(6, DOLIT, 0x80, LAST, AT, PSTOR, EXITT);
	int ENDD = P;

	// Boot Up

	printf("\n\nIZ=%X R-stack=%X", P, (popR << 2));
	P = 0;
	int RESET = LABEL(2, 6, COLD);
	P = 0x90;
	int USER = LABEL(8, 0x100, 0x10, IMMED - 12, ENDD, IMMED - 12, INTER, QUITT, 0);

	P = 0;
	WP = 4;
	IP = 0;
	S = 0;
	R = 0;
	top = 0;
	printf("\nceForth v3.3, 01jul19cht\n");
	for (;;) {
		primitives[cData[P++]]();
	}
}
