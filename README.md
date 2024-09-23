# 三阶魔方还原机器人

## 介绍
低成本魔方机器人设计，总物料成本200元左右。 含单片机源码（c语言）、结构图和PCB工程。 使用**树莓派pico单片机**控制，控制和魔方求解都使用单片机完成。 
工程的主要部分参照了B站“爱跑步的小何”的三阶魔方机器人开源工程，但是在这个工程的基础上，添加了江协科技的OLED驱动库，制作了用户菜单页面，使得机器人不仅能还原魔方，还能根据用户的选择，做出不同的魔方花样。
代码结构图是我在阅读原版开源项目时为了方便理解绘制的，仅供参考
## 关于硬件设计
使用嘉立创专业版打开pcb文件夹下的工程文件即可。主控选用树莓派pcio单片机，步进电机驱动模块接口连接A4988模块模块，其他元件参照电路板工程设计，焊接完成后，参照演示视频，设置拨码开关，三个电机的细分调节档位全部拨到1，颜色传感器灯开关全部拨到0，电机待机开关全部设置为无效。


## 关于结构设计
使用**ABS材料**3D打印，尤其是夹魔方的结构件，对强度要求较高。


## 单片机固件编译(./src)
推荐使用**Linux**系统进行开发，可按照官方文档中的脚本搭建开发环境。不推荐Windows系统下搭建，坑很多的。

搭建好之后，修改**CMakeLists.txt**，把**PICO_SDK_PATH**设置为正确的路径
````
set(PICO_SDK_PATH "/home/wang/pico2/pico-sdk")
#"wang"need to change to your user name!
````
然后编译项目
````
cd src
mkdir build
cmake .
make -j2
````
然后找到cube_robot.uf2，刷写到单片机即可。

可以连接USB，使用minicom -D /dev/ttyACM0指令查看调试信息，例如：
````
color_detect: FLDFUUUDBDLRURRFDLBFRDFBFLRDDURDBBUBLURBLRLBLFFUFBLDRU
stage=0, D' B U' L F' 
stage=1, B2 R2 U2 F2 D R' L' U L' 
stage=2, R2 D' F2 D2 R2 D R2 D' B2 D' 
stage=3, B2 R2 U2 B2 U2 L2 U2 L2 F2 R2 B2 
````
## 参考资料
1.B站“爱跑步的小何”三阶魔方机器人工程 https://gitee.com/hemn1990/rubiks-cube-robot
2.江协科技OLED驱动 https://www.bilibili.com/video/BV1EN41177Pc






