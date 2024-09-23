#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "solve.h"
#include "prun_table.h"

//                           2-----------2------------1
//                           | U1(0)   U2(1)   U3(2)  |
//                           |                        |
//                           3 U4(3)   U5(4)   U6(5)  1
//                           |                        |
//                           | U7(6)   U8(7)   U9(8)  |
//  2-----------3------------3-----------0------------0-----------1------------1------------2------------2
//  | L1(36)  L2(37)  L3(38) | F1(18)  F2(19)  F3(20) | R1(9)   R2(10)  R3(11) |  B1(45)  B2(46)  B3(47) |
//  |                        |                        |                        |                         |
// 11 L4(39)  L5(40)  L6(41) 9 F4(21)  F5(22)  F6(23) 8 R4(12)  R5(13)  R6(14) 10 B4(48)  B5(49)  B6(50) 11
//  |                        |                        |                        |                         |
//  | L7(42)  L8(43)  L9(44) | F7(24)  F8(25)  F9(26) | R7(15)  R8(16)  R9(17) |  B7(51)  B8(52)  B9(53) |
//  3-----------7------------5-----------4------------4-----------5------------7------------6------------3
//                           | D1(27)  D2(28)  D3(29) |
//                           |                        |
//                           7 D4(30)  D5(31)  D6(32) 5
//                           |                        |
//                           | D7(33)  D8(34)  D9(35) |
//                           6-----------6------------7

// 用于转换两种表示方式 20个棱块角块 <---> 54个面
// [UF, UR, UB, UL,DF, DR, DB, DL, FR, FL, BR, BL] [UFR, URB, UBL, ULF, DRF, DFL, DLB, DBR]
// UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB
const uint8_t edge_to_face[12][2] = {
    {7, 19},  {5, 10},  {1, 46},  {3, 37},
    {28, 25}, {32, 16}, {34, 52}, {30, 43},
    {23, 12}, {21, 41}, {48, 14}, {50, 39}
};
const uint8_t corner_to_face[8][3] = {
    {8, 20, 9}, {2, 11, 45}, {0, 47, 36}, {6, 38, 18},
    {29, 15, 26}, {27, 24, 44}, {33, 42, 53}, {35, 51, 17}
};
const char edge_index[24][2] = {
    "UF","UR","UB","UL","DF","DR","DB","DL","FR","FL","BR","BL",
    "FU","RU","BU","LU","FD","RD","BD","LD","RF","LF","RB","LB"
};
const char corner_index[24][3] = {
    "UFR","URB","UBL","ULF","DRF","DFL","DLB","DBR",
    "FRU","RBU","BLU","LFU","RFD","FLD","LBD","BRD",
    "RUF","BUR","LUB","FUL","FDR","LDF","BDL","RDB"
};
// 魔方旋转时各个块的变化
const char route_index[18][3] = {
    "L","L'","L2","R","R'","R2",
    "U","U'","U2","D","D'","D2",
    "F","F'","F2","B","B'","B2"
};
typedef enum route_enum {
    L=0, L3, L2, R, R3, R2, U, U3, U2, D, D3, D2, F, F3, F2, B, B3, B2
}route_t;
//Group  # positions      Factor
//G0=<U  D  L  R  F  B >  4.33·10^19  2,048 (2^11)
//G1=<U  D  L  R  F2 B2>  2.11·10^16  1,082,565 (12C4 ·3^7)
//G2=<U  D  L2 R2 F2 B2>  1.95·10^10  29,400 ( 8C4^2 ·2·3)
//G3=<U2 D2 L2 R2 F2 B2>  6.63·10^5   663,552 (4!^5/12)
const uint8_t G0_allow[18] = {L, L3, L2, R, R3, R2, U, U3, U2, D, D3, D2, F, F3, F2, B, B3, B2};
const uint8_t G1_allow[14] = {L, L3, L2, R, R3, R2, U, U3, U2, D, D3, D2, F2, B2};
const uint8_t G2_allow[10] = {U, U3, D, D3, L2, R2, U2, D2, F2, B2};
const uint8_t G3_allow[6] =  {L2, R2, U2, D2, F2, B2};

