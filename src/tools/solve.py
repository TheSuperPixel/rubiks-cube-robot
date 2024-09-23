#!/usr/bin/pypy3
# 推荐使用pypy，建立查找表的过程远比cpython快
import random
from collections import deque
from itertools import permutations
from itertools import combinations
import json
import os

# 参考资料
# https://www.jaapsch.net/puzzles/thistle.htm
# https://zhuanlan.zhihu.com/p/111623841
# https://www.zhihu.com/question/67518391
# http://bbs.mf8-china.com/forum.php?mod=viewthread&tid=104760
# http://bbs.mf8-china.com/forum.php?mod=viewthread&tid=106671
# https://tomas.rokicki.com/cubecontest/jaap.txt

#Group  # positions      Factor
#G0=<U  D  L  R  F  B >  4.33·10^19  2,048 (2^11)
#G1=<U  D  L  R  F2 B2>  2.11·10^16  1,082,565 (12C4 ·3^7)
#G2=<U  D  L2 R2 F2 B2>  1.95·10^10  29,400 ( 8C4^2 ·2·3)
#G3=<U2 D2 L2 R2 F2 B2>  6.63·10^5   663,552 (4!^5/12)


#G4=I   1

#                           2-----------2------------1
#                           | U1(0)   U2(1)   U3(2)  |
#                           |                        |
#                           3 U4(3)   U5(4)   U6(5)  1
#                           |                        |
#                           | U7(6)   U8(7)   U9(8)  |
#  2-----------3------------3-----------0------------0-----------1------------1------------2------------2
#  | L1(36)  L2(37)  L3(38) | F1(18)  F2(19)  F3(20) | R1(9)   R2(10)  R3(11) |  B1(45)  B2(46)  B3(47) |
#  |                        |                        |                        |                         |
# 11 L4(39)  L5(40)  L6(41) 9 F4(21)  F5(22)  F6(23) 8 R4(12)  R5(13)  R6(14) 10 B4(48)  B5(49)  B6(50) 11
#  |                        |                        |                        |                         |
#  | L7(42)  L8(43)  L9(44) | F7(24)  F8(25)  F9(26) | R7(15)  R8(16)  R9(17) |  B7(51)  B8(52)  B9(53) |
#  3-----------7------------5-----------4------------4-----------5------------7------------6------------3
#                           | D1(27)  D2(28)  D3(29) |
#                           |                        |
#                           7 D4(30)  D5(31)  D6(32) 5
#                           |                        |
#                           | D7(33)  D8(34)  D9(35) |
#                           6-----------6------------7
# 用于转换两种表示方式 20个棱块角块 <---> 54个面
# [UF, UR, UB, UL,DF, DR, DB, DL, FR, FL, BR, BL] [UFR, URB, UBL, ULF, DRF, DFL, DLB, DBR]
# UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB
edge_to_face =(('UF', 7, 19),  ('UR', 5, 10),  ('UB', 1, 46),  ('UL', 3, 37),
               ('DF', 28, 25), ('DR', 32, 16), ('DB', 34, 52), ('DL', 30, 43),
               ('FR', 23, 12), ('FL', 21, 41), ('BR', 48, 14), ('BL', 50, 39))
corner_to_face =(('UFR', 8, 20, 9), ('URB', 2, 11, 45), ('UBL', 0, 47, 36), ('ULF', 6, 38, 18),
                ('DRF', 29, 15, 26), ('DFL', 27, 24, 44), ('DLB', 33, 42, 53), ('DBR', 35, 51, 17))
edge_index = ('UF','UR','UB','UL','DF','DR','DB','DL','FR','FL','BR','BL',
              'FU','RU','BU','LU','FD','RD','BD','LD','RF','LF','RB','LB')
corner_index = ('UFR','URB','UBL','ULF','DRF','DFL','DLB','DBR',
                'FRU','RBU','BLU','LFU','RFD','FLD','LBD','BRD',
                'RUF','BUR','LUB','FUL','FDR','LDF','BDL','RDB')
