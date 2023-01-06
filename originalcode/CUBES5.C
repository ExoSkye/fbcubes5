//CUBES5 by Ken Silverman (http://advsys.net/ken)

#include <dos.h>
#include <malloc.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pragmas.h"

#define MAXXDIM 1600L
#define MAXYDIM 1200L
#define BSIZ 16
#define WALLDIST 128
#define MAXPOINTS 4096
#define MAXMESHS 1024
#define MAXSPLITCNT 1024
#define MAXPALOOKUPS 32
#define MAXTILES 4096
#define MAXTILEFILES 256
#define NUMOPTIONS 8
#define NUMKEYS 19
#define MAXSECTORS ((BSIZ*BSIZ*BSIZ)>>2)

#define AVERAGEFRAMES 32
static long frameval[AVERAGEFRAMES], numframes = 0;

static float rxx, rxy, rxz, ryx, ryy, ryz, rzx, rzy, rzz;       //rotation

#define rotate3d(nx,ny,nz)                                       \
{                                                                \
	float rotate3dox = nx, rotate3doy = ny, rotate3doz = nz;      \
	nx = rotate3dox*rxx + rotate3doy*ryx + rotate3doz*rzx;        \
	ny = rotate3dox*rxy + rotate3doy*ryy + rotate3doz*rzy;        \
	nz = rotate3dox*rxz + rotate3doy*ryz + rotate3doz*rzz;        \
}

#define backrotate3d(nx,ny,nz)                                   \
{                                                                \
	float rotate3dox = nx, rotate3doy = ny, rotate3doz = nz;      \
	nx = rotate3dox*rxx + rotate3doy*rxy + rotate3doz*rxz;        \
	ny = rotate3dox*ryx + rotate3doy*ryy + rotate3doz*ryz;        \
	nz = rotate3dox*rzx + rotate3doy*rzy + rotate3doz*rzz;        \
}

typedef struct { long x, y; } point2d;
typedef struct { float x, y, z; } point3d;
typedef struct { short p1, p2, a1, a2; } edgetype;

static short board[6*BSIZ*BSIZ*BSIZ];    //Board layout

static short mousx, mousy, bstatus, obstatus, moustat;
static long searchx, searchy;

	//positions/angles and their velocities
static long posx, posy, posz, a1, a2, a3;
static long fvel, svel, hvel, a1vel, a2vel, a3vel;

	//New polygon variables
static point3d r[MAXPOINTS];
static point2d sc[MAXPOINTS];
static short plist[MAXPOINTS], elist[MAXPOINTS], ecnt;
static char got[MAXPOINTS];

static short mesh[MAXMESHS], nummeshs, numedges, numpoints, endopoints;
static short meshtag[MAXMESHS], edgetag[MAXPOINTS];
static float tval[MAXPOINTS];
static edgetype edge[MAXPOINTS];

static short split1[MAXSPLITCNT], splitag1[MAXSPLITCNT];
static short split2[MAXSPLITCNT], splitag2[MAXSPLITCNT];

	//Edge reduction variables (poly-union)
static long ssx[MAXPOINTS], ssy[MAXPOINTS];
static short ltag[MAXPOINTS];

	//Screen BSP variables
static point3d bn[MAXPOINTS];
static short front[MAXPOINTS], back[MAXPOINTS], pt[MAXPOINTS], pnum;

static short tempshort[4096]; //4096 = max(BSIZ*BSIZ*BSIZ,MAXTILES)
static short numsectors;
static short cubesect[BSIZ*BSIZ*BSIZ];
static short sectx1[MAXSECTORS], secty1[MAXSECTORS], sectz1[MAXSECTORS];
static short sectx2[MAXSECTORS], secty2[MAXSECTORS], sectz2[MAXSECTORS];

	//Cube definition variables
long coordofpoint[3][8] = {0,1,0,1,0,1,0,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,1,1};
short pointofface[6][4] = {0,4,6,2,5,1,3,7,0,1,5,4,6,7,3,2,1,0,2,3,4,5,7,6};

	//Region definition variables
static short facetag[BSIZ*BSIZ*6];
static short facesid[BSIZ*BSIZ*6];

static short sectlist[MAXSECTORS];
static short bspstart[MAXSECTORS];
static short gotsect[MAXSECTORS];

static long globalregioncnt, globalsplitcnt;

	//Diagonal filling variables
long hplc[MAXXDIM+1], ui[8], vi[8], pinc[4];
static short h1[(MAXXDIM<<1)+4], h2[(MAXXDIM<<1)+4];
typedef struct { short n, p; } diagdat;
static diagdat p1[MAXXDIM<<5], p2[MAXXDIM<<5];
char hind[MAXXDIM+1];

	//texturizing
static float gm, gd, gdx, gdy, gu, gux, guy, gv, gvx, gvy, gt;
static float gxoffs, gyoffs;
static long lgm, lgd, lgdx, lgdy, lgu, lgux, lguy, lgv, lgvx, lgvy, lgt;
static long *gmp, *gdp, *gdxp, *gdyp;
static long *gup, *guxp, *guyp, *gvp, *gvxp, *gvyp, *gtp;
static long *gxoffsp, *gyoffsp;

static char *gbuf;
static long gsect, gcube, gface, gtext, gmip, gshade, gmask1, gmask2;
static long asm[2];

static char *screen, drawmode = 0;
static long frameplace, ylookup[MAXYDIM], zoom;
static long xdim, ydim, xdimup15, ydimup15, visualpage = 0;
static float xreszoom, ydimscale;

static long chainxres[4] = {256,320,360,400};
static long chainyres[11] = {200,240,256,270,300,350,360,400,480,512,540};
static long vesares[7][2] = {320,200,640,400,640,480,800,600,1024,768,
									  1280,1024,1600,1200};

static char vesapageshift, curpag;

	//Palette variables
static char paletteloaded = 0, translucloaded = 0;
static short numpalookups;
static char palette[768], whitecol, blackcol;
static char palookup[MAXPALOOKUPS][256], *transluc;
static long colookup[256][256];

	//Tiles???.ART variables
static char *pic, picsiz[MAXTILES];        //pic-pointer to BIG tile buffer
static long numtiles, waloff[4][MAXTILES], picanm[MAXTILES];  //waloff-offset
static long tilesizx[MAXTILES], tilesizy[MAXTILES];    //dimensions of tile
	//Internal art variables
static char artfilename[20];
static long artsize = 0, artversion, picsloaded = 0;
static long artfil, artfilnum, artfilplc, numtilefiles;
static char tilefilenum[MAXTILES];
static long tilefileoffs[MAXTILES];

	//Temp variables - Save memory by using these
static char tempbuf[4096];

	//Editor variables (tab&enter)
char boardfilename[13];
long temppicnum = -1, tempshade = -1, dragpoint = -1;
short searchcube, searchcubak, searchedge;
long searchitx, searchity, searchitz;

static char palette[768];

unsigned short sqrtable[2048];
long reciptable[2048], fpuasm;

static short sintable[2048];
static float fsintable[2048];
static char britable[16][64];
static char textfont[1024], smalltextfont[1024];
char option[NUMOPTIONS], keys[NUMKEYS];
long randomseed;

long totalclock, baktotalclock, lockspeed;
static void (__interrupt __far *oldtimerhandler)();
static void __interrupt __far timerhandler(void);

volatile char keystatus[256], readch, oldreadch, extended, keytemp;
static void (__interrupt __far *oldkeyhandler)();
static void __interrupt __far keyhandler(void);

extern long setupdline(long,long,long);
#pragma aux setupdline parm [eax][ebx][ecx];
extern long dline(long,long,long,long,long,long);
#pragma aux dline parm [eax][ebx][ecx][edx][esi][edi];
extern long setupdline24(long,long,long);
#pragma aux setupdline24 parm [eax][ebx][ecx];
extern long dline24(long,long,long,long,long,long);
#pragma aux dline24 parm [eax][ebx][ecx][edx][esi][edi];

extern void initcache(long dacachestart, long dacachesize);
extern void cachebox(long *cacheptr, long xsiz, long ysiz);

void splitmesh1(long splitval, long meshnum, float nx, float ny, float nz);
void splitmesh2(long splitval, long meshnum, float nx, float ny, float nz);
void draw3dclipline(float fx0, float fy0, float fz0, float fx1, float fy1, float fz1, char col);
long clipline(float *x1, float *y1, float *z1, float *x2, float *y2, float *z2);
long loadpics(char *filename);
long animateoffs(long tilenum);
long setvesamode(short vesamode);
long gettile(short dapicnum);
long initmouse();
void getmousevalues(short *mousx, short *mousy, short *bstatus);
long rayscan(long *x, long *y, long *z, long *xv, long *yv, long *zv);

long ksqrt(long);
#pragma aux ksqrt =\
	"mov asm, eax",\
	"fild dword ptr asm",\
	"fstp dword ptr asm",\
	"mov bl, byte ptr asm[3]",\
	"sub bl, 63+5",\
	"jge over2048",\
	"xor ebx, ebx",\
	"mov bx, sqrtable[eax*2]",\
	"jmp endksqrtasm",\
	"over2048: lea ecx, [ebx*2]",\
	"shr eax, cl",\
	"mov cl, bl",\
	"xor ebx, ebx",\
	"mov bx, sqrtable[eax*2]",\
	"shl ebx, cl",\
	"endksqrtasm: shr ebx, 10",\
	parm [eax]\
	value [ebx]\
	modify exact [eax ebx ecx]\

	//0x007ff000 is (11<<13), 0x3f800000 is (127<<23)
long krecipasm(long);
#pragma aux krecipasm =\
	"mov fpuasm, eax",\
	"fild dword ptr fpuasm",\
	"add eax, eax",\
	"fstp dword ptr fpuasm",\
	"sbb ebx, ebx",\
	"mov eax, fpuasm",\
	"mov ecx, eax",\
	"and eax, 0x007ff000",\
	"shr eax, 10",\
	"sub ecx, 0x3f800000",\
	"shr ecx, 23",\
	"mov eax, dword ptr reciptable[eax]",\
	"sar eax, cl",\
	"xor eax, ebx",\
	parm [eax]\
	modify exact [eax ebx ecx]\

long vesasetmode(long);
#pragma aux vesasetmode =\
	"mov ax, 0x4f02",\
	"int 0x10",\
	"and eax, 0x0000ffff",\
	parm [ebx]\
	modify [eax ebx]\

void vesasetpage(long);
#pragma aux vesasetpage =\
	"mov cl, vesapageshift",\
	"shl dx, cl",\
	"cmp dl, curpag",\
	"je skipsetpage",\
	"mov curpag, dl",\
	"mov ax, 0x4f05",\
	"xor bx, bx",\
	"int 0x10",\
	"skipsetpage:",\
	parm [edx]\
	modify [eax ebx ecx]\

void truecolorblit(void *,void *,long);
#pragma aux truecolorblit =\
	"begin: mov eax, [esi]",\
	"mov ebx, [esi+4]",\
	"mov [edi], eax",\
	"mov [edi+4], eax",\
	"add esi, 8",\
	"mov [edi+8], ebx",\
	"mov [edi+12], ebx",\
	"add edi, 16",\
	"dec ecx",\
	"jnz begin",\
	parm [esi][edi][ecx]\
	modify exact [eax ebx ecx esi edi]\

//ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
//³ Precision: bits 8-9: ³ Rounding: bits 10-11: ³
//ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
//³ 00 = 24-bit      (0) ³ 00 = nearest/even (0) ³
//³ 01 = reserved    (1) ³ 01 = -ì           (4) ³
//³ 10 = 53-bit      (2) ³ 10 = ì            (8) ³
//³ 11 = 64-bit      (3) ³ 11 = 0            (c) ³
//ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

void fpuinit(long);
#pragma aux fpuinit =\
	"fninit",\
	"fstcw asm",\
	"mov ah, byte ptr asm[1]",\
	"and ah, 0f0h",\
	"or ah, al",\
	"mov byte ptr asm[1], ah",\
	"fldcw asm",\
	parm [eax]\

void ftol(float *,long *);
#pragma aux ftol =\
	"fld dword ptr [eax]",\
	"fistp dword ptr [ebx]",\
	parm [eax][ebx]\

void fpustartdiv(float *,float *);
#pragma aux fpustartdiv =\
	"fld dword ptr [eax]",\
	"fdiv dword ptr [ebx]",\
	parm [eax][ebx]\
	modify exact \

void fpugetnum(float *);
#pragma aux fpugetnum =\
	"fstp dword ptr [eax]",\
	parm [eax]\
	modify exact \

void __interrupt __far timerhandler()
{
	totalclock++;
	keytimerstuff();

	_chain_intr(oldtimerhandler);
	koutp(0x20,0x20);
}

void keytimerstuff ()
{
		//Acceleration
	if (keystatus[keys[0]]) fvel = min(fvel+8,127);
	if (keystatus[keys[1]]) fvel = max(fvel-8,-128);
	if (keystatus[0x2c]) hvel = min(hvel+8,127);
	if (keystatus[0x1e]) hvel = max(hvel-8,-128);
	if (keystatus[keys[5]])
	{
		if (keystatus[keys[3]]) svel = min(svel+8,127);
		if (keystatus[keys[2]]) svel = max(svel-8,-128);
	}
	else
	{
		if (keystatus[keys[3]]) a3vel = min(a3vel+16,127);
		if (keystatus[keys[2]]) a3vel = max(a3vel-16,-128);
	}
	if (keystatus[0x33]) a1vel = min(a1vel+32,127);
	if (keystatus[0x34]) a1vel = max(a1vel-32,-128);
	if (keystatus[keys[11]]) a2vel = min(a2vel+16,127);
	if (keystatus[keys[10]]) a2vel = max(a2vel-16,-128);

		//Friction
	if (fvel < 0) fvel = min(fvel+2,0);
	if (fvel > 0) fvel = max(fvel-2,0);
	if (svel < 0) svel = min(svel+2,0);
	if (svel > 0) svel = max(svel-2,0);
	if (hvel < 0) hvel = min(hvel+2,0);
	if (hvel > 0) hvel = max(hvel-2,0);
	if (a1vel < 0) a1vel = min(a1vel+12,0);
	if (a1vel > 0) a1vel = max(a1vel-12,0);
	if (a2vel < 0) a2vel = min(a2vel+12,0);
	if (a2vel > 0) a2vel = max(a2vel-12,0);
	if (a3vel < 0) a3vel = min(a3vel+12,0);
	if (a3vel > 0) a3vel = max(a3vel-12,0);
}

void __interrupt __far keyhandler()
{
	oldreadch = readch; readch = kinp(0x60);
	keytemp = kinp(0x61); koutp(0x61,keytemp|128); koutp(0x61,keytemp&127);
	if ((readch|1) == 0xe1)
		extended = 128;
	else
	{
		if (oldreadch != readch)
			keystatus[(readch&127)+extended] = ((readch>>7)^1);
		extended = 0;
	}
	koutp(0x20,0x20);
	if (keystatus[0x46])
	{
		koutp(0x43,54); koutp(0x40,255); koutp(0x40,255);
		_dos_setvect(0x9, oldkeyhandler);
		_dos_setvect(0x8, oldtimerhandler);
		free(screen);
		setvmode(0x3);
		exit(0);
	}
}