const uint8_t route_tab_ep[18][12]={
    {0,1,2,11,4,5,6,9,8,3,10,7},// L  0
    {0,1,2,9,4,5,6,11,8,7,10,3},// L' 1
    {0,1,2,7,4,5,6,3,8,11,10,9},// L2 2
    {0,8,2,3,4,10,6,7,5,9,1,11},// R  3
    {0,10,2,3,4,8,6,7,1,9,5,11},// R' 4
    {0,5,2,3,4,1,6,7,10,9,8,11},// R2 5
    {1,2,3,0,4,5,6,7,8,9,10,11},// U  6
    {3,0,1,2,4,5,6,7,8,9,10,11},// U' 7
    {2,3,0,1,4,5,6,7,8,9,10,11},// U2 8
    {0,1,2,3,7,4,5,6,8,9,10,11},// D  9
    {0,1,2,3,5,6,7,4,8,9,10,11},// D' 10
    {0,1,2,3,6,7,4,5,8,9,10,11},// D2 11
    {9,1,2,3,8,5,6,7,0,4,10,11},// F  12
    {8,1,2,3,9,5,6,7,4,0,10,11},// F' 13
    {4,1,2,3,0,5,6,7,9,8,10,11},// F2 14
    {0,1,10,3,4,5,11,7,8,9,6,2},// B  15
    {0,1,11,3,4,5,10,7,8,9,2,6},// B' 16
    {0,1,6,3,4,5,2,7,8,9,11,10} // B2 17
};
const uint8_t route_tab_er[18][12]={
    {0,0,0,0,0,0,0,0,0,0,0,0},// L  0
    {0,0,0,0,0,0,0,0,0,0,0,0},// L' 1
    {0,0,0,0,0,0,0,0,0,0,0,0},// L2 2
    {0,0,0,0,0,0,0,0,0,0,0,0},// R  3
    {0,0,0,0,0,0,0,0,0,0,0,0},// R' 4
    {0,0,0,0,0,0,0,0,0,0,0,0},// R2 5
    {0,0,0,0,0,0,0,0,0,0,0,0},// U  6
    {0,0,0,0,0,0,0,0,0,0,0,0},// U' 7
    {0,0,0,0,0,0,0,0,0,0,0,0},// U2 8
    {0,0,0,0,0,0,0,0,0,0,0,0},// D  9
    {0,0,0,0,0,0,0,0,0,0,0,0},// D' 10
    {0,0,0,0,0,0,0,0,0,0,0,0},// D2 11
    {1,0,0,0,1,0,0,0,1,1,0,0},// F  12
    {1,0,0,0,1,0,0,0,1,1,0,0},// F' 13
    {0,0,0,0,0,0,0,0,0,0,0,0},// F2 14
    {0,0,1,0,0,0,1,0,0,0,1,1},// B  15
    {0,0,1,0,0,0,1,0,0,0,1,1},// B' 16
    {0,0,0,0,0,0,0,0,0,0,0,0} // B2 17
};
const uint8_t route_tab_cp[18][8]={
    {0,1,6,2,4,3,5,7},// L  0
    {0,1,3,5,4,6,2,7},// L' 1
    {0,1,5,6,4,2,3,7},// L2 2
    {4,0,2,3,7,5,6,1},// R  3
    {1,7,2,3,0,5,6,4},// R' 4
    {7,4,2,3,1,5,6,0},// R2 5
    {1,2,3,0,4,5,6,7},// U  6
    {3,0,1,2,4,5,6,7},// U' 7
    {2,3,0,1,4,5,6,7},// U2 8
    {0,1,2,3,5,6,7,4},// D  9
    {0,1,2,3,7,4,5,6},// D' 10
    {0,1,2,3,6,7,4,5},// D2 11
    {3,1,2,5,0,4,6,7},// F  12
    {4,1,2,0,5,3,6,7},// F' 13
    {5,1,2,4,3,0,6,7},// F2 14
    {0,7,1,3,4,5,2,6},// B  15
    {0,2,6,3,4,5,7,1},// B' 16
    {0,6,7,3,4,5,1,2} // B2 17
};
const uint8_t route_tab_cr[18][8]={
    {0,0,2,1,0,2,1,0},// L  0
    {0,0,2,1,0,2,1,0},// L' 1
    {0,0,0,0,0,0,0,0},// L2 2
    {2,1,0,0,1,0,0,2},// R  3
    {2,1,0,0,1,0,0,2},// R' 4
    {0,0,0,0,0,0,0,0},// R2 5
    {0,0,0,0,0,0,0,0},// U  6
    {0,0,0,0,0,0,0,0},// U' 7
    {0,0,0,0,0,0,0,0},// U2 8
    {0,0,0,0,0,0,0,0},// D  9
    {0,0,0,0,0,0,0,0},// D' 10
    {0,0,0,0,0,0,0,0},// D2 11
    {1,0,0,2,2,1,0,0},// F  12
    {1,0,0,2,2,1,0,0},// F' 13
    {0,0,0,0,0,0,0,0},// F2 14
    {0,2,1,0,0,0,2,1},// B  15
    {0,2,1,0,0,0,2,1},// B' 16
    {0,0,0,0,0,0,0,0} // B2 17
};

