typedef PVOID HFONT;

BOOL  PresInitFonts(HPS hps);
HFONT PresGetFont(HPS hps, PSZ pszFace, USHORT usSize, USHORT fsSelection);
BOOL  PresSetFont(HPS hps, HFONT hf);


#define FONTFACE_SYSTEM_PROP	0
#define FONTFACE_COUR			1
#define FONTFACE_HELV			2
#define FONTFACE_TIMES			3
#define FONTFACE_SYSTEM_MONO	4

#define FONTFACES					5

#define FONTSIZE_8				0
#define FONTSIZE_10				1
#define FONTSIZE_12				2
#define FONTSIZE_14				3
#define FONTSIZE_18				4
#define FONTSIZE_24				5

#define FONTSIZES					6


