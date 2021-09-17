/*
** Present.c		(NC)Not Copyrighted by The Frobozz Magic Software Company
**
**	Present a slide show 
*/

#define INCL_WIN
#define INCL_DOS
#define INCL_GPI
#define INCL_BITMAPFILEFORMAT
#define INCL_WINSTDFILE

#include <os2.h>
#include "presfont.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <libc\ctype.h>
#include "present.h"

CHAR  szApp[] = "Present!";
HAB	hab;
HWND	hwndFrame, hwndClient;

HPS	hps;
HDC	hdcMem;

HPS	hpsBmp;
HDC	hdcBmpmem;

BOOL	fPresSelected;

POINTL	ptlBmpsz;

struct _sheet;

typedef struct _sheet SHEET;
typedef SHEET *PSHEET;
typedef struct _line SLINE;
typedef SLINE *PSLINE;
typedef struct _pres PRES;
typedef PRES *PPRES;

struct _line {
	PSLINE		plNext;
	PSHEET 	pshSheet;
	PSZ		pszText;
	LONG		lColour;
	HFONT		hf;
	USHORT	usAlign;
	POINTL	aptl[TXTBOX_COUNT];
	USHORT	usShadow;
	USHORT	usDepth;
};

struct _sheet {
	PSHEET	pshNext;
	PSHEET	pshFollowing;
	PSHEET	pshPrev;
	PSLINE	plFirst;
	PSZ		pszBitmap;
	USHORT	usMode;
	USHORT	usID;
	USHORT	usNextID;
};


struct _pres {
	PSZ		pszTitle;
	POINTL	ptlSize;
	PSHEET	pshFirst;
	BOOL		fHasbmp;
};

char foo[400];

PRES pres;


typedef struct _nlin NLIN;
typedef NLIN *PNLIN;

struct _nlin {
	LONG		lColour;
	PSZ		pszText;
	CHAR		chFont[20];
	USHORT	usAlign;
	USHORT	usType;
	USHORT	usPoint;
	USHORT	usSub;
	USHORT	usFx;
};

#define	FX_SHADOW		1
#define	FX_DEPTH			2



/* ------------- Keyword defines */

#define K_INVALID		256

#define K_TITLE		0
#define K_SIZE			1
#define K_PAGE			2
#define K_SHEET		3
#define K_MODE			4
#define K_NEXT			5
#define K_SLINE			6
#define K_ALIGN		7
#define K_FONT			8
#define K_TYPE			9
#define K_POINT		10
#define K_COLOUR		11
#define K_SUB			12
#define K_BITMAP		13
#define K_FX			14

USHORT KeywordId(PSZ pszKey)
{
	static struct {
		CHAR 		szWord[10];
		USHORT	usID;
	} idLst[] = {
		"title",	K_TITLE,
		"size",	K_SIZE,
		"page",	K_PAGE,
		"sheet",	K_SHEET,
		"mode",	K_MODE,
		"next",	K_NEXT,
		"line",	K_SLINE,
		"align",	K_ALIGN,
		"font",	K_FONT,
		"type",	K_TYPE,
		"point",	K_POINT,
		"colour",K_COLOUR,
		"sub",	K_SUB,
		"bitmap",K_BITMAP,
		"fx",		K_FX
	};
	register int i;
	for (i = 0; i < sizeof(idLst)/sizeof(idLst[0]); i++) {
		if (stricmp(pszKey, idLst[i].szWord) == 0) {
			return idLst[i].usID;
		}
	}
	return K_INVALID;
}

VOID PresInit(VOID)
{
	pres.ptlSize.x = pres.ptlSize.y = 0L;
	pres.fHasbmp = FALSE;
}

VOID PresSetSize(PSZ pszLine)
{
	sscanf(pszLine, "%ld,%ld", &pres.ptlSize.x, &pres.ptlSize.y);
}

#define BLOCK_FACT	64

