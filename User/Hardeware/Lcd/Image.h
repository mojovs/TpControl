//
// Created by Meng on 2025/12/11.
//

#ifndef TPCONTROL_IMAGES_H
#define TPCONTROL_IMAGES_H
#define WRITE_IMAGE
#ifdef WRITE_IMAGE
void write_image();

void write_img_alarms(void);
void write_img_fire(void);
void write_img_ble(void);
void write_img_buzz(void);
void write_img_humidity(void);
void write_img_clock(void);
void write_img_days(void);
void write_img_fan(void);
void write_img_egg(void);
void write_img_chicken(void);
extern const unsigned char gImage_alarm1[288];
extern const unsigned char gImage_alarm2[288];
extern const unsigned char gImage_fire[288] ;
extern const unsigned char gImage_set_fire[288] ;
extern const unsigned char gImage_ble[288];
extern const unsigned char gImage_buzz[288];
extern const unsigned char gImage_chicken[1152];
extern const unsigned char gImage_clock[288];
extern const unsigned char gImage_days[288] ;
extern const unsigned char gImage_egg[1152] ;
extern const unsigned char gImage_fan[288] ;
extern const unsigned char gImage_humidity[288];
#endif



#endif //TPCONTROL_IMAGES_H