# 魔方旋转时各个块的变化
route_index=("L","L'","L2","R","R'","R2","U","U'","U2","D","D'","D2","F","F'","F2","B","B'","B2");
route_tab=(
    ((0,1,2,11,4,5,6,9,8,3,10,7),(0,0,0,0,0,0,0,0,0,0,0,0),(0,1,6,2,4,3,5,7),(0,0,2,1,0,2,1,0)),# L  0
    ((0,1,2,9,4,5,6,11,8,7,10,3),(0,0,0,0,0,0,0,0,0,0,0,0),(0,1,3,5,4,6,2,7),(0,0,2,1,0,2,1,0)),# L' 1
    ((0,1,2,7,4,5,6,3,8,11,10,9),(0,0,0,0,0,0,0,0,0,0,0,0),(0,1,5,6,4,2,3,7),(0,0,0,0,0,0,0,0)),# L2 2
    ((0,8,2,3,4,10,6,7,5,9,1,11),(0,0,0,0,0,0,0,0,0,0,0,0),(4,0,2,3,7,5,6,1),(2,1,0,0,1,0,0,2)),# R  3
    ((0,10,2,3,4,8,6,7,1,9,5,11),(0,0,0,0,0,0,0,0,0,0,0,0),(1,7,2,3,0,5,6,4),(2,1,0,0,1,0,0,2)),# R' 4
    ((0,5,2,3,4,1,6,7,10,9,8,11),(0,0,0,0,0,0,0,0,0,0,0,0),(7,4,2,3,1,5,6,0),(0,0,0,0,0,0,0,0)),# R2 5
    ((1,2,3,0,4,5,6,7,8,9,10,11),(0,0,0,0,0,0,0,0,0,0,0,0),(1,2,3,0,4,5,6,7),(0,0,0,0,0,0,0,0)),# U  6
    ((3,0,1,2,4,5,6,7,8,9,10,11),(0,0,0,0,0,0,0,0,0,0,0,0),(3,0,1,2,4,5,6,7),(0,0,0,0,0,0,0,0)),# U' 7
    ((2,3,0,1,4,5,6,7,8,9,10,11),(0,0,0,0,0,0,0,0,0,0,0,0),(2,3,0,1,4,5,6,7),(0,0,0,0,0,0,0,0)),# U2 8
    ((0,1,2,3,7,4,5,6,8,9,10,11),(0,0,0,0,0,0,0,0,0,0,0,0),(0,1,2,3,5,6,7,4),(0,0,0,0,0,0,0,0)),# D  9
    ((0,1,2,3,5,6,7,4,8,9,10,11),(0,0,0,0,0,0,0,0,0,0,0,0),(0,1,2,3,7,4,5,6),(0,0,0,0,0,0,0,0)),# D' 10
    ((0,1,2,3,6,7,4,5,8,9,10,11),(0,0,0,0,0,0,0,0,0,0,0,0),(0,1,2,3,6,7,4,5),(0,0,0,0,0,0,0,0)),# D2 11
    ((9,1,2,3,8,5,6,7,0,4,10,11),(1,0,0,0,1,0,0,0,1,1,0,0),(3,1,2,5,0,4,6,7),(1,0,0,2,2,1,0,0)),# F  12
    ((8,1,2,3,9,5,6,7,4,0,10,11),(1,0,0,0,1,0,0,0,1,1,0,0),(4,1,2,0,5,3,6,7),(1,0,0,2,2,1,0,0)),# F' 13
    ((4,1,2,3,0,5,6,7,9,8,10,11),(0,0,0,0,0,0,0,0,0,0,0,0),(5,1,2,4,3,0,6,7),(0,0,0,0,0,0,0,0)),# F2 14
    ((0,1,10,3,4,5,11,7,8,9,6,2),(0,0,1,0,0,0,1,0,0,0,1,1),(0,7,1,3,4,5,2,6),(0,2,1,0,0,0,2,1)),# B  15
    ((0,1,11,3,4,5,10,7,8,9,2,6),(0,0,1,0,0,0,1,0,0,0,1,1),(0,2,6,3,4,5,7,1),(0,2,1,0,0,0,2,1)),# B' 16
    ((0,1,6,3,4,5,2,7,8,9,11,10),(0,0,0,0,0,0,0,0,0,0,0,0),(0,6,7,3,4,5,1,2),(0,0,0,0,0,0,0,0)) # B2 17
)