VOID PresLoadBmp(HPS hpsBmp, PSZ pszFile)
{
	register int i;
	PCHAR pchBuf;
	BITMAPFILEHEADER bfh;
	ULONG ulWidth;
	PBITMAPINFO pbmi;
	HBITMAP hbm;
	FILE *f = fopen(pszFile, "rb");
	if (f == NULL) {
		CHAR szMsg[120];
		sprintf(szMsg, "Cannot access bitmap file '%s'", pszFile);
		WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, szMsg, szApp, 0, MB_ICONEXCLAMATION | MB_ENTER);
	}
	fread(&bfh, 1, sizeof(bfh), f);
	pbmi = malloc(bfh.offBits - sizeof(bfh) + sizeof(BITMAPINFOHEADER));
	fseek(f, -sizeof(BITMAPINFOHEADER), SEEK_CUR);
	fread(pbmi, 1, bfh.offBits - sizeof(bfh) + sizeof(BITMAPINFOHEADER), f);

	ulWidth = bfh.cbSize - bfh.offBits;
	pchBuf = malloc(ulWidth);
	fread(pchBuf, 1, ulWidth, f);
	hbm = GpiCreateBitmap(hpsBmp, (PBITMAPINFOHEADER2)pbmi, CBM_INIT, pchBuf, (PBITMAPINFO2)pbmi);
	GpiSetBitmap(hpsBmp, hbm);
	pres.fHasbmp = TRUE;
	ptlBmpsz.x = bfh.bmp.cx - 1;
	ptlBmpsz.y = bfh.bmp.cy - 1;
	free(pbmi);
	free(pchBuf);
	fclose(f);
}

VOID PresAddSheet(PPRES ppr, PSHEET psh)
{
	if (psh->usID != 0) {			/* new sheet added ?? */
		PSHEET pshNew = malloc(sizeof(SHEET));
		PSHEET *ppsh;
		*pshNew = *psh;
		pshNew->pshNext = pshNew->pshPrev = NULL;
		for (ppsh = &(ppr->pshFirst); *ppsh != NULL; ppsh = &((*ppsh)->pshNext)) {
			/* EMPTY, find last sheet in list */
		}
		*ppsh = pshNew;

		/* reset edit sheet */
		psh->pshFollowing = NULL;
		psh->plFirst = NULL;
		psh->usID = psh->usNextID = 0;
	}
}

VOID PresLinkSheets(PPRES ppr)
{
	PSHEET psh1, psh2;
	for (psh1 = ppr->pshFirst; psh1 != NULL; psh1 = psh1->pshNext) {
		if (psh1->usNextID != 0) {
			for (psh2 = ppr->pshFirst; psh2 != NULL; psh2 = psh2->pshNext) {
				if (psh2->usID == psh1->usNextID) {
					psh1->pshFollowing = psh2;
					psh2->pshPrev = psh1;
					break;
				}
			}
		}
	}
}

VOID SheetAddLine(PSHEET psh, PNLIN pnl)
{
	if (pnl->pszText != NULL) {
		PSLINE pl = malloc(sizeof(SLINE));
		PSLINE *ppl;
		pl->lColour = pnl->lColour;
		pl->pszText = pnl->pszText;
		pl->usAlign = pnl->usAlign;
		pl->usShadow = (pnl->usFx & FX_SHADOW) ? (pnl->usPoint / 10) : 0;
		pl->usDepth = (pnl->usFx & FX_DEPTH) ? (pnl->usPoint / 20) : 0;
		pl->pshSheet = NULL;
		pl->plNext = NULL;
		pl->hf = PresGetFont(hps, pnl->chFont, pnl->usPoint, pnl->usType);
		PresSetFont(hps, pl->hf);
		GpiQueryTextBox(hps, strlen(pl->pszText), pl->pszText, TXTBOX_COUNT, pl->aptl);
		for (ppl = &(psh->plFirst); *ppl != NULL; ppl = &((*ppl)->plNext)) {
			/* EMPTY, find last slot */
		}
		*ppl = pl;
		pnl->pszText = NULL;
	}
}

USHORT LineParseType(PSZ psz)
{
	USHORT usType = 0;
	for (;*psz != 0; psz++) {
		switch (*psz) {
		case 'B' :
		case 'b' :		usType |= FATTR_SEL_BOLD;			break;
		case 'I' :
		case 'i' :		usType |= FATTR_SEL_ITALIC;		break;
		case 'U' :
		case 'u' :		usType |= FATTR_SEL_UNDERSCORE;	break;
		case 'S' :
		case 's' :		usType |= FATTR_SEL_STRIKEOUT;	break;
		}
	}
	return usType;
}