int main (int argc, char **argv)
{
	long i, j, x, y, z, *fxp, *fyp, *fzp;
	float fx, fy, fz, ftemp;
	char shiftstate;

	strcpy(&boardfilename,"board");
	if (argc >= 2) strcpy(&boardfilename,argv[1]);
	if(strchr(boardfilename,'.') == 0) strcat(boardfilename,".cub");

	fpuinit(0x03);  //64 bit, round to nearest/even

	loadtables();

	xdim = 320; ydim = 200;
	if (option[0] == 0)
	{
		if ((chainxres[option[6]&15] > 320) || (chainyres[option[6]>>4] > 200))
			{ xdim = 640; ydim = 480; }
	}
	if ((option[0] == 1) || (option[0] == 2))
	{
		xdim = vesares[option[6]&15][0];
		ydim = vesares[option[6]&15][1];
	}
	xdimup15 = ((xdim+1)<<15);
	ydimup15 = ((ydim+1)<<15);
	searchx = (xdim>>1);
	searchy = (ydim>>1);

	if (option[3] == 1) initmouse();
	loadpics("tiles000.art");
	loadpalette();

	switch(option[0])
	{
		case 0:
			if ((screen = (char *)malloc(xdim*ydim*4)) == NULL)
			{
				setvmode(0x3);
				printf("Not enough memory for screen buffer\n");
				exit(0);
			}
			frameplace = FP_OFF(screen);

			if (setvesamode(0x112) < 0)
			{
				setvmode(0x3);
				printf("640 * 480 VESA true color mode not supported\n");
				exit(0);
			}

			if ((xdim == 320) && (ydim == 200))
			{
				koutp(0x3d4,0x11); koutp(0x3d5,kinp(0x3d5)&0x7f);

				koutpw(0x3c4,0x0100);
				koutp(0x3c2,(kinp(0x3cc)&0x3f)|0x40);
				koutpw(0x3c4,0x0300);
				koutpw(0x3d4,0xbf06);
				koutpw(0x3d4,0x1f07);
				koutpw(0x3d4,0x4109);
				koutpw(0x3d4,0x9c10);
				koutpw(0x3d4,0x8e11);
				koutpw(0x3d4,0x8f12);
				koutpw(0x3d4,0x9615);
				koutpw(0x3d4,0xb916);
			}
			break;
		case 1: case 2:
			if ((screen = (char *)malloc(xdim*ydim)) == NULL)
			{
				setvmode(0x3);
				printf("Not enough memory for screen buffer\n");
				exit(0);
			}
			frameplace = FP_OFF(screen);

			i = -1;
			if ((xdim == 320) && (ydim == 200)) setvmode(0x13); i = 1;
			if ((xdim == 640) && (ydim == 400)) i = setvesamode(0x100);
			if ((xdim == 640) && (ydim == 480)) i = setvesamode(0x101);
			if ((xdim == 800) && (ydim == 600)) i = setvesamode(0x103);
			if ((xdim == 1024) && (ydim == 768)) i = setvesamode(0x105);
			if ((xdim == 1280) && (ydim == 1024)) i = setvesamode(0x107);
			if ((xdim == 1600) && (ydim == 1200)) i = setvesamode(0x120);
			if (i < 0)
			{
				setvmode(0x3);
				printf("%ld * %ld Vesa mode not supported\n",xdim,ydim);
				exit(0);
			}
			break;
		case 3:
			setvmode(0x13);
			frameplace = 0xa0000;
			visualpage = 0; koutp(0x3cd,visualpage+(visualpage<<4));
			break;
		case 4:
			setvmode(0x13);
			koutp(0x3ce,0xf); koutp(0x3cf,0x5);
			koutp(0x3d4,0x29); koutp(0x3d5,0x85);
			koutp(0x3c4,0x6); koutp(0x3c5,0x48);
			koutp(0x3d4,0x2f); koutp(0x3d5,0);   //set mod = 256k
			frameplace = 0xa0000; visualpage = 0; koutp(0x3ce,0x9); koutp(0x3cf,visualpage<<4);
			break;
		case 5:
			setvmode(0x13);
			koutp(0x3d4,0x38); koutp(0x3d5,0x48);           //S3 Extensions enable #1
			koutp(0x3d4,0x39); koutp(0x3d5,0xa5);           //S3 Extensions enable #2
			koutp(0x3d4,0x31); koutp(0x3d5,kinp(0x3d5)|9);  //Init for start address

			frameplace = 0xa0000;
			visualpage = 0; koutp(0x3d4,0x35); koutp(0x3d5,visualpage);
			break;
	}

	koutp(0x3c8,0);
	for(i=0;i<768;i++) koutp(0x3c9,palette[i]);

	j = 0;
	for(i=0;i<MAXYDIM;i++) ylookup[i] = j, j += xdim;

	srand(readtimer());

	loadboard(boardfilename);

	oldtimerhandler = _dos_getvect(0x8);
	_dos_setvect(0x8, timerhandler);
	koutp(0x43,54); koutp(0x40,9942&255); koutp(0x40,9942>>8);
	oldkeyhandler = _dos_getvect(0x9);
	_disable(); _dos_setvect(0x9, keyhandler); _enable();
	lockspeed = 0;

	zoom = 65536;
	xreszoom = (float)zoom * (float)xdim * .5;
	ydimscale = (float)ydim / (float)xdim;
	gmp = (long *)&gm;
	gdp = (long *)&gd; gdxp = (long *)&gdx; gdyp = (long *)&gdy;
	gup = (long *)&gu; guxp = (long *)&gux; guyp = (long *)&guy;
	gvp = (long *)&gv; gvxp = (long *)&gvx; gvyp = (long *)&gvy;
	gtp = (long *)&gt;
	gxoffsp = (long *)&gxoffs; gyoffsp = (long *)&gyoffs;

	fxp = (long *)&fx; fyp = (long *)&fy; fzp = (long *)&fz;

	while (keystatus[1] == 0)
	{
		setuprotate3d();

		drawscreen();

		shiftstate = (keystatus[0x2a]|keystatus[0x36]);

		fx = (float)(svel>>(3-shiftstate));
		fy = (float)(hvel>>(4-shiftstate));
		fz = (float)(fvel>>(3-shiftstate));
		if (fxp[0]|fyp[0]|fzp[0])
		{
			ftemp = (float)lockspeed; fx *= ftemp; fy *= ftemp; fz *= ftemp;
			backrotate3d(fx,fy,fz);
			clipmove(&posx,&posy,&posz,(long)fx,(long)fy,(long)fz);
		}

		if (keystatus[0x52] > 0)    //0 on keypad (return to normal angle)
		{
			if ((keystatus[0x1d]|keystatus[0x9d]) > 0)
			{
				a2 = ((a2+1536)&2047);
				if (a2 < 1024) { a2 -= (lockspeed<<1); if (a2 < 0) a2 = 0; }
				else           { a2 += (lockspeed<<1); if (a2 >= 2048) a2 = 0; }
				a2 = ((a2+512)&2047);
			}
			else
			{
				if (a2 < 1024) { a2 -= (lockspeed<<1); if (a2 < 0) a2 = 0; }
				else           { a2 += (lockspeed<<1); if (a2 >= 2048) a2 = 0; }
			}
			if (a1 < 1024) { a1 -= (lockspeed<<1); if (a1 < 0) a1 = 0; }
			else           { a1 += (lockspeed<<1); if (a1 >= 2048) a1 = 0; }
		}

		a1 = ((a1+((a1vel*lockspeed)>>(5-shiftstate))+2048)&2047);
		a2 = ((a2+((a2vel*lockspeed)>>(5-shiftstate))+2048)&2047);
		a3 = ((a3+((a3vel*lockspeed)>>(5-shiftstate))+2048)&2047);
			//Auto-Tilting
		if (a3vel == 0)
		{
			if (a1 < 1024)
			{
				a1 -= (lockspeed<<1);
				if (a1 < 0) a1 = 0;
			}
			else
			{
				a1 += (lockspeed<<1);
				if (a1 >= 2048) a1 = 0;
			}
		}
		else
		{
			a1 = (((((((a1+1024)&2047)*7) + (1024-a3vel))>>3)+1024)&2047);
		}

		if (keystatus[0x13])     // R (re-align)
		{
			keystatus[0x13] = 0;
			if (((posx&1023) > WALLDIST) && ((posx&1023) < 1024-WALLDIST)) posx = ((posx+64)&0xffffff80);
			if (((posy&1023) > WALLDIST) && ((posy&1023) < 1024-WALLDIST)) posy = ((posy+64)&0xffffff80);
			if (((posz&1023) > WALLDIST) && ((posz&1023) < 1024-WALLDIST)) posz = ((posz+64)&0xffffff80);
			a1 = ((a1+128)&0x700);
			a2 = ((a2+128)&0x700);
			a3 = ((a3+128)&0x700);
		}

		if (keystatus[0x26])     // L (load)
		{
			keystatus[0x26] = 0;
			loadboard(boardfilename);
		}
		if (keystatus[0x1f]&(keystatus[0x38]|keystatus[0xb8]))     // Alt-S (save)
		{
			keystatus[0x1f] = 0;
			saveboard(boardfilename);
		}

		if (keystatus[0xd2])  // Insert cube
		{
			keystatus[0xd2] = 0;
			searchit();
			x = ((searchcubak/(BSIZ*BSIZ))<<10)+512;
			y = (((searchcubak/BSIZ)%BSIZ)<<10)+512;
			z = ((searchcubak%BSIZ)<<10)+512;
			if ((klabs(posx-x) > 512+WALLDIST) ||
				 (klabs(posy-y) > 512+WALLDIST) ||
				 (klabs(posz-z) > 512+WALLDIST))
			{
				if (temppicnum >= 0) i = temppicnum; else i = 0;
				for(j=0;j<6;j++) board[j*BSIZ*BSIZ*BSIZ+searchcubak] = i;
				makeboxbsp();
			}
		}
		if (keystatus[0xd3])  // Delete cube
		{
			keystatus[0xd3] = 0;
			searchit();
			x = searchcube/(BSIZ*BSIZ);
			y = (searchcube/BSIZ)%BSIZ;
			z = searchcube%BSIZ;
			if ((x > 0) && (x < BSIZ-1))
				if ((y > 0) && (y < BSIZ-1))
					if ((z > 0) && (z < BSIZ-1))
					{
						board[searchcube] = -1;
						makeboxbsp();
					}
		}
		if (keystatus[0xf])   // Tab
		{
			keystatus[0xf] = 0;
			searchit();
			temppicnum = board[searchcube+searchedge*BSIZ*BSIZ*BSIZ];
		}
		if (keystatus[0x1c])  // L. Enter
		{
			keystatus[0x1c] = 0;
			if (temppicnum >= 0)
			{
				searchit();
				board[searchcube+searchedge*BSIZ*BSIZ*BSIZ] = temppicnum;
			}
		}
		if (keystatus[0x38]&keystatus[0x2e]) //Alt-C
		{
			keystatus[0x38] = 0;
			if (temppicnum >= 0)
			{
				searchit();
				j = board[searchcube+searchedge*BSIZ*BSIZ*BSIZ];
				for(i=BSIZ*BSIZ*BSIZ*6-1;i>=0;i--)
					if (board[i] == j) board[i] = temppicnum;
			}
		}
		if ((keystatus[0x2f]) || ((bstatus&1) > (obstatus&1)))  //V/button 1
		{
			keystatus[0x2f] = 0;
			searchit();
			i = searchcube+searchedge*BSIZ*BSIZ*BSIZ;
			if (board[i] >= 0) board[i] = gettile(board[i]);
		}
		if (keystatus[0x9c] != 0)
		{
			keystatus[0x9c] = 0;
			drawmode++; if (drawmode == 3) drawmode = 0;
		}

		lockspeed = totalclock-baktotalclock;
		baktotalclock += lockspeed;
	}
	_dos_setvect(0x9, oldkeyhandler);
	_dos_setvect(0x8, oldtimerhandler);
	koutp(0x43,54); koutp(0x40,255); koutp(0x40,255);
	unloadpics();
	setvmode(0x3);
	free(screen);
	return(0);
}

void loadboard (char *filename)
{
	long fil, i, j;

	totalclock = 0L;
	baktotalclock = 0L;
	numframes = 0L;

	if (coordofpoint[0][0] == 0)
		for(i=0;i<8;i++)
		{
			if (coordofpoint[0][i] == 0) coordofpoint[0][i] = -512; else coordofpoint[0][i] = 512;
			if (coordofpoint[1][i] == 0) coordofpoint[1][i] = -512; else coordofpoint[1][i] = 512;
			if (coordofpoint[2][i] == 0) coordofpoint[2][i] = -512; else coordofpoint[2][i] = 512;
		}

	if ((fil = open(filename,O_BINARY|O_RDONLY,S_IREAD)) == -1)
	{
		for(i=6*BSIZ*BSIZ*BSIZ-1;i>=0;i--) board[i] = 0;
		posx = (BSIZ<<9)+512;
		posy = (BSIZ<<9)+512;
		posz = (BSIZ<<9)+512;
		board[(posx>>10)*BSIZ*BSIZ+(posy>>10)*BSIZ+(posz>>10)] = -1;
		a1 = 0; a2 = 0; a3 = 0;
		makeboxbsp();
		return;
	}
	read(fil,&i,4);
	if (i != 0xcbe)
	{
		printf("Wrong map version\n");
		exit(0);
	}
	read(fil,&posx,4);
	read(fil,&posy,4);
	read(fil,&posz,4);
	read(fil,&a1,4);
	read(fil,&a2,4);
	read(fil,&a3,4);
	read(fil,board,6*BSIZ*BSIZ*BSIZ*2);
	makeboxbsp();
	close(fil);
}

void saveboard (char *filename)
{
	long i, fil;

	if ((fil = open(filename,O_BINARY|O_TRUNC|O_CREAT|O_WRONLY,S_IWRITE)) == -1)
		return;

	i = 0xcbe;
	write(fil,&i,4);
	write(fil,&posx,4);
	write(fil,&posy,4);
	write(fil,&posz,4);
	write(fil,&a1,4);
	write(fil,&a2,4);
	write(fil,&a3,4);
	write(fil,board,6*BSIZ*BSIZ*BSIZ*2);
	close(fil);
}

void loadtables ()
{
	long i, fil;

	initksqrt();
	for(i=0;i<2048;i++) reciptable[i] = divscale30(2048L,i+2048);

	if ((fil = open("tables.dat",O_BINARY|O_RDWR,S_IREAD)) != -1)
	{
		read(fil,&sintable[0],4096);
		for(i=0;i<2048;i++) fsintable[i] = (((float)sintable[i])/16384);
		lseek(fil,4096+4096+640,SEEK_SET);
		read(fil,&textfont[0],1024);
		read(fil,&smalltextfont[0],1024);
		read(fil,&britable[0],1024);
		close(fil);
	}
	if ((fil = open("setup.dat",O_BINARY|O_RDWR,S_IREAD)) != -1)
	{
		read(fil,&option[0],NUMOPTIONS);
		read(fil,&keys[0],NUMKEYS);
		close(fil);
	}
}

void loadpalette ()
{
	long i, j, fil;

	if (paletteloaded == 0)
	{
		if ((fil = open("palette.dat",O_BINARY|O_RDWR,S_IREAD)) != -1)
		{
			read(fil,palette,768);
			read(fil,&numpalookups,2);
			read(fil,palookup,numpalookups<<8);

			if (translucloaded == 0)
				if ((transluc = (char *)malloc(65536L)) != 0)
					translucloaded = 1;

			read(fil,transluc,65536);

			blackcol = 0; j = 0x7fffffff;
			for(i=0;i<256;i++)
				if (palette[i*3]+palette[i*3+1]+palette[i*3+2] < j)
				{
					j = palette[i*3]+palette[i*3+1]+palette[i*3+2];
					blackcol = i;
				}

			whitecol = 0; j = 0x80000000;
			for(i=0;i<256;i++)
				if (palette[i*3]+palette[i*3+1]+palette[i*3+2] > j)
				{
					j = palette[i*3]+palette[i*3+1]+palette[i*3+2];
					whitecol = i;
				}

			close(fil);
		}
		paletteloaded = 1;
	}
}

void drawscreen ()
{
	long i, j, k;

	if (drawmode == 2)
	{
		if (option[0] == 0)
			clearbuf((void *)frameplace,xdim*ydim,0L);
		else
			clearbuf((void *)frameplace,(xdim*ydim)>>2,0L);
	}

	drawcubes();

	i = totalclock; j = (numframes&(AVERAGEFRAMES-1));
	if (i != frameval[j])
	{
		if (numframes >= AVERAGEFRAMES)
		{
			k = divmod((AVERAGEFRAMES*1200)/(i-frameval[j]),10);
			sprintf(tempbuf,"%ld.%ld",k,dmval);
			if (option[0] == 0)
				printext256(0L,0L,(long)0x00ffffff,-1L,tempbuf,1);
			else
				printext256(0L,0L,31L,-1L,tempbuf,1);
		}
		frameval[j] = i;
	}

	//sprintf(tempbuf,"Regions: %ld",globalregioncnt);
	//printext256(148L,0L,31,-1,tempbuf,1);
	//sprintf(tempbuf,"Splits: %ld",globalsplitcnt);
	//printext256(148L,6L,31,-1,tempbuf,1);
	//sprintf(tempbuf,"BSPlines: %ld",pnum);
	//printext256(148L,12L,31,-1,tempbuf,1);
	//sprintf(tempbuf,"Cursectnum: %ld",cubesect[(posx>>10)*BSIZ*BSIZ + (posy>>10)*BSIZ + (posz>>10)]);
	//printext256(148L,18L,31,-1,tempbuf,1);

	obstatus = bstatus, getmousevalues(&mousx,&mousy,&bstatus);
	searchx += mousx;
	searchy += mousy;
	if (searchx < 4) searchx = 4;
	if (searchx >= xdim-4) searchx = xdim-4-1;
	if (searchy < 4) searchy = 4;
	if (searchy >= ydim-4) searchy = ydim-4-1;

	if (option[0] == 0)
	{
		for(i=1;i<=4;i++)
		{
			drawpixelses(frameplace+((ylookup[searchy+i]+(searchx+0))<<2),0xffffffff);
			drawpixelses(frameplace+((ylookup[searchy-i]+(searchx+0))<<2),0xffffffff);
			drawpixelses(frameplace+((ylookup[searchy+0]+(searchx+i))<<2),0xffffffff);
			drawpixelses(frameplace+((ylookup[searchy+0]+(searchx-i))<<2),0xffffffff);
		}
	}
	else
	{
		for(i=1;i<=4;i++)
		{
			drawpixel(frameplace+ylookup[searchy+i]+(searchx+0),whitecol);
			drawpixel(frameplace+ylookup[searchy-i]+(searchx+0),whitecol);
			drawpixel(frameplace+ylookup[searchy+0]+(searchx+i),whitecol);
			drawpixel(frameplace+ylookup[searchy+0]+(searchx-i),whitecol);
		}
	}

	nextpage();
}

void nextpage ()
{
	long i, totbytes;

	switch(option[0])
	{
		case 0:
			if ((xdim == 320) && (ydim == 200))
			{
				totbytes = xdim*ydim*4;
				for(i=0;i<totbytes;i+=32768)
				{
					vesasetpage(i>>15);
					truecolorblit((void *)(frameplace+i),(void *)0xa0000,min(totbytes-i,32768)>>3);
				}
			}
			else
			{
				totbytes = xdim*ydim*4;
				for(i=0;i<totbytes;i+=65536)
				{
					vesasetpage(i>>16);
					copybuf((void *)(frameplace+i),(void *)0xa0000,min(totbytes-i,65536)>>2);
				}
			}
			break;
		case 1: case 2:
			totbytes = xdim*ydim;
			for(i=0;i<totbytes;i+=65536)
			{
				if ((xdim > 320) || (ydim > 200)) vesasetpage(i>>16);
				copybuf((void *)(frameplace+i),(void *)0xa0000,min(totbytes-i,65536)>>2);
			}
			break;
		case 3:
			koutp(0x3d4,0xc); koutp(0x3d5,visualpage<<6);
			visualpage = ((visualpage+1)&3);
			koutp(0x3cd,visualpage+(visualpage<<4));
			break;
		case 4:
			koutp(0x3d4,0xc); koutp(0x3d5,visualpage<<6);
			visualpage = ((visualpage+1)&3);
			koutp(0x3ce,0x9); koutp(0x3cf,visualpage<<4);
			break;
		case 5:
			koutp(0x3d4,0xc); koutp(0x3d5,visualpage<<6);
			visualpage = ((visualpage+1)&3);
			koutp(0x3d4,0x35); koutp(0x3d5,visualpage);
			break;
	}
	numframes++;
}