class Cube():
    def __init__(self):
        self.ep = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11] # 楞块位置
        self.er = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0]  # 楞块方向
        self.cp = [0, 1, 2, 3, 4, 5, 6, 7] # 角块位置 
        self.cr = [0, 0, 0, 0, 0, 0, 0, 0] # 角块方向
        self.step = 0
        self.last_step = -99
        #self.root = None
    def dump(self):
#         s = []
#         for i in range(0,12):
#             s.append(edge_index[self.ep[i] + 12*self.er[i]])
#         for i in range(0,8):
#             s.append(corner_index[self.cp[i] + 8*self.cr[i]])
#         print(s)
        print(self.ep)
        print(self.er)
        print(self.cp)
        print(self.cr)
        
    
    def from_face_54(self, cube_str):
        # 54个面的表示方式，转换为棱块角块表示方式
        for i in range(0,12):
            index_a = edge_to_face[i][1]
            index_b = edge_to_face[i][2]
            tmp = edge_index.index(cube_str[index_a] + cube_str[index_b])
            self.ep[i] = tmp % 12
            self.er[i] = tmp // 12
        for i in range(0,8):
            index_a = corner_to_face[i][1]
            index_b = corner_to_face[i][2]
            index_c = corner_to_face[i][3]
            tmp = corner_index.index(cube_str[index_a] + cube_str[index_b] + cube_str[index_c])
            self.cp[i] = tmp % 8
            self.cr[i] = tmp // 8
    def to_face_54(self):
        # 转换为54个面的状态
        cube_str = [' ']*54;
        cube_str[4]  = 'U'
        cube_str[13] = 'R'
        cube_str[22] = 'F'
        cube_str[31] = 'D'
        cube_str[40] = 'L'
        cube_str[49] = 'B'
        for i in range(0,12):
            index_a = edge_to_face[i][1]
            index_b = edge_to_face[i][2]
            s = edge_index[self.ep[i] + self.er[i]*12]
            cube_str[index_a] = s[0]
            cube_str[index_b] = s[1]
        for i in range(0,8):
            index_a = corner_to_face[i][1]
            index_b = corner_to_face[i][2]
            index_c = corner_to_face[i][3]
            s = corner_index[self.cp[i] + self.cr[i]*8]
            cube_str[index_a] = s[0]
            cube_str[index_b] = s[1]
            cube_str[index_c] = s[2]
        return ''.join(cube_str)
    # 拧魔方d=0-17，或者字母表示
    def route(self, d):
        new_ep = [0]*12
        new_er = [0]*12
        new_cp = [0]*8
        new_cr = [0]*8
        if(type(d) == int):
            r = route_tab[d]
        else:
            r = route_tab[route_index.index(d)]
        for i in range(0,12):
            new_ep[i] = self.ep[r[0][i]]
            new_er[i] = self.er[r[0][i]] ^ r[1][i]
        for i in range(0,8):
            new_cp[i] = self.cp[r[2][i]]
            new_cr[i] = (self.cr[r[2][i]] + r[3][i]) % 3
        self.ep = new_ep
        self.er = new_er
        self.cp = new_cp
        self.cr = new_cr
    def route_and_new(self, d):
        new_ep = [0]*12
        new_er = [0]*12
        new_cp = [0]*8
        new_cr = [0]*8
        if(type(d) == int):
            r = route_tab[d]
        else:
            r = route_tab[route_index.index(d)]
        for i in range(0,12):
            new_ep[i] = self.ep[r[0][i]]
            new_er[i] = self.er[r[0][i]] ^ r[1][i]
        for i in range(0,8):
            new_cp[i] = self.cp[r[2][i]]
            new_cr[i] = (self.cr[r[2][i]] + r[3][i]) % 3
        c = Cube()
        c.ep = new_ep
        c.er = new_er
        c.cp = new_cp
        c.cr = new_cr
        c.step = self.step + 1
        c.last_step = d
        #c.root = self
        return c
    
    # 绘制魔方
    def draw(self):
        cube_str = self.to_face_54()
        cube_map = [99, 99, 99, 0,  1,  2,  99, 99, 99, 99, 99, 99, 98,
                    99, 99, 99, 3,  4,  5,  99, 99, 99, 99, 99, 99, 98,
                    99, 99, 99, 6,  7,  8,  99, 99, 99, 99, 99, 99, 98,
                    36, 37, 38, 18, 19, 20, 9,  10, 11, 45, 46, 47, 98,
                    39, 40, 41, 21, 22, 23, 12, 13, 14, 48, 49, 50, 98,
                    42, 43, 44, 24, 25, 26, 15, 16, 17, 51, 52, 53, 98,
                    99, 99, 99, 27, 28, 29, 99, 99, 99, 99, 99, 99, 98,
                    99, 99, 99, 30, 31, 32, 99, 99, 99, 99, 99, 99, 98,
                    99, 99, 99, 33, 34, 35, 99, 99, 99, 99, 99, 99, 98]
        for x in cube_map:
            if x == 99:
                print("  ", end='')
            elif x == 98:
                print("")
            else:
                print(cube_str[x] + " ", end='')
    
    def random(self):
        for i in range(0,100):
            self.route(random.choice(("L","R","U","D","F","B","L'","R'","U'","D'","F'","B'","L2","R2","U2","D2","F2","B2")))
    
    def encode(self, x):
        if x == 0:
            # G0 编码角块方向 2^11 = 2048
            x = 0
            for i in range(11):
                x = x | (self.er[i] << i)
            return x
        elif x == 1:
            # 编码角块方向 3^7 = 2187
            x = 0
            for i in range(7):
                x = x*3 + self.cr[i]
            # 编码中间层棱块(>=8)位置 12C4 = 495
            y = encode_12C4_8_to_11(self.ep)
            return 495 * x + y 
        elif x == 4:
            # G2 编码角块，楞块的分轨道情况,8C4 * 8C4 * 6
            # 编码角块排列0-69
            encode_c = encode_8C4_1346(self.cp)
            # 编码棱块排列0-69
            encode_e = encode_8C4_1357(self.ep)
                    
            
            # 两组位于不同轨道角块之间的关系，0-5
            # 分离两个轨道的角块，保持相对顺序不变
            corn_a = [0]*4
            corn_b = [0]*4
            corn_a_index = 0
            corn_b_index = 0
            
            for i in range(8):
                if(self.cp[i] in (1,3,4,6)):
                    corn_a[corn_a_index] = self.cp[i] # 记录阴轨角顺序
                    corn_a_index += 1
                else:# 0 2 5 7
                    corn_b[corn_b_index] = self.cp[i] # 记录阳轨角顺序
                    corn_b_index += 1
            
            # 还原corn_a 1,3,4,6，记录此时corn_b的状态
            if corn_a[0] != 1:
                if corn_a[1] == 1:
                    # U2 corn_a 01交换 corn_b 01交换
                    corn_a[0], corn_a[1] = corn_a[1], corn_a[0]
                    corn_b[0], corn_b[1] = corn_b[1], corn_b[0]
                elif corn_a[2] == 1:
                    # R2 02 03
                    corn_a[0], corn_a[2] = corn_a[2], corn_a[0]
                    corn_b[0], corn_b[3] = corn_b[3], corn_b[0]
                else:
                    # B2 03 13
                    corn_a[0], corn_a[3] = corn_a[3], corn_a[0]
                    corn_b[1], corn_b[3] = corn_b[3], corn_b[1]
            if corn_a[1] != 3:
                if corn_a[2] == 3:
                    # F2 12 02
                    corn_a[1], corn_a[2] = corn_a[2], corn_a[1]
                    corn_b[0], corn_b[2] = corn_b[2], corn_b[0]
                else:
                    # L2 13 12
                    corn_a[1], corn_a[3] = corn_a[3], corn_a[1]
                    corn_b[1], corn_b[2] = corn_b[2], corn_b[1]
            if corn_a[2] != 4:
                # D2 23 23
                corn_a[2], corn_a[3] = corn_a[3], corn_a[2]
                corn_b[2], corn_b[3] = corn_b[3], corn_b[2]

            # 保持corn_a不变，还原corn_b[3]     
            if corn_b[3] != 7:
                if corn_b[2] == 7:
                    corn_b[0],corn_b[1] = corn_b[1],corn_b[0]
                    corn_b[2],corn_b[3] = corn_b[3],corn_b[2]
                elif corn_b[1] == 7:
                    corn_b[0],corn_b[2] = corn_b[2],corn_b[0]
                    corn_b[1],corn_b[3] = corn_b[3],corn_b[1]
                else: #corn_b[0] == 7
                    corn_b[0],corn_b[3] = corn_b[3],corn_b[0]
                    corn_b[1],corn_b[2] = corn_b[2],corn_b[1]
            return encode_3P3(corn_b) + encode_c*6 + encode_e*70*6
        
        elif x == 3:
            encode_c = encode_8C4_1346(self.cp)
            encode_e = encode_8C4_1357(self.ep)
            corn_a = [0]*4
            corn_b = [0]*4
            corn_a_index = 0
            corn_b_index = 0
            for i in range(8):
                if(self.cp[i] in (1,3,4,6)):
                    corn_a[corn_a_index] = self.cp[i] # 记录阴轨角顺序
                    corn_a_index += 1
                else:# 0 2 5 7
                    corn_b[corn_b_index] = self.cp[i] # 记录阳轨角顺序
                    corn_b_index += 1
            z = encode_4P4(corn_a)*24 + encode_4P4(corn_b)###
            return z + encode_c*576 + encode_e*576*70
        
        elif x == 5:
            # 角块编码0-95 根据5个角块的状态计算 4！ * 4 = 96
            if self.cp[7] == 0:
                encode_c = 0
            elif self.cp[7] == 2:
                encode_c = 1
            elif self.cp[7] == 5:
                encode_c = 2
            else:
                encode_c = 3
            encode_c += 4 * encode_4P4( (self.cp[1],self.cp[3],self.cp[4],self.cp[6]) )
            
            # 棱块编码 0 - 6911 根据10个棱块的状态计算 4！ * 4！ * 4P2 = 6912 
            encode_e1 = encode_4P4((self.ep[1],self.ep[3],self.ep[5],self.ep[7]))
            encode_e2 = encode_4P4((self.ep[0],self.ep[2],self.ep[4],self.ep[6]))
            encode_e3 = encode_4P2(self.ep[8],self.ep[9])
            encode_e = encode_e1 * 288 + encode_e2 * 12 + encode_e3
            return encode_c  + 96 * encode_e
        
    def estimate(self, stage):
        if stage == 0:
            return G0_tab[self.encode(0)]
        elif stage == 1:
            return G1_tab[self.encode(1)]
        elif stage == 2:
            return G2_tab[self.encode(4)]
        elif stage == 3:
            return G3_tab[self.encode(5)]
            #return max(a,b)