// 54个面的表示方式，转换为棱块角块表示方式
void cube_from_face_54(cube_t *c, const char *cube_str)
{
    for(int i=0; i<12; i++){
        int index_a = edge_to_face[i][0];
        int index_b = edge_to_face[i][1];
        int tmp;
        for(tmp=0;tmp<24;tmp++){
            if( edge_index[tmp][0] == cube_str[index_a] &&
                edge_index[tmp][1] == cube_str[index_b] ){
                break;
            }
        }
        c -> ep[i] = tmp % 12;
        c -> er[i] = tmp / 12;
    }
    for(int i=0; i<8; i++){
        int index_a = corner_to_face[i][0];
        int index_b = corner_to_face[i][1];
        int index_c = corner_to_face[i][2];
        int tmp;
        for(tmp=0;tmp<24;tmp++){
            if( corner_index[tmp][0] == cube_str[index_a] &&
                corner_index[tmp][1] == cube_str[index_b] &&
                corner_index[tmp][2] == cube_str[index_c]){
                break;
            }
        }
        c -> cp[i] = tmp % 8;
        c -> cr[i] = tmp / 8;
    }
}
// 输出54个面的状态
void cube_to_face_54(cube_t *c, char *cube_str)
{
    cube_str[4]  = 'U';
    cube_str[13] = 'R';
    cube_str[22] = 'F';
    cube_str[31] = 'D';
    cube_str[40] = 'L';
    cube_str[49] = 'B';
    cube_str[54] = '\0';
    for(int i=0; i<12; i++){
        int index_a = edge_to_face[i][0];
        int index_b = edge_to_face[i][1];
        const char *s = edge_index[c->ep[i] + c->er[i] * 12];
        cube_str[index_a] = s[0];
        cube_str[index_b] = s[1];
    }
    for(int i=0; i<8; i++){
        int index_a = corner_to_face[i][0];
        int index_b = corner_to_face[i][1];
        int index_c = corner_to_face[i][2];
        const char *s = corner_index[c->cp[i] + c->cr[i] * 8];
        cube_str[index_a] = s[0];
        cube_str[index_b] = s[1];
        cube_str[index_c] = s[2];
    }
}
// 拧魔方输入d=0-17,输入c1, 输出c2
void cube_route_and_new(cube_t *c1, cube_t *c2, route_t d)
{
    for(int i=0; i<12; i++){
        c2 -> ep[i] = c1 -> ep[route_tab_ep[d][i]];
        c2 -> er[i] = c1 -> er[route_tab_ep[d][i]] ^ route_tab_er[d][i];
    }
    for(int i=0; i<8; i++){
        c2 -> cp[i] = c1 -> cp[route_tab_cp[d][i]];
        c2 -> cr[i] = (c1 -> cr[route_tab_cp[d][i]] + route_tab_cr[d][i] ) % 3;
    }
    c2 -> step = c1 -> step + 1;
    c2 -> last_step = d;
}
void cube_route(cube_t *c1, route_t d)
{
    uint8_t ep_tmp[12], er_tmp[12], cp_tmp[12], cr_tmp[12];
    for(int i=0; i<12; i++){
        ep_tmp[i] = c1 -> ep[route_tab_ep[d][i]];
        er_tmp[i] = c1 -> er[route_tab_ep[d][i]] ^ route_tab_er[d][i];
    }
    for(int i=0; i<8; i++){
        cp_tmp[i] = c1 -> cp[route_tab_cp[d][i]];
        cr_tmp[i] = (c1 -> cr[route_tab_cp[d][i]] + route_tab_cr[d][i] ) % 3;
    }
    for(int i=0; i<12; i++){
        c1 -> ep[i] = ep_tmp[i];
        c1 -> er[i] = er_tmp[i];
    }
    for(int i=0; i<8; i++){
        c1 -> cp[i] = cp_tmp[i];
        c1 -> cr[i] = cr_tmp[i];
    }
}