void drawcubes ()
{
	long i, j, k, e, f, x, y, z, p, dapoint[4], areasucked;
	long p1, p2, sectlistplc, sectlistnum, numfaces, lnum, totbytes;
	long plistnum, newsect, gotsectcnt, *fxp, *fyp, *fzp, *ptr;
	float fx, fy, fz, ftemp;

	fxp = (long *)&fx; fyp = (long *)&fy; fzp = (long *)&fz;

	initmesh();

	sectlist[0] = cubesect[(posx>>10)*BSIZ*BSIZ + (posy>>10)*BSIZ + (posz>>10)];
	bspstart[0] = 0;
	sectlistplc = 0; sectlistnum = 1;

	if (keystatus[0x29] == 2) keystatus[0x29] = 0;
	if (keystatus[0x29] == 1)
	{
		keystatus[0x29] = 2;
		if (option[0] == 0)
			clearbuf((void *)frameplace,xdim*ydim,0L);
		else
			clearbuf((void *)frameplace,(xdim*ydim)>>2,0L);
	}

	do
	{
		gsect = sectlist[sectlistplc];
		gotsectcnt = 0;

		areasucked = 0;

		numfaces = 0;
		if ((sectx1[gsect]<<10) < posx)
		{
			 x = sectx1[gsect]-1;
			 for(y=secty1[gsect];y<=secty2[gsect];y++)
				 for(z=sectz1[gsect];z<=sectz2[gsect];z++)
					 { facetag[numfaces] = x*BSIZ*BSIZ+y*BSIZ+z; facesid[numfaces++] = 0; }
		}
		if (((sectx2[gsect]+1)<<10) > posx)
		{
			 x = sectx2[gsect]+1;
			 for(y=secty1[gsect];y<=secty2[gsect];y++)
				 for(z=sectz1[gsect];z<=sectz2[gsect];z++)
					 { facetag[numfaces] = x*BSIZ*BSIZ+y*BSIZ+z; facesid[numfaces++] = 1; }
		}
		if ((secty1[gsect]<<10) < posy)
		{
			 y = secty1[gsect]-1;
			 for(x=sectx1[gsect];x<=sectx2[gsect];x++)
				 for(z=sectz1[gsect];z<=sectz2[gsect];z++)
					 { facetag[numfaces] = x*BSIZ*BSIZ+y*BSIZ+z; facesid[numfaces++] = 2; }
		}
		if (((secty2[gsect]+1)<<10) > posy)
		{
			 y = secty2[gsect]+1;
			 for(x=sectx1[gsect];x<=sectx2[gsect];x++)
				 for(z=sectz1[gsect];z<=sectz2[gsect];z++)
					 { facetag[numfaces] = x*BSIZ*BSIZ+y*BSIZ+z; facesid[numfaces++] = 3; }
		}
		if ((sectz1[gsect]<<10) < posz)
		{
			 z = sectz1[gsect]-1;
			 for(x=sectx1[gsect];x<=sectx2[gsect];x++)
				 for(y=secty1[gsect];y<=secty2[gsect];y++)
					 { facetag[numfaces] = x*BSIZ*BSIZ+y*BSIZ+z; facesid[numfaces++] = 4; }
		}
		if (((sectz2[gsect]+1)<<10) > posz)
		{
			 z = sectz2[gsect]+1;
			 for(x=sectx1[gsect];x<=sectx2[gsect];x++)
				 for(y=secty1[gsect];y<=secty2[gsect];y++)
					 { facetag[numfaces] = x*BSIZ*BSIZ+y*BSIZ+z; facesid[numfaces++] = 5; }
		}

		numedges = 0; numpoints = 0;
		for(f=0;f<numfaces;f++)
		{
			gcube = facetag[f]; gface = facesid[f];
			x = ((gcube/(BSIZ*BSIZ))<<10) + 512 - posx;
			y = (((gcube/BSIZ)%BSIZ)<<10) + 512 - posy;
			z = ((gcube%BSIZ)<<10) + 512 - posz;
			switch(gface)
			{
				case 0: x += 1024; break;
				case 1: x -= 1024; break;
				case 2: y += 1024; break;
				case 3: y -= 1024; break;
				case 4: z += 1024; break;
				case 5: z -= 1024; break;
			}
			for(e=3;e>=0;e--)
			{
				i = pointofface[gface][e];
				fx = coordofpoint[0][i] + x;
				fy = coordofpoint[1][i] + y;
				fz = coordofpoint[2][i] + z;
				dapoint[e] = -1;
				for(i=numpoints-1,ptr=(long *)&r[i].x;i>=0;i--,ptr-=3)
					if ((fxp[0] == ptr[0]) && (fyp[0] == ptr[1]) && (fzp[0] == ptr[2]))
						{ dapoint[e] = i; break; }
				if (i < 0)
				{
					r[numpoints].x = fx;
					r[numpoints].y = fy;
					r[numpoints].z = fz;
					dapoint[e] = numpoints;
					numpoints++;
				}
			}
			for(e=3;e>=0;e--)
			{
				p1 = dapoint[e]; p2 = dapoint[(e+1)&3];
				j = p2+(p1<<16);

				for(i=numedges-1,ptr=(long *)&edge[i].p1;i>=0;i--,ptr-=2)
					if (ptr[0] == j) { edge[i].a2 = gcube; break; } //2-sided edge
				if (i < 0)
				{
					edge[numedges].p1 = p1; edge[numedges].p2 = p2;
					edge[numedges].a1 = gcube; edge[numedges].a2 = -1;
					numedges++;
				}
			}
		}

		mesh[0] = 0; mesh[1] = numedges; nummeshs = 1;

		addmesh(bspstart[sectlistplc]);

		for(f=numfaces-1;f>=0;f--)
		{
			gcube = facetag[f];
			if (board[gcube] >= 0)
			{
				lnum = 0;

				endopoints = numpoints;

				for(j=ecnt-1;j>=0;j--)
				{
					p = elist[j]; k = mesh[p+1];
					clearbuf(got,(numpoints+3)>>2,0L);
					for(i=mesh[p];i<k;i++)
						if ((edge[i].a1 == gcube) || (edge[i].a2 == gcube))
						{
							if (edge[i].a1 == gcube)
								p1 = edge[i].p1, p2 = edge[i].p2;
							else
								p2 = edge[i].p1, p1 = edge[i].p2;

							if (got[p1] == 0)
							{
								fx = r[p1].x; fy = r[p1].y; fz = r[p1].z;
								rotate3d(fx,fy,fz);
								ftemp = xreszoom / fz;
								got[p1] = 1;
								fx *= ftemp; ftol(&fx,&sc[p1].x);
								fy *= ftemp; ftol(&fy,&sc[p1].y);
								sc[p1].x += xdimup15;
								sc[p1].y += ydimup15;
								//if (sc[p1].x < 0) sc[p1].x = 0;
								//if (sc[p1].x > (xdim<<16)) sc[p1].x = (xdim<<16);
								//if (sc[p1].y < 0) sc[p1].y = 0;
								//if (sc[p1].y > (ydim<<16)) sc[p1].y = (ydim<<16);
							}
							if (got[p2] == 0)
							{
								fx = r[p2].x; fy = r[p2].y; fz = r[p2].z;
								rotate3d(fx,fy,fz);
								ftemp = xreszoom / fz;
								got[p2] = 1;
								fx *= ftemp; ftol(&fx,&sc[p2].x);
								fy *= ftemp; ftol(&fy,&sc[p2].y);
								sc[p2].x += xdimup15;
								sc[p2].y += ydimup15;
								//if (sc[p2].x < 0) sc[p2].x = 0;
								//if (sc[p2].x > (xdim<<16)) sc[p2].x = (xdim<<16);
								//if (sc[p2].y < 0) sc[p2].y = 0;
								//if (sc[p2].y > (ydim<<16)) sc[p2].y = (ydim<<16);
							}
							//if (edgetag[i]&0xc000)
							//{
								sc[endopoints].x = sc[p1].x;
								sc[endopoints].y = sc[p1].y;
								sc[endopoints+1].x = sc[p2].x;
								sc[endopoints+1].y = sc[p2].y;
								endopoints += 2;
							//}
							//else
							//{
							//   ssx[lnum] = sc[p1].x; ssx[lnum+1] = sc[p2].x;
							//   ssy[lnum] = sc[p1].y; ssy[lnum+1] = sc[p2].y;
							//   ltag[lnum] = (edgetag[i]&0x3fff);
							//   lnum += 2;
							//}
						}
				}

				//if (lnum > 0) reduceedges(lnum);

				j = 0;
				for(i=numpoints;i<endopoints;i+=2)
					j += mulscale28(sc[i].x+sc[i+1].x,sc[i+1].y-sc[i].y);
				if (j > 0)
				{
					areasucked += j;
					gface = facesid[f];
					drawfasttextureplane((long)numpoints,(long)endopoints);
				}
			}
			else
			{
				newsect = cubesect[gcube];
				for(i=gotsectcnt-1;i>=0;i--) if (gotsect[i] == newsect) break;
				if (i < 0)
				{
					gotsect[gotsectcnt++] = newsect;
					plistnum = 0;
					for(j=ecnt-1;j>=0;j--)
					{
						p = elist[j]; k = mesh[p+1];
						for(i=mesh[p];i<k;i++)
						{
							if (edge[i].a2 >= 0)
							{
								if (cubesect[edge[i].a1] == newsect)
								{
									if (cubesect[edge[i].a2] != newsect)
										plist[plistnum++] = edgetag[i];
								}
								else if (cubesect[edge[i].a2] == newsect)
									plist[plistnum++] = (edgetag[i]|0x2000);
							}
							else if (cubesect[edge[i].a1] == newsect)
								plist[plistnum++] = edgetag[i];
						}
					}
					if (plistnum >= 3)
					{
						sectlist[sectlistnum] = newsect;
						bspstart[sectlistnum++] = pnum;
						add2bsp(plistnum);
					}
				}
			}
		}

		if ((keystatus[0x29] == 2) && (areasucked > 0))
		{
			switch(option[0])
			{
				case 0:
					if ((xdim == 320) && (ydim == 200))
					{
						totbytes = xdim*ydim*4;
						for(i=0;i<totbytes;i+=32768)
						{
							vesasetpage(i>>15);
							truecolorblit((void *)(frameplace+i),(void *)0xa0000,min(totbytes-i,32768)>>3);
						}
					}
					else
					{
						totbytes = xdim*ydim*4;
						for(i=0;i<totbytes;i+=65536)
						{
							vesasetpage(i>>16);
							copybuf((void *)(frameplace+i),(void *)0xa0000,min(totbytes-i,65536)>>2);
						}
					}
					break;
				case 1: case 2:
					totbytes = xdim*ydim;
					for(i=0;i<totbytes;i+=65536)
					{
						if ((xdim > 320) || (ydim > 200)) vesasetpage(i>>16);
						copybuf((void *)(frameplace+i),(void *)0xa0000,min(totbytes-i,65536)>>2);
					}
					break;
				case 3: case 4: case 5:
					koutp(0x3d4,0xc); koutp(0x3d5,visualpage<<6); break;
			}
			while (!keystatus[0xe]);
			while (keystatus[0xe]);
		}

		sectlistplc++;
	} while (sectlistplc < sectlistnum);
}

void reduceedges (long lnum)
{
	long i, x, y, z, zz, zx, zzx, i1, i2, lt, p, x1, y1, x2, y2;
	long dap, pcnt, gap, s, baks, bakx, baky;

	lnum >>= 1;

	for(gap=lnum>>1;gap>0;gap>>=1)
		for(z=0;z<lnum-gap;z++)
			for(zz=z;zz>=0;zz-=gap)
			{
				i1 = (zz<<1); i2 = ((zz+gap)<<1);
				if (ltag[i1] >= ltag[i2]) break;
				swap64bit(&ssx[i1],&ssx[i2]);
				swap64bit(&ssy[i1],&ssy[i2]);
				swapshort(&ltag[i1],&ltag[i2]);
			}

	lnum += lnum;

	z = 0;
	while (z < lnum)
	{
		lt = ltag[z]; zz = z;
		do
		{
			ltag[zz] = 1; ltag[zz+1] = -1; zz += 2;
		} while ((ltag[zz] == lt) && (zz < lnum));

		pcnt = zz-z;

		if (klabs(ssx[z+1]-ssx[z]) >= klabs(ssy[z+1]-ssy[z]))
		{
			for(gap=pcnt>>1;gap>0;gap>>=1)
				for(zx=0;zx<pcnt-gap;zx++)
					for(zzx=zx;zzx>=0;zzx-=gap)
					{
						i1 = zzx+z; i2 = i1+gap;
						if (ssx[i1] <= ssx[i2]) break;
						swaplong(&ssx[i1],&ssx[i2]);
						swaplong(&ssy[i1],&ssy[i2]);
						swapshort(&ltag[i1],&ltag[i2]);
					}
		}
		else
		{
			for(gap=pcnt>>1;gap>0;gap>>=1)
				for(zx=0;zx<pcnt-gap;zx++)
					for(zzx=zx;zzx>=0;zzx-=gap)
					{
						i1 = zzx+z; i2 = i1+gap;
						if (ssy[i1] <= ssy[i2]) break;
						swaplong(&ssx[i1],&ssx[i2]);
						swaplong(&ssy[i1],&ssy[i2]);
						swapshort(&ltag[i1],&ltag[i2]);
					}
		}

		s = 0; baks = 0;
		for(dap=z;dap<zz-1;dap++)
		{
			s += ltag[dap];
			if (s != 0)
			{
				x1 = ssx[dap]; x2 = ssx[dap+1];
				y1 = ssy[dap]; y2 = ssy[dap+1];
				if ((x1 != x2) || (y1 != y2))
				{
					if ((baks == s) && (bakx == x1) && (baky == y1))
					{
						if (s > 0) { sc[endopoints-1].x = x2; sc[endopoints-1].y = y2; }
								else { sc[endopoints-2].x = x2; sc[endopoints-2].y = y2; }
					}
					else
					{
						if (s > 0)
						{
							sc[endopoints].x = x1; sc[endopoints+1].x = x2;
							sc[endopoints].y = y1; sc[endopoints+1].y = y2;
						}
						else
						{
							sc[endopoints].x = x2; sc[endopoints+1].x = x1;
							sc[endopoints].y = y2; sc[endopoints+1].y = y1;
						}
						endopoints += 2;
						baks = s;
					}
					bakx = x2; baky = y2;
				}
			}
		}
		z = zz;
	}
}

void setuprotate3d ()
{
	float c1, s1, c2, s2, c3, s3, c1c3, c1s3, s1c3, s1s3;

	c1 = fsintable[(a1+512)&2047]; s1 = fsintable[a1&2047];
	c2 = fsintable[(a2+512)&2047]; s2 = fsintable[a2&2047];
	c3 = fsintable[(a3+512)&2047]; s3 = fsintable[a3&2047];
	c1c3 = c1*c3; c1s3 = c1*s3; s1c3 = s1*c3; s1s3 = s1*s3;

	rxx = s1s3*s2 + c1c3;    ryx = -s1*c2;   rzx = s1c3*s2 - c1s3;
	rxy = -c1s3*s2 + s1c3;   ryy = c1*c2;    rzy = -c1c3*s2 - s1s3;
	rxz = c2*s3;             ryz = s2;       rzz = c2*c3;
}

void initksqrt ()
{
	unsigned long i, j, num, root;

	for(i=0;i<2048;i++)
	{
		num = (i<<20); root = 128;
		do
		{
			j = root; root = ((num/root+root)>>1);
		} while (klabs(j-root) > 1);
		j = root*root-num;
		while (klabs((j-(root<<1)+1)) < klabs(j)) { j += -(root<<1)+1; root--; }
		while (klabs((j+(root<<1)+1)) < klabs(j)) { j += (root<<1)+1; root++; }
		sqrtable[i] = (unsigned short)root;
	}
}

void setupgetreadpos ()
{
	long i, i0, i1, i2, x, y, z;
	float x0, y0, z0, x1, y1, z1, x2, y2, z2, ftx, fty, ftemp;

	i0 = pointofface[gface][0];
	i1 = pointofface[gface][1];
	i2 = pointofface[gface][3];

	x = ((gcube/(BSIZ*BSIZ))<<10)+512-posx;
	y = (((gcube/BSIZ)%BSIZ)<<10)+512-posy;
	z = ((gcube%BSIZ)<<10)+512-posz;
	switch(gface)
	{
		case 0: x += 1024; break;
		case 1: x -= 1024; break;
		case 2: y += 1024; break;
		case 3: y -= 1024; break;
		case 4: z += 1024; break;
		case 5: z -= 1024; break;
	}

	x0 = (float)(coordofpoint[0][i0]+x);
	y0 = (float)(coordofpoint[1][i0]+y);
	z0 = (float)(coordofpoint[2][i0]+z);
	x1 = (float)(coordofpoint[0][i1]-coordofpoint[0][i0]);
	y1 = (float)(coordofpoint[1][i1]-coordofpoint[1][i0]);
	z1 = (float)(coordofpoint[2][i1]-coordofpoint[2][i0]);
	x2 = (float)(coordofpoint[0][i2]-coordofpoint[0][i0]);
	y2 = (float)(coordofpoint[1][i2]-coordofpoint[1][i0]);
	z2 = (float)(coordofpoint[2][i2]-coordofpoint[2][i0]);

	i = picsiz[gtext];
	if (((1<<(i&15)) != tilesizx[gtext]) || ((1<<(i>>4)) != tilesizy[gtext])
			  || ((tilesizx[gtext] >= 64) && (tilesizy[gtext] >= 64)))
	{
		ftx = 64/(float)tilesizx[gtext]; x1 *= ftx; y1 *= ftx; z1 *= ftx;
		fty = 64/(float)tilesizy[gtext]; x2 *= fty; y2 *= fty; z2 *= fty;
	}
	ftemp = (float)(1<<gmip);
	x1 *= ftemp; y1 *= ftemp; z1 *= ftemp;
	x2 *= ftemp; y2 *= ftemp; z2 *= ftemp;

	rotate3d(x0,y0,z0);
	rotate3d(x1,y1,z1);
	rotate3d(x2,y2,z2);
	gdx = (y1*z2-y2*z1); gdy = (x2*z1-x1*z2);
	gux = (y2*z0-y0*z2); guy = (x0*z2-x2*z0);
	gvx = (y0*z1-y1*z0); gvy = (x1*z0-x0*z1);
	ftemp = xreszoom / 65536;
	ftx = ((float)(xdim-1))*.5; fty = ((float)(ydim-1))*.5;
	gd = (x1*y2-x2*y1)*ftemp - (gdx*ftx+gdy*fty);
	gu = (x2*y0-x0*y2)*ftemp - (gux*ftx+guy*fty);
	gv = (x0*y1-x1*y0)*ftemp - (gvx*ftx+gvy*fty);
	gt = (x0*gdx+x1*gux+x2*gvx)*(ftemp/16384);
}