def creat_table(encode_id, file_name, table_length, search):
    cube = Cube()
    visited = [-1]*table_length
    q = deque()
    q.append(cube)
    visited[cube.encode(encode_id)] = cube.step
    count = 0
    while len(q) != 0:
        count += 1
        if count%10000 == 0:
            print("%.1f%%"%(count*100/table_length))
        c = q.popleft()
        for s in search:
            c_new = c.route_and_new(s)
            code = c_new.encode(encode_id)
            if visited[code] < 0:# 如果无数据记录
                visited[code] = c_new.step
                q.append(c_new)
            elif visited[code] > c_new.step:#有数据记录，但是数据不是最优的
                print("visited[code] > c_new.step, code =", code)
                # 实测不存在这种情况
                visited[code] = c_new.step
                q.append(c_new)
               
    with open(file_name,"w") as f:
        json.dump(visited,f)
        print("write file",file_name)
    for i in range(-1,20):
        print("steps = %d, totel %d."%(i,visited.count(i)))
        

        
        
        
def load_table(file):
    if not os.path.isfile(file):
        print("can not find",file)
        return None
    with open(file,'r') as f:
        load_dict = json.load(f)
    return load_dict




# 编码4个数字的全排列，例如(1, 0, 2, 3) => 6
# 基于康托展开，支持不连续的数据
def encode_4P4(p):
    # (0, 1, 3, 2)
    # 0x6 0x2 1x1
    n=0;
    for a in range(3):         # 3、4都一样
        n *= 4-a;              # 计算阶乘
        for b in range(a+1, 4):# 位于右边的比自己小的
            if p[b] < p[a]:
                n += 1;
    return n

