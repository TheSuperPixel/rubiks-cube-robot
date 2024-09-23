#include <stdio.h>
#include <inttypes.h>

static int max(int a, int b, int c)
{
    int max = a;
    if (b > max)
        max = b;
    if (c > max)
        max = c;
    return max;
}
static int min(int a, int b, int c)
{
    int min = a;
    if (b < min)
        min = b;
    if (c < min)
        min = c;
    return min;
}

// rgb转hsv 色调，饱和度，亮度 h,s,v,值的范围分别是0-360, 0-1023, 0-1023
// 输入范围0-1023
void rgb2hsv(int r, int g, int b, int *h, int *s, int *v)
{
    int mx = max(r, g, b);
    int mn = min(r, g, b);
    int m = mx - mn;
    if (mx == mn)
    {
        *h = 0;
    }
    else if (mx == r)
    {
        if (g >= b)
            *h = (g - b) * 60 / m;
        else
            *h = (g - b) * 60 / m + 360;
    }
    else if (mx == g)
    {
        *h = (b - r) * 60 / m + 120;
    }
    else if (mx == b)
    {
        *h = (r - g) * 60 / m + 240;
    }
    if (mx == 0)
    {
        *s = 0;
    }
    else
    {
        *s = 1023 * m / mx;
    }
    *v = mx;
}

// 输入uint16_t data[54][3]，rgb格式格式0-1023, char cube_str[55]
void color_detect(uint16_t *data, char *cube_str)
{
    int dark_count = 0;
    // 对魔方的6*9=54个颜色进行遍历。把data里的rgb数据转换为hsv
    for (int i = 0; i < 54; i++)
    {
        int h, s, v;
        rgb2hsv(data[i * 3 + 0], data[i * 3 + 1], data[i * 3 + 2], &h, &s, &v);
        // 红色和橙色h接近，而且接近色调接近0或者360，偏移处理，方便后续排序
        // 颜色    L橙 U黄 B绿 F蓝  R红
        // 典型色调 14  82 134 203 359
        // 偏移之后 93 161 213 282 75
        h -= 281;
        if (h < 0)
            h += 360;
        data[i * 3 + 0] = (uint16_t)h;
        data[i * 3 + 1] = (uint16_t)s;
        data[i * 3 + 2] = (uint16_t)v;
        if (v < 30)
        {
            dark_count++;
        }
        // printf("%d -- %d %d %d\n",i,h,s,v);
    }
    // 没有检测到魔方
    if (dark_count > 10)
    {
        printf("no rubiks cube find.\n");
        cube_str[0] = 0;
        return;
    }
    char index[54];
    for (int i = 0; i < 54; i++)
    {
        index[i] = i;
    }

    // 此时：
    // index[1] =1  颜色1
    // index[2] =2  颜色2
    // index[3] =3  颜色3
    // index[4] =4  颜色4
    // index[5] =5  颜色5
    // index[6] =6  颜色6
    // index[7] =7  颜色7

    //  按照data里的数据，把冒泡排序的动作加到临时索引index[i]
    // 寻找饱和度最小的14个,6个中心快，8个白色
    for (int i = 0; i < 14; i++)
    {
        for (int j = 0; j < 54 - i - 1; j++)
        {
            if (data[index[j] * 3 + 1] < data[index[j + 1] * 3 + 1])
            {
                char tmp = index[j];
                index[j] = index[j + 1];
                index[j + 1] = tmp;
            }
        }
    }
    // 剩下的按照色调排序
    for (int i = 0; i < 40 - 1; i++)
    {
        for (int j = 0; j < 40 - i - 1; j++)
        {
            if (data[index[j] * 3 + 0] < data[index[j + 1] * 3 + 0])
            {
                char tmp = index[j];
                index[j] = index[j + 1];
                index[j + 1] = tmp;
            }
        }
    }
    // for(int i=0; i<54; i++){
    //     uint16_t *p = data + index[i]*3;
    //     printf("%d -- %d %d %d\n",index[i],p[0],p[1],p[2]);
    // }
    // char cube_str[55];
    for (int i = 0; i < 8; i++)
        cube_str[index[i]] = 'F';
    for (int i = 8; i < 16; i++)
        cube_str[index[i]] = 'B';
    for (int i = 16; i < 24; i++)
        cube_str[index[i]] = 'U';
    for (int i = 24; i < 32; i++)
        cube_str[index[i]] = 'L';
    for (int i = 32; i < 40; i++)
        cube_str[index[i]] = 'R';
    for (int i = 40; i < 48; i++)
        cube_str[index[i]] = 'D';
    cube_str[4] = 'U';
    cube_str[13] = 'R';
    cube_str[22] = 'F';
    cube_str[31] = 'D';
    cube_str[40] = 'L';
    cube_str[49] = 'B';
    cube_str[54] = 0;
    // printf("%s\n",cube_str);
}