USHORT LineParseFx(PSZ psz)
{
	USHORT usFx = 0;
	for (;*psz != 0; psz++) {
		switch (*psz) {
		case 'D' :
		case 'd' :		usFx |= FX_DEPTH;			break;
		case 'S' :
		case 's' :		usFx |= FX_SHADOW;		break;
		}
	}
	return usFx;
}

struct {
	PSZ pszCol;
	LONG lValue;
} sCol[] = {
	"white",		CLR_WHITE,
	"black",		CLR_BLACK,
	"blue",		CLR_BLUE,
	"red",		CLR_RED,
	"pink",		CLR_PINK,
	"green",		CLR_GREEN,
	"cyan",		CLR_CYAN,
	"yellow",	CLR_YELLOW,
	"darkgray",	CLR_DARKGRAY,
	"darkblue",	CLR_DARKBLUE,
	"darkred",	CLR_DARKRED,
	"darkpink",	CLR_DARKPINK,
	"darkgreen",CLR_DARKGREEN,
	"darkcyan",	CLR_DARKCYAN,
	"brown",		CLR_BROWN,
	"palegray",	CLR_PALEGRAY
};


LONG LineParseColour(PSZ psz)
{
	register int i;
	for (i = 0; i < sizeof(sCol)/sizeof(sCol[0]); i++) {
		if (stricmp(psz, sCol[i].pszCol) == 0) {
			return sCol[i].lValue;
		}
	}
	return CLR_BLACK;
}

NLIN nlDefault = {CLR_BLACK, NULL, "Helv", 'C', 0, 12, 0, 0 };

BOOL LoadPresentation(PSZ pszFile)
{
	char szKeyword[10];
	char szLine[80];
	SHEET sheet;
	NLIN line;
	FILE *f = fopen(pszFile, "r");
	if (f == NULL) {
		sprintf(szLine, "Presentation file '%s' not found", pszFile);
		WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, szLine, szApp, 0, MB_ICONEXCLAMATION | MB_ENTER);
		return FALSE;
	}
	PresInit();
	sheet.usID = 0;
	sheet.plFirst = NULL;
	line = nlDefault;
	while (!feof(f)) {
		fscanf(f, "%s %[^\n]\n", szKeyword, szLine);
		switch (KeywordId(szKeyword)) {
		case K_TITLE	:	pres.pszTitle = strdup(szLine);			break;
		case K_SIZE		:	PresSetSize(szLine);							break;
		case K_PAGE		:	PresLoadBmp(hpsBmp, szLine);				break;
		case K_SHEET	:	SheetAddLine(&sheet, &line);
								PresAddSheet(&pres, &sheet);
								sheet.usID = atoi(szLine);					break;
		case K_MODE		:	sheet.usMode = toupper((*szLine));		break;
		case K_NEXT		:	sheet.usNextID = atoi(szLine);			break;
		case K_SLINE		:	SheetAddLine(&sheet, &line);
								line.pszText = strdup(szLine);			break;
		case K_ALIGN	:	line.usAlign = *szLine;						break;
		case K_FONT		:	strcpy(line.chFont, szLine);				break;
		case K_TYPE		:	line.usType = LineParseType(szLine);	break;
		case K_FX		:	line.usFx = LineParseFx(szLine);			break;
		case K_POINT	:	line.usPoint = atoi(szLine);				break;
		case K_COLOUR	:	line.lColour = LineParseColour(szLine);break;
		case K_SUB		:	break;
		case K_INVALID	:
			sprintf(szLine, "Fout in presentatie bestand. (%s)", szKeyword);
			WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, szLine, szApp, 0, MB_ICONEXCLAMATION | MB_ENTER);
			fclose(f);
			return FALSE;
		}
	}
	/* term active line, term active sheet */
	SheetAddLine(&sheet, &line);
	PresAddSheet(&pres, &sheet);
	PresLinkSheets(&pres);
	fclose(f);
	return TRUE;
}

CHAR  *apszFace[] = {
	"Helv",
	"Tms Rmn",
   "Courier"
};