def encode_3P3(p):
    n=0;
    for a in range(2):
        n *= 3-a;
        for b in range(a+1, 3):
            if p[b] < p[a]:
                n += 1;
    return n

# 只支持8、9、10、11中选2个
def encode_4P2(a, b):
    a = a - 8
    b = b - 8
    if b > a:
        b -= 1
    return a * 3 + b

# 组合 n*(n-1)*(n-m+1)/m!
def get_combinations(n,m):
    c = 1
    for i in range(m):
        c *=  n - i
        c //= i + 1
    return c
# 组合编码
# https://www.jaapsch.net/puzzles/compindx.htm
# encode_12C4([0,1,2,3,4,5,6,7,8,9,10,11])
# encode_12C4([0,1,2,3,4,5,6,7,10,9,8,11])
# encode_12C4([10,9,8,11,0,1,2,3,4,5,6,7])
# encode_12C4([10,5,8,11,0,1,2,3,4,9,6,7])
def encode_12C4_8_to_11(x):
    comb = 0
    j = 4
    for i in range(12-1,-1,-1):# 11,10,...,2,1,0
        if x[i] >= 8:          # 11 10 9 8
            comb += get_combinations(i,j)
            j -= 1
    return comb

def encode_8C4_1346(x):
    comb = 0
    j = 4
    for i in range(8-1,-1,-1):
        if x[i] == 1 or x[i] == 3 or x[i] == 4 or x[i] == 6:
            comb += get_combinations(i,j)
            j -= 1
    assert(comb < 70)
    return comb

