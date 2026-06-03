#ifndef __LCDFONT_H
#define __LCDFONT_H



typedef struct 
{
	unsigned char Index[3];
	unsigned char Msk[24];
}typFNT_GB12; 
typedef struct
{
	unsigned char Index[3];
	unsigned char Msk[32];
}typFNT_GB16;
typedef struct _typFNT_GB_24
{
	unsigned char Index[2];
	unsigned char Msk[72];
}typFNT_GB24;

#define TFONT_12_NUM 20
#define TFONT_16_NUM 20
#define TFONT_24_NUM 5
extern const unsigned char sz32[];
extern const unsigned char ascii_1206[][12];

extern const unsigned char ascii_1608[][16];

extern const unsigned char ascii_2412[][48];
#define FLASH_WRITE
#ifdef FLASH_WRITE


extern const typFNT_GB12 tfont12[TFONT_12_NUM];
extern const typFNT_GB16 tfont16[TFONT_16_NUM];
void lfs_write_GB12(void);
void lfs_write_GB16(void);
void lfs_write_asc_1206(void);
void lfs_write_asc_1608(void);
void lfs_write_sz_32(void);	//写数码管文件
#endif






#endif
