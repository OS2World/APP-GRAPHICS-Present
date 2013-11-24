/*
**		PresFont.c	(NC)Not Copyrighted by The Frobozz Magic Software Company
**
**		Font management functions
*/

#define INCL_GPI

#include <os2.h>
#include <presfont.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SHORT sFontSize[FONTSIZES]   = {80, 100, 120, 140, 180, 240};

static CHAR  *szFacename[FONTFACES] = {
	"System Proportional",
   "Courier",
	"Helv",
	"Tms Rmn",
	"System Monospaced"
};

static LONG  alMatch[FONTFACES][FONTSIZES];
static LONG	 lcidFont = 1L;						// font id dispenser


typedef struct _fl {				/* fl */
	struct _fl *pflNext;
	USHORT		idFace;
	USHORT		usSize;
	USHORT		fsSelection;
	BOOL			fOutline;
	LONG			lcid;
} FONTLIST;

typedef FONTLIST *PFONTLIST;	/* pfl */

PFONTLIST pflFirst = NULL;

BOOL PresInitFonts(HPS hps)
{
	FONTMETRICS *pfm;
	HDC         hdc;
	LONG        lHorzRes, lVertRes, lRequestFonts, lNumberFonts;
	SHORT       sIndex, sFace, sSize;

	hdc = GpiQueryDevice(hps);
	DevQueryCaps(hdc, CAPS_HORIZONTAL_FONT_RES, 1L, &lHorzRes);
	DevQueryCaps(hdc, CAPS_VERTICAL_FONT_RES,   1L, &lVertRes);

	for (sFace = 0; sFace < FONTFACES; sFace++) {
		lRequestFonts = 0;
		lNumberFonts = GpiQueryFonts(hps, QF_PUBLIC, szFacename[sFace],
                              		&lRequestFonts, 0L, NULL);
		if (lNumberFonts == 0)
   		continue; 

		if (lNumberFonts * sizeof(FONTMETRICS) >= 65536L)
   		return FALSE;

		pfm = malloc((SHORT)lNumberFonts * sizeof(FONTMETRICS));

		if (pfm == NULL)
   		return FALSE;

		GpiQueryFonts(hps, QF_PUBLIC, szFacename[sFace],
        		&lNumberFonts, (LONG)sizeof(FONTMETRICS), pfm);

		for (sIndex = 0; sIndex < (SHORT)lNumberFonts; sIndex++) {
			if (pfm[sIndex].sXDeviceRes == (SHORT)lHorzRes &&
				 pfm[sIndex].sYDeviceRes == (SHORT)lVertRes &&
             (pfm[sIndex].fsDefn & 1) == 0) {
				for (sSize = 0; sSize < FONTSIZES; sSize++) {
					if (pfm[sIndex].sNominalPointSize == sFontSize[sSize])
						break;
				}
            if (sSize != FONTSIZES) {
					alMatch[sFace][sSize] = pfm[sIndex].lMatch;
				}
			}
		}
      free(pfm);
	}
	return TRUE;
}

LONG EzfCreateLogFont(HPS hps, LONG lcid, USHORT idFace, USHORT idSize,
                                           USHORT fsSelection)
{
	static FATTRS fat = {sizeof(fat)};

	if (idFace >= FONTFACES || idSize >= FONTSIZES || alMatch[idFace][idSize] == 0) {
		return FALSE;
	}
	fat.fsSelection    = fsSelection;
	fat.lMatch         = alMatch[idFace][idSize];

	strcpy(fat.szFacename, szFacename[idFace]);

	return GpiCreateLogFont(hps, NULL, lcid, &fat);
}

LONG EzfCreateOutlineFont(HPS hps, LONG lcid, PSZ pszFace)
{
	FATTRS fat;
	fat.usRecordLength	= sizeof(fat);
	fat.fsSelection   	= 0;
	fat.lMatch 				= 0;
	fat.idRegistry			= 0;
	fat.usCodePage			= GpiQueryCp(hps);
	fat.lMaxBaselineExt	= 0;
	fat.lAveCharWidth		= 0;
	fat.fsType				= 0;
	fat.fsFontUse			= FATTR_FONTUSE_OUTLINE |
								  FATTR_FONTUSE_TRANSFORMABLE;
	strcpy(fat.szFacename, pszFace);
	return GpiCreateLogFont(hps, NULL, lcid, &fat);
}

PFONTLIST FindFont(USHORT idFace, USHORT usSize, USHORT fsSelection)
{
	PFONTLIST pfl;
	for (pfl = pflFirst; pfl != NULL; pfl = pfl->pflNext) {
		if (idFace == pfl->idFace &&
			 usSize == pfl->usSize &&
			 fsSelection == pfl->fsSelection) {
			return pfl;
		}
	}
	return NULL;
}

PFONTLIST AddFont(USHORT idFace, USHORT usSize, USHORT fsSelection, LONG lcid, BOOL fOutline)
{
	PFONTLIST pfl = malloc(sizeof(FONTLIST));
	pfl->pflNext = pflFirst;
	pflFirst = pfl;
	pfl->idFace = idFace;
	pfl->usSize = usSize;
	pfl->fsSelection = fsSelection;
	pfl->lcid = lcid;
	pfl->fOutline = fOutline;
	return pfl;
}

LONG FindOutlineFont(USHORT idFace)
{
	PFONTLIST pfl;
	for (pfl = pflFirst; pfl != NULL; pfl = pfl->pflNext) {
		if (pfl->fOutline && pfl->idFace == idFace) {
			return pfl->lcid;
		}
	}
	return 0L;
}

HFONT PresGetFont(HPS hps, PSZ pszFace, USHORT usSize, USHORT fsSelection)
{
	register int i;
	USHORT idFace;
	PFONTLIST pfl;
	LONG lcid;
	BOOL fOutline = FALSE;
	for (i = 0; i < FONTFACES; i++) {
		if (stricmp(pszFace, szFacename[i]) == 0) {
			break;
		}
	}
	if (i == FONTFACES) {
		return NULL;
	}
	idFace = i;
	pfl = FindFont(idFace, usSize, fsSelection);
	if (pfl == NULL) {
		for (i = 0; i < FONTSIZES; i++) {
			if (sFontSize[i] == 10 * usSize) {
				break;
			}
		}
		lcid = lcidFont;
		if (i == FONTSIZES ||
		 	 EzfCreateLogFont(hps, lcidFont, idFace, i, fsSelection) == GPI_ERROR) {
			if ((lcid = FindOutlineFont(idFace)) == 0L) {
				EzfCreateOutlineFont(hps, lcidFont, pszFace);
				lcid = lcidFont++;
			}
			fOutline = TRUE;
		} else {
			lcidFont++;
		}
		pfl = AddFont(idFace, usSize, fsSelection, lcid, fOutline);
	}
	return (HFONT)pfl;
}

BOOL PresSetFont(HPS hps, PVOID hf)
{
	PFONTLIST pfl = hf;
	if (GpiSetCharSet(hps, pfl->lcid) == GPI_ERROR) {
		return FALSE;
	}
	if (pfl->fOutline) {			// outline ?
		SIZEF sizf;
		POINTL ptl;
		sizf.cx = sizf.cy = MAKEFIXED(pfl->usSize, 0);
		GpiSetCharBox(hps, &sizf);
		if (pfl->fsSelection & FATTR_SEL_ITALIC) {
			ptl.x = 2; ptl.y = 4;
		} else {
			ptl.x = 0; ptl.y = 1;
		}
		GpiSetCharShear(hps, &ptl);
	}
	return TRUE;
}