VOID ClearScreen(HPS hps)
{
	GpiErase(hps);
	if (pres.fHasbmp) {
		POINTL aptl[3];
		aptl[0].x = aptl[0].y = aptl[2].x = aptl[2].y = 0L;
		aptl[1]   = ptlBmpsz;
		GpiBitBlt(hps, hpsBmp, 3L, aptl, ROP_SRCCOPY, 0);
	}
}

#define X_MARGIN		10L
#define Y_MARGIN		10L
#define Y_DISTANCE	10L

VOID DrawScreen(HPS hps, PSHEET psh)
{
	PSLINE pl;
	POINTL ptl;
	ClearScreen(hps);
	ptl.y = pres.ptlSize.y - Y_MARGIN;		
	for (pl = psh->plFirst; pl != NULL; pl = pl->plNext) {
		PresSetFont(hps, pl->hf);
		switch (pl->usAlign) {
		case 'C' :
			ptl.x = (pres.ptlSize.x - pl->aptl[TXTBOX_BOTTOMRIGHT].x) / 2;
			break;
		case 'L' :
			ptl.x = X_MARGIN;
			if (psh->usMode == 'B') {				/* bullet list */
				POINTL ptlBullet;
				ULONG ulSize = pl->aptl[TXTBOX_TOPLEFT].y / 3;
				ptlBullet.x = X_MARGIN + ulSize;
				ptlBullet.y = ptl.y - pl->aptl[TXTBOX_TOPLEFT].y;
				GpiSetColor(hps, pl->lColour);
				GpiMove(hps, &ptlBullet);
				GpiFullArc(hps, DRO_FILL, MAKEFIXED(ulSize / 2, 0));
				ptl.x += ulSize * 3;
			}
			break;
		case 'R' :
			ptl.x = pres.ptlSize.x - pl->aptl[TXTBOX_BOTTOMRIGHT].x - X_MARGIN;
			break;
		case 'P' :		/* previous so don't change */
			break;
		}
		ptl.y -= pl->aptl[TXTBOX_TOPLEFT].y * 5 / 4;
		if (pl->usShadow) {
			POINTL ptlSh;
			ptlSh = ptl;
			ptlSh.y -= pl->usShadow;
			ptlSh.x += pl->usShadow;
			GpiSetColor(hps, CLR_BLACK);
			GpiCharStringAt(hps, &ptlSh, strlen(pl->pszText), pl->pszText);
		}
		if (pl->usDepth) {
			POINTL ptlSh;
			int i;
			ptlSh = ptl;
			ptlSh.y -= pl->usDepth;
			ptlSh.x += pl->usDepth;
			GpiSetColor(hps, CLR_BLACK);
			for (i = pl->usDepth; i > 0; i--) {
				GpiCharStringAt(hps, &ptlSh, strlen(pl->pszText), pl->pszText);
				ptlSh.x--;
				ptlSh.y++;
			}
		}
		GpiSetColor(hps, pl->lColour);
		GpiCharStringAt(hps, &ptl, strlen(pl->pszText), pl->pszText);
		ptl.y -= pl->aptl[TXTBOX_TOPLEFT].y / 4;
	}
}

VOID InitPresentation()
{
	CHAR szBuf[80];
	SHORT y = (SHORT)pres.ptlSize.y +
		WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR) +
		WinQuerySysValue(HWND_DESKTOP, SV_CYMENU);
	sprintf(szBuf, "%s : %s", szApp, pres.pszTitle);
	WinSetWindowText(hwndFrame, szBuf);
	WinSetWindowPos(hwndFrame, HWND_TOP, 0, 0,
		(SHORT)pres.ptlSize.x, y, /*SWP_ACTIVATE |*/ SWP_SIZE /*| SWP_SHOW*/);
}


MRESULT EXPENTRY AboutDlgProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
	switch (msg) {
	case WM_COMMAND :
	  	switch (COMMANDMSG(&msg)->cmd) {
	  	case ID_OK :
	   	WinDismissDlg(hwnd, TRUE);
	   	return 0;
		}
		break;
	}
   return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY GotoDlgProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
	switch (msg) {
	case WM_COMMAND :			  
	  	switch (COMMANDMSG(&msg)->cmd) {
	  	case ID_OK :
	   	WinDismissDlg(hwnd, TRUE);
	   	return 0;
		}
		break;
	}
   return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