void drawfasttextureplane (long pstart, long pend)
{
	long i, j, z, zz, x, y, x1, y1, x2, y2, d1, d2, dapicnum;
	long plc, inc, minx, maxx, miny, maxy, c1, c2, npoints;
	float ftemp;

	switch(gface)
	{
		case 0: gtext = board[gcube+3*BSIZ*BSIZ*BSIZ]; break;
		case 1: gtext = board[gcube+0*BSIZ*BSIZ*BSIZ]; break;
		case 2: gtext = board[gcube+4*BSIZ*BSIZ*BSIZ]; break;
		case 3: gtext = board[gcube+1*BSIZ*BSIZ*BSIZ]; break;
		case 4: gtext = board[gcube+5*BSIZ*BSIZ*BSIZ]; break;
		case 5: gtext = board[gcube+2*BSIZ*BSIZ*BSIZ]; break;
	}

	x = ((gcube/(BSIZ*BSIZ))<<10)+512-posx;
	y = (((gcube/BSIZ)%BSIZ)<<10)+512-posy;
	z = ((gcube%BSIZ)<<10)+512-posz;

	i = ksqrt(x*x+y*y+z*z);
	if (xdim != 320) i = scale(i,320,xdim);
	if (i < 4096) gmip = 0;
	else if (i < 8192) gmip = 1;
	else if (i < 16384) gmip = 2;
	else gmip = 3;

	if (keystatus[2]) gmip = 0;
	if (keystatus[3]) gmip = 1;
	if (keystatus[4]) gmip = 2;
	if (keystatus[5]) gmip = 3;

	gtext += animateoffs(gtext);
	if (waloff[gmip][gtext] == 0) loadtile((short)gtext);
	if (waloff[gmip][gtext] == 0) return;
	gbuf = (char *)(waloff[gmip][gtext]); if (gbuf == NULL) return;

	gshade = ((gface>>1)<<5);

	i = max((picsiz[gtext]&15)-gmip,0) + (max((picsiz[gtext]>>4)-gmip,0)<<4);
	if (option[0] == 0)
		setupdline24(waloff[gmip][gtext],FP_OFF(&colookup[0][0]),(256<<(i&15))+(1<<(i>>4))-257);
	else
		setupdline(waloff[gmip][gtext],FP_OFF(palookup[0]),(256<<(i&15))+(1<<(i>>4))-257);

	gmask1 = (((1<<(i&15))-1)<<8); gmask2 = (1<<(i>>4))-1;

	setupgetreadpos();

	globalregioncnt++;
	if (drawmode == 2)
	{
		for(z=pstart;z<pend;z+=2)
		{
			if (sc[z].x >= (xdim<<16)) sc[z].x = (xdim<<16)-1;
			if (sc[z].y >= (ydim<<16)) sc[z].y = (ydim<<16)-1;
			if (sc[z+1].x >= (xdim<<16)) sc[z+1].x = (xdim<<16)-1;
			if (sc[z+1].y >= (ydim<<16)) sc[z+1].y = (ydim<<16)-1;

			//x1 = ((sc[z].x+sc[z+1].x)>>1); y1 = ((sc[z].y+sc[z+1].y)>>1);
			//x2 = (sc[z].y-sc[z+1].y); y2 = (sc[z+1].x-sc[z].x);
			//i = ksqrt(dmulscale24(x2,x2,y2,y2));
			//if (i > 0)
			//   drawline256(x1,y1,x1+divscale7(x2,i),y1+divscale7(y2,i),((globalregioncnt<<5)&0xe0)+15);

			drawline256(sc[z].x,sc[z].y,sc[z+1].x,sc[z+1].y,16);
		}
		return;
	}
	if (drawmode == 1)
	{
		ftol(&gd,&lgd); ftol(&gdx,&lgdx); ftol(&gdy,&lgdy);
		ftol(&gu,&lgu); ftol(&gux,&lgux); ftol(&guy,&lguy);
		ftol(&gv,&lgv); ftol(&gvx,&lgvx); ftol(&gvy,&lgvy);

		for(z=pstart;z<pend;z++) sc[z].y -= 65536;

		miny = 0x7fffffff; maxy = 0x80000000;
		for(z=pstart;z<pend;z+=2)
		{
			y = (sc[z].y>>16);
			if (y < miny) miny = y;
			if (y > maxy) maxy = y;
		}
		miny--; maxy++;

		c1 = 1; clearbuf((void *)&h1[miny],((maxy+1-miny)+1)>>1,0L);
		c2 = 1; clearbuf((void *)&h2[miny],((maxy+1-miny)+1)>>1,0L);

		for(z=pstart;z<pend;z+=2)
		{
			x1 = sc[z].x; y1 = sc[z].y; x2 = sc[z+1].x; y2 = sc[z+1].y;

			if (((x1^x2)&0xffff0000) == 0)
			{
				x = (x1>>16); y = (y1>>16); j = (y2>>16);
				while (j < y) { p1[c1].p = x; p1[c1].n = h1[y]; h1[y] = c1++; y--; }
				while (j > y) { y++; p2[c2].p = x; p2[c2].n = h2[y]; h2[y] = c2++; }
			}
			else
			{
				if ((klabs(y2-y1)>>15) < klabs(x2-x1))
					inc = divscale16(y2-y1,x2-x1);
				else
					inc = 0;

				d1 = (x1>>16); d2 = (x2>>16);
				if (x1 < x2)
				{
					plc = y1 + mulscale16((~x1)&65535,inc);
					y = (y1>>16);
					for(x=d1;x<d2;x++)
					{
						j = (plc>>16); plc += inc;
						while (j < y) { p1[c1].p = x; p1[c1].n = h1[y]; h1[y] = c1++; y--; }
						while (j > y) { y++; p2[c2].p = x; p2[c2].n = h2[y]; h2[y] = c2++; }
					}
					j = (y2>>16);
					while (j < y) { p1[c1].p = x; p1[c1].n = h1[y]; h1[y] = c1++; y--; }
					while (j > y) { y++; p2[c2].p = x; p2[c2].n = h2[y]; h2[y] = c2++; }
				}
				else
				{
					plc = y2 + mulscale16((-x2)&65535,inc);
					y = (y2>>16);
					for(x=d2;x<d1;x++)
					{
						j = (plc>>16); plc += inc;
						while (j < y) { p2[c2].p = x; p2[c2].n = h2[y]; h2[y] = c2++; y--; }
						while (j > y) { y++; p1[c1].p = x; p1[c1].n = h1[y]; h1[y] = c1++; }
					}
					j = (y1>>16);
					while (j < y) { p2[c2].p = x; p2[c2].n = h2[y]; h2[y] = c2++; y--; }
					while (j > y) { y++; p1[c1].p = x; p1[c1].n = h1[y]; h1[y] = c1++; }
				}
			}
		}

		for(y=miny;y<=maxy;y++)
			for(y1=h1[y],y2=h2[y];y1!=0;y1=p1[y1].n,y2=p2[y2].n)
			{
				d1 = y1; d2 = y2;
				for(i=p1[y1].n,j=p2[y2].n;i!=0;i=p1[i].n,j=p2[j].n)
				{
					if (p1[i].p < p1[d1].p) d1 = i;
					if (p2[j].p < p2[d2].p) d2 = j;
				}
				if (p1[d1].p < p2[d2].p) hline((long)p1[d1].p,(long)p2[d2].p,y);
				p1[d1].p = p1[y1].p; p2[d2].p = p2[y2].p;
			}
		return;
	}

	if ((gdxp[0]&0x7fffffff) <= (gdyp[0]&0x7fffffff))
	{
		if (gdxp[0] == 0)
			gm = 0;
		else
		{
			gm = gdx/gdy;   //d& = y& + mulscale&(x&, m&, 16)
			if ((gmp[0]&0x7fffffff) < 0.0005) gm = 0;
			if (gm > 0.999) gm = 1;
			if (gm < -0.999) gm = -1;
		}

		if (gmp[0] > 0) hplc[0] = 0L; else hplc[0] = xdim;

		gxoffs = gux-guy*gm;
		gyoffs = gvx-gvy*gm;

		pinc[0] = 1L;
		if (gmp[0] < 0) pinc[2] = 1+xdim; else pinc[2] = 1-xdim;
		if (option[0] == 0) pinc[0] <<= 2, pinc[2] <<= 2;

		ftemp = gm*65536; ftol(&ftemp,&lgm);


		for(z=pstart;z<pend;z++) sc[z].y -= 65536;

		minx = sc[pstart].x; maxx = sc[pstart].x;
		for(z=pstart+2;z<pend;z+=2)
		{
			x = sc[z].x;
			if (x < minx) minx = x;
			if (x > maxx) maxx = x;
		}
		minx >>= 16; maxx >>= 16;

		qinterpolatedown16(&hplc[minx],maxx-minx+1,(((long)hplc[0])<<16)+(lgm*minx)+32768,lgm);

		if (gmp[0] != 0)
		{
			if (gmp[0] < 0) i = xdim; else i = -xdim;
			for(x=minx;x<=maxx;x++) hind[x] = ((hplc[x]!=hplc[x+1])<<3);
		}
		else
			{ for(x=minx;x<=maxx;x++) hind[x] = 0; }

		miny = 0x7fffffff; maxy = 0x80000000;
		for(z=pstart;z<pend;z+=2)
		{
			y = (sc[z].y>>16) + hplc[sc[z].x>>16];
			if (y < miny) miny = y;
			if (y > maxy) maxy = y;
		}
		miny--; maxy++;

		c1 = 1; clearbuf(&h1[miny],((maxy+1-miny)+1)>>1,0L);
		c2 = 1; clearbuf(&h2[miny],((maxy+1-miny)+1)>>1,0L);

		for(z=pstart;z<pend;z+=2)
		{
			x1 = sc[z].x; y1 = sc[z].y; x2 = sc[z+1].x; y2 = sc[z+1].y;

			if (((x1^x2)&0xffff0000) == 0)
			{
				x = (x1>>16);
				y = (y1>>16) + hplc[x];
				j = (y2>>16) + hplc[x];
				while (j < y) { p1[c1].p = x; p1[c1].n = h1[y]; h1[y] = c1++; y--; }
				while (j > y) { y++; p2[c2].p = x; p2[c2].n = h2[y]; h2[y] = c2++; }
			}
			else
			{
				if ((klabs(y2-y1)>>15) < klabs(x2-x1))
					inc = divscale16(y2-y1,x2-x1);
				else
					inc = 0;

				d1 = (x1>>16); d2 = (x2>>16);
				if (x1 < x2)
				{
					plc = y1 + mulscale16((~x1)&65535,inc);
					y = (y1>>16) + hplc[d1];
					for(x=d1;x<d2;x++)
					{
						j = (plc>>16) + hplc[x]; plc += inc;
						while (j < y) { p1[c1].p = x; p1[c1].n = h1[y]; h1[y] = c1++; y--; }
						while (j > y) { y++; p2[c2].p = x; p2[c2].n = h2[y]; h2[y] = c2++; }
					}
					j = (y2>>16) + hplc[d2];
					while (j < y) { p1[c1].p = x; p1[c1].n = h1[y]; h1[y] = c1++; y--; }
					while (j > y) { y++; p2[c2].p = x; p2[c2].n = h2[y]; h2[y] = c2++; }
				}
				else
				{
					plc = y2 + mulscale16((-x2)&65535,inc);
					y = (y2>>16) + hplc[d2];
					for(x=d2;x<d1;x++)
					{
						j = (plc>>16) + hplc[x]; plc += inc;
						while (j < y) { p2[c2].p = x; p2[c2].n = h2[y]; h2[y] = c2++; y--; }
						while (j > y) { y++; p1[c1].p = x; p1[c1].n = h1[y]; h1[y] = c1++; }
					}
					j = (y1>>16) + hplc[d1];
					while (j < y) { p2[c2].p = x; p2[c2].n = h2[y]; h2[y] = c2++; y--; }
					while (j > y) { y++; p1[c1].p = x; p1[c1].n = h1[y]; h1[y] = c1++; }
				}
			}
		}

		for(y=miny;y<=maxy;y++)
			for(y1=h1[y],y2=h2[y];y1!=0;y1=p1[y1].n,y2=p2[y2].n)
			{
				d1 = y1; d2 = y2;
				for(i=p1[y1].n,j=p2[y2].n;i!=0;i=p1[i].n,j=p2[j].n)
				{
					if (p1[i].p < p1[d1].p) d1 = i;
					if (p2[j].p < p2[d2].p) d2 = j;
				}
				if (p1[d1].p < p2[d2].p) diaghline((long)p1[d1].p,(long)p2[d2].p-1,y);
				p1[d1].p = p1[y1].p; p2[d2].p = p2[y2].p;
			}
	}
	else
	{
		if (gdyp[0] == 0)
			gm = 0;
		else
		{
			gm = gdy/gdx;
			if ((gmp[0]&0x7fffffff) < 0.0005) gm = 0;
			if (gm > 0.999) gm = 1;
			if (gm < -0.999) gm = -1;
		}

		if (gmp[0] > 0) hplc[0] = 0L; else hplc[0] = xdim;

		gxoffs = guy-gux*gm;
		gyoffs = gvy-gvx*gm;

		pinc[0] = xdim;
		if (gmp[0] < 0) pinc[2] = xdim+1L; else pinc[2] = xdim-1L;
		if (option[0] == 0) pinc[0] <<= 2, pinc[2] <<= 2;

		ftemp = gm*65536; ftol(&ftemp,&lgm);


		for(z=pstart;z<pend;z++) sc[z].x -= 65536;

		miny = sc[pstart].y; maxy = sc[pstart].y;
		for(z=pstart+2;z<pend;z+=2)
		{
			y = sc[z].y;
			if (y < miny) miny = y;
			if (y > maxy) maxy = y;
		}
		miny >>= 16; maxy >>= 16;

		qinterpolatedown16(&hplc[miny],maxy-miny+1,(((long)hplc[0])<<16)+lgm*miny+32768,lgm);

		if (gmp[0] != 0)
		{
			if (gmp[0] < 0) i = 1; else i = -1;
			for(y=miny;y<=maxy;y++) hind[y] = ((hplc[y]!=hplc[y+1])<<3);
		}
		else
			{ for(y=miny;y<=maxy;y++) hind[y] = 0; }

		minx = 0x7fffffff; maxx = 0x80000000;
		for(z=pstart;z<pend;z+=2)
		{
			x = (sc[z].x>>16) + hplc[sc[z].y>>16];
			if (x < minx) minx = x;
			if (x > maxx) maxx = x;
		}
		minx--; maxx++;

		c1 = 1; clearbuf(&h1[minx],((maxx+1-minx)+1)>>1,0L);
		c2 = 1; clearbuf(&h2[minx],((maxx+1-minx)+1)>>1,0L);

		for(z=pstart;z<pend;z+=2)
		{
			x1 = sc[z].x; y1 = sc[z].y; x2 = sc[z+1].x; y2 = sc[z+1].y;

			if (((y1^y2)&0xffff0000) == 0)
			{
				y = (y1>>16);
				x = (x1>>16) + hplc[y];
				j = (x2>>16) + hplc[y];
				while (j < x) { p1[c1].p = y; p1[c1].n = h1[x]; h1[x] = c1++; x--; }
				while (j > x) { x++; p2[c2].p = y; p2[c2].n = h2[x]; h2[x] = c2++; }
			}
			else
			{
				if ((klabs(x2-x1)>>15) < klabs(y2-y1))
					inc = divscale16(x2-x1,y2-y1);
				else
					inc = 0;

				d1 = (y1>>16); d2 = (y2>>16);
				if (y1 < y2)
				{
					plc = x1 + mulscale16((~y1)&65535,inc);
					x = (x1>>16) + hplc[d1];
					for(y=d1;y<d2;y++)
					{
						j = (plc>>16) + hplc[y]; plc += inc;
						while (j < x) { p1[c1].p = y; p1[c1].n = h1[x]; h1[x] = c1++; x--; }
						while (j > x) { x++; p2[c2].p = y; p2[c2].n = h2[x]; h2[x] = c2++; }
					}
					j = (x2>>16) + hplc[d2];
					while (j < x) { p1[c1].p = y; p1[c1].n = h1[x]; h1[x] = c1++; x--; }
					while (j > x) { x++; p2[c2].p = y; p2[c2].n = h2[x]; h2[x] = c2++; }
				}
				else
				{
					plc = x2 + mulscale16((-y2)&65535,inc);
					x = (x2>>16) + hplc[d2];
					for(y=d2;y<d1;y++)
					{
						j = (plc>>16) + hplc[y]; plc += inc;
						while (j < x) { p2[c2].p = y; p2[c2].n = h2[x]; h2[x] = c2++; x--; }
						while (j > x) { x++; p1[c1].p = y; p1[c1].n = h1[x]; h1[x] = c1++; }
					}
					j = (x1>>16) + hplc[d1];
					while (j < x) { p2[c2].p = y; p2[c2].n = h2[x]; h2[x] = c2++; x--; }
					while (j > x) { x++; p1[c1].p = y; p1[c1].n = h1[x]; h1[x] = c1++; }
				}
			}
		}

		for(x=minx;x<=maxx;x++)
			for(x1=h1[x],x2=h2[x];x1!=0;x1=p1[x1].n,x2=p2[x2].n)
			{
				d1 = x1; d2 = x2;
				for(i=p1[x1].n,j=p2[x2].n;i!=0;i=p1[i].n,j=p2[j].n)
				{
					if (p1[i].p < p1[d1].p) d1 = i;
					if (p2[j].p < p2[d2].p) d2 = j;
				}
				if (p2[d2].p < p1[d1].p) diagvline((long)p2[d2].p,(long)p1[d1].p-1,x);
				p1[d1].p = p1[x1].p; p2[d2].p = p2[x2].p;
			}
	}
}

void hline (long sx, long end, long sy)      //Warning: end is 1 higher here
{
	long shade, cnt, i, x, y, xinc, yinc, d, u, v, *longptr;
	char *ptr, *dapal, dat1, dat2, dat3, dat4;
	float ft;

	longptr = (long *)(ylookup[sy]+sx+frameplace);
	shade = max(min(gshade,255),0);
	dapal = (char *)(((shade>>3)<<8)+FP_OFF(palookup[0]));

	ft = (gd+sx*gdx+sy*gdy)/8; ftol(&ft,&d);
	ft = (gu+sx*gux+sy*guy)/8; ftol(&ft,&u);
	ft = (gv+sx*gvx+sy*gvy)/8; ftol(&ft,&v);
	cnt = end-sx;

	i = krecipasm(d>>8);
	x = mulscale8(u,i);
	y = mulscale8(v,i);

	while (cnt >= 8)
	{
		d += lgdx; u += lgux; v += lgvx;
		i = krecipasm(d>>8);
		xinc = mulscale11(u,i)-(x>>3);
		yinc = mulscale11(v,i)-(y>>3);

		dat1 = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		dat2 = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		dat3 = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		dat4 = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		*longptr++ = dat1+(dat2<<8)+(dat3<<16)+(dat4<<24);
		dat1 = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		dat2 = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		dat3 = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		dat4 = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		*longptr++ = dat1+(dat2<<8)+(dat3<<16)+(dat4<<24);
		cnt -= 8;
	}

	d += lgdx; u += lgux; v += lgvx;
	i = krecipasm(d>>8);
	xinc = mulscale11(u,i)-(x>>3);
	yinc = mulscale11(v,i)-(y>>3);
	if (cnt >= 4)
	{
		dat1 = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		dat2 = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		dat3 = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		dat4 = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		*longptr++ = dat1+(dat2<<8)+(dat3<<16)+(dat4<<24);
		cnt -= 4;
	}
	ptr = (char *)longptr;
	while (cnt != 0)
	{
		*ptr++ = dapal[gbuf[((x>>16)&gmask1)+((y>>24)&gmask2)]]; x += xinc; y += yinc;
		cnt--;
	}
}