int cube_route_str(cube_t *c1, const char *str)
{
    const char *p = str;
    int count = 0;
    // 整理格式，顺便统计数量
    while(1)
    {
        if(*p == 'U' || *p == 'R' || *p == 'F' || *p == 'D' || *p == 'L' || *p == 'B'){
            route_t r;
            switch(*p){
                case 'L':
                    r = L;
                    break;
                case 'R':
                    r = R;
                    break;
                case 'U':
                    r = U;
                    break;
                case 'D':
                    r = D;
                    break;
                case 'F':
                    r = F;
                    break;
                case 'B':
                    r = B;
                    break;
                //default:
                    //assert(0);
            }
            p++;
            switch(*p){
                case '\'':
                    r += 1;
                    break;
                case '2':
                    r += 2;
                    break;
            }
            cube_route(c1, r);
            count++;
            if(*p == 0){
                break;
            }
        }
        p++;
        if(*p == 0){
            break;
        }
    }
    return count;
}

// 编码4个数字的全排列，例如(1, 0, 2, 3) => 6
// 基于康托展开，支持不连续的数据
int encode_4P4(uint8_t *p){
    int n=0;
    for(int a=0;a<3;a++){      // 3、4都一样
        n *= 4-a;              // 计算阶乘
        for(int b = a+1; b < 4; b++){// 位于右边的比自己小的
            if(p[b] < p[a]){
                n += 1;
            }
        }
    }
    return n;
}
int encode_3P3(uint8_t *p){
    int n=0;
    for(int a=0;a<2;a++){
        n *= 3-a;                // 计算阶乘
        for(int b = a+1; b < 3; b++){// 位于右边的比自己小的
            if(p[b] < p[a]){
                n += 1;
            }
        }
    }
    return n;
}
// 只支持8、9、10、11中选2个
int encode_4P2(int a, int b)
{
    a = a - 8;
    b = b - 8;
    if(b > a){
        b -= 1;
    }
    return a * 3 + b;
}
// 组合 n*(n-1)*(n-m+1)/m!
int get_combinations(int n, int m)
{
    int c = 1;
    for(int i=0; i<m; i++){
        c *= n - i;
        c /= i + 1;
    }
    return c;
}
// 组合编码
// https://www.jaapsch.net/puzzles/compindx.htm
int encode_12C4_8_to_11(uint8_t *x){
    int comb = 0;
    int j = 4;
    for(int i=12-1; i>=0; i--){
        if(x[i] >= 8){ // 11 10 9 8
            comb += get_combinations(i,j);
            j -= 1;
        }
    }
    return comb;
}
int encode_8C4_1346(uint8_t *x){
    int comb = 0;
    int j = 4;
    for(int i=8-1; i>=0; i--){
        if(x[i] == 1 || x[i] == 3 || x[i] == 4 || x[i] == 6){
            comb += get_combinations(i,j);
            j -= 1;
        }
    }
    return comb;
}
int encode_8C4_1357(uint8_t *x){
    int comb = 0;
    int j = 4;
    for(int i=8-1; i>=0; i--){
        if(x[i] == 1 || x[i] == 3 || x[i] == 5 || x[i] == 7){
            comb += get_combinations(i,j);
            j -= 1;
        }
    }
    return comb;
}