def encode_8C4_1357(x):
    comb = 0
    j = 4
    for i in range(8-1,-1,-1):
        if x[i] == 1 or x[i] == 3 or x[i] == 5 or x[i] == 7:
            comb += get_combinations(i,j)
            j -= 1
    assert(comb < 70)
    return comb

#print("init lookup table.")

max_stack_size = 0

def search(stage, cube, max_deep, allow_route):
    cube.step = 0
    cube.last_step = -99
    q = deque()
    q.append(cube)
    steps_record = [-99]*18
    count = 0
    while len(q) != 0:
        global max_stack_size
        max_stack_size = max(len(q),max_stack_size)
        c = q.pop()
        estimate = c.estimate(stage)
        #print(c.last_step, estimate, c.step, max_deep)
        steps_record[c.step - 1] = c.last_step
        if estimate == 0:
            print("max_deep =", max_deep, "search count =", count)
            #print("solved cube.")
            print(steps_record[0 : c.step])
            return c, steps_record[0 : c.step]
        # 舍弃max_deep步骤内无解的
        if estimate + c.step <= max_deep:
            count += 1
            for i in allow_route:
                # 如果旋转的面和上次旋转的一致，舍弃，可减少1/6的情况
                if c.last_step // 3 != i // 3:
                    c_new = c.route_and_new(i)
                    q.append(c_new)
    if count > 1:
        print("max_deep =", max_deep, "search count =", count)
    return None,None

