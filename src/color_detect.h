#ifndef COLOR_DETECT
#define COLOR_DETECT

void rgb2hsv(int r, int g, int b, int *h, int *s ,int *v);
void color_detect(uint16_t *data, char *cube_str);

#endif