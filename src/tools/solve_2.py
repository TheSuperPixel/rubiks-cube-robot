#!/usr/bin/pypy3
# 推荐使用pypy，建立查找表的过程远比cpython快
import random
from collections import deque
from itertools import permutations
import json
import os

# 参考kociemba算法编写，分两步还原
# https://www.bilibili.com/read/cv12330074  魔方还原算法(一) 概述
# https://www.bilibili.com/read/cv12331495  魔方还原算法(二) 科先巴二阶段算法

#G0=<U  D  L  R  F  B >  4.33·10^19  2,048 (2^11)
#G1=<U  D  L  R  F2 B2>  2.11·10^16  1,082,565 (12C4 ·3^7)

#G2=<U  D  L2 R2 F2 B2>  1.95·10^10  29,400 ( 8C4^2 ·2·3)
#G3=<U2 D2 L2 R2 F2 B2>  6.63·10^5   663,552 (4!^5/12)

# 步骤1：角块方向正确，棱块方向正确，中间层棱块在中间


# phrase2_corners_size = 40320; // 8!
# phrase2_edges1_size = 40320;  // 8!
# phrase2_edges2_size = 24;     // 4!



# 角块、棱块、面的编码格式如下
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
        s = []
        for i in range(0,12):
            s.append(edge_index[self.ep[i] + 12*self.er[i]])
        for i in range(0,8):
            s.append(corner_index[self.cp[i] + 8*self.cr[i]])
        print(s)
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
        #self.c.step = self.step + 1
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
    def encode(self, e):
        global table_12C4 
        if  e == 0:
            # 2**11 = 2048   顶面和底面12个棱块的方向 
            x = 0
            for i in range(11):
                x = (x<<1) + self.er[i]
            # 12C4 =  495    中间层4个棱块的相对位置
            bin_ep = 0
            for i in range(12):
                if(self.ep[i] >= 8):
                    bin_ep += (1 << i)   
            return x*495 + table_12C4[bin_ep]
        elif e == 1:
            # 3**7 =  2187   顶面和底面8个角块的方向
            x = 0
            for i in range(7):
                x = x*3 + self.cr[i]
            # 12C4 =  495    中间层4个棱块的相对位置
            bin_ep = 0
            for i in range(12):
                if(self.ep[i] >= 8):
                    bin_ep += (1 << i)
            return x*495 + table_12C4[bin_ep]
        elif e == 2:
            # 中层4个棱块的位置 4! = 24
            mid_edge = encode_4P4(self.ep[8:12])
            # 8个角块的位置 8! = 40320
            corners = encode_8P8(self.cp)
            return 40320 * mid_edge + corners
        elif e == 3:
            # 中层4个棱块的位置 4! = 24
            mid_edge = encode_4P4(self.ep[8:12])
            # 其他4个棱块的位置 8! = 40320
            other_edge = encode_8P8(self.ep[0:8])
            return 40320 * mid_edge + other_edge
    def estimate(self, stage):
        if stage == 1:
            a = p1_tab0[self.encode(0)]
            b = p1_tab1[self.encode(1)]
            return max(a,b)
        else:
            a = p2_tab0[self.encode(2)]
            b = p2_tab1[self.encode(3)]
            return max(a,b)            
        
# 给N选S排列的没一个元素，进行唯一编号
def encode_perm(N, S, p):
    pos = [i for i in range(N)]
    elem = [i for i in range(N)]
    v = 0
    k = 1# for N=4 k=1,4,12,24
    kk = N
    for i in range(S):
        t = pos[p[i]]
        v += k * t
        k *= kk
        kk -= 1
        pos[elem[N - i - 1]] = t
        elem[t] = elem[N - i - 1]
    return v;

# p = permutations((0,1,2,3),3)
# for x in p:
#     print(x, encode_perm(4,3,x))


# c = Cube()
# c.draw()
def create_12C4_encode_table():
    # 2 ** 12 = 4096 
    encode = 0;
    encode_table = [-1] * 4096
    for x in range(4096):
        count_one = 0;
        for i in range(12):
            if x & (1<<i):
                count_one += 1
        if count_one == 4:
            encode_table[x] = encode
            encode += 1
    #print("create_12C4_encode_table: ",encode)
    return encode_table

# 编码4个数字的全排列，例如(1, 0, 2, 3) => 6
# 不限制数字是否连续
def encode_4P4(p):
    n=0;
    for a in range(4):
        n *= 4-a;
        for b in range(a+1,4):
            if p[b] < p[a]:
                n += 1;
    return n

# 编码8个数字的全排列
# 不限制数字是否连续
def encode_8P8(p):
    n=0;
    for a in range(8):
        n *= 8-a;
        for b in range(a+1,8):
            if p[b] < p[a]:
                n += 1;
    return n

