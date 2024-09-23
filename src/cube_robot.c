#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "color_detect.h"
// #include "hardware/i2c.h"
#include "solve.h"

#include "OLED.h"
// 步进电机使能信号 低有效 (20220706 由16改为2)
#define SPEPPER_EN 2
// 步进电机控制信号
#define SPEPPER_STEP0 11
#define SPEPPER_DIR0 10
#define SPEPPER_STEP1 9
#define SPEPPER_DIR1 8
#define SPEPPER_STEP2 7
#define SPEPPER_DIR2 6
// 霍尔开关，用于寻找零点
#define HALL_0 5
#define HALL_1 3
#define HALL_2 4
// 按键
#define BUTTON_0 0
#define BUTTON_1 1

#define BUTTON_FRONT 20
#define BUTTON_BACK 21
#define BUTTON_LEFT 18
#define BUTTON_RIGHT 19
#define BUTTON_PUSH 22

#define LED_RED 27
#define LED_GREEN 28
#define LED_BLUE 26

#define LED_PIN PICO_DEFAULT_LED_PIN

#define LED_PIN PICO_DEFAULT_LED_PIN
// 颜色传感器信号{{SDA,SCL},{SDA,SCL}}
const uint i2c_gpio[2][2] = {{14, 15}, {12, 13}};

// 根据实际测试结果调整电机初始角度
#define STEPPER0_OFFSET 485
#define STEPPER1_OFFSET 440
#define STEPPER2_OFFSET 325

// GPIO操作掩码，可以同时操作多个GPIO
#define MASK_STEPRR_01 ((1 << SPEPPER_STEP0) | (1 << SPEPPER_STEP1))
#define MASK_STEPRR_0 (1 << SPEPPER_STEP0)
#define MASK_STEPRR_1 (1 << SPEPPER_STEP1)
#define MASK_STEPRR_2 (1 << SPEPPER_STEP2)

// 顺时针、逆时针的定义，取决与驱动芯片的类型和电机的接线顺序
#define DIR0_CCW false
#define DIR0_CW true
#define DIR1_CCW false
#define DIR1_CW true
#define DIR2_CCW true
#define DIR2_CW false

#define RPM (200 * 16 / 60)
// #define SLOW_MODE
#ifdef SLOW_MODE
#define MAX_ACCEL 150000
#define V_START (30 * RPM)
#define V_MAX (90 * RPM)
#else
#define MAX_ACCEL 120000
#define V_START (150 * RPM)
#define V_MAX (300 * RPM)
#endif
// stepper_move(SPEPPER_STEP2,1600,180*RPM,300*RPM,200000);
// stepper_move(SPEPPER_STEP2,1600,180*RPM,300*RPM,150000);
int solve_color_flag = 0;