static inline void swap(uint8_t *x, uint8_t *y)
{
    uint8_t temp = *y;
    *y = *x;
    *x = temp;
}
static inline uint8_t get_min_move(const uint8_t *tab, int index)
{
    uint8_t val = tab[index >> 1];
    if(index & 1){
        val >>= 4;
    }else{
        val &= 0x0F;
    }
    return val;
}
// 获取魔方状态的编码，查询这一编码对应的最小还原步骤数
int cube_estimate(cube_t *c, int encode_type)
{
    int ret = 0;
    if(encode_type == 0){
        // G0 编码角块方向 2^11 = 2048
        for(int i=0; i<11; i++){
            ret = ret | (c -> er[i] << i);
        }
        ret = get_min_move(G0_tab, ret);
    }else if(encode_type == 1){
        // 编码角块方向 3^7 = 2187
        for(int i=0; i<7; i++){
            ret = ret*3 + c -> cr[i];
        }
        // 编码中间层棱块(>=8)位置 12C4 = 495
        ret = 495 * ret + encode_12C4_8_to_11(c -> ep);
        ret = get_min_move(G1_tab, ret);
    }else if(encode_type == 2){
        // G2 编码角块，楞块的分轨道情况,8C4 * 8C4 * 6
        // 编码角块排列0-69
        int encode_c = encode_8C4_1346(c -> cp);
        // 编码棱块排列0-69
        int encode_e = encode_8C4_1357(c -> ep);
        // 两组位于不同轨道角块之间的关系，0-5
        // 分离两个轨道的角块，保持相对顺序不变
        uint8_t corn_a[4];
        uint8_t corn_b[4];
        int corn_a_index = 0;
        int corn_b_index = 0;
        
        for(int i=0; i<8; i++){
            int cpi = c -> cp[i];
            if(cpi == 1 || cpi == 3 || cpi == 4 || cpi == 6){
                corn_a[corn_a_index] = cpi; // 记录阴轨角顺序
                corn_a_index += 1;
            }else{// 0 2 5 7
                corn_b[corn_b_index] = cpi; // 记录阳轨角顺序
                corn_b_index += 1;
            }
        }
        // 还原corn_a 1,3,4,6，记录此时corn_b的状态
        if(corn_a[0] != 1){
            if(corn_a[1] == 1){
                // U2 corn_a 01交换 corn_b 01交换
                swap(&corn_a[0], &corn_a[1]);
                swap(&corn_b[0], &corn_b[1]);
            }else if(corn_a[2] == 1){
                // R2 02 03
                swap(&corn_a[0], &corn_a[2]);
                swap(&corn_b[0], &corn_b[3]);
            }else{
                // B2 03 13
                swap(&corn_a[0], &corn_a[3]);
                swap(&corn_b[1], &corn_b[3]);
            }
        }
        if(corn_a[1] != 3){
            if(corn_a[2] == 3){
                // F2 12 02
                swap(&corn_a[1], &corn_a[2]);
                swap(&corn_b[0], &corn_b[2]);
            }else{
                // L2 13 12
                swap(&corn_a[1], &corn_a[3]);
                swap(&corn_b[1], &corn_b[2]);
            }
        }
        if(corn_a[2] != 4){
            // D2 23 23
            swap(&corn_a[2], &corn_a[3]);
            swap(&corn_b[2], &corn_b[3]);
        }

        // 保持corn_a不变，还原corn_b[3]     
        if(corn_b[3] != 7){
            if(corn_b[2] == 7){
                swap(&corn_b[0], &corn_b[1]);
                swap(&corn_b[2], &corn_b[3]);
            }else if(corn_b[1] == 7){
                swap(&corn_b[0], &corn_b[2]);
                swap(&corn_b[1], &corn_b[3]);
            }else{ //corn_b[0] == 7
                swap(&corn_b[0], &corn_b[3]);
                swap(&corn_b[1], &corn_b[2]);
            }
        }
        ret = encode_3P3(corn_b) + encode_c*6 + encode_e*70*6;
        ret = get_min_move(G2_tab, ret);
    }else if(encode_type == 3){
        // 角块编码0-95 根据5个角块的状态计算 4！ * 4 = 96
        int encode_c;
        uint8_t tmp[4];
        if(c -> cp[7] == 0){
            encode_c = 0;
        }else if(c -> cp[7] == 2){
            encode_c = 1;
        }else if(c -> cp[7] == 5){
            encode_c = 2;
        }else{
            encode_c = 3;
        }
        tmp[0] = c -> cp[1];
        tmp[1] = c -> cp[3];
        tmp[2] = c -> cp[4];
        tmp[3] = c -> cp[6];
        encode_c += 4 * encode_4P4(tmp);// 0 - 95
        // 棱块编码 0 - 6911 根据10个棱块的状态计算 4！ * 4！ * 4P2 = 6912 
        tmp[0] = c -> ep[1];
        tmp[1] = c -> ep[3];
        tmp[2] = c -> ep[5];
        tmp[3] = c -> ep[7];
        int encode_e1 = encode_4P4(tmp);// 0 - 23
        tmp[0] = c -> ep[0];
        tmp[1] = c -> ep[2];
        tmp[2] = c -> ep[4];
        tmp[3] = c -> ep[6];
        int encode_e2 = encode_4P4(tmp);// 0 - 23
        int encode_e3 = encode_4P2(c -> ep[8], c -> ep[9]);// 0 - 11
        int encode_e = encode_e1 * 24 * 12 + encode_e2 * 12 + encode_e3;// 0 - 6911
        ret = encode_c  + 96 * encode_e;
        ret = get_min_move(G3_tab, ret);
    }
    return ret;
}
// stage       阶段标志0,1,2,3
// cube        魔方初始状态
// max_deep    最大搜索深度

