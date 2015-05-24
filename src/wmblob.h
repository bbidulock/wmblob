
#ifndef WMBLOB_H
#define WMBLOB_H

#define MAX_N_BLOBS 50

typedef struct
{
	unsigned char r, g, b;
} COLOR;

typedef struct
{
	COLOR color[3];
	int n_blobs;         /* 1-50  */
	int gravity;         /* 0-1   */
	int blob_size;       /* 0-220 */
	int blob_falloff;    /* 0-255 */
	int blob_presence;   /* 0-255 */
	int border_size;     /* 0-220 */
	int border_falloff;  /* 0-255 */
	int border_presence; /* 0-255 */
	int multiplication;  /* 0-127 */
} SETTINGS;

extern SETTINGS current_settings;
extern int exit_wmblob;

extern void apply_settings(SETTINGS *);

#endif

