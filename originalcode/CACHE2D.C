//CUBES5 by Ken Silverman (http://advsys.net/ken)

//   This module keeps track of a 2-D cacheing system in an x*256 rectangle.
//   To use this module, here's all you need to do:
//
//   Step 1: Allocate a nice BIG buffer, like from 1MB-4MB and
//           Call initcache(long cachestart, long cachesize) where
//
//              cachestart = (long)(pointer to start of BIG buffer)
//              cachesize = length of BIG buffer
//
//   Step 2: Call cachebox(long boxptr, long xsiz, long ysiz) whenever
//              you need to allocate a rectangular region, where
//
//              boxptr = (long)(pointer to (4-byte pointer to box))
//                 Confused?  Using this method, cache2d can take previously
//                 allocated things out of the cache safely.  For each box
//                 you allocate with this function, you should have some
//                 form of a 4-byte pointer.  This pointer will be the start
//                 of the box if it is in cache, otherwise it should be -1.
//                 Given the pointer to this variables, Cache2d can now set
//                 it to -1 for you if it needs to remove that box from the
//                 cache.
//              xsiz,ysiz = dimensions of rectangle (such as 64*64)
//
//   Step 3: If you need to re-size the cacheing buffer then your should
//           Call uninitcache() just before changing the buffer since it
//           try to remove everything from the cache by setting all pointers
//           to -1.  You only need to call uninitcache() if you are changing
//           the BIG buffer somehow or need to call initcache(?,?) again.

#define MAXCACHELINES 4096*4
#define MAXCACHEBOXES 4096

static long cachestart = 0, cachesize = 0;
static long cachenumpoints, cachenumboxes = 0;
static long cachecurxsiz, cachecurysiz, cachecurx;
static long cachepx1[MAXCACHELINES], cachepy1[MAXCACHELINES];
static long cachepx2[MAXCACHELINES], cachepy2[MAXCACHELINES];
static short cacheslist[MAXCACHELINES], cachesbuf[MAXCACHELINES<<1];
static long cachespos[MAXCACHELINES>>1];
static long cachebox1[MAXCACHEBOXES], cacheboy1[MAXCACHEBOXES];
static long cachebox2[MAXCACHEBOXES], cacheboy2[MAXCACHEBOXES];
static long *cacheboxptr[MAXCACHEBOXES];

#pragma aux ksgn =\
	"add ebx, ebx",\
	"sbb eax, eax",\
	"cmp eax, ebx",\
	"adc eax, 0",\
	parm [ebx]\

void initcache (long dacachestart, long dacachesize)
{
	long i;

	cachestart = dacachestart;
	cachesize = (dacachesize&0xffffff00);

		//Cache out stuff already in cache!
	while (cachenumboxes > 0)
	{
		cachenumboxes--;
		*cacheboxptr[cachenumboxes] = 0;
	}

	cachenumpoints = 4; cachecurxsiz = -1; cachecurx = 0;
	i = (cachesize>>8);
	cachepx1[0] = 0; cachepy1[0] = 0; cachepx2[0] = i; cachepy2[0] = 0;
	cachepx1[1] = i; cachepy1[1] = 0; cachepx2[1] = i; cachepy2[1] = 256;
	cachepx1[2] = i; cachepy1[2] = 256; cachepx2[2] = 0; cachepy2[2] = 256;
	cachepx1[3] = 0; cachepy1[3] = 256; cachepx2[3] = 0; cachepy2[3] = 0;
}

void uninitcache ()
{
		//Cache out stuff already in cache!
	while (cachenumboxes > 0)
	{
		cachenumboxes--;
		*cacheboxptr[cachenumboxes] = 0;
	}

	cachestart = 0;
	cachesize = 0;
	cachenumpoints = 0;
}