VOID PresOpenFile(HWND hwnd)
{
	FILEDLG fdlg;
	HWND hwndDlg;
	memset(&fdlg, 0, sizeof(fdlg));
	fdlg.cbSize = sizeof(fdlg);
	fdlg.fl = FDS_CENTER | FDS_OPEN_DIALOG;
	fdlg.pszTitle = "Select presentation file";
	strcpy(fdlg.szFullFile, "*.PRS");
	hwndDlg = WinFileDlg(HWND_DESKTOP, hwnd, &fdlg);
	if (fdlg.lReturn == DID_OK) {
		if (fPresSelected = LoadPresentation(fdlg.szFullFile)) {
			InitPresentation();
		}
	}				
}

MRESULT EXPENTRY PresWndProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
	static PSHEET psh = NULL;
	static HWND hwndList;
	HPS hpsPaint;
	RECTL rcl;
	static HPOINTER hptr;
	POINTL aptl[3];
	SIZEF sizf;
	switch (msg) {
	case WM_CREATE :
		hptr = WinLoadPointer(HWND_DESKTOP, 0L, IDP_PTR);
		GpiErase(hps);
		PresSetFont(hps, PresGetFont(hps, "Tms Rmn", 120, FATTR_SEL_ITALIC));
		GpiSetColor(hps, CLR_RED);
		sizf.cx = MAKEFIXED(120, 0);
		sizf.cy = MAKEFIXED(200, 0);
		aptl[0].x = 20;
		aptl[0].y = 80;
		GpiSetCharBox(hps, &sizf);			/* fix for this screen */
		GpiCharStringAt(hps, &aptl[0], 8L, "Present!");
		PresSetFont(hps, PresGetFont(hps, "System Proportional", 10, FATTR_SEL_ITALIC));
		aptl[0].y -= 24;
		GpiSetColor(hps, CLR_DARKRED);
		GpiCharStringAt(hps, &aptl[0], 57L, "(NC)Not Copyrighted by the Frobozz Magic Software Company");
		break;
	case WM_DESTROY :
		WinDestroyPointer(hptr);
		break;
	case WM_MOUSEMOVE :
		WinSetPointer(HWND_DESKTOP, hptr);
		return (MRESULT)1L;
	case WM_PAINT :
		hpsPaint = WinBeginPaint(hwnd, 0L, &rcl);
		WinFillRect(hpsPaint, &rcl, CLR_BACKGROUND);
		aptl[1].x = rcl.xRight;
		aptl[1].y = rcl.yTop;
		aptl[0].x = aptl[2].x = rcl.xLeft;
		aptl[0].y = aptl[2].y = rcl.yBottom;
		GpiBitBlt(hpsPaint, hps, 3L, aptl, ROP_SRCCOPY, BBO_AND);
		WinEndPaint(hpsPaint);
		break;
	case WM_BUTTON1UP :
		if (psh->pshFollowing) {
			psh = psh->pshFollowing;
			WinPostMsg(hwnd, WM_USER, 0L, 0L);
		}
		break;
	case WM_BUTTON2UP :
		if (psh->pshPrev) {
			psh = psh->pshPrev;
			WinPostMsg(hwnd, WM_USER, 0L, 0L);
		}
		break;
	case WM_SIZE :
		WinInvalidateRect(hwnd, NULL, FALSE);	/* repaint everything */
		break;
	case WM_USER :
		if (psh) {
			DrawScreen(hps, psh);
			WinInvalidateRect(hwnd, NULL, FALSE);
		}
		break;
	case WM_CONTROL :
		if (SHORT2FROMMP(mp1) == LN_ENTER) {
			psh = WinSendMsg(hwndList, LM_QUERYITEMHANDLE,
				(MPARAM)WinSendMsg(hwndList, LM_QUERYSELECTION, 0L, 0L),
				0L);
			WinSendMsg(hwndList, WM_DESTROY, 0L, 0L);	/* drop list */
			WinSendMsg(hwnd, WM_USER, 0L, 0L);	/* draw next sheet */
		}
		break;
	case WM_COMMAND :
		switch (COMMANDMSG(&msg)->cmd) {
		case IDM_OPEN :
			PresOpenFile(hwnd);
			psh = pres.pshFirst;
			WinPostMsg(hwnd, WM_USER, 0L, 0L);
			break;
		case IDM_ABOUT :
			WinDlgBox(HWND_DESKTOP, hwnd, (PFNWP)AboutDlgProc, 0L, IDD_ABOUT, 0L);
			break;
		case IDM_GOTO :
		{
			PSHEET psh = pres.pshFirst;
			POINTL aptl[TXTBOX_COUNT];
			hpsPaint = WinGetPS(hwnd);
			GpiQueryTextBox(hpsPaint, 30, "The quick brown fox jumps over", TXTBOX_COUNT, aptl);
			hwndList = WinCreateWindow(hwnd, WC_LISTBOX, "",
				WS_VISIBLE, 100, 100, (SHORT)aptl[TXTBOX_TOPRIGHT].x,
				6 * (SHORT)aptl[TXTBOX_TOPRIGHT].y, hwnd, HWND_TOP, ID_LISTWINDOW, NULL, NULL);
			WinGetLastError(hab);
			while (psh != NULL) {
				WinSendMsg(hwndList, LM_SETITEMHANDLE,
					(MPARAM)WinSendMsg(hwndList, LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(psh->plFirst->pszText)),
					MPFROMP(psh));
				psh = psh->pshNext;
			}
			break;
		}
		case IDM_EXIT :
			WinPostMsg(hwnd, WM_QUIT, 0L, 0L);
			break;
		}
		break;
	case WM_ERASEBACKGROUND :
		return (MRESULT)TRUE;
	default :
   	return WinDefWindowProc(hwnd, msg, mp1, mp2);
	}
	return 0L;
}