def solve(x):
    cube = Cube()
    cube.from_face_54(x)
    print("调整全部棱块的方向")
    for max_deep in range(0,18):#最大值7
        c, s1 = search(0, cube, max_deep, (0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17))
        if s1 != None:
            for i in range(len(s1)):
                s1[i] = route_index[s1[i]]
            print(s1,"LENGTH1",len(s1))
            cube = c
            break
    print("调整全部角块的方向，同时把中间层棱块放置到中间层")
    for max_deep in range(0,18):#最大值应该是10
        c, s2 = search(1, cube, max_deep, (0,1,2,3,4,5,6,7,8,9,10,11,14,17))
        if s2 != None:
            for i in range(len(s2)):
                s2[i] = route_index[s2[i]]
            print(s2,"LENGTH2",len(s2))
            cube = c
            break
    print("调整角块和棱块的相对位置，放置到各自的轨道") #这个步骤有些小问题，偶尔出现搜索时间特别长的情况
    #print(cube.to_face_54())
    #cube.dump()
    #print(cube.encode(4))
    for max_deep in range(0,18): #最大值可能是13
        c, s3 = search(2, cube, max_deep, (6,7,9,10,2,5,8,11,14,17))
        if s3 != None:
            for i in range(len(s3)):
                s3[i] = route_index[s3[i]]
            print(s3,"LENGTH3",len(s3))
            cube = c
            break
    print("还原魔方")
    for max_deep in range(0,18): #最大值15
        c, s4 = search(3, cube, max_deep, (2,5,8,11,14,17))
        if s4 != None:
            for i in range(len(s4)):
                s4[i] = route_index[s4[i]]
            print(s4,"LENGTH4",len(s4))
            cube = c
            cube.step = 0
            break    

    return s1 + s2 + s3 + s4
    
def do_test(num_test_case):
    print("load test_case.")
    test_case = [None]*num_test_case
    with open("test_case.txt",'r') as f:
        for i in range(num_test_case):
            test_case[i] = f.readline()
    
    #test_case =["BBDDUFBRRDRFFRBUUFDBFBFLLUFBLRFDFBDULRRRLDUDULLDUBULLR"]
    max_step = 0
    min_step = 99
    avg_step = 0
    count2 = 0
    for x in test_case:
        count2+=1
        print("-------- count=",count2,x)
        route = solve(x)
        print(" ".join(route))
        length = len(route)
        print("step =", length)
        # 测试还原步骤的正确性
        cube = Cube()
        cube.from_face_54(x)
        for i in route:
            #print(cube.estimate(4))
            cube.route(i)
            
        assert(cube.to_face_54()=='UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB')
        max_step = max(max_step, length)
        min_step = min(min_step, length)
        avg_step += length
    avg_step = avg_step // num_test_case
    print("最大步骤数",max_step, "最小步骤数",min_step, "平均步骤数",avg_step)
    
def do_test_c_version(num_test_case):
    index = 0
    test_case = [None]*num_test_case
    with open("test_case.txt",'r') as f:
        for i in range(num_test_case):
            test_case[i] = f.readline()
    for x in test_case:
        print("index =",index)
        index+=1
        os.system("./solve "+x)