void diaghline (long sx, long end, long sy)
{
	long p, d2, shade, u, v, uu[2], vv[2];
	float d, ftemp, ft;
	char *dapal;

	ftemp = 65536*16384; d = gd+((float)(sy-hplc[0]))*gdy;
	fpustartdiv(&ftemp,&d);

	shade = ((keystatus[0x20]<<2)&sy)+gshade;
	sy -= hplc[sx]; p = ylookup[sy]+sx;

	fpugetnum(&d);

	ft = (gt*d)/4149304; ftol(&ft,&d2);
	shade = max(min(shade+d2,255),0);
	if (sx >= end)
	{
		ft = (gu+sx*gux+sy*guy)*d; ftol(&ft,&u);
		ft = (gv+sx*gvx+sy*gvy)*d; ftol(&ft,&v);
		if (option[0] == 0)
			drawpixelses((p<<2)+frameplace,colookup[shade][gbuf[((u>>16)&gmask1)+((v>>24)&gmask2)]]);
		else
		{
			dapal = (char *)(((shade>>3)<<8)+FP_OFF(palookup[0]));
			drawpixel(p+frameplace,dapal[gbuf[((u>>16)&gmask1)+((v>>24)&gmask2)]]);
		}
		return;
	}

	if ((((gmp[0]&65535) == 0) || (klabs(d2) <= 8) || (keystatus[0xb8] != 0)) && (keystatus[0x36] == 0))
	{
		ftemp = (float)(sy+hplc[sx]-hplc[0])-(float)sx*gm;
		ft = (gu+(float)sx*gux+ftemp*guy)*d; ftol(&ft,&u);
		ft = (gv+(float)sx*gvx+ftemp*gvy)*d; ftol(&ft,&v);
		ft = gxoffs*d; ftol(&ft,&uu[0]);
		ft = gyoffs*d; ftol(&ft,&vv[0]);
		uu[1] = uu[0]; vv[1] = vv[0];
	}
	else
	{
		ft = (gu+(float)sx*gux+(float)sy*guy)*d; ftol(&ft,&u);
		ft = (gv+(float)sx*gvx+(float)sy*gvy)*d; ftol(&ft,&v);
		ft = gux*d; ftol(&ft,&uu[0]);
		ft = gvx*d; ftol(&ft,&vv[0]);
		ft = guy*d; ftol(&ft,&uu[1]);
		ft = gvy*d; ftol(&ft,&vv[1]);
		if (gmp[0] < 0) { uu[1] = uu[0]+uu[1]; vv[1] = vv[0]+vv[1]; }
					  else { uu[1] = uu[0]-uu[1]; vv[1] = vv[0]-vv[1]; }
	}
	ui[0] = (uu[0]<<8); ui[1] = (uu[0]>>24);
	ui[2] = (uu[1]<<8); ui[3] = (uu[1]>>24);
	vi[0] = (vv[0]<<8); vi[1] = (vv[0]>>24);
	vi[2] = (vv[1]<<8); vi[3] = (vv[1]>>24);
	if (option[0] == 0) dline24(end,u,v,shade,sx,(p<<2)+frameplace);
						else dline(end,u,v,shade>>3,sx,p+frameplace);
}

void diagvline (long sy, long end, long sx)
{
	long p, d2, shade, u, v, uu[2], vv[2];
	float d, ftemp, ft;
	char *dapal;

	ftemp = 65536*16384; d = gd+((float)(sx-hplc[0]))*gdx;
	fpustartdiv(&ftemp,&d);

	shade = ((keystatus[0x20]<<2)&sx)+gshade;
	sx -= hplc[sy]; p = ylookup[sy]+sx;

	fpugetnum(&d);

	ft = (gt*d)/4149304; ftol(&ft,&d2);
	shade = max(min(shade+d2,255),0);
	if (sy >= end)
	{
		ft = (gu+sx*gux+sy*guy)*d; ftol(&ft,&u);
		ft = (gv+sx*gvx+sy*gvy)*d; ftol(&ft,&v);
		if (option[0] == 0)
			drawpixelses((p<<2)+frameplace,colookup[shade][gbuf[((u>>16)&gmask1)+((v>>24)&gmask2)]]);
		else
		{
			dapal = (char *)(((shade>>3)<<8)+FP_OFF(palookup[0]));
			drawpixel(p+frameplace,dapal[gbuf[((u>>16)&gmask1)+((v>>24)&gmask2)]]);
		}
		return;
	}

	if ((((gmp[0]&65535) == 0) || (klabs(d2) <= 8) || (keystatus[0xb8] != 0)) && (keystatus[0x36] == 0))
	{
		ftemp = (float)(sx+hplc[sy]-hplc[0])-(float)sy*gm;
		ft = (gu+ftemp*gux+(float)sy*guy)*d; ftol(&ft,&u);
		ft = (gv+ftemp*gvx+(float)sy*gvy)*d; ftol(&ft,&v);
		ft = gxoffs*d; ftol(&ft,&uu[0]);
		ft = gyoffs*d; ftol(&ft,&vv[0]);
		uu[1] = uu[0]; vv[1] = vv[0];
	}
	else
	{
		ft = (gu+(float)sx*gux+(float)sy*guy)*d; ftol(&ft,&u);
		ft = (gv+(float)sx*gvx+(float)sy*gvy)*d; ftol(&ft,&v);
		ft = guy*d; ftol(&ft,&uu[0]);
		ft = gvy*d; ftol(&ft,&vv[0]);
		ft = gux*d; ftol(&ft,&uu[1]);
		ft = gvx*d; ftol(&ft,&vv[1]);
		if (gmp[0] < 0) { uu[1] = uu[0]+uu[1]; vv[1] = vv[0]+vv[1]; }
					  else { uu[1] = uu[0]-uu[1]; vv[1] = vv[0]-vv[1]; }
	}
	ui[0] = (uu[0]<<8); ui[1] = (uu[0]>>24);
	ui[2] = (uu[1]<<8); ui[3] = (uu[1]>>24);
	vi[0] = (vv[0]<<8); vi[1] = (vv[0]>>24);
	vi[2] = (vv[1]<<8); vi[3] = (vv[1]>>24);
	if (option[0] == 0) dline24(end,u,v,shade,sy,(p<<2)+frameplace);
						else dline(end,u,v,shade>>3,sy,p+frameplace);
}

void initmesh ()
{
	long i;

	pnum = 4;
	bn[0].x = -1; bn[0].y = 0; bn[0].z = -1;           //left
	bn[1].x = 1; bn[1].y = 0; bn[1].z = -1;            //right
	bn[2].x = 0; bn[2].y = -1; bn[2].z = -ydimscale;   //top
	bn[3].x = 0; bn[3].y = 1; bn[3].z = -ydimscale;    //bottom

	for(i=0;i<pnum;i++)
	{
		backrotate3d(bn[i].x,bn[i].y,bn[i].z);
		front[i] = -1; back[i] = i+1;
	}
	back[pnum-1] = -2;

	globalregioncnt = 0;
	globalsplitcnt = 0;
}

void addmesh (short bsphead)
{
	long i, j, k, p, pcnt, *longptr, *ftp;
	float x, y, z, x0, x1, y0, y1, z0, z1, ft;

	for(i=numedges-1;i>=0;i--) edgetag[i] = (i|0x8000);

			//Get bounding box
	ftp = (long *)&ft;
	x0=x1=r[0].x; y0=y1=r[0].y; z0=z1=r[0].z;
	for(i=numpoints-1;i>0;i--)
	{
		ft = r[i].x-x0; if (*ftp < 0) x0 = r[i].x;
		ft = r[i].y-y0; if (*ftp < 0) y0 = r[i].y;
		ft = r[i].z-z0; if (*ftp < 0) z0 = r[i].z;
		ft = r[i].x-x1; if (*ftp > 0) x1 = r[i].x;
		ft = r[i].y-y1; if (*ftp > 0) y1 = r[i].y;
		ft = r[i].z-z1; if (*ftp > 0) z1 = r[i].z;
	}

	ecnt = 0;
	for(meshtag[0]=bsphead,pcnt=0;pcnt<nummeshs;pcnt++)
	{
		p = meshtag[pcnt];

		if (p < 0)
		{
			if (p == -2) elist[ecnt++] = pcnt;
			continue;
		}

			//Oops!  Dead branch
		if ((front[p] < 0) && (back[p] == -1)) continue;

			//Bounding box code
		x = bn[p].x; y = bn[p].y; z = bn[p].z;
		longptr = (long *)&bn[p].x;

		ft = 0;
		if (longptr[0] < 0) ft += x*x1; else if (longptr[0] != 0) ft += x*x0;
		if (longptr[1] < 0) ft += y*y1; else if (longptr[1] != 0) ft += y*y0;
		if (longptr[2] < 0) ft += z*z1; else if (longptr[2] != 0) ft += z*z0;
		if (*ftp >= 0) { meshtag[pcnt--] = front[p]; continue; }

		ft = 0;
		if (longptr[0] < 0) ft += x*x0; else if (longptr[0] != 0) ft += x*x1;
		if (longptr[1] < 0) ft += y*y0; else if (longptr[1] != 0) ft += y*y1;
		if (longptr[2] < 0) ft += z*z0; else if (longptr[2] != 0) ft += z*z1;
		if (*ftp <= 0) { meshtag[pcnt--] = back[p]; continue; }

			//Bounding box fails
		if (front[p] < 0)
		{
			meshtag[nummeshs] = back[p];
			splitmesh1(p,pcnt,-x,-y,-z);
		}
		else if (back[p] == -1)    //Already filled region
		{
			meshtag[nummeshs] = front[p];
			splitmesh1(p,pcnt,x,y,z);
		}
		else
		{
			//meshtag[nummeshs] = front[p];
			//splitmesh1(p,pcnt,x,y,z);
			//if (mesh[nummeshs] == mesh[nummeshs-1]) nummeshs--;
			//meshtag[nummeshs] = back[p];
			//splitmesh1(p,pcnt,-x,-y,-z);

			meshtag[nummeshs+0] = front[p];
			meshtag[nummeshs+1] = back[p];
			splitmesh2(p,pcnt,x,y,z);
			if (mesh[nummeshs-1] == mesh[nummeshs-2])
			{
				mesh[nummeshs-1] = mesh[nummeshs];
				nummeshs--;
				meshtag[nummeshs-1] = meshtag[nummeshs];
			}
		}
		if (mesh[nummeshs] == mesh[nummeshs-1]) nummeshs--;
	}
}

void add2bsp (long plistnum)
{
	long i, j, k;
	point3d *rp1, *rp2;
	//long z, oz, p, startpnum, *t1p, *t2p;
	//float t1, t2;

	for(i=plistnum-1;i>=0;i--)
	{
		j = plist[i]; k = (j&4095);
		if (j&0x8000)
		{
			if (j&0x2000) { rp1 = &r[edge[k].p1]; rp2 = &r[edge[k].p2]; }
						else { rp1 = &r[edge[k].p2]; rp2 = &r[edge[k].p1]; }
			bn[pnum].x = rp1->y*rp2->z - rp2->y*rp1->z;
			bn[pnum].y = rp2->x*rp1->z - rp1->x*rp2->z;
			bn[pnum].z = rp1->x*rp2->y - rp2->x*rp1->y;
		}
		else
		{
			bn[pnum].x = bn[k].x;
			bn[pnum].y = bn[k].y;
			bn[pnum].z = bn[k].z;
		}
		front[pnum] = -1; back[pnum] = pnum+1;
		pnum++;
	}
	back[pnum-1] = -2;

	/*
	startpnum = pnum; p = pnum+1;

	t1p = (long *)&t1; t2p = (long *)&t2;

	for(i=plistnum-1;i>=0;i--)
	{
		j = plist[i]; k = (j&4095);
		if (j&0x8000)
		{
			if (j&0x2000) { rp1 = &r[edge[k].p1]; rp2 = &r[edge[k].p2]; }
						else { rp1 = &r[edge[k].p2]; rp2 = &r[edge[k].p1]; }
			bn[pnum].x = rp1->y*rp2->z - rp2->y*rp1->z;
			bn[pnum].y = rp2->x*rp1->z - rp1->x*rp2->z;
			bn[pnum].z = rp1->x*rp2->y - rp2->x*rp1->y;
		}
		else
		{
			bn[pnum].x = bn[k].x;
			bn[pnum].y = bn[k].y;
			bn[pnum].z = bn[k].z;
				//What about rp1 and rp2!!!
		}

		front[pnum] = -1; back[pnum] = -2; pt[pnum] = startpnum;
		pnum++;

		while (p < pnum)
		{
			z = pt[p];
			while (z >= 0)
			{
				oz = z;
				t1 = bn[z].x*rp1->x + bn[z].y*rp1->y + bn[z].z*rp1->z;
				t2 = bn[z].x*rp2->x + bn[z].y*rp2->y + bn[z].z*rp2->z;

				if ((t1p[0]|t2p[0]) >= 0)
				{
					if ((t1p[0]|t2p[0]) == 0) { pnum--; goto breakadd2bsploop; }
					if ((z=front[z]) < 0) front[oz] = p;
				}
				else if ((t1p[0] <= 0) && (t2p[0] <= 0))
					{ if ((z=back[z]) < 0) back[oz] = p; }
				else
				{
					copybuf(&bn[p],&bn[pnum],3);
					front[pnum] = -1; back[pnum] = -2;

					if ((pt[pnum]=back[z]) < 0) back[z] = pnum;
					pnum++;
					if ((z=front[z]) < 0) front[oz] = p;
				}
			}
			p++;
		}
breakadd2bsploop:
	}*/
}

void splitmesh1 (long splitval, long meshnum, float fnx, float fny, float fnz)
{
	long i, j, z, zz, p, pp, splitcnt1, pstart, pend, gap, i1, i2;
	long *lt1, *lt2, *nxp, *nyp, *nzp;
	float t, t1, t2;
	short sval, a, aa;

	nxp = (long *)&fnx; nyp = (long *)&fny; nzp = (long *)&fnz;
	lt1 = (long *)&t1; lt2 = (long *)&t2;

	globalsplitcnt++;
	pstart = mesh[meshnum]; pend = mesh[meshnum+1];

	clearbuf(got,(numpoints+3)>>2,0L);

	splitcnt1 = 0;
	for(z=pstart;z<pend;z++)
	{
		p = edge[z].p1; pp = edge[z].p2;
		a = edge[z].a1; aa = edge[z].a2;
		if (!got[p]) tval[p] = r[p].x*fnx + r[p].y*fny + r[p].z*fnz, got[p] = 1;
		if (!got[pp]) tval[pp] = r[pp].x*fnx + r[pp].y*fny + r[pp].z*fnz, got[pp] = 1;
		t1 = tval[p]; t2 = tval[pp];
		if (*lt1 > 0)
		{
			if (*lt2 >= 0)                      //t1>0,t2>=0
			{
				edge[numedges].p1 = p; edge[numedges].p2 = pp;
				edge[numedges].a1 = a; edge[numedges].a2 = aa;
				if (*lt2 == 0)
				{
					split1[splitcnt1] = pp; splitag1[splitcnt1++] = a;
					if (aa >= 0) { split1[splitcnt1] = pp; splitag1[splitcnt1++] = aa; }
				}
				edgetag[numedges++] = edgetag[z];
			}
			else
			{                                 //t1>0,t2<0
				t = t1/(t1-t2);
				edge[numedges].p1 = p; edge[numedges].p2 = numpoints;
				edge[numedges].a1 = a; edge[numedges].a2 = aa;

				split1[splitcnt1] = numpoints; splitag1[splitcnt1++] = a;
				if (aa >= 0) { split1[splitcnt1] = numpoints; splitag1[splitcnt1++] = aa; }

				edgetag[numedges++] = edgetag[z];

				r[numpoints].x = r[p].x + (r[pp].x-r[p].x)*t;
				r[numpoints].y = r[p].y + (r[pp].y-r[p].y)*t;
				r[numpoints].z = r[p].z + (r[pp].z-r[p].z)*t;
				numpoints++;
			}
		}
		else if (*lt1 != 0)
		{
			if (*lt2 > 0)
			{                                 //t1<0,t2>0
				t = t1/(t1-t2);
				edge[numedges].p1 = numpoints; edge[numedges].p2 = pp;
				edge[numedges].a1 = a; edge[numedges].a2 = aa;

				split1[splitcnt1] = numpoints; splitag1[splitcnt1++] = a;
				if (aa >= 0) { split1[splitcnt1] = numpoints; splitag1[splitcnt1++] = aa; }

				edgetag[numedges++] = edgetag[z];
				r[numpoints].x = r[p].x + (r[pp].x-r[p].x)*t;
				r[numpoints].y = r[p].y + (r[pp].y-r[p].y)*t;
				r[numpoints].z = r[p].z + (r[pp].z-r[p].z)*t;
				numpoints++;
			}
		}
		else if (*lt2 > 0)                     //t1=0,t2>0
		{
			edge[numedges].p1 = p; edge[numedges].p2 = pp;
			edge[numedges].a1 = a; edge[numedges].a2 = aa;

			split1[splitcnt1] = p; splitag1[splitcnt1++] = a;
			if (aa >= 0) { split1[splitcnt1] = p; splitag1[splitcnt1++] = aa; }

			edgetag[numedges++] = edgetag[z];
		}
	}

		/*if (((nxp[0]&0x7fffffff) >= (nyp[0]&0x7fffffff)) && ((nxp[0]&0x7fffffff) >= (nzp[0]&0x7fffffff)))
		{
			j = (nxp[0] > 0);
			for(gap=splitcnt1>>1;gap>0;gap>>=1)
				for(z=0;z<splitcnt1-gap;z++)
				{
					zz = z; i1 = split1[zz]; i2 = split1[zz+gap];
					while ((r[i1].y*r[i2].z < r[i1].z*r[i2].y) == j)
					{
						i = split1[zz]; split1[zz] = split1[zz+gap]; split1[zz+gap] = i;
						i = splitag1[zz]; splitag1[zz] = splitag1[zz+gap]; splitag1[zz+gap] = i;
						zz -= gap; if (zz < 0) break;
						i2 = i1; i1 = split1[zz];
					}
				}
		}
		else if ((nyp[0]&0x7fffffff) >= (nzp[0]&0x7fffffff))
		{
			j = (nyp[0] > 0);
			for(gap=splitcnt1>>1;gap>0;gap>>=1)
				for(z=0;z<splitcnt1-gap;z++)
				{
					zz = z; i1 = split1[zz]; i2 = split1[zz+gap];
					while ((r[i1].z*r[i2].x < r[i1].x*r[i2].z) == j)
					{
						i = split1[zz]; split1[zz] = split1[zz+gap]; split1[zz+gap] = i;
						i = splitag1[zz]; splitag1[zz] = splitag1[zz+gap]; splitag1[zz+gap] = i;
						zz -= gap; if (zz < 0) break;
						i2 = i1; i1 = split1[zz];
					}
				}
		}
		else
		{
			j = (nzp[0] > 0);
			for(gap=splitcnt1>>1;gap>0;gap>>=1)
				for(z=0;z<splitcnt1-gap;z++)
				{
					zz = z; i1 = split1[zz]; i2 = split1[zz+gap];
					while ((r[i1].x*r[i2].y < r[i1].y*r[i2].x) == j)
					{
						i = split1[zz]; split1[zz] = split1[zz+gap]; split1[zz+gap] = i;
						i = splitag1[zz]; splitag1[zz] = splitag1[zz+gap]; splitag1[zz+gap] = i;
						zz -= gap; if (zz < 0) break;
						i2 = i1; i1 = split1[zz];
					}
				}
		}*/

	for(z=splitcnt1-1;z>0;z--)
	{
		sval = splitag1[z];
		if (sval >= 0)
		{
			for(zz=z-1;zz>=0;zz--) if (splitag1[zz] == sval) break;
			splitag1[zz] = -1;

			i1 = split1[z]; i2 = split1[zz];
			if (((nxp[0]&0x7fffffff) >= (nyp[0]&0x7fffffff)) && ((nxp[0]&0x7fffffff) >= (nzp[0]&0x7fffffff)))
				{ i = ((r[i1].y*r[i2].z < r[i1].z*r[i2].y) == (nxp[0] > 0)); }
			else if ((nyp[0]&0x7fffffff) >= (nzp[0]&0x7fffffff))
				{ i = ((r[i1].z*r[i2].x < r[i1].x*r[i2].z) == (nyp[0] > 0)); }
			else
				{ i = ((r[i1].x*r[i2].y < r[i1].y*r[i2].x) == (nzp[0] > 0)); }

			if (i == 0) { edge[numedges].p1 = i1; edge[numedges].p2 = i2; }
					 else { edge[numedges].p1 = i2; edge[numedges].p2 = i1; }

			edge[numedges].a1 = sval;
			edge[numedges].a2 = -1;
			edgetag[numedges++] = (splitval|0x4000);  //WARNING: |0x4000 for splitmesh1 only!
		}
	}
	mesh[++nummeshs] = numedges;
}

