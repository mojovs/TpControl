//
// Created by Meng on 2025/12/11.
//

#ifndef TPCONTROL_IMAGES_H
#define TPCONTROL_IMAGES_H

#ifndef WRITE_IMAGE
#define WRITE_IMAGE 1
#endif

#ifndef WRITE_IMAGE_ICON_12X12
#define WRITE_IMAGE_ICON_12X12 0
#endif

#ifndef WRITE_IMAGE_ICON_24X24
#define WRITE_IMAGE_ICON_24X24 0
#endif

#ifndef WRITE_IMAGE_ICON_48x24
#define WRITE_IMAGE_ICON_24X48 0
#endif

void write_images(void);

#if WRITE_IMAGE

void write_img(const char *path,const char *img_buf,uint32_t size);

#if WRITE_IMAGE_ICON_12X12
void write_icon_12x12(void);
#endif

#if WRITE_IMAGE_ICON_24X24
void write_icon_24x24(void);
#endif

#if WRITE_IMAGE_ICON_48x24
void write_icon_48x24(void);
#endif

#endif

#endif //TPCONTROL_IMAGES_H