// 电机加速控制
// gpio_mask 1<<GPIO编号 steps：步数，v0：初始速度（脉冲/s），v：最高速度（脉冲/s），a：加速度（脉冲/s2）
void stepper_move(uint gpio_mask, int steps, float v0, float v, float a, const uint32_t *color_cmd)
{
    float vi = v0;
    const float F = 1000000.0f;                       // 定时器频率
    int s = (int)roundf((v * v - v0 * v0) / (2 * a)); // 计算加速过程需要的步数
    for (int i = 0; i < steps; i++)
    {
        // 计算需要的延时，单次计算过程耗时4us左右，debug和release模式差不多
        // 性能满足要求，不用考虑优化了
        //      4us
        //     +----+                      +----+
        //     |    |                      |    |
        //-----+    +----------------------+    +------
        //     |<----- roundf(F / vi) ----->
        absolute_time_t time_to_delay = get_absolute_time();
        gpio_set_mask(gpio_mask);
        float accel;
        if (i < s && i < steps / 2)
        {
            accel = a;
        }
        else if (i >= steps - s)
        {
            accel = -a;
        }
        else
        {
            accel = 0;
        }
        vi = sqrtf(vi * vi + 2.0f * accel);
        time_to_delay = delayed_by_us(time_to_delay, (uint64_t)roundf(F / vi));
        gpio_clr_mask(gpio_mask);
        // 旋转过程中采集颜色
        if (color_cmd && (i % 400) == 0)
        {
            uint32_t cmd = color_cmd[i / 400];
            if (cmd != 0)
            {
                multicore_fifo_push_blocking(cmd);
            }
        }
        sleep_until(time_to_delay);
    }
    if (color_cmd && (steps % 400) == 0)
    {
        uint32_t cmd = color_cmd[steps / 400];
        if (cmd != 0)
        {
            multicore_fifo_push_blocking(cmd); // 把魔方面的编号塞到FIFO里供core0使用
        }
    }
}
// 电机移动归零
void stepper_zero(void)
{
    bool s0, s1, s2;
    bool s0_ok = false;
    bool s1_ok = false;
    bool s2_ok = false;
    s0 = gpio_get(HALL_0);
    s1 = gpio_get(HALL_1);
    s2 = gpio_get(HALL_2);

    // 让三个电机一直转直到霍尔检测到
    while (!(s0_ok && s1_ok && s2_ok)) // s0_ok和s1_ok和s2_ok都为0时才循环
    {
        if (s0_ok == false)
        {
            gpio_put(SPEPPER_STEP0, 1);
            sleep_us(4);
            gpio_put(SPEPPER_STEP0, 0);
            if (!s0 && gpio_get(HALL_0))
                s0_ok = true;
            s0 = gpio_get(HALL_0);
        }
        if (s1_ok == false)
        {
            gpio_put(SPEPPER_STEP1, 1);
            sleep_us(4);
            gpio_put(SPEPPER_STEP1, 0);
            if (!s1 && gpio_get(HALL_1))
                s1_ok = true;
            s1 = gpio_get(HALL_1);
        }
        if (s2_ok == false)
        {
            gpio_put(SPEPPER_STEP2, 1);
            sleep_us(4);
            gpio_put(SPEPPER_STEP2, 0);
            if (!s2 && gpio_get(HALL_2))
                s2_ok = true;
            s2 = gpio_get(HALL_2);
        }
        sleep_us(400);
    }

    // 三个电机都转到初始位置
    stepper_move(MASK_STEPRR_0, STEPPER0_OFFSET, 100 * RPM, 300 * RPM, 150000, 0);
    stepper_move(MASK_STEPRR_1, STEPPER1_OFFSET, 100 * RPM, 300 * RPM, 150000, 0);
    stepper_move(MASK_STEPRR_2, STEPPER2_OFFSET, 100 * RPM, 300 * RPM, 150000, 0);
}
// IO初始化
void init_io(void)
{
    // 初始化引脚
    gpio_init(LED_PIN);
    gpio_init(SPEPPER_EN);
    gpio_init(SPEPPER_DIR0);
    gpio_init(SPEPPER_DIR1);
    gpio_init(SPEPPER_DIR2);
    gpio_init(SPEPPER_STEP0);
    gpio_init(SPEPPER_STEP1);
    gpio_init(SPEPPER_STEP2);
    gpio_init(SPEPPER_STEP2);
    gpio_init(HALL_0);
    gpio_init(HALL_1);
    gpio_init(HALL_2);
    gpio_init(BUTTON_0);
    gpio_init(BUTTON_1);

    gpio_init(BUTTON_LEFT);
    gpio_init(BUTTON_RIGHT);
    gpio_init(BUTTON_FRONT);
    gpio_init(BUTTON_BACK);
    gpio_init(BUTTON_PUSH);

    gpio_init(LED_RED);
    gpio_init(LED_GREEN);
    gpio_init(LED_BLUE);

    // 设置为低电平
    gpio_put(SPEPPER_EN, 0);
    gpio_put(SPEPPER_DIR0, 0);
    gpio_put(SPEPPER_DIR1, 0);
    gpio_put(SPEPPER_DIR2, 0);
    gpio_put(SPEPPER_STEP0, 0);
    gpio_put(SPEPPER_STEP1, 0);
    gpio_put(SPEPPER_STEP2, 0);

    gpio_put(LED_RED, 0);
    gpio_put(LED_GREEN, 0);
    gpio_put(LED_BLUE, 0);
    // 设置引脚方向
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_dir(SPEPPER_EN, GPIO_OUT);
    gpio_set_dir(SPEPPER_DIR0, GPIO_OUT);
    gpio_set_dir(SPEPPER_DIR1, GPIO_OUT);
    gpio_set_dir(SPEPPER_DIR2, GPIO_OUT);
    gpio_set_dir(SPEPPER_STEP0, GPIO_OUT);
    gpio_set_dir(SPEPPER_STEP1, GPIO_OUT);
    gpio_set_dir(SPEPPER_STEP2, GPIO_OUT);

    gpio_set_dir(HALL_0, GPIO_IN);
    gpio_set_dir(HALL_1, GPIO_IN);
    gpio_set_dir(HALL_2, GPIO_IN);
    gpio_set_dir(BUTTON_0, GPIO_IN);
    gpio_set_dir(BUTTON_1, GPIO_IN);

    gpio_set_dir(BUTTON_LEFT, GPIO_IN);
    gpio_set_dir(BUTTON_RIGHT, GPIO_IN);
    gpio_set_dir(BUTTON_FRONT, GPIO_IN);
    gpio_set_dir(BUTTON_BACK, GPIO_IN);
    gpio_set_dir(BUTTON_PUSH, GPIO_IN);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_set_dir(LED_BLUE, GPIO_OUT);
    // 设置上拉输入
    gpio_pull_up(HALL_0);
    gpio_pull_up(HALL_1);
    gpio_pull_up(HALL_2);
    gpio_pull_up(BUTTON_0);
    gpio_pull_up(BUTTON_1);

    gpio_pull_up(BUTTON_LEFT);
    gpio_pull_up(BUTTON_RIGHT);
    gpio_pull_up(BUTTON_FRONT);
    gpio_pull_up(BUTTON_BACK);
    gpio_pull_up(BUTTON_PUSH);
    /*
        gpio_set_function(12, GPIO_FUNC_I2C);
        gpio_set_function(13, GPIO_FUNC_I2C);
        gpio_set_function(14, GPIO_FUNC_I2C);
        gpio_set_function(15, GPIO_FUNC_I2C);
        i2c_init(i2c0, 20000);
        i2c_init(i2c1, 20000);
    */
    // 初始化iic引脚
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            gpio_init(i2c_gpio[i][j]);
            gpio_pull_up(i2c_gpio[i][j]);
            gpio_set_dir(i2c_gpio[i][j], GPIO_IN);
            gpio_put(i2c_gpio[i][j], 0);
        }
    }
}
void LED_Set(int led_color)
{
    if (led_color == 0)
    {
        gpio_put(LED_RED, 0);
        gpio_put(LED_GREEN, 0);
        gpio_put(LED_BLUE, 0);
    }
    if (led_color == 1)
    {
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 0);
        gpio_put(LED_BLUE, 0);
    }
    if (led_color == 2)
    {
        gpio_put(LED_RED, 0);
        gpio_put(LED_GREEN, 1);
        gpio_put(LED_BLUE, 0);
    }
    if (led_color == 3)
    {
        gpio_put(LED_RED, 0);
        gpio_put(LED_GREEN, 0);
        gpio_put(LED_BLUE, 1);
    }
    if (led_color == 4)
    {
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 1);
        gpio_put(LED_BLUE, 0);
    }
    if (led_color == 5)
    {
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 0);
        gpio_put(LED_BLUE, 1);
    }
    if (led_color == 6)
    {
        gpio_put(LED_RED, 0);
        gpio_put(LED_GREEN, 1);
        gpio_put(LED_BLUE, 1);
    }
    if (led_color == 7)
    {
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 1);
        gpio_put(LED_BLUE, 1);
    }
}
// --------------- 魔方控制部分 -----------------------
// 旋转魔方的上下(U D)面，注意参数只支持正数，U为逆时针，D为顺时针
// 参数取值范围0 800 1600 2400
void cube_ud(int face_u, int face_d)
{
    const int gap = 24;
    // 步骤1：卡住魔方中间层
    gpio_put(SPEPPER_DIR0, DIR0_CCW);
    gpio_put(SPEPPER_DIR1, DIR1_CW);
    stepper_move(MASK_STEPRR_01, gap, V_START, V_MAX, MAX_ACCEL, 0);
    // 步骤2：旋转需要的角度
    int min_val = face_u < face_d ? face_u : face_d;
    if (min_val >= 0)
        stepper_move(MASK_STEPRR_01, min_val, V_START, V_MAX, MAX_ACCEL, 0);
    face_u -= min_val;
    face_d -= min_val;
    if (face_u >= 0)
        stepper_move(MASK_STEPRR_0, face_u, V_START, V_MAX, MAX_ACCEL, 0);
    if (face_d >= 0)
        stepper_move(MASK_STEPRR_1, face_d, V_START, V_MAX, MAX_ACCEL, 0);
    // 步骤3：恢复初始位置
    gpio_put(SPEPPER_DIR0, DIR0_CW);
    gpio_put(SPEPPER_DIR1, DIR1_CCW);
    stepper_move(MASK_STEPRR_01, gap, V_START, V_MAX, MAX_ACCEL, 0);
}
// 旋转魔方的左前右后面(L F R B),参数正负都可以，正数表示顺时针，负数表示逆时针
// 参数取值范围 800 1600 -800 -1600
void cube_lfrb(int steps)
{
    bool dir;
    if (steps > 0)
    {
        dir = DIR2_CW;
    }
    else
    {
        dir = DIR2_CCW;
        steps = -steps;
    }
    // 步骤1：旋转臂接触魔方，接触之前应当减速，否则接触后受力突变，会丢步
    gpio_put(SPEPPER_DIR2, dir);
    stepper_move(MASK_STEPRR_2, 256, V_START, V_MAX, MAX_ACCEL, 0);
    // 步骤2：旋转需要的角度
    stepper_move(MASK_STEPRR_2, steps - 64, V_START, V_MAX, MAX_ACCEL, 0);
    // 步骤3：最后一小段要低速运转
    stepper_move(MASK_STEPRR_2, 64, V_START / 2, V_START / 2, MAX_ACCEL, 0);
    // 步骤4：恢复初始位置
    gpio_put(SPEPPER_DIR2, !dir);
    stepper_move(MASK_STEPRR_2, 256, V_START, V_MAX, MAX_ACCEL, 0);
}