void splitmesh2 (long splitval, long meshnum, float fnx, float fny, float fnz)
{
	long i, j, z, zz, p, pp, splitcnt1, splitcnt2, pstart, pend, i1, i2;
	long *lt1, *lt2, *nxp, *nyp, *nzp, numedges2;
	float t, t1, t2;
	short sval, a, aa;

	nxp = (long *)&fnx; nyp = (long *)&fny; nzp = (long *)&fnz;
	lt1 = (long *)&t1; lt2 = (long *)&t2;

	globalsplitcnt++;
	pstart = mesh[meshnum]; pend = mesh[meshnum+1];

	clearbuf(got,(numpoints+3)>>2,0L);

	numedges2 = MAXPOINTS-1; splitcnt1 = 0; splitcnt2 = 0;
	for(z=pstart;z<pend;z++)
	{
		p = edge[z].p1; pp = edge[z].p2;
		a = edge[z].a1; aa = edge[z].a2;
		if (!got[p]) tval[p] = r[p].x*fnx + r[p].y*fny + r[p].z*fnz, got[p] = 1;
		if (!got[pp]) tval[pp] = r[pp].x*fnx + r[pp].y*fny + r[pp].z*fnz, got[pp] = 1;
		t1 = tval[p]; t2 = tval[pp];
		if (*lt1 > 0)
		{
			if (*lt2 >= 0)                      //t1>0,t2>=0
			{
				edge[numedges].p1 = p; edge[numedges].p2 = pp;
				edge[numedges].a1 = a; edge[numedges].a2 = aa;
				if (*lt2 == 0)
				{
					split1[splitcnt1] = pp; splitag1[splitcnt1++] = a;
					if (aa >= 0) { split1[splitcnt1] = pp; splitag1[splitcnt1++] = aa; }
				}
				edgetag[numedges++] = edgetag[z];
			}
			else
			{                                 //t1>0,t2<0
				t = t1 / (t1-t2);
				edge[numedges].p1 = p; edge[numedges].p2 = numpoints;
				edge[numedges].a1 = a; edge[numedges].a2 = aa;
				edge[numedges2].p1 = numpoints; edge[numedges2].p2 = pp;
				edge[numedges2].a1 = a; edge[numedges2].a2 = aa;
				split1[splitcnt1] = numpoints; splitag1[splitcnt1++] = a;
				split2[splitcnt2] = numpoints; splitag2[splitcnt2++] = a;
				if (aa >= 0)
				{
					split1[splitcnt1] = numpoints; splitag1[splitcnt1++] = aa;
					split2[splitcnt2] = numpoints; splitag2[splitcnt2++] = aa;
				}
				edgetag[numedges++] = edgetag[z];
				edgetag[numedges2--] = edgetag[z];
				r[numpoints].x = r[p].x + (r[pp].x-r[p].x)*t;
				r[numpoints].y = r[p].y + (r[pp].y-r[p].y)*t;
				r[numpoints].z = r[p].z + (r[pp].z-r[p].z)*t;
				numpoints++;
			}
		}
		else if (*lt1 != 0)
		{
			if (*lt2 <= 0)                      //t1<0,t2<=0
			{
				edge[numedges2].p1 = p; edge[numedges2].p2 = pp;
				edge[numedges2].a1 = a; edge[numedges2].a2 = aa;
				if (*lt2 == 0)
				{
					split2[splitcnt2] = pp; splitag2[splitcnt2++] = a;
					if (aa >= 0) { split2[splitcnt2] = pp; splitag2[splitcnt2++] = aa; }
				}
				edgetag[numedges2--] = edgetag[z];
			}
			else
			{                                 //t1<0,t2>0
				t = t1 / (t1-t2);
				edge[numedges].p1 = numpoints; edge[numedges].p2 = pp;
				edge[numedges].a1 = a; edge[numedges].a2 = aa;
				edge[numedges2].p1 = p; edge[numedges2].p2 = numpoints;
				edge[numedges2].a1 = a; edge[numedges2].a2 = aa;
				split1[splitcnt1] = numpoints; splitag1[splitcnt1++] = a;
				split2[splitcnt2] = numpoints; splitag2[splitcnt2++] = a;
				if (aa >= 0)
				{
					split1[splitcnt1] = numpoints; splitag1[splitcnt1++] = aa;
					split2[splitcnt2] = numpoints; splitag2[splitcnt2++] = aa;
				}
				edgetag[numedges++] = edgetag[z];
				edgetag[numedges2--] = edgetag[z];
				r[numpoints].x = r[p].x + (r[pp].x-r[p].x)*t;
				r[numpoints].y = r[p].y + (r[pp].y-r[p].y)*t;
				r[numpoints].z = r[p].z + (r[pp].z-r[p].z)*t;
				numpoints++;
			}
		}
		else
		{
			if (*lt2 > 0)                       //t1=0,t2>0
			{
				edge[numedges].p1 = p; edge[numedges].p2 = pp;
				edge[numedges].a1 = a; edge[numedges].a2 = aa;
				split1[splitcnt1] = p; splitag1[splitcnt1++] = a;
				if (aa >= 0) { split1[splitcnt1] = p; splitag1[splitcnt1++] = aa; }
				edgetag[numedges++] = edgetag[z];
			}
			else if (*lt2 != 0)                 //t1=0,t2<0
			{
				edge[numedges2].p1 = p; edge[numedges2].p2 = pp;
				edge[numedges2].a1 = a; edge[numedges2].a2 = aa;
				split2[splitcnt2] = p; splitag2[splitcnt2++] = a;
				if (aa >= 0) { split2[splitcnt2] = p; splitag2[splitcnt2++] = aa; }
				edgetag[numedges2--] = edgetag[z];
			}
		}
	}

	for(z=splitcnt1-1;z>0;z--)
	{
		sval = splitag1[z];
		if (sval >= 0)
		{
			for(zz=z-1;zz>=0;zz--) if (splitag1[zz] == sval) break;
			splitag1[zz] = -1;

			i1 = split1[z]; i2 = split1[zz];
			if (((nxp[0]&0x7fffffff) >= (nyp[0]&0x7fffffff)) && ((nxp[0]&0x7fffffff) >= (nzp[0]&0x7fffffff)))
				{ i = ((r[i1].y*r[i2].z < r[i1].z*r[i2].y) == (nxp[0] > 0)); }
			else if ((nyp[0]&0x7fffffff) >= (nzp[0]&0x7fffffff))
				{ i = ((r[i1].z*r[i2].x < r[i1].x*r[i2].z) == (nyp[0] > 0)); }
			else
				{ i = ((r[i1].x*r[i2].y < r[i1].y*r[i2].x) == (nzp[0] > 0)); }

			if (i == 0) { edge[numedges].p1 = i1; edge[numedges].p2 = i2; }
					 else { edge[numedges].p1 = i2; edge[numedges].p2 = i1; }

			edge[numedges].a1 = sval;
			edge[numedges].a2 = -1;
			edgetag[numedges++] = splitval;
		}
	}
	mesh[++nummeshs] = numedges;


		//Reverse points of back side!
	for(z=MAXPOINTS-1;z>numedges2;z--)
	{
		copybuf(&edge[z],&edge[numedges],2);
		edgetag[numedges++] = edgetag[z];
	}

	for(z=splitcnt2-1;z>0;z--)
	{
		sval = splitag2[z];
		if (sval >= 0)
		{
			for(zz=z-1;zz>=0;zz--) if (splitag2[zz] == sval) break;
			splitag2[zz] = -1;

			i1 = split2[z]; i2 = split2[zz];
			if (((nxp[0]&0x7fffffff) >= (nyp[0]&0x7fffffff)) && ((nxp[0]&0x7fffffff) >= (nzp[0]&0x7fffffff)))
				{ i = ((r[i1].y*r[i2].z < r[i1].z*r[i2].y) == (nxp[0] < 0)); }
			else if ((nyp[0]&0x7fffffff) >= (nzp[0]&0x7fffffff))
				{ i = ((r[i1].z*r[i2].x < r[i1].x*r[i2].z) == (nyp[0] < 0)); }
			else
				{ i = ((r[i1].x*r[i2].y < r[i1].y*r[i2].x) == (nzp[0] < 0)); }

			if (i == 0) { edge[numedges].p1 = i1; edge[numedges].p2 = i2; }
					 else { edge[numedges].p1 = i2; edge[numedges].p2 = i1; }

			edge[numedges].a1 = sval;
			edge[numedges].a2 = -1;
			edgetag[numedges++] = splitval;
		}
	}
	mesh[++nummeshs] = numedges;
}

long krand ()
{
	randomseed = ((randomseed*21+1)&65535);
	return(randomseed);
}

void draw3dclipline (float fx0, float fy0, float fz0, float fx1, float fy1, float fz1, char col)
{
	long sx0, sy0, sx1, sy1;
	float ftemp;

	rotate3d(fx0,fy0,fz0);
	rotate3d(fx1,fy1,fz1);

	if ((fz0 > 0) || (fz1 > 0))
	{
		if (clipline(&fx0,&fy0,&fz0,&fx1,&fy1,&fz1) == 1)
		{
			if ((fz0 > 0) && (fz1 > 0))
			{
				ftemp = xreszoom / fz0;
				fx0 *= ftemp; ftol(&fx0,&sx0);
				fy0 *= ftemp; ftol(&fy0,&sy0);
				sx0 = min(max(sx0+xdimup15,0),(xdim<<16)-1);
				sy0 = min(max(sy0+ydimup15,0),(ydim<<16)-1);

				ftemp = xreszoom / fz1;
				fx1 *= ftemp; ftol(&fx1,&sx1);
				fy1 *= ftemp; ftol(&fy1,&sy1);
				sx1 = min(max(sx1+xdimup15,0),(xdim<<16)-1);
				sy1 = min(max(sy1+ydimup15,0),(ydim<<16)-1);

				drawline256(sx0,sy0,sx1,sy1,col);
			}
		}
	}
}

long clipline (float *x1, float *y1, float *z1, float *x2, float *y2, float *z2)
{
	float t, t1, t2;
	long *t1p, *t2p;

	t1p = (long *)&t1;
	t2p = (long *)&t2;

	t1 = *z1 + *x1; t2 = *z2 + *x2;
	if (((*t1p)&(*t2p)) < 0) return(0);
	if (((*t1p)^(*t2p)) < 0)
	{
		t = t1/(t1-t2);
		if (*t1p < 0) { *x1 = (*x2-*x1)*t+*x1; *y1 = (*y2-*y1)*t+*y1; *z1 = (*z2-*z1)*t+*z1; }
					else { *x2 = (*x2-*x1)*t+*x1; *y2 = (*y2-*y1)*t+*y1; *z2 = (*z2-*z1)*t+*z1; }
	}

	t1 = *z1 - *x1; t2 = *z2 - *x2;
	if (((*t1p)&(*t2p)) < 0) return(0);
	if (((*t1p)^(*t2p)) < 0)
	{
		t = t1/(t1-t2);
		if (*t1p < 0) { *x1 = (*x2-*x1)*t+*x1; *y1 = (*y2-*y1)*t+*y1; *z1 = (*z2-*z1)*t+*z1; }
					else { *x2 = (*x2-*x1)*t+*x1; *y2 = (*y2-*y1)*t+*y1; *z2 = (*z2-*z1)*t+*z1; }
	}

	t1 = *z1*ydimscale + *y1; t2 = *z2*ydimscale + *y2;
	if (((*t1p)&(*t2p)) < 0) return(0);
	if (((*t1p)^(*t2p)) < 0)
	{
		t = t1/(t1-t2);
		if (*t1p < 0) { *x1 = (*x2-*x1)*t+*x1; *y1 = (*y2-*y1)*t+*y1; *z1 = (*z2-*z1)*t+*z1; }
					else { *x2 = (*x2-*x1)*t+*x1; *y2 = (*y2-*y1)*t+*y1; *z2 = (*z2-*z1)*t+*z1; }
	}

	t1 = *z1*ydimscale - *y1; t2 = *z2*ydimscale - *y2;
	if (((*t1p)&(*t2p)) < 0) return(0);
	if (((*t1p)^(*t2p)) < 0)
	{
		t = t1/(t1-t2);
		if (*t1p < 0) { *x1 = (*x2-*x1)*t+*x1; *y1 = (*y2-*y1)*t+*y1; *z1 = (*z2-*z1)*t+*z1; }
					else { *x2 = (*x2-*x1)*t+*x1; *y2 = (*y2-*y1)*t+*y1; *z2 = (*z2-*z1)*t+*z1; }
	}

	return(1);
}

void drawline256 (long x1, long y1, long x2, long y2, char col)
{
	long dx, dy, i, j, p, inc, plc, daend, dacol;
	long wx1 = 0L, wy1 = 0L, wx2 = (xdim<<12), wy2 = (ydim<<12);

	if (option[0] == 0)
		dacol = colookup[0][col];
	else
		dacol = palookup[0][col];

	x1 >>= 4; y1 >>= 4; x2 >>= 4; y2 >>= 4;

	dx = x2-x1; dy = y2-y1;
	if (dx >= 0)
	{
		if ((x1 >= wx2) || (x2 < wx1)) return;
		if (x1 < wx1) y1 += scale(wx1-x1,dy,dx), x1 = wx1;
		if (x2 > wx2) y2 += scale(wx2-x2,dy,dx), x2 = wx2;
	}
	else
	{
		if ((x2 >= wx2) || (x1 < wx1)) return;
		if (x2 < wx1) y2 += scale(wx1-x2,dy,dx), x2 = wx1;
		if (x1 > wx2) y1 += scale(wx2-x1,dy,dx), x1 = wx2;
	}
	if (dy >= 0)
	{
		if ((y1 >= wy2) || (y2 < wy1)) return;
		if (y1 < wy1) x1 += scale(wy1-y1,dx,dy), y1 = wy1;
		if (y2 > wy2) x2 += scale(wy2-y2,dx,dy), y2 = wy2;
	}
	else
	{
		if ((y2 >= wy2) || (y1 < wy1)) return;
		if (y2 < wy1) x2 += scale(wy1-y2,dx,dy), y2 = wy1;
		if (y1 > wy2) x1 += scale(wy2-y1,dx,dy), y1 = wy2;
	}

	if (klabs(dx) >= klabs(dy))
	{
		if (dx == 0) return;
		if (dx < 0)
		{
			i = x1; x1 = x2; x2 = i;
			i = y1; y1 = y2; y2 = i;
		}

		inc = divscale12(dy,dx);
		plc = y1+mulscale12((2047-x1)&4095,inc);
		i = ((x1+2048)>>12); daend = ((x2+2048)>>12);
		if (option[0] == 0)
		{
			for(;i<daend;i++)
			{
				p = ylookup[plc>>12]+i;
				drawpixelses((p<<2)+frameplace,dacol);
				plc += inc;
			}
		}
		else
		{
			for(;i<daend;i++)
			{
				p = ylookup[plc>>12]+i;
				drawpixel(p+frameplace,dacol);
				plc += inc;
			}
		}
	}
	else
	{
		if (dy < 0)
		{
			i = x1; x1 = x2; x2 = i;
			i = y1; y1 = y2; y2 = i;
		}

		inc = divscale12(dx,dy);
		plc = x1+mulscale12((2047-y1)&4095,inc);
		i = ((y1+2048)>>12); daend = ((y2+2048)>>12);
		p = ylookup[i];
		if (option[0] == 0)
		{
			for(;i<daend;i++)
			{
				drawpixelses((((plc>>12)+p)<<2)+frameplace,dacol);
				plc += inc; p += xdim;
			}
		}
		else
		{
			for(;i<daend;i++)
			{
				drawpixel((plc>>12)+p+frameplace,dacol);
				plc += inc; p += xdim;
			}
		}
	}
}