cube_t cube_stack[256]; // 测试10000组数据，最大使用量91
int search(int stage, cube_t *cube, int max_deep, uint8_t *steps_record)
{
    const uint8_t *allow_route;
    int allow_route_length;
    int count = 0;
    cube_t *sp = cube_stack;
    cube -> step = 0;
    cube -> last_step = 0xFF;
    switch(stage){
        case 0:
            allow_route = G0_allow;
            allow_route_length = sizeof(G0_allow);
            break;
        case 1:
            allow_route = G1_allow;
            allow_route_length = sizeof(G1_allow);
            break;
        case 2:
            allow_route = G2_allow;
            allow_route_length = sizeof(G2_allow);
            break;
        default:
            allow_route = G3_allow;
            allow_route_length = sizeof(G3_allow);
            break;
    }
    memcpy(sp, cube, sizeof(cube_t));
    sp += 1;
    while(sp > cube_stack){
        cube_t c;
        sp -= 1;
        memcpy(&c, sp, sizeof(cube_t)); // 取栈顶部的元素
        int estimate = cube_estimate(&c, stage);
        //printf("%d,%d,%d,%d\n",c.last_step,estimate,c.step,max_deep);
        if( c.step - 1 >= 0){
            steps_record[c.step - 1] = c.last_step;
        }
        if(estimate == 0){
            //printf("max_deep = %d, search count = %d\n", max_deep, count);
            memcpy(cube, &c, sizeof(cube_t));
            return c.step; 
        }
        // 舍弃max_deep步骤内无解的
        if(estimate + c.step <= max_deep){
            count += 1;
            for(int i=0; i<allow_route_length; i++){
                int r = allow_route[i];
                // 如果旋转的面和上次旋转的一致，舍弃，可减少1/6的情况
                if(c.last_step / 3 != r / 3){
                    cube_route_and_new(&c, sp, r);
                    sp += 1;
                }
            }
        }
    }
    //if(count > 1){
    //    printf("max_deep = %d, search count = %d\n", max_deep, count);
    //}
    return -1;
}

char *solve(int stage, cube_t *cube, char *p)
{
    uint8_t steps_record[18];
    for(int deep=0; deep<18; deep++){
        int step = search(stage, cube, deep, steps_record);
        if(step >= 0){
            for(int x=0; x<step; x++){
                const char *p_src = route_index[ steps_record[x] ];
                while(*p_src != 0){
                    *p++ = *p_src++;
                }
                *p++ = ' ';
            }
            break;
        }
    }
    *p++ = '\0';
    return p;
}
/*
void test_case_1(const char *str)
{
    cube_t cube;
    char solution[200];
    char *p = solution;
    // BBBFULRUBUURFRRRDFDFLUFDLRUUUFFDRLDLRRFLLBBLFDLUBBDDBD
    // RULDUFRLULDDBRFRRFDDBUFRDLBRUULDUBRUFRFBLFDBBFBULBDLFL
    cube_from_face_54(&cube, str);
    for(int stage=0; stage<4; stage++){
        p = solve(stage, &cube, p);
    }
    *p++ = '\0';
    
    printf("%s\n",solution);
    //solve("BBBFULRUBUURFRRRDFDFLUFDLRUUUFFDRLDLRRFLLBBLFDLUBBDDBD");
    
    // 测试结果的正确性
    cube_from_face_54(&cube, str);
    int count = cube_route_str(&cube, solution);
    printf("totel steps = %d ",count);
    char cube_str[55];
    cube_to_face_54(&cube, cube_str);
    if(strcmp(cube_str,"UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB") == 0){
        printf("PASS\n");
    }else{
        printf("FAIL\n");
    }
}



int main(int argc ,char **argv)
{
    if(argc == 2){
        printf("argv[1]=%s\n", argv[1]);
        test_case_1(argv[1]);
        
    }else{
        printf("no input.\n");
        printf("%s BBBFULRUBUURFRRRDFDFLUFDLRUUUFFDRLDLRRFLLBBLFDLUBBDDBD\n", argv[0]);
    }
    
    
    return 0;
}
*/