// 翻转魔方,参数只能为正
// 参数取值范围 800 1600 2400
void flip_cube(int steps)
{
    gpio_put(SPEPPER_DIR0, DIR0_CW);
    gpio_put(SPEPPER_DIR1, DIR1_CCW);
    stepper_move(MASK_STEPRR_01, steps, V_START, V_MAX, MAX_ACCEL, 0);
}
// 采集颜色
void cube_get_color(void)
{
#define UD(x, y) (((x) << 8) | (y)) // 8位的x左移8位后和y拼合
    const uint32_t color_cmd_0_270_L[7] = {UD(36, 42), UD(23, 48), UD(38, 44), UD(10, 43), UD(9, 15), UD(50, 21), UD(11, 17)};
    const uint32_t color_cmd_1_180_R[5] = {0, UD(37, 16), UD(33, 27), UD(19, 52), UD(0, 6)};
    const uint32_t color_cmd_2_270_F[7] = {0, 0, UD(29, 35), UD(46, 25), UD(8, 2), 0, 0};
    const uint32_t color_cmd_3_180_B[5] = {0, 0, UD(18, 51), UD(12, 41), UD(47, 26)};
    const uint32_t color_cmd_4_270_R[7] = {0, 0, UD(45, 24), UD(39, 14), UD(20, 53), 0, 0};
    const uint32_t color_cmd_5_180_L[5] = {0, 0, 0, UD(5, 32), 0};
    // const uint32_t color_cmd_6_90_F[3]= {0,0,0};
    const uint32_t color_cmd_7_180_B[5] = {0, UD(3, 30), 0, UD(7, 28), 0};
    const uint32_t color_cmd_8_180[5] = {0, 0, 0, UD(1, 34), 0};

    gpio_put(SPEPPER_DIR0, DIR0_CW);
    gpio_put(SPEPPER_DIR1, DIR1_CCW);
    stepper_move(MASK_STEPRR_01, 2400, V_START, V_MAX, MAX_ACCEL, color_cmd_0_270_L);
    cube_lfrb(800);
    stepper_move(MASK_STEPRR_01, 1600, V_START, V_MAX, MAX_ACCEL, color_cmd_1_180_R);
    cube_lfrb(800);
    stepper_move(MASK_STEPRR_01, 2400, V_START, V_MAX, MAX_ACCEL, color_cmd_2_270_F);
    cube_lfrb(800);
    stepper_move(MASK_STEPRR_01, 1600, V_START, V_MAX, MAX_ACCEL, color_cmd_3_180_B);
    cube_lfrb(800);
    stepper_move(MASK_STEPRR_01, 2400, V_START, V_MAX, MAX_ACCEL, color_cmd_4_270_R);
    cube_lfrb(800);
    stepper_move(MASK_STEPRR_01, 1600, V_START, V_MAX, MAX_ACCEL, color_cmd_5_180_L);
    cube_lfrb(800);
    stepper_move(MASK_STEPRR_01, 800, V_START, V_MAX, MAX_ACCEL, 0);
    cube_lfrb(800);
    stepper_move(MASK_STEPRR_01, 1600, V_START, V_MAX, MAX_ACCEL, color_cmd_7_180_B);
    cube_lfrb(800);
    stepper_move(MASK_STEPRR_01, 1600, V_START, V_MAX, MAX_ACCEL, color_cmd_8_180);
}
// 拧魔方,支持如下参数
// "U", "R", "F", "D", "L", "B"
// "U'", "R'", "F'", "D'", "L'", "B'"
// "U2", "R2", "F2", "D2", "L2", "B2"
// 关于U、D的任意操作可以合并执行，分别传递action action2参数即可
char cube_tweak(char face_on_stepper_2, char *action, char *action2)
{
    char face = action[0];
    char face_after_tweak = face_on_stepper_2;
    int angle_u = 0, angle_d = 0;
    int angle_lfrb = 0;
    int flip_time = 0;
    assert(face == 'U' || face == 'R' || face == 'F' || face == 'D' || face == 'L' || face == 'B');

    // 如果要转动的面是F B L R 可以同时转0，1号电机，使目标面面对2号电机，然后转动2号电机即可
    if (face == 'L' || face == 'F' || face == 'R' || face == 'B')
    {
        // 顺时针转90度，逆时针转90度，转180度的判断
        if (action[1] == '\'')
        {
            angle_lfrb = -800;
        }
        else if (action[1] == '2')
        {
            angle_lfrb = 1600;
        }
        else
        {
            angle_lfrb = 800;
        }
        face_after_tweak = face;
        // F -> R -> B -> L -> F
        // 一共16种组合
        // face_on_stepper_2 -> 用于翻转魔方的0，1号电机转动前，面向2号电机的面
        // face_after_tweak -> 用于翻转魔方的0，1号电机转动后，面向2号电机的面
        // flip_time -> 0，1号电机应该转动的次数
        if (face_on_stepper_2 == 'F')
        {
            if (face_after_tweak == 'F')
            {
                flip_time = 0;
            }
            else if (face_after_tweak == 'R')
            {
                flip_time = 1;
            }
            else if (face_after_tweak == 'B')
            {
                flip_time = 2;
            }
            else if (face_after_tweak == 'L')
            {
                flip_time = 3;
            }
        }
        else if (face_on_stepper_2 == 'R')
        {
            if (face_after_tweak == 'F')
            {
                flip_time = 3;
            }
            else if (face_after_tweak == 'R')
            {
                flip_time = 0;
            }
            else if (face_after_tweak == 'B')
            {
                flip_time = 1;
            }
            else if (face_after_tweak == 'L')
            {
                flip_time = 2;
            }
        }
        else if (face_on_stepper_2 == 'B')
        {
            if (face_after_tweak == 'F')
            {
                flip_time = 2;
            }
            else if (face_after_tweak == 'R')
            {
                flip_time = 3;
            }
            else if (face_after_tweak == 'B')
            {
                flip_time = 0;
            }
            else if (face_after_tweak == 'L')
            {
                flip_time = 1;
            }
        }
        else if (face_on_stepper_2 == 'L')
        {
            if (face_after_tweak == 'F')
            {
                flip_time = 1;
            }
            else if (face_after_tweak == 'R')
            {
                flip_time = 2;
            }
            else if (face_after_tweak == 'B')
            {
                flip_time = 3;
            }
            else if (face_after_tweak == 'L')
            {
                flip_time = 0;
            }
        }
        // 执行动作
        if (flip_time != 0)
        {
            flip_cube(flip_time * 800);
        }
        cube_lfrb(angle_lfrb);
    }
    else // 如果要转动的面是 U D 可以只转0或1号电机（反方向）
    {
        // U只能逆时针
        if (face == 'U')
        {
            if (action[1] == '\'')
            {
                angle_u = 800;
            }
            else if (action[1] == '2')
            {
                angle_u = 1600;
            }
            else
            {
                angle_u = 2400;
            }
            // D只能顺时针
        }
        else if (face == 'D')
        {
            if (action[1] == '\'')
            {
                angle_d = 2400;
            }
            else if (action[1] == '2')
            {
                angle_d = 1600;
            }
            else
            {
                angle_d = 800;
            }
        }
        if (action2 != 0)
        {
            // 可以同时拧2个面
            assert(action2[0] == 'U' || action2[0] == 'D');
            if (action2[0] == 'U')
            {
                assert(angle_u == 0);
                if (action2[1] == '\'')
                {
                    angle_u = 800;
                }
                else if (action2[1] == '2')
                {
                    angle_u = 1600;
                }
                else
                {
                    angle_u = 2400;
                }
            }
            else if (action2[0] == 'D')
            {
                assert(angle_d == 0);
                if (action2[1] == '\'')
                {
                    angle_d = 2400;
                }
                else if (action2[1] == '2')
                {
                    angle_d = 1600;
                }
                else
                {
                    angle_d = 800;
                }
            }
        }
        // 执行动作
        cube_ud(angle_u, angle_d);
    }

    return face_after_tweak;
}
// 按照求解结果执行动作
char cube_tweak_str(char face_on_stepper_2, char *str)
{

    printf("face_on_stepper_2=%d", face_on_stepper_2);
    int my_p;
    for (my_p = 0; my_p < 32; my_p++)
    {
        printf(" str[%d]=%c ", my_p, str[my_p]);
    }

    char *p = str;
    char *p_dst = str;
    int count = 0;
    // 整理格式，顺便统计数量
    while (1)
    {
        if (*p == 'U' || *p == 'R' || *p == 'F' || *p == 'D' || *p == 'L' || *p == 'B')
        {
            p_dst[count * 2] = *p;
            p++;
            p_dst[count * 2 + 1] = *p;
            count++;
            if (*p == 0)
            {
                break;
            }
        }
        p++;
        if (*p == 0)
        {
            break;
        }
    }
    p_dst[count * 2] = 0;
    for (int i = 0; i < count;)
    {
        if (i <= count - 2 &&
            ((p_dst[i * 2] == 'U' && p_dst[i * 2 + 2] == 'D') ||
             (p_dst[i * 2] == 'D' && p_dst[i * 2 + 2] == 'U')))
        {
            // printf("%c%c %c%c\n",p_dst[i*2],p_dst[i*2+1],p_dst[i*2+2],p_dst[i*2+3]);
            face_on_stepper_2 = cube_tweak(face_on_stepper_2, p_dst + i * 2, p_dst + i * 2 + 2);
            i += 2;
        }
        else
        {
            // printf("%c%c\n",p_dst[i*2],p_dst[i*2+1]);
            face_on_stepper_2 = cube_tweak(face_on_stepper_2, p_dst + i * 2, 0);
            i += 1;
        }
    }
    return face_on_stepper_2;
}