void printext256 (long xpos, long ypos, long col, long backcol, char name[82], char fontsize)
{
	long stx, zx, zxbitoffs, x, y, charxsiz, dat, dacol, dastrlen, p;
	char *fontptr, fontdat, pow2, pow2start;

	stx = xpos;

	if (fontsize) { charxsiz = 4; zxbitoffs = 1; fontptr = smalltextfont;}
				else { charxsiz = 8; zxbitoffs = 0; fontptr = textfont;}

	pow2start = (1<<(7-zxbitoffs));

	dastrlen = strlen(name);
	for(zx=0;zx<dastrlen;zx++)
	{
		dat = (name[zx]<<3);
		for(y=0;y<8;y++)
		{
			fontdat = fontptr[dat+y];
			p = ylookup[y+ypos]+stx-zxbitoffs;
			pow2 = pow2start;
			if (option[0] == 0)
			{
				for(x=charxsiz-1;x>=0;x--)
				{
					if (pow2&fontdat) drawpixelses((p<<2)+frameplace,col);
					else if (backcol >= 0) drawpixelses((p<<2)+frameplace,backcol);
					pow2 >>= 1; p++;
				}
			}
			else
			{
				p += frameplace;
				for(x=charxsiz-1;x>=0;x--)
				{
					if (pow2&fontdat) drawpixel(p,col);
					else if (backcol >= 0) drawpixel(p,backcol);
					pow2 >>= 1; p++;
				}
			}
		}
		stx += charxsiz;
	}
}

void loadtile (short tnum)
{
	char *ptr, *ptr2;
	long i, j, k, l, xsiz, ysiz, damip;

	if ((tilesizx[tnum] <= 0) || (tilesizy[tnum] <= 0)) return;
	if (tilesizy[tnum] > 256) return;

	i = tilefilenum[tnum];
	if (i != artfilnum)
	{
		if (artfil != -1) close(artfil);
		artfilnum = i;
		artfilplc = 0L;

		artfilename[7] = (i%10)+48;
		artfilename[6] = ((i/10)%10)+48;
		artfilename[5] = ((i/100)%10)+48;
		artfil = open(artfilename,O_BINARY|O_RDONLY,S_IREAD);
	}

	xsiz = tilesizx[tnum];
	ysiz = tilesizy[tnum]; if (ysiz > 256) ysiz = 256;

	if (waloff[0][tnum] == 0) cachebox(&waloff[0][tnum],xsiz,ysiz);

	if (artfilplc != tilefileoffs[tnum])
	{
		artfilplc = tilefileoffs[tnum];
		lseek(artfil,artfilplc,SEEK_SET);
	}
	ptr = (char *)waloff[0][tnum];

	l = 16;
	if (ysiz <= 128)
	{
		l = 32;
		if (ysiz <= 64)
		{
			l = 64;
			if (ysiz <= 32) l = 128;
		}
	}
	for(i=0;i<xsiz;i+=l)
	{
		j = min(xsiz-i,l);
		read(artfil,tempbuf,j*ysiz);

		k = FP_OFF(tempbuf);
		while (j > 0)
		{
			copybufbyte((void *)k,ptr,ysiz);
			j--; k += ysiz; ptr += 256;
		}
	}
	artfilplc += xsiz*ysiz;

	for(damip=1;damip<4;damip++)
	{
		xsiz >>= 1; ysiz >>= 1;

		if (waloff[damip][tnum] == 0) cachebox(&waloff[damip][tnum],xsiz,ysiz);

		ptr = (char *)waloff[damip-1][tnum];
		ptr2 = (char *)waloff[damip][tnum];

		for(i=0;i<xsiz;i++)
			for(j=0;j<ysiz;j++)
			{
				k = (i<<8)+j; l = (k<<1);
				ptr2[k] = transluc[transluc[ptr[l+0]+(((long)ptr[l+256])<<8)]+
							  (((long)transluc[ptr[l+1]+(((long)ptr[l+257])<<8)])<<8)];
			}
	}
}

long loadpics (char *filename)
{
	long i, j, k, fil, offscount, siz, templongbuf[4];
	long lstart, lend;

	strcpy(artfilename,filename);

	for(i=0;i<MAXTILES;i++)
	{
		tilesizx[i] = 0L; tilesizy[i] = 0L; picanm[i] = 0L;
		for(j=0;j<4;j++) waloff[j][i] = 0;
	}

	artsize = 0L;

	numtilefiles = 0;
	do
	{
		k = numtilefiles;

		artfilename[7] = (k%10)+48;
		artfilename[6] = ((k/10)%10)+48;
		artfilename[5] = ((k/100)%10)+48;
		if ((fil = open(artfilename,O_BINARY|O_RDONLY,S_IREAD)) != -1)
		{
			read(fil,templongbuf,16);
			artversion = templongbuf[0]; if (artversion != 1) return(-1);
			numtiles = templongbuf[1];
			lstart = templongbuf[2]; lend = templongbuf[3];
			read(fil,tempshort,(lend-lstart+1)<<1);
			for(i=lend-lstart;i>=0;i--) tilesizx[i+lstart] = tempshort[i];
			read(fil,tempshort,(lend-lstart+1)<<1);
			for(i=lend-lstart;i>=0;i--) tilesizy[i+lstart] = tempshort[i];
			read(fil,&picanm[lstart],(lend-lstart+1)<<2);

			offscount = 4+4+4+4+((lend-lstart+1)<<3);
			for(i=lstart;i<=lend;i++)
			{
				tilefilenum[i] = k;
				tilefileoffs[i] = offscount;
				j = tilesizx[i]*tilesizy[i];
				offscount += j; artsize += j;
			}
			close(fil);

			numtilefiles++;
		}
	}
	while (k != numtilefiles);

		//150% of art size
	i = ((artsize+(artsize>>1))&0xffffff00);
	while ((pic = (char *)malloc(i)) == NULL) i -= 65536;
	initcache(FP_OFF(pic),i);

	for(i=0;i<MAXTILES;i++)
	{
		j = 15;
		while ((j > 0) && ((1<<j) > tilesizx[i])) j--;
		if ((1<<j) != tilesizx[i]) j++;    //THIS FOR NONREPEATING TEXTURES!
		picsiz[i] = ((char)j);
		j = 15;
		while ((j > 0) && ((1<<j) > tilesizy[i])) j--;
		if ((1<<j) != tilesizy[i]) j++;    //THIS FOR NONREPEATING TEXTURES!
		picsiz[i] += ((char)(j<<4));
	}

	picsloaded = 1;

	artfil = -1;
	artfilnum = -1;
	artfilplc = 0L;

	return(0);
}

void unloadpics ()
{
	if (artfil != -1) close(artfil);
	free(pic);
	picsloaded = 0;
}

long animateoffs (long tilenum)
{
	long i, k, offs;

	offs = 0;
	i = (baktotalclock>>((picanm[tilenum]>>24)&15));
	if ((picanm[tilenum]&63) > 0)
	{
		switch(picanm[tilenum]&192)
		{
			case 64:
				k = (i%((picanm[tilenum]&63)<<1));
				if (k < (picanm[tilenum]&63))
					offs = k;
				else
					offs = (((picanm[tilenum]&63)<<1)-k);
				break;
			case 128:
				offs = (i%((picanm[tilenum]&63)+1));
					break;
			case 192:
				offs = -(i%((picanm[tilenum]&63)+1));
		}
	}
	return(offs);
}

long setvesamode (short vesamode)
{
	static struct rminfo
	{
		long EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX;
		short flags, ES, DS, FS, GS, IP, CS, SP, SS;
	} RMI;

	union REGS regs;
	struct SREGS sregs;
	short selector, segment;
	short *ptr, gran;

	memset(&sregs,0,sizeof(sregs));
	regs.w.ax = 0x0100;
	regs.w.bx = (256>>4);             //Number of paragraphs
	int386x(0x31,&regs,&regs,&sregs); // DPMI call 100h allocates DOS memory
	segment = regs.w.ax;
	selector = regs.w.dx;

	memset(&RMI,0,sizeof(RMI));    // Set up real-mode call structure
	RMI.EAX = 0x00004f01;
	RMI.ECX = (long)vesamode;
	RMI.ES = segment;
	RMI.EDI = 0;

	regs.w.ax = 0x0300;            // Use DMPI call 300h to issue the DOS interrupt
	regs.h.bl = 0x10;
	regs.h.bh = 0;
	regs.w.cx = 0;
	sregs.es = FP_SEG(&RMI);
	regs.x.edi = FP_OFF(&RMI);
	int386x(0x31,&regs,&regs,&sregs);

	if ((RMI.EAX&65535) != 0x004f)
		return(-1);

	ptr = (short *)((((long)segment)<<4)+4);
	gran = 64 / (*ptr);
	vesapageshift = 0;
	while (gran > 1)
	{
		vesapageshift++;
		gran >>= 1;
	}

	if (vesasetmode(vesamode) != 0x4f)
		return(-1);

	return(0);
}

long gettile (short dapicnum)
{
	long xpicnum, ypicnum, xypicnum, cury, dacol;
	char *ptr;

	xpicnum = (xdim>>6); ypicnum = (ydim>>6);
	xypicnum = xpicnum*ypicnum;

	searchx = ((dapicnum%xpicnum)<<6)+32;
	searchy = ((dapicnum/xpicnum)<<6)+32;
	cury = searchy;
	while (1)
	{
		cury = ((cury*7+searchy)>>3);
		drawtilescreen((long)max(cury-(ydim>>1),0L));
		if (option[0] == 0) dacol = 0xffffff; else dacol = whitecol;
		drawmousecursor(searchx,searchy-(long)max(cury-(ydim>>1),0L),dacol);
		drawrectangle(searchx&0xffffffc0,(searchy&0xffffffc0)-(long)max(cury-(ydim>>1),0L),(searchx&0xffffffc0)+63,(searchy&0xffffffc0)+63-(long)max(cury-(ydim>>1),0L),dacol);
		drawrectangle((searchx&0xffffffc0)+1,((searchy&0xffffffc0)+1-(long)max(cury-(ydim>>1),0L)),(searchx&0xffffffc0)+62,(searchy&0xffffffc0)+62-(long)max(cury-(ydim>>1),0L),dacol);
		nextpage();

		obstatus = bstatus, getmousevalues(&mousx,&mousy,&bstatus);
		if (mousx|mousy)
		{
			searchx += mousx;
			searchy += mousy;
			if (searchx < 4) searchx = 4;
			if (searchx >= (xpicnum<<6)-4) searchx = (xpicnum<<6)-4-1;
			if (searchy < 4) searchy = 4;
			if (searchy >= ((MAXTILES/xpicnum)<<6)) searchy = ((MAXTILES/xpicnum)<<6)-1;
		}

		if (keystatus[0xcb] > 0)
		{
			keystatus[0xcb] = 0;
			searchx = max((searchx&0xffffffc0)-32,4);
			searchy = (searchy&0xffffffc0)+32;
		}
		if (keystatus[0xcd] > 0)
		{
			keystatus[0xcd] = 0;
			searchx = min((searchx&0xffffffc0)+96,(xpicnum<<6)-4-1);
			searchy = (searchy&0xffffffc0)+32;
		}
		if (keystatus[0xc8] > 0)
		{
			keystatus[0xc8] = 0;
			searchx = (searchx&0xffffffc0)+32;
			searchy = max((searchy&0xffffffc0)-32,32);
		}
		if (keystatus[0xd0] > 0)
		{
			keystatus[0xd0] = 0;
			searchx = (searchx&0xffffffc0)+32;
			searchy = min((searchy&0xffffffc0)+96,((MAXTILES/xpicnum)<<6)-1);
		}
		if (keystatus[0xc9] > 0)  //PGUP
		{
			keystatus[0xc9] = 0;
			searchy = max(searchy-(ypicnum<<6),4);
		}
		if (keystatus[0xd1] > 0)  //PGDN
		{
			keystatus[0xd1] = 0;
			searchy = min(searchy+(ypicnum<<6),((MAXTILES/xpicnum)<<6)-1);
		}

		if ((keystatus[0x1c] > 0) || ((bstatus&1) > (obstatus&1)))
		{
			keystatus[0x1c] = 0;
			return((searchy>>6)*xpicnum+(searchx>>6));
		}
		if ((keystatus[1] > 0) || ((bstatus&2) > (obstatus&2)))
		{
			keystatus[1] = 0;
			return(dapicnum);
		}
	}
}

void drawtilescreen (long cury)
{
	long i, j, xsiz, ysiz, cnt, y, yend, dapic, dashade, *startvptr, *vptr;
	long xinc, xinc2, xinc3, xinc4, yinc, col;
	char *startptr, *ptr;

	dapic = (cury>>6)*(xdim>>6);
	for(j=(cury&63^63)-63;j<ydim;j+=64)
		for(i=0;i<=xdim-64;i+=64)
		{
			if (waloff[0][dapic] == 0) loadtile(dapic);
			xsiz = (long)tilesizx[dapic]; ysiz = (long)tilesizy[dapic];

			if (option[0] == 0)
			{
				if ((xsiz > 0) && (ysiz > 0) && (waloff[0][dapic] != 0))
				{
					yinc = 1;
					while ((xsiz > 64) || (ysiz > 64))
						{ xsiz >>= 1; ysiz >>= 1; yinc <<= 1; }
					if (xsiz > 64) xsiz = 64;
					if (ysiz > 64) ysiz = 64;

					xinc = (yinc<<8);
					xinc2 = xinc+xinc;
					xinc3 = xinc+xinc2;
					xinc4 = xinc2+xinc2;

					y = max(j,0); yend = min(j+ysiz,ydim);
					startvptr = (long *)(frameplace+((ylookup[y]+i)<<2));
					startptr = (char *)(waloff[0][dapic]+(y-j)*yinc);
					while (y < yend)
					{
						dashade = divscale9(klabs(y-(ydim>>1)),ydim);  //0-255
						dashade = ((16384-sintable[(dashade+512)&2047])>>5);
						vptr = startvptr; ptr = startptr; cnt = xsiz;
						while (cnt >= 4)
						{
							*vptr++ = colookup[dashade][ptr[0]];
							*vptr++ = colookup[dashade][ptr[xinc]];
							*vptr++ = colookup[dashade][ptr[xinc2]];
							*vptr++ = colookup[dashade][ptr[xinc3]];
							ptr += xinc4; cnt -= 4;
						}
						if (cnt > 0)
						{
							*vptr++ = colookup[dashade][ptr[0]];
							if (cnt >= 2) *vptr++ = colookup[dashade][ptr[xinc]]; else *vptr++ = 0L;
							if (cnt >= 3) *vptr++ = colookup[dashade][ptr[xinc2]]; else *vptr++ = 0L;
							*vptr++ = 0L;
						}
						if (xsiz <= 60) clearbuf(vptr,64L-xsiz,0L);

						startvptr = (long *)(FP_OFF(startvptr)+ylookup[4]);
						startptr += yinc;
						y++;
					}
					yend = min(j+64,ydim);
					while (y < yend)
					{
						clearbuf(startvptr,64L,0L);
						startvptr = (long *)(FP_OFF(startvptr)+ylookup[4]);
						y++;
					}
				}
				else
				{
					y = max(j,0); yend = min(j+64,ydim);
					startvptr = (long *)(frameplace+((ylookup[y]+i)<<2));
					while (y < yend)
					{
						clearbuf(startvptr,64,0L);
						startvptr = (long *)(FP_OFF(startvptr)+ylookup[4]);
						y++;
					}
				}
			}
			else
			{
				if ((xsiz > 0) && (ysiz > 0) && (waloff[0][dapic] != 0))
				{
					yinc = 1;
					while ((xsiz > 64) || (ysiz > 64))
						{ xsiz >>= 1; ysiz >>= 1; yinc <<= 1; }
					if (xsiz > 64) xsiz = 64;
					if (ysiz > 64) ysiz = 64;

					xinc = (yinc<<8);
					xinc2 = xinc+xinc;
					xinc3 = xinc+xinc2;
					xinc4 = xinc2+xinc2;

					y = max(j,0); yend = min(j+ysiz,ydim);
					startvptr = (long *)(frameplace+ylookup[y]+i);
					startptr = (char *)(waloff[0][dapic]+(y-j)*yinc);
					while (y < yend)
					{
						vptr = startvptr; ptr = startptr; cnt = xsiz;
						while (cnt >= 4)
						{
							*vptr++ = ((long)ptr[0])+(((long)ptr[xinc])<<8)+
								 (((long)ptr[xinc2])<<16)+(((long)ptr[xinc3])<<24);
							ptr += xinc4; cnt -= 4;
						}
						if (cnt > 0)
						{
							col = 0x06070800 + ((long)ptr[0]);
							if (cnt >= 2) col = (col&0xffff00ff)+(((long)ptr[xinc])<<8);
							if (cnt >= 3) col = (col&0xff00ffff)+(((long)ptr[xinc2])<<16);
							*vptr++ = col;
						}
						if (xsiz <= 60) clearbuf(vptr,(64-xsiz)>>2,0x06070807);

						startvptr = (long *)(FP_OFF(startvptr)+ylookup[1]);
						startptr += yinc;
						y++;
					}
					yend = min(j+64,ydim);
					while (y < yend)
					{
						clearbuf(startvptr,64>>2,0x06070807);
						startvptr = (long *)(FP_OFF(startvptr)+ylookup[1]);
						y++;
					}
				}
				else
				{
					y = max(j,0); yend = min(j+64,ydim);
					startvptr = (long *)(frameplace+ylookup[y]+i);
					while (y < yend)
					{
						clearbuf((void *)startvptr,64>>2,0x06070807);
						startvptr = (long *)(FP_OFF(startvptr)+ylookup[1]);
						y++;
					}
				}
			}
			dapic++;
		}
}

