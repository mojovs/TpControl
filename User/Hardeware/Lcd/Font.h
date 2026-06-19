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

#define TFONT_12_NUM 20
#define TFONT_16_NUM 20
extern const unsigned char ascii_1206[][12];



#endif