// ----------------------- 软件模拟I2C ------------------------------------
// 不知为何硬件I2C用不了，从设备不回复ACK
#define SDA0() gpio_set_dir(i2c_gpio[i2c][0], GPIO_OUT)
#define SDA1() gpio_set_dir(i2c_gpio[i2c][0], GPIO_IN)
#define SDA_READ() gpio_get(i2c_gpio[i2c][0])
#define SCL0() gpio_set_dir(i2c_gpio[i2c][1], GPIO_OUT)
#define SCL1() gpio_set_dir(i2c_gpio[i2c][1], GPIO_IN)
#define I2C_DELAY() sleep_us(4)
#define I2C_ACK 0
#define I2C_NACK 1
#define I2C_READ 1
#define I2C_WRITE 0
void i2c_start(uint i2c)
{
    // SCL -----__
    // SDA ---____
    SDA1();
    I2C_DELAY();
    SCL1();
    I2C_DELAY();
    SDA0();
    I2C_DELAY();
    SCL0();
    I2C_DELAY();
}
void i2c_stop(uint i2c)
{
    // SCL _------
    // SDA ____---
    SDA0();
    I2C_DELAY();
    SCL1();
    I2C_DELAY();
    SDA1();
    I2C_DELAY();
}
bool i2c_send(uint i2c, uint8_t date)
{
    bool ack;
    int i;
    for (i = 0; i < 8; i++)
    {
        if (date & 0x80)
            SDA1();
        else
            SDA0();
        I2C_DELAY();
        SCL1();
        I2C_DELAY();
        SCL0();
        date = date << 1;
    }
    SDA1(); // 释放总线，等待ACK
    I2C_DELAY();
    SCL1();
    I2C_DELAY();
    ack = SDA_READ();
    SCL0();
    // I2C_DELAY();
    return ack;
}