void drawmousecursor (long dax, long day, long col)
{
	long x, y, xx, yy;

	for(x=1;x>=0;x--)
		for(y=1;y>=0;y--)
		{
			xx = 0; if (x == 0) xx = -xx;
			yy = 4; if (y == 0) yy = -yy;
			xx += dax; yy += day;
			if ((yy >= 0) && (yy < ydim))
			{
				if (option[0] == 0) drawpixelses(frameplace+((ylookup[yy]+xx)<<2),col);
									else drawpixel(frameplace+ylookup[yy]+xx,col);
			}

			xx = 1; if (x == 0) xx = -xx;
			yy = -4; if (y == 0) yy = -yy;
			xx += dax; yy += day;
			if ((yy >= 0) && (yy < ydim))
			{
				if (option[0] == 0) drawpixelses(frameplace+((ylookup[yy]+xx)<<2),col);
									else drawpixel(frameplace+ylookup[yy]+xx,col);
			}

			xx = 2; if (x == 0) xx = -xx;
			yy = -3; if (y == 0) yy = -yy;
			xx += dax; yy += day;
			if ((yy >= 0) && (yy < ydim))
			{
				if (option[0] == 0) drawpixelses(frameplace+((ylookup[yy]+xx)<<2),col);
									else drawpixel(frameplace+ylookup[yy]+xx,col);
			}

			xx = 3; if (x == 0) xx = -xx;
			yy = -2; if (y == 0) yy = -yy;
			xx += dax; yy += day;
			if ((yy >= 0) && (yy < ydim))
			{
				if (option[0] == 0) drawpixelses(frameplace+((ylookup[yy]+xx)<<2),col);
									else drawpixel(frameplace+ylookup[yy]+xx,col);
			}

			xx = 4; if (x == 0) xx = -xx;
			yy = -1; if (y == 0) yy = -yy;
			xx += dax; yy += day;
			if ((yy >= 0) && (yy < ydim))
			{
				if (option[0] == 0) drawpixelses(frameplace+((ylookup[yy]+xx)<<2),col);
									else drawpixel(frameplace+ylookup[yy]+xx,col);
			}

			xx = 4; if (x == 0) xx = -xx;
			yy = 0; if (y == 0) yy = -yy;
			xx += dax; yy += day;
			if ((yy >= 0) && (yy < ydim))
			{
				if (option[0] == 0) drawpixelses(frameplace+((ylookup[yy]+xx)<<2),col);
									else drawpixel(frameplace+ylookup[yy]+xx,col);
			}

			xx = 0; if (x == 0) xx = -xx;
			yy = -3; if (y == 0) yy = -yy;
			xx += dax; yy += day;
			if ((yy >= 0) && (yy < ydim))
			{
				if (option[0] == 0) drawpixelses(frameplace+((ylookup[yy]+xx)<<2),col);
									else drawpixel(frameplace+ylookup[yy]+xx,col);
			}

			xx = 1; if (x == 0) xx = -xx;
			yy = -3; if (y == 0) yy = -yy;
			xx += dax; yy += day;
			if ((yy >= 0) && (yy < ydim))
			{
				if (option[0] == 0) drawpixelses(frameplace+((ylookup[yy]+xx)<<2),col);
									else drawpixel(frameplace+ylookup[yy]+xx,col);
			}

			xx = 2; if (x == 0) xx = -xx;
			yy = -2; if (y == 0) yy = -yy;
			xx += dax; yy += day;
			if ((yy >= 0) && (yy < ydim))
			{
				if (option[0] == 0) drawpixelses(frameplace+((ylookup[yy]+xx)<<2),col);
									else drawpixel(frameplace+ylookup[yy]+xx,col);
			}

			xx = 3; if (x == 0) xx = -xx;
			yy = -1; if (y == 0) yy = -yy;
			xx += dax; yy += day;
			if ((yy >= 0) && (yy < ydim))
			{
				if (option[0] == 0) drawpixelses(frameplace+((ylookup[yy]+xx)<<2),col);
									else drawpixel(frameplace+ylookup[yy]+xx,col);
			}

			xx = 3; if (x == 0) xx = -xx;
			yy = 0; if (y == 0) yy = -yy;
			xx += dax; yy += day;
			if ((yy >= 0) && (yy < ydim))
			{
				if (option[0] == 0) drawpixelses(frameplace+((ylookup[yy]+xx)<<2),col);
									else drawpixel(frameplace+ylookup[yy]+xx,col);
			}
		}
}

void drawrectangle (long x1, long y1, long x2, long y2, long col)
{
	long i;

	if ((y2 < 0) || (y1 >= ydim)) return;

	for(i=x1;i<=x2;i++)
	{
		if ((y1 >= 0) && (y1 < ydim))
		{
			if (option[0] == 0) drawpixelses(frameplace+((ylookup[y1]+i)<<2),col);
								else drawpixel(frameplace+ylookup[y1]+i,col);
		}
		if ((y2 >= 0) && (y2 < ydim))
		{
			if (option[0] == 0) drawpixelses(frameplace+((ylookup[y2]+i)<<2),col);
								else drawpixel(frameplace+ylookup[y2]+i,col);
		}
	}
	y1 = max(y1,0);
	y2 = min(y2,ydim-1);
	for(i=y1;i<=y2;i++)
	{
		if (option[0] == 0) drawpixelses(frameplace+((ylookup[i]+x1)<<2),col);
							else drawpixel(frameplace+ylookup[i]+x1,col);
		if (option[0] == 0) drawpixelses(frameplace+((ylookup[i]+x2)<<2),col);
							else drawpixel(frameplace+ylookup[i]+x2,col);
	}
}

long initmouse ()
{
	return(moustat = (short)setupmouse());
}

void getmousevalues (short *mousx, short *mousy, short *bstatus)
{
	if (moustat == 0) { *mousx = 0; *mousy = 0; *bstatus = 0; return; }
	readmousexy(mousx,mousy);
	readmousebstatus(bstatus);
}

void searchit ()
{
	long xv, yv, zv, xp, yp, zp, tx, ty, tz, xd, yd, zd;
	float fx, fy, fz;

	searchitx = posx;
	searchity = posy;
	searchitz = posz;

	fx = ((searchx-(xdim>>1))<<8);
	fy = ((searchy-(ydim>>1))<<8);
	fz = (xdim<<7);
	backrotate3d(fx,fy,fz);

	xv = (long)fx; yv = (long)fy; zv = (long)fz;

	searchcube = (posx>>10)*BSIZ*BSIZ+(posy>>10)*BSIZ+(posz>>10);
	if (xv < 0) { xp = (posx&1023); xd = -BSIZ*BSIZ; xv = -xv; }
			 else { xp = (-posx&1023); xd = BSIZ*BSIZ; }
	if (yv < 0) { yp = (posy&1023); yd = -BSIZ; yv = -yv; }
			 else { yp = (-posy&1023); yd = BSIZ; }
	if (zv < 0) { zp = (posz&1023); zd = -1; zv = -zv; }
			 else { zp = (-posz&1023); zd = 1; }
	if (xv <= 2) xv = 0x001fffff; else xv = divscale32(1L,xv);
	if (yv <= 2) yv = 0x001fffff; else yv = divscale32(1L,yv);
	if (zv <= 2) zv = 0x001fffff; else zv = divscale32(1L,zv);
	tx = mulscale10(xp,xv); ty = mulscale10(yp,yv); tz = mulscale10(zp,zv);
	while (1)
	{
		if (tx < ty)
		{
			if (tx < tz)
			{
				searchcube += xd; if (board[searchcube] >= 0) { searchcubak = searchcube-xd; searchedge = 0+((xd<0)*3); return; }
				ty -= tx; tz -= tx; tx = xv;
			}
			else
			{
				searchcube += zd; if (board[searchcube] >= 0) { searchcubak = searchcube-zd; searchedge = 2+((zd<0)*3); return; }
				tx -= tz; ty -= tz; tz = zv;
			}
		}
		else
		{
			if (ty < tz)
			{
				searchcube += yd; if (board[searchcube] >= 0) { searchcubak = searchcube-yd; searchedge = 1+((yd<0)*3); return; }
				tx -= ty; tz -= ty; ty = yv;
			}
			else
			{
				searchcube += zd; if (board[searchcube] >= 0) { searchcubak = searchcube-zd; searchedge = 2+((zd<0)*3); return; }
				tx -= tz; ty -= tz; tz = zv;
			}
		}
	}
}

void clipmove (long *x, long *y, long *z, long xv, long yv, long zv)
{
	if (rayscan(x,y,z,&xv,&yv,&zv) != 255)
		if (rayscan(x,y,z,&xv,&yv,&zv) != 255)
			rayscan(x,y,z,&xv,&yv,&zv);

	*x = min(max(*x,1024+128),(BSIZ<<10)-(1024+128));
	*y = min(max(*y,1024+128),(BSIZ<<10)-(1024+128));
	*z = min(max(*z,1024+128),(BSIZ<<10)-(1024+128));
}

long rayscan (long *x, long *y, long *z, long *xv, long *yv, long *zv)
{
	long i, j, t, gridx, gridy, gridz, xdir, ydir, zdir;
	long nintx, ninty, nintz, intx, inty, intz;
	char bad, lasthit;

	lasthit = 255;
	intx = (*x) + (*xv);
	inty = (*y) + (*yv);
	intz = (*z) + (*zv);

	if (*xv != 0)
	{
		xdir = ksgn(*xv);
		if (xdir < 0)
			{ gridx = (((*x)-WALLDIST)>>10)-1; nintx = (gridx<<10)+1024+WALLDIST; }
		else
			{ gridx = (((*x)+WALLDIST-1)>>10)+1; nintx = (gridx<<10)-WALLDIST; }
		j = xdir*BSIZ*BSIZ;

		while (klabs(nintx-(*x)) <= klabs(intx-(*x)))
		{
			i = divscale16(nintx-(*x),*xv);
			ninty = *y + mulscale16(*yv,i);
			nintz = *z + mulscale16(*zv,i);

			bad = 0;
			i = gridx*BSIZ*BSIZ+((ninty-WALLDIST)>>10)*BSIZ+((nintz-WALLDIST)>>10); if ((board[i] >= 0) && (board[i-j] < 0)) bad = 1;
			i = gridx*BSIZ*BSIZ+((ninty+WALLDIST)>>10)*BSIZ+((nintz-WALLDIST)>>10); if ((board[i] >= 0) && (board[i-j] < 0)) bad = 1;
			i = gridx*BSIZ*BSIZ+((ninty-WALLDIST)>>10)*BSIZ+((nintz+WALLDIST)>>10); if ((board[i] >= 0) && (board[i-j] < 0)) bad = 1;
			i = gridx*BSIZ*BSIZ+((ninty+WALLDIST)>>10)*BSIZ+((nintz+WALLDIST)>>10); if ((board[i] >= 0) && (board[i-j] < 0)) bad = 1;
			if (bad != 0)
				{ lasthit = 0; intx = nintx; inty = ninty; intz = nintz; break; }
			gridx += xdir; nintx += (xdir<<10);
		}
	}
	if (*yv != 0)
	{
		ydir = ksgn(*yv);
		if (ydir < 0)
			{ gridy = (((*y)-WALLDIST)>>10)-1; ninty = (gridy<<10)+1024+WALLDIST; }
		else
			{ gridy = (((*y)+WALLDIST-1)>>10)+1; ninty = (gridy<<10)-WALLDIST; }
		j = ydir*BSIZ;

		while (klabs(ninty-(*y)) <= klabs(inty-(*y)))
		{
			i = divscale16(ninty-(*y),*yv);
			nintx = *x + mulscale16(*xv,i);
			nintz = *z + mulscale16(*zv,i);

			bad = 0;
			i = ((nintx-WALLDIST)>>10)*BSIZ*BSIZ+gridy*BSIZ+((nintz-WALLDIST)>>10); if ((board[i] >= 0) && (board[i-j] < 0)) bad = 1;
			i = ((nintx+WALLDIST)>>10)*BSIZ*BSIZ+gridy*BSIZ+((nintz-WALLDIST)>>10); if ((board[i] >= 0) && (board[i-j] < 0)) bad = 1;
			i = ((nintx-WALLDIST)>>10)*BSIZ*BSIZ+gridy*BSIZ+((nintz+WALLDIST)>>10); if ((board[i] >= 0) && (board[i-j] < 0)) bad = 1;
			i = ((nintx+WALLDIST)>>10)*BSIZ*BSIZ+gridy*BSIZ+((nintz+WALLDIST)>>10); if ((board[i] >= 0) && (board[i-j] < 0)) bad = 1;
			if (bad != 0)
				{ lasthit = 1; intx = nintx; inty = ninty; intz = nintz; break; }
			gridy += ydir; ninty += (ydir<<10);
		}
	}
	if (*zv != 0)
	{
		zdir = ksgn(*zv);
		if (zdir < 0)
			{ gridz = (((*z)-WALLDIST)>>10)-1; nintz = (gridz<<10)+1024+WALLDIST; }
		else
			{ gridz = (((*z)+WALLDIST-1)>>10)+1; nintz = (gridz<<10)-WALLDIST; }
		j = zdir;

		while (klabs(nintz-(*z)) <= klabs(intz-(*z)))
		{
			i = divscale16(nintz-(*z),*zv);
			nintx = *x + mulscale16(*xv,i);
			ninty = *y + mulscale16(*yv,i);

			bad = 0;
			i = ((nintx-WALLDIST)>>10)*BSIZ*BSIZ+((ninty-WALLDIST)>>10)*BSIZ+gridz; if ((board[i] >= 0) && (board[i-j] < 0)) bad = 1;
			i = ((nintx+WALLDIST)>>10)*BSIZ*BSIZ+((ninty-WALLDIST)>>10)*BSIZ+gridz; if ((board[i] >= 0) && (board[i-j] < 0)) bad = 1;
			i = ((nintx-WALLDIST)>>10)*BSIZ*BSIZ+((ninty+WALLDIST)>>10)*BSIZ+gridz; if ((board[i] >= 0) && (board[i-j] < 0)) bad = 1;
			i = ((nintx+WALLDIST)>>10)*BSIZ*BSIZ+((ninty+WALLDIST)>>10)*BSIZ+gridz; if ((board[i] >= 0) && (board[i-j] < 0)) bad = 1;
			if (bad != 0)
				{ lasthit = 2; intx = nintx; inty = ninty; intz = nintz; break; }
			gridz += zdir; nintz += (zdir<<10);
		}
	}

	if (lasthit == 0) *xv = 0; else *xv = (*x) + (*xv) - intx;
	if (lasthit == 1) *yv = 0; else *yv = (*y) + (*yv) - inty;
	if (lasthit == 2) *zv = 0; else *zv = (*z) + (*zv) - intz;
	*x = intx; *y = inty; *z = intz;
	return(lasthit);
}

void makeboxbsp ()
{
	long i, a, x, y, z, x1, y1, z1, x2, y2, z2, bad, xx, yy, zz;

	a = 0;
	for(i=BSIZ*BSIZ*BSIZ-1;i>=0;i--)
	{
		if (board[i] < 0)
			{ cubesect[i] = -2; tempshort[a++] = i; }
		else
			cubesect[i] = -1;
	}

	numsectors = 0;
	while (1)
	{
		i = mulscale16(krand(),a);
		x = (tempshort[i]/(BSIZ*BSIZ));
		y = ((tempshort[i]/BSIZ)%BSIZ);
		z = (tempshort[i]%BSIZ);
		if (cubesect[tempshort[i]] == -2)
		{
			x1 = x; y1 = y; z1 = z; x2 = x; y2 = y; z2 = z; bad = 0;
			do
			{
				if ((bad&1) == 0)
				{
					x2++;
					for(yy=y1;yy<=y2;yy++)
						for(zz=z1;zz<=z2;zz++)
							if (cubesect[x2*BSIZ*BSIZ+yy*BSIZ+zz] != -2) { bad |= 1; x2--; break; }
				}
				if ((bad&2) == 0)
				{
					y2++;
					for(xx=x1;xx<=x2;xx++)
						for(zz=z1;zz<=z2;zz++)
							if (cubesect[xx*BSIZ*BSIZ+y2*BSIZ+zz] != -2) { bad |= 2; y2--; break; }
				}
				if ((bad&4) == 0)
				{
					z2++;
					for(xx=x1;xx<=x2;xx++)
						for(yy=y1;yy<=y2;yy++)
							if (cubesect[xx*BSIZ*BSIZ+yy*BSIZ+z2] != -2) { bad |= 4; z2--; break; }
				}
				if ((bad&8) == 0)
				{
					x1--;
					for(yy=y1;yy<=y2;yy++)
						for(zz=z1;zz<=z2;zz++)
							if (cubesect[x1*BSIZ*BSIZ+yy*BSIZ+zz] != -2) { bad |= 8; x1++; break; }
				}
				if ((bad&16) == 0)
				{
					y1--;
					for(xx=x1;xx<=x2;xx++)
						for(zz=z1;zz<=z2;zz++)
							if (cubesect[xx*BSIZ*BSIZ+y1*BSIZ+zz] != -2) { bad |= 16; y1++; break; }
				}
				if ((bad&32) == 0)
				{
					z1--;
					for(xx=x1;xx<=x2;xx++)
						for(yy=y1;yy<=y2;yy++)
							if (cubesect[xx*BSIZ*BSIZ+yy*BSIZ+z1] != -2) { bad |= 32; z1++; break; }
				}
			} while (bad != 63);

			for(x=x1;x<=x2;x++)
				for(y=y1;y<=y2;y++)
					for(z=z1;z<=z2;z++)
						cubesect[x*BSIZ*BSIZ+y*BSIZ+z] = numsectors;

			sectx1[numsectors] = x1; sectx2[numsectors] = x2;
			secty1[numsectors] = y1; secty2[numsectors] = y2;
			sectz1[numsectors] = z1; sectz2[numsectors] = z2;
			numsectors++;
		}
		else
		{
			a--; if (a <= 0) return;
			tempshort[i] = tempshort[a];
		}
	}
}

void makecolookup ()
{
	long i, j, r, g, b, rinc, ginc, binc;

	for(j=0;j<256;j++)
	{
		rinc = (((long)palette[j*3+0])<<2); r = (rinc<<8);
		ginc = (((long)palette[j*3+1])<<2); g = (ginc<<8);
		binc = (((long)palette[j*3+2])<<2); b = (binc<<8);
		for(i=0;i<256;i++)
		{
			colookup[i][j] = ((r>>8)<<16)+((g>>8)<<8)+(b>>8);
			r -= rinc; g -= ginc; b -= binc;
		}
	}
}