def creat_table2(encode_id,encode_id_visited, file_name, table_length, table_length_visited, search):
    cube = Cube()
    record = [-1]*table_length
    visited = [-1]*table_length_visited #可能会出现需要多步骤才能转换到另一个状态的情况，用另一种hash方式记录访问过的节点
    q = deque()
    q.append(cube)
    record[cube.encode(encode_id)] = cube.step
    visited[cube.encode(encode_id_visited)] = cube.step
    count = 0
    while len(q) != 0:
        count += 1
        if count%10000 == 0:
            print("%.1f%%"%(count*100/table_length_visited))
        c = q.popleft()
        # 一步的情况
        for s in search:
            c_new = c.route_and_new(s)
            code_visited = c_new.encode(encode_id_visited)
            code_record = c_new.encode(encode_id)
            if visited[code_visited] < 0:# 如果无数据记录
                visited[code_visited] = c_new.step
                q.append(c_new)
                if record[code_record] < 0:#or record[code_record] > c_new.step:
                   record[code_record] = c_new.step
    with open(file_name,"w") as f:
        json.dump(record,f)
        print("write file",file_name)
    for i in range(-1,20):
        print("steps = %d, totel %d."%(i,record.count(i)))
       
# 这种太慢，不使用
def creat_table3(encode_id, file_name, table_length, search):
    visited = [-1] * table_length
    for max_deep in range(5):
        print("creat_table", max_deep)
        cube = Cube()
        q = deque()
        q.append(cube)
        count = 0
        while len(q) != 0:
            c = q.pop()
            code = c.encode(encode_id)
            if c.step <= max_deep:
                if visited[code] < 0:# 如果无数据记录
                    visited[code] = c.step
                    count += 1
                    # 显示进度
                    if count % 100 == 0:
                        print(count)
                # 搜索子节点
                for i in search:
                    # 如果旋转的面和上次旋转的一致，舍弃，可减少1/6的情况
                    if c.last_step // 3 != i // 3:
                        c_new = c.route_and_new(i)
                        q.append(c_new)
        print(count)
               
    print(visited)

def table_to_c_array(tab, name):
    length = len(tab)
    if length % 2 == 0:
        length = length // 2
    else:
        length = length // 2 + 1
    print("const uint8_t %s[%d] = {"%(name, length))
    for i in range(length):
        low = tab[i*2]
        if i*2+1 == len(tab):
            high = 0
        else:
            high = tab[i*2+1]
        print("%d,"%( (high << 4) | low ),end='')
        if i%32 == 31:
            print("")
    print("};")
def table_to_c_array_all():
    print("#include <inttypes.h>")
    table_to_c_array(G0_tab, "G0_tab");
    table_to_c_array(G1_tab, "G1_tab");
    table_to_c_array(G2_tab, "G2_tab");
    table_to_c_array(G3_tab, "G3_tab");
    
# 生成查找表，首次使用时执行下面5条代码
#creat_table(0, "G0.json", 2048, (0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17))
#creat_table(1, "G1.json", 2187*495, (0,1,2,3,4,5,6,7,8,9,10,11,14,17))
#creat_table2(4, 3, "G2.json", 29400,29400*96, (2,5,6,7,8,9,10,11,14,17))
#creat_table(5, "G3.json", 663552, (2,5,8,11,14,17))
#table_to_c_array_all()

# 加载查找表
# G0_tab  = load_table("G0.json")
# G1_tab  = load_table("G1.json")
# G2_tab  = load_table("G2.json")
# G3_tab  = load_table("G3.json")
# do_test(10)

do_test_c_version(10000)

# 
# route = solve("LLDBUFUBRFDFFRUBBRFLUBFRDLURURUDUBRFDRLLLDLRBLDBFBFDDU")
# 测试还原步骤的正确性
# cube = Cube()
# solve("BBBFULRUBUURFRRRDFDFLUFDLRUUUFFDRLDLRRFLLBBLFDLUBBDDBD")