uint8_t i2c_get(uint i2c, bool ack)
{
    uint8_t get;
    int i;
    SDA1();
    I2C_DELAY();
    for (i = 0; i < 8; i++)
    {
        get = get << 1;
        SCL1();
        I2C_DELAY();
        get = get | SDA_READ();
        SCL0();
        I2C_DELAY();
    }
    if (ack)
        SDA1(); // NACK
    else
        SDA0(); // ACK
    I2C_DELAY();
    SCL1();
    I2C_DELAY();
    SCL0();
    return get;
}
// ----------------------- TCS3472驱动 ------------------------------------
// 确认传感器通信
// 返回true表示ID正确
bool tcs3472_check_id(int i2c)
{
    i2c_start(i2c);
    i2c_send(i2c, (0x29 << 1) | I2C_WRITE);
    i2c_send(i2c, 0x80 | 0x12); // 选择ID寄存器
    i2c_start(i2c);
    i2c_send(i2c, (0x29 << 1) | I2C_READ);
    uint8_t id = i2c_get(i2c, I2C_NACK);
    i2c_stop(0);
    return id == 0x4D;
}

void tcs3472_enable(int i2c)
{
    // ATIME = 0xFF
    i2c_start(i2c);
    i2c_send(i2c, (0x29 << 1) | I2C_WRITE);
    i2c_send(i2c, 0x80 | 0x01);
    i2c_send(i2c, 0xFF);
    i2c_stop(0);
    // WTIME = 0xFF
    i2c_start(i2c);
    i2c_send(i2c, (0x29 << 1) | I2C_WRITE);
    i2c_send(i2c, 0x80 | 0x03);
    i2c_send(i2c, 0xFF);
    i2c_stop(0);
    // AGAIN = 0x1
    i2c_start(i2c);
    i2c_send(i2c, (0x29 << 1) | I2C_WRITE);
    i2c_send(i2c, 0x80 | 0x0F);
    i2c_send(i2c, 0x1);
    i2c_stop(0);

    // ENABLE = PON
    i2c_start(i2c);
    i2c_send(i2c, (0x29 << 1) | I2C_WRITE);
    i2c_send(i2c, 0x80 | 0x00); // 选择ENABLE寄存器
    i2c_send(i2c, 0x01);        // PON
    i2c_stop(0);
    // delay >= 2.4ms
    sleep_ms(3);
    // ENABLE = PON + AEN
    i2c_start(i2c);
    i2c_send(i2c, (0x29 << 1) | I2C_WRITE);
    i2c_send(i2c, 0x80 | 0x00); // 选择ENABLE寄存器
    i2c_send(i2c, 0x03);        // PON + AEN
    i2c_stop(0);
}

