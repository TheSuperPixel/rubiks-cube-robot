#ifndef _SOLVE_H
#define _SOLVE_H

typedef struct cube_struct{
    uint8_t ep[12];
    uint8_t er[12];
    uint8_t cp[8];
    uint8_t cr[8];
    uint8_t step;
    uint8_t last_step;
} cube_t;

// 54个面的表示方式，转换为棱块角块表示方式
void cube_from_face_54(cube_t *c, const char *cube_str);
void cube_to_face_54(cube_t *c, char *cube_str);
char *solve(int stage, cube_t *cube, char *p);

#endif