long findbestplace(long *cx1, long *cy1);
void cachebox (long *cacheptr, long xsiz, long ysiz)
{
	long xoff, yoff, xplc, yplc, x1, y1, x2, y2, z;

	if ((xsiz <= 0) || (ysiz <= 0) || (cachesize <= 0)) return;

	xoff = xsiz; yoff = ysiz;
	if (findbestplace(&xoff,&yoff) < 0)
	{
			//Reset cache to overwrite from beginning
		cachecurx = 0;

		xplc = (cachesize>>8); yplc = 256;
		cachenumpoints = 4; cachecurxsiz = -1;
		cachepx1[0] = 0; cachepy1[0] = 0; cachepx2[0] = xplc; cachepy2[0] = 0;
		cachepx1[1] = xplc; cachepy1[1] = 0; cachepx2[1] = xplc; cachepy2[1] = yplc;
		cachepx1[2] = xplc; cachepy1[2] = yplc; cachepx2[2] = 0; cachepy2[2] = yplc;
		cachepx1[3] = 0; cachepy1[3] = yplc; cachepx2[3] = 0; cachepy2[3] = 0;

		xoff = xsiz; yoff = ysiz;
		if (findbestplace(&xoff,&yoff) < 0)
		{
			printf("Tile can't fit in cache!\n");
			exit(0);
		}
	}

	x1 = xoff; x2 = xoff+xsiz;
	y1 = yoff; y2 = yoff+ysiz;

		//Suck box
	xorline(x1,y1,y2,cachepx1,cachepy1,cachepx2,cachepy2);
	xorline(y2,x1,x2,cachepy1,cachepx1,cachepy2,cachepx2);
	xorline(x2,y2,y1,cachepx1,cachepy1,cachepx2,cachepy2);
	xorline(y1,x2,x1,cachepy1,cachepx1,cachepy2,cachepx2);

	for (z=cachenumboxes-1;z>=0;z--)
		if ((x1 < cachebox2[z]) && (cachebox1[z] < x2))
			if ((y1 < cacheboy2[z]) && (cacheboy1[z] < y2))
			{
				*cacheboxptr[z] = 0;  //Cached out!

				cachenumboxes--;
				cachebox1[z] = cachebox1[cachenumboxes]; cacheboy1[z] = cacheboy1[cachenumboxes];
				cachebox2[z] = cachebox2[cachenumboxes]; cacheboy2[z] = cacheboy2[cachenumboxes];
				cacheboxptr[z] = cacheboxptr[cachenumboxes];
			}

	cachebox1[cachenumboxes] = x1; cachebox2[cachenumboxes] = x2;
	cacheboy1[cachenumboxes] = y1; cacheboy2[cachenumboxes] = y2;
	cacheboxptr[cachenumboxes] = cacheptr;
	cachenumboxes++;

	*cacheptr = cachestart+(xoff<<8)+yoff;
}

long findbestplace (long *cx1, long *cy1)
{
	long xsiz, ysiz, gx, z, zz, zzz, pcnt1, pcnt2, scnt, p, s;
	long x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, x6, y6;
	long ncx1, ncy1, ncx2, ncy2, bad;

	xsiz = *cx1; ysiz = *cy1;

	*cx1 = 0x7fffffff;

	gx = cachecurx-2048; if (gx < 0) gx = 0;

	for(z=cachenumpoints-1;z>=0;z--)
		if ((cachepx1[z] < gx) && (cachepx2[z] < gx))
		{
			cachenumpoints--;
			cachepx1[z] = cachepx1[cachenumpoints];
			cachepy1[z] = cachepy1[cachenumpoints];
			cachepx2[z] = cachepx2[cachenumpoints];
			cachepy2[z] = cachepy2[cachenumpoints];
		}

	if ((cachecurxsiz == xsiz) && (cachecurysiz == ysiz)) gx = cachecurx;

	pcnt1 = 0;
	for(z=cachenumpoints-1;z>=0;z--)
	{
		x1 = cachepx1[z]; y1 = cachepy1[z]; x2 = cachepx2[z]; y2 = cachepy2[z];
		if ((x1 == x2) && (y2 > y1) && (x1 >= gx))
			cachesbuf[pcnt1++] = z;
	}
	pcnt2 = pcnt1;
	for(z=cachenumpoints-1;z>=0;z--)
	{
		x1 = cachepx1[z]; y1 = cachepy1[z]; x2 = cachepx2[z]; y2 = cachepy2[z];
		if ((y1 == y2) && ((x1 >= gx) || (x2 >= gx)))
			cachesbuf[pcnt2++] = z;
	}

	scnt = 0;
	for(z=cachenumpoints-1;z>=0;z--)
	{
		x1 = cachepx1[z]; y1 = cachepy1[z]; x2 = cachepx2[z]; y2 = cachepy2[z];
		if ((y1 == y2) && (x2 > x1) && (x2 >= gx))
			cacheslist[scnt++] = z;
	}

	for(z=cachenumpoints-1;z>=0;z--)
	{
		x1 = cachepx1[z]; y1 = cachepy1[z]; x2 = cachepx2[z]; y2 = cachepy2[z];
		if ((x1 >= gx) && (x1 == x2) && (y2 < y1))
			for(s=scnt-1;s>=0;s--)
			{
				zz = cacheslist[s]; x3 = cachepx1[zz]; x4 = cachepx2[zz];
				if ((x4 > x1) && (x3 < x1+xsiz))
				{
					y3 = cachepy1[zz]; y4 = cachepy2[zz];
					if ((y3 < y1) && (y3 > y2-ysiz))
					{
						ncx1 = x1; ncx2 = ncx1+xsiz;
						ncy1 = y3; ncy2 = ncy1+ysiz;

						bad = 0;
						for(p=pcnt1-1;p>=0;p--)
						{
							zzz = cachesbuf[p]; x5 = cachepx1[zzz];
							if ((x5 > ncx1) && (x5 < ncx2))
							{
								y5 = cachepy1[zzz]; y6 = cachepy2[zzz];
								if ((y5 > ncy1) != (y6 > ncy1)) { bad = 1; break; }
								if ((y5 < ncy2) != (y6 < ncy2)) { bad = 1; break; }
							}
						}
						if (bad == 0)
						{
							for(p=pcnt1;p<pcnt2;p++)
							{
								zzz = cachesbuf[p]; y5 = cachepy1[zzz];
								if ((y5 > ncy1) && (y5 < ncy2))
								{
									x5 = cachepx1[zzz]; x6 = cachepx2[zzz];
									if ((x5 > ncx1) != (x6 > ncx1)) { bad = 1; break; }
									if ((x5 < ncx2) != (x6 < ncx2)) { bad = 1; break; }
								}
							}
							if ((bad == 0) && (ncx1 <= *cx1))
								*cx1 = ncx1, *cy1 = ncy1;
						}
					}
				}
			}
	}

	cachecurxsiz = xsiz; cachecurysiz = ysiz; cachecurx = *cx1;

	if (*cx1 == 0x7fffffff) return(-1);
	return(0);
}