void tcs3472_dump(int i2c, uint16_t *r, uint16_t *g, uint16_t *b)
{
    int i;
    i2c_start(i2c);
    i2c_send(i2c, (0x29 << 1) | I2C_WRITE);
    i2c_send(i2c, 0xA0 | 0x16); // 选择RDATAL寄存器,并且开启地址累加
    i2c_start(i2c);
    i2c_send(i2c, (0x29 << 1) | I2C_READ);
    *r = i2c_get(i2c, I2C_ACK);
    *r |= i2c_get(i2c, I2C_ACK) << 8;
    *g = i2c_get(i2c, I2C_ACK);
    *g |= i2c_get(i2c, I2C_ACK) << 8;
    *b = i2c_get(i2c, I2C_ACK);
    *b |= i2c_get(i2c, I2C_NACK) << 8;
    i2c_stop(0);
}
// 调试用的，根据stdin输入的字符执行不同动作
/*
void debug_zero_offset(void)
{
    int offset[3] = {0,0,0};
    while (true) {
        uint dir_pin=SPEPPER_DIR0, step_pin=SPEPPER_STEP0;
        int index=0, value=0;
        bool dir = false;
        printf("input: ");
        scanf("%d,%d", &index, &value);
        switch (index)
        {
        case 0:
            dir_pin=SPEPPER_DIR0;
            step_pin=SPEPPER_STEP0;
            break;
        case 1:
            dir_pin=SPEPPER_DIR1;
            step_pin=SPEPPER_STEP1;
            break;
        case 2:
            dir_pin=SPEPPER_DIR2;
            step_pin=SPEPPER_STEP2;
            break;
        case 3: // debug
            cube_ud(800,0);
            cube_lfrb(800);
            flip_cube(800);
            cube_ud(0,800);
            cube_ud(800,800);
            cube_lfrb(-800);
            flip_cube(1600);
            cube_ud(1600,800);
            cube_lfrb(1600);
            cube_ud(1600,2400);
            cube_lfrb(1600);
            break;
        case 4: // debug
            flip_cube(800);
            break;
        default:
            printf("index must be 0,1 or 2.\n");
            continue;
            break;
        }
        offset[index] += value;
        if(value > 0){
            dir = false;
        }else{
            dir = true;
            value = -value;
        }
        gpio_put(dir_pin, dir);
        stepper_move(1 << step_pin, value, 30*RPM,90*RPM, MAX_ACCEL,0);
        printf("offset = [%d, %d, %d]\n", offset[0], offset[1], offset[2]);
    }
}
*/