VOID CreateHPS(VOID)
{
	BITMAPINFOHEADER bmp;
	HBITMAP hbm;
	SIZEL sizl;
	hdcMem = DevOpenDC(hab, OD_MEMORY, "*", 0L, (PDEVOPENDATA)0, (HDC)0);
	sizl.cx = 
	sizl.cy = 0;
	hps = GpiCreatePS(hab, hdcMem, &sizl, PU_PELS | GPIF_DEFAULT |
				GPIT_MICRO | GPIA_ASSOC);

	bmp.cbFix = sizeof(bmp);
	bmp.cx = 640;
	bmp.cy = 480;
	bmp.cPlanes = 1;
	bmp.cBitCount = 4;
	hbm = GpiCreateBitmap(hps, (PBITMAPINFOHEADER2)&bmp, 0L, NULL, NULL);

	GpiSetBitmap(hps, hbm);

	hdcBmpmem = DevOpenDC(hab, OD_MEMORY, "*", 0L, NULL, (HDC)0);
	hpsBmp = GpiCreatePS(hab, hdcBmpmem, &sizl, PU_PELS | GPIF_DEFAULT |
						GPIT_MICRO | GPIA_ASSOC);
	PresInitFonts(hps);
}

main()
{
	HMQ	hmq;
	QMSG	qmsg;
	ULONG	flFrameFlags = FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER | FCF_TASKLIST |
								FCF_SHELLPOSITION | FCF_ICON | FCF_MENU | FCF_MINBUTTON;

	hab = WinInitialize(0);
	hmq = WinCreateMsgQueue(hab, 0);

	CreateHPS();

	WinRegisterClass(hab, szApp, (PFNWP)PresWndProc, 0,  0);

	hwndFrame = WinCreateStdWindow(HWND_DESKTOP, WS_VISIBLE | WS_SYNCPAINT,
										    &flFrameFlags, szApp, szApp,
										    0L, (HMODULE)0, ID_APP, &hwndClient);

	while (WinGetMsg(hab, &qmsg, (HWND)0, 0, 0)) {
		WinDispatchMsg(hab, &qmsg);
	}
	WinDestroyWindow(hwndFrame);
	WinDestroyMsgQueue(hmq);
	WinTerminate(hab);
	return 0;
}