void xorline (long cx, long cy1, long cy2, long *dapx1, long *dapy1, long *dapx2, long *dapy2)
{
	long y1, y2, z, zz, pcnt, gap, s, baks, baky, i;

	y1 = cy1; y2 = cy2;

	cachespos[0] = y1; cachespos[1] = y2; pcnt = 2;
	cachesbuf[0] = 1; cachesbuf[1] = -1;

	if (y1 > y2) i = y1, y1 = y2, y2 = i;

	for(z=cachenumpoints-1;z>=0;z--)
		if ((dapx1[z] == cx) && (dapx2[z] == cx))
			if ((dapy1[z] >= y1) || (dapy2[z] >= y1))
				if ((dapy1[z] <= y2) || (dapy2[z] <= y2))
				{
					cachespos[pcnt] = dapy1[z]; cachespos[pcnt+1] = dapy2[z];
					cachesbuf[pcnt] = 1; cachesbuf[pcnt+1] = -1;
					pcnt += 2;

					cachenumpoints--;
					dapx1[z] = dapx1[cachenumpoints];
					dapy1[z] = dapy1[cachenumpoints];
					dapx2[z] = dapx2[cachenumpoints];
					dapy2[z] = dapy2[cachenumpoints];
				}

	gap = pcnt;
	do
	{
		gap >>= 1;
		for(z=0;z<pcnt-gap;z++)
		{
			zz = z;
			while (cachespos[zz] > cachespos[zz+gap])
			{
				i = cachespos[zz], cachespos[zz] = cachespos[zz+gap], cachespos[zz+gap] = i;
				i = cachesbuf[zz], cachesbuf[zz] = cachesbuf[zz+gap], cachesbuf[zz+gap] = i;
				zz -= gap; if (zz < 0) break;
			}
		}
	} while (gap > 1);

	s = 0; baks = 0;
	for(i=0;i<pcnt-1;i++)
	{
		s += cachesbuf[i];
		if (s != 0)
		{
			y1 = cachespos[i]; y2 = cachespos[i+1];
			if (y1 != y2)
			{
				if ((baks == s) && (baky == y1))
				{
					if (s > 0)
						dapy2[cachenumpoints-1] = y2;
					else
						dapy1[cachenumpoints-1] = y2;
				}
				else
				{
					dapx1[cachenumpoints] = cx; dapx2[cachenumpoints] = cx;
					if (s > 0)
						dapy1[cachenumpoints] = y1, dapy2[cachenumpoints] = y2;
					else
						dapy1[cachenumpoints] = y2, dapy2[cachenumpoints] = y1;
					cachenumpoints++;
					baks = s;
				}
				baky = y2;
			}
		}
	}
}


#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <stdlib.h>
#include <stdio.h>
#define MAXOPENFILES 64

static long numfiles, groupfil = -1, groupfilpos;
static char *filelist;
static long *fileoffs;

static long filehandle[MAXOPENFILES], filepos[MAXOPENFILES];