// 主程序
// 1#核心 负责电机控制
void main_core1()
{
    // 步进电机回零
    // debug_zero_offset();
    stepper_zero();
    // gpio_put(SPEPPER_EN, 1);
    // char cmd[16];
    // char cmd2[16];
    // char x = 'F';
    while (true)
    {
        // 等待按键按下
        while (!solve_color_flag)
        {
            sleep_ms(10);
        }
        solve_color_flag = 0;
        // 采集每一块的颜色
        cube_get_color();
        // 等待计算完成后执行对应的动作，后3组可以边计算边执行前面的动作
        char face = 'F';
        for (int stage = 0; stage < 4; stage++)
        {
            char *solution = (char *)multicore_fifo_pop_blocking();
            if (solution != 0 && solution[0] != 0)
            {
                face = cube_tweak_str(face, solution); // 按照求解结果执行动作
            }
        }
    }
}

// 主程序
// 0#核心 负责颜色采集分析，魔方求解

// MCU的栈较小，因此比较大的数组，放在堆上，防止栈溢出
uint16_t color_buffer[54 * 3] = {0};
char cube_str[55] = {0};
char solution_str[4][55] = {0};

int main(void)
{
    // sleep_ms(5000);
    stdio_init_all();
    init_io();   // 初始化所有io
    OLED_Init(); // oled初始化

    multicore_launch_core1(main_core1); // 启动core 1 核心

    // 检查颜色传感器是否正常，并且初始化
    for (int i = 0; i < 2; i++)
    {
        if (tcs3472_check_id(i))
        {
            tcs3472_enable(i);
            printf("tcs3472_check_id(%d) pass\n", i);
            OLED_Printf(0, i * 8, OLED_6X8, "color_id(%d) pass\n", i);
        }
        else
        {
            printf("tcs3472_check_id(%d) fail\n", i);
            OLED_Printf(0, 0, OLED_6X8, "color_id(%d) fail\n", i);
            while (1)
                ;
        }
        sleep_ms(100);
    }
    OLED_Printf(0, 16, OLED_6X8, "start stepper zero");
    OLED_Update();
    sleep_ms(1000);
    LED_Set(1);
    //  显示菜单
    int menu_1 = 0; // 菜单1高亮标记
    int menu_2 = 0; // 菜单2高亮标记
    int page = 0;   // 页数标记
    while (1)
    {
        if (gpio_get(BUTTON_FRONT) == 0)
        {
            sleep_ms(10);
            while (gpio_get(BUTTON_FRONT) == 0)
                ;
            sleep_ms(10);
            if (page == 0)
            {
                if (menu_1 == 1)
                {
                    menu_1 = 0;
                }
            }
            else if (page == 1)
            {
                if (menu_2 > 0)
                {
                    menu_2--;
                }
            }
        }
        if (gpio_get(BUTTON_BACK) == 0)
        {
            sleep_ms(10);
            while (gpio_get(BUTTON_BACK) == 0)
                ;
            sleep_ms(10);
            if (page == 0)
            {
                if (menu_1 == 0)
                {
                    menu_1 = 1;
                }
            }
            else if (page == 1)
            {
                if (menu_2 < 2)
                {
                    menu_2++;
                }
            }
        }
        if (gpio_get(BUTTON_LEFT) == 0)
        {
            sleep_ms(10);
            while (gpio_get(BUTTON_LEFT) == 0)
                ;
            sleep_ms(10);
            if (page == 1)
            {
                page = 0;
            }
        }
        if (page == 0)
        {
            OLED_Clear();
            OLED_Printf(0, 0, OLED_8X16, "function list");
            OLED_Printf(0, 16, OLED_8X16, "1: solve!");
            OLED_Printf(0, 32, OLED_8X16, "2: pattern");
            if (menu_1 == 0)
            {
                OLED_ShowChar(100, 16, '*', OLED_8X16);
                OLED_ShowChar(100, 32, ' ', OLED_8X16);
            }
            if (menu_1 == 1)
            {
                OLED_ShowChar(100, 16, ' ', OLED_8X16);
                OLED_ShowChar(100, 32, '*', OLED_8X16);
            }
            OLED_Update();
        }
        else if (page == 1)
        {
            OLED_Clear();
            OLED_Printf(0, 0, OLED_8X16, "pattern list");
            OLED_Printf(0, 16, OLED_8X16, "1: pattern1");
            OLED_Printf(0, 32, OLED_8X16, "2: pattern2");
            OLED_Printf(0, 48, OLED_8X16, "3: pattern3");
            if (menu_2 == 0)
            {
                OLED_ShowChar(100, 16, '*', OLED_8X16);
                OLED_ShowChar(100, 32, ' ', OLED_8X16);
                OLED_ShowChar(100, 48, ' ', OLED_8X16);
            }
            if (menu_2 == 1)
            {
                OLED_ShowChar(100, 16, ' ', OLED_8X16);
                OLED_ShowChar(100, 32, '*', OLED_8X16);
                OLED_ShowChar(100, 48, ' ', OLED_8X16);
            }
            if (menu_2 == 2)
            {
                OLED_ShowChar(100, 16, ' ', OLED_8X16);
                OLED_ShowChar(100, 32, ' ', OLED_8X16);
                OLED_ShowChar(100, 48, '*', OLED_8X16);
            }
            OLED_Update();
        }

        if (gpio_get(BUTTON_RIGHT) == 0)
        {
            sleep_ms(10);
            while (gpio_get(BUTTON_RIGHT) == 0)
                ;
            sleep_ms(10);
            if (page == 0)
            {
                if (menu_1 == 0)
                {
                    solve_color_flag = 1;
                    // OLED_Clear();
                    // OLED_Update();
                    break;
                }
                if (menu_1 == 1)
                {
                    page = 1;
                }
            }
            else if (page == 1)
            {
                if (menu_2 == 0)
                {
                    LED_Set(2);
                    char my_solution[40] = {
                        'F', '0', '0',
                        'F', '\'', '0',
                        'B', '0', '0',
                        'B', '\'', '0',
                        'L', '0', '0',
                        'L', '\'', '0',
                        'R', '0', '0',
                        'R', '\'', '0',
                        'U', '0', '0',
                        'U', '\'', '0',
                        'D', '0', '0',
                        'D', '\'', '0'};
                    cube_tweak_str('F', my_solution);
                    LED_Set(3);
                }
                else if (menu_2 == 1)
                {
                    LED_Set(2);
                    char my_solution[40] = {
                        'F', '0', '0',
                        'F', '0', '0',
                        'B', '0', '0',
                        'B', '0', '0',
                        'L', '0', '0',
                        'L', '0', '0',
                        'R', '0', '0',
                        'R', '0', '0',
                        'U', '0', '0',
                        'U', '0', '0',
                        'D', '0', '0',
                        'D', '0', '0'};
                    cube_tweak_str('F', my_solution);
                    LED_Set(3);
                }
                else if (menu_2 == 2) // F D2 L2 B D B' F2 U' F U F2 U2 F' L D F' U
                {
                    LED_Set(2);
                    char my_solution[75] = {
                        'F', '0', '0',
                        'D', '0', '0',
                        'D', '0', '0',
                        'L', '0', '0',
                        'L', '0', '0',
                        'B', '0', '0',
                        'D', '0', '0',
                        'B', '\'', '0',
                        'F', '0', '0',
                        'F', '0', '0',
                        'U', '\'', '0',
                        'F', '0', '0',
                        'U', '0', '0',
                        'F', '0', '0',
                        'F', '0', '0',
                        'U', '0', '0',
                        'U', '0', '0',
                        'F', '\'', '0',
                        'L', '0', '0',
                        'D', '0', '0',
                        'F', '\'', '0',
                        'U', '0', '0'};
                    cube_tweak_str('F', my_solution);
                    LED_Set(3);
                }
            }
        }
        solve_color_flag = 0;
    }

    // 还原魔方部分
    while (1)
    {
        // 在接到core1的通知时采集颜色
        for (int i = 0; i < 24; i++)
        {
            uint32_t g = multicore_fifo_pop_blocking();
            // FIFO存储器是系统的缓冲环节
            // 这个函数从FIFO中“弹出”数据。数据以int32格式返回。 同样，该函数将阻塞程序运行，直到有数据准备好被读取。
            if (g < 0xFFFF)
            {
                uint16_t r0, g0, b0, r1, g1, b1;
                tcs3472_dump(0, &r0, &g0, &b0); // 把传感器0上之前识别的颜色取回来,通过指针返回r，g，b
                tcs3472_dump(1, &r1, &g1, &b1); // 把传感器1上之前识别的颜色取回来,通过指针返回r，g，b
                color_buffer[(g >> 8) * 3 + 0] = r0;
                color_buffer[(g >> 8) * 3 + 1] = g0;
                color_buffer[(g >> 8) * 3 + 2] = b0;
                color_buffer[(g & 0xff) * 3 + 0] = r1;
                color_buffer[(g & 0xff) * 3 + 1] = g1;
                color_buffer[(g & 0xff) * 3 + 2] = b1;
                // printf("ID=(%u,%u) r0=%u g0=%u b0=%u r1=%u g1=%u b1=%u\n",g>>8,g&0xFF,r0,g0,b0,r1,g1,b1);
            }
        }
        // 识别颜色
        printf("color_buffer: %s\n", color_buffer);
        color_detect(color_buffer, cube_str); // 把color_buffer的rgb值转换为颜色的编号，存到cube_str
        for (int i = 0; i < 54 * 3; i++)
        {
            // 用完清理数据，这很重要，这BUG调了一小时
            // 魔方中间块对应的6个数据，每次重新采样是不会更新的
            color_buffer[i] = 0;
        }
        printf("color_detect: %s\n", cube_str);

        // OLED_Printf(0, 0, OLED_6X8, "get color ok!");
        // OLED_Printf(0, 8, OLED_6X8, "start solveing!");
        // OLED_Update();

        if (cube_str[0] != 0)
        {
            // 求解魔方
            cube_t cube;
            cube_from_face_54(&cube, cube_str); // 54个面的表示方式，转换为棱块角块表示方式
            for (int stage = 0; stage < 4; stage++)
            {
                char *p = solution_str[stage];
                p = solve(stage, &cube, p); // 求解魔方

                printf("stage=%d, %s\n", stage, solution_str[stage]);

                // OLED_Clear();
                // OLED_Printf(0, 8 + 8 * stage, OLED_6X8, "stage=%d, %s\n", stage, solution_str[stage]);

                multicore_fifo_push_blocking((uint32_t)solution_str[stage]); // 把求解结果送到FIFO供core1使用
            }
        }
        else
        {
            for (int stage = 0; stage < 4; stage++)
            {
                multicore_fifo_push_blocking(0);
            }
        }
    }
    return 0;
}