def creat_table(encode_id, file_name, table_length, search):
    cube = Cube()
    visited = [-1]*table_length
    q = deque()
    q.append(cube)
    visited[cube.encode(encode_id)] = cube.step
    count = 0
    while len(q) != 0:
        count += 1
        if count%1000 == 0:
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
                assert(0)
                visited[code] = c_new.step
                q.append(c_new)
               
    with open(file_name,"w") as f:
        json.dump(visited,f)
        print("write file",file_name)
    for i in range(-1,20):
        print("steps = %d, totel %d."%(i,visited.count(i)))
    print("sum", count)
    
def load_table(file):
    if not os.path.isfile(file):
        print("can not find",file)
        return None
    with open(file,'r') as f:
        load_dict = json.load(f)
    return load_dict

def search(stage, cube, max_deep, allow_route):
    print("max_deep =", max_deep)
    q = deque()
    q.append(cube)
    steps2 = [-99]*18
    count = 0
    while len(q) != 0:
        count += 1
        c = q.pop()
        estimate = c.estimate(stage)
        #print(c.step, estimate, max_deep)
        steps2[c.step-1] = c.last_step
        if estimate == 0:
            print(count)
            #print("solved cube.")
            print(steps2[0:c.step])
            return c, steps2[0:c.step]
        # 舍弃max_deep步骤内无解的
        if estimate + c.step <= max_deep:
            for i in allow_route:
                # 如果旋转的面和上次旋转的一致，舍弃，可减少1/6的情况
                if c.last_step // 3 != i // 3:
                    c_new = c.route_and_new(i)
                    q.append(c_new)
    print(count)
    return None,None

    

table_12C4 = create_12C4_encode_table()
# route_all = [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17]
# route_p2 = [2,5,6,7,8,9,10,11,14,17]

# 阶段1,测试100组随机魔方，性能如下
# 11步骤还原 1
# 10步骤还原 62
# 9步骤还原  31
# 8步骤还原  6
#creat_table(0, "P1_0.json", 2048*495, route_all)
#creat_table(1, "P1_1.json", 2187*495, route_all)
#creat_table(2, "P2_0.json", 40320*24, route_p2)
#creat_table(3, "P2_1.json", 40320*24, route_p2)

p1_tab0 = load_table("P1_0.json")
p1_tab1 = load_table("P1_1.json")
p2_tab0 = load_table("P2_0.json")
p2_tab1 = load_table("P2_1.json")

#print(max(p1_tab0))
#print(max(p1_tab1))

def solve_s1_s2(case):
    print(case)
    
    route_all = list(range(18))
    route_p2 = [2,5,6,7,8,9,10,11,14,17]
    
    exit_flag = False
    #for offset in range(18):
    cube = Cube()
    cube.from_face_54(case)
        
    #    for i in range(18):
    #        route_all[i] = route_all[i] = (i+offset)%18
        
    print("stage 1",route_all)
    for max_deep in range(0,15): # 通常为9-10步,延长stage1步骤，可以缩短stage2步骤
        c, s1 = search(1, cube, max_deep, route_all)
        if s1 != None:
            for i in range(len(s1)):
                s1[i] = route_index[s1[i]]
            print(s1,"LENGTH1",len(s1))
            cube = c
            break
    
    #cube.dump()
    cube.step = 0
    
    print("stage 2")
    for max_deep in range(0,18): #大于14的情况
        c, s2 = search(2, cube, max_deep, route_p2)
        if s2 != None:
            for i in range(len(s2)):
                s2[i] = route_index[s2[i]]
            print(s2,"LENGTH2",len(s2))
            exit_flag = True
            break
        
    # 测试还原步骤的正确性
    cube = Cube()
    cube.from_face_54(case)
    for x in s1:
        cube.route(x)
    for x in s2:
        cube.route(x)
    cube.draw()
        
def do_test():
    num_test_case = 1
    print("load test_case.")
    test_case = [None]*num_test_case
    with open("test_case.txt",'r') as f:
        for i in range(num_test_case):
            test_case[i] = f.readline()
    for case in test_case:
        solve_s1_s2(case)
        
def do_test_2():   
    cube = Cube()
    cube.route('F2')
    cube.route('U')
    cube.route('R2')
    cube.route('B2')
    cube.route('U2')
    cube.step = 0
    
    print("stage 2")
    for max_deep in range(0,18):
        c, s2 = search(2, cube, max_deep, route_p2)
        if s2 != None:
            for i in range(len(s2)):
                s2[i] = route_index[s2[i]]
            print(s2,"LENGTH",len(s2))
            cube = c
            break
        
#import cProfile
#cProfile.run('do_test()')
do_test()
#do_test_2()