long initgroupfile (char *filename)
{
	char buf[16];
	long i, j, k;

	uninitgroupfile();

	for(i=0;i<MAXOPENFILES;i++) filehandle[i] = -1;

	groupfil = open(filename,O_BINARY|O_RDWR,S_IREAD);
	if (groupfil != -1)
	{
		groupfilpos = 0;
		read(groupfil,buf,16);
		if ((buf[0] != 'K') || (buf[1] != 'e') || (buf[2] != 'n') ||
			 (buf[3] != 'S') || (buf[4] != 'i') || (buf[5] != 'l') ||
			 (buf[6] != 'v') || (buf[7] != 'e') || (buf[8] != 'r') ||
			 (buf[9] != 'm') || (buf[10] != 'a') || (buf[11] != 'n'))
		{
			close(groupfil);
			groupfil = -1;
			return(-1);
		}
		numfiles = ((long)buf[12])+(((long)buf[13])<<8)+(((long)buf[14])<<16)+(((long)buf[15])<<24);

		if ((filelist = (char *)malloc(numfiles<<4)) == 0)
			{ printf("Not enough memory for file grouping system\n"); exit(0); }
		if ((fileoffs = (long *)malloc((numfiles+1)<<2)) == 0)
			{ printf("Not enough memory for file grouping system\n"); exit(0); }

		read(groupfil,filelist,numfiles<<4);

		j = 0;
		for(i=0;i<numfiles;i++)
		{
			k = ((long)filelist[(i<<4)+12])+(((long)filelist[(i<<4)+13])<<8)+(((long)filelist[(i<<4)+14])<<16)+(((long)filelist[(i<<4)+15])<<24);
			filelist[(i<<4)+12] = 0;
			fileoffs[i] = j;
			j += k;
		}
		fileoffs[numfiles] = j;
	}
	return(groupfil);
}

void uninitgroupfile ()
{
	if (groupfil != -1)
	{
		free(filelist); free(fileoffs);
		close(groupfil);
		groupfil = -1;
	}
}

long kopen4load (char *filename, char searchfirst)
{
	long i, j, fil, newhandle;
	char ch1, ch2, bad;

	newhandle = MAXOPENFILES-1;
	while (filehandle[newhandle] != -1)
	{
		newhandle--;
		if (newhandle < 0)
		{
			printf("TOO MANY FILES OPEN IN FILE GROUPING SYSTEM!");
			exit(0);
		}
	}

	if (searchfirst == 0)
		if ((fil = open(filename,O_BINARY|O_RDONLY)) != -1)
		{
			filehandle[newhandle] = fil;
			filepos[newhandle] = -1;
			return(newhandle);
		}
	if (groupfil != -1)
	{
		for(i=numfiles-1;i>=0;i--)
		{
			j = 0; bad = 0;
			while ((filename[j] != 0) && (j < 13))
			{
				ch1 = filename[j]; if ((ch1 >= 97) && (ch1 <= 123)) ch1 -= 32;
				ch2 = filelist[(i<<4)+j]; if ((ch2 >= 97) && (ch2 <= 123)) ch2 -= 32;
				if (ch1 != ch2) { bad = 1; break; }
				j++;
			}
			if (bad != 0) continue;

			filepos[newhandle] = 0;
			filehandle[newhandle] = i;
			return(newhandle);
		}
	}
	return(-1);
}

long kread (long handle, void *buffer, long leng)
{
	long i, filenum;

	filenum = filehandle[handle];
	if (filepos[handle] < 0) return(read(filenum,buffer,leng));

	if (groupfil != -1)
	{
		i = fileoffs[filenum]+filepos[handle];
		if (i != groupfilpos)
		{
			lseek(groupfil,i+((numfiles+1)<<4),SEEK_SET);
			groupfilpos = i;
		}
		leng = min(leng,(fileoffs[filenum+1]-fileoffs[filenum])-filepos[handle]);
		leng = read(groupfil,buffer,leng);
		filepos[handle] += leng;
		groupfilpos += leng;
		return(leng);
	}

	return(0);
}

long klseek (long handle, long offset, long whence)
{
	long i;

	if (filepos[handle] < 0) return(lseek(filehandle[handle],offset,whence));
	if (groupfil != -1)
	{
		switch(whence)
		{
			case SEEK_SET: filepos[handle] = offset; break;
			case SEEK_END: i = filehandle[handle];
								filepos[handle] = (fileoffs[i+1]-fileoffs[i])+offset;
								break;
			case SEEK_CUR: filepos[handle] += offset; break;
		}
		return(filepos[handle]);
	}
	return(-1);
}

long kfilelength (long handle)
{
	long i;

	if (filepos[handle] < 0) return(filelength(filehandle[handle]));
	i = filehandle[handle];
	return(fileoffs[i+1]-fileoffs[i]);
}

void kclose (long handle)
{
	if (filepos[handle] < 0) close(filehandle[handle]);
	filehandle[handle] = -1;
}
