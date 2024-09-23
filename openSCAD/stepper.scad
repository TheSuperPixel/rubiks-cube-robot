$fa=6;
$fs=0.5;



stepper42();
translate([0,0,23]) rotate([180,0,0]) flange();
module flange(){
    difference(){
        union(){
            cylinder(h=2,d=22);
            cylinder(h=12,d=10);
        }
        cylinder(h=99,d=5,center=true);
        translate([8,0,0]) cylinder(h=99,d=3,center=true);
        translate([-8,0,0]) cylinder(h=99,d=3,center=true);
        translate([0,-8,0]) cylinder(h=99,d=3,center=true);
        translate([0,8,0]) cylinder(h=99,d=3,center=true);
    }
}


module stepper42(){
    union(){
        top();
        translate([0,0,-8]) mid();
        translate([0,0,-8-9.5-23-8]) mirror([0,0,1]) bot();
    }
}


module mid(){
    mid_r = 8;
    offset_xy = 42/2 - mid_r;
    length = 23+8; // 硅钢片高度
    hull(){
        translate([offset_xy, offset_xy, -length]) cylinder(h=length, r=mid_r, $fn=4);
        translate([offset_xy, -offset_xy, -length]) cylinder(h=length, r=mid_r, $fn=4);
        translate([-offset_xy, offset_xy, -length]) cylinder(h=length, r=mid_r, $fn=4);
        translate([-offset_xy, -offset_xy, -length]) cylinder(h=length, r=mid_r, $fn=4);
    }
}
module top(){
    top_r = 4;
    offset_xy = 42/2 - top_r;
    length = 23+2; // 电机轴长度
    union(){
        difference(){
            hull(){
                translate([offset_xy, offset_xy, -8]) cylinder(h=8, r=top_r, $fn=4);
                translate([offset_xy, -offset_xy, -8]) cylinder(h=8, r=top_r, $fn=4);
                translate([-offset_xy, offset_xy, -8]) cylinder(h=8, r=top_r, $fn=4);
                translate([-offset_xy, -offset_xy, -8]) cylinder(h=8, r=top_r, $fn=4);
            }
            translate([31/2, 31/2, 0]) cylinder(h=20,d=2.5,center=true); 
            translate([-31/2, 31/2, 0]) cylinder(h=20,d=2.5,center=true);
            translate([31/2, -31/2, 0]) cylinder(h=20,d=2.5,center=true);
            translate([-31/2, -31/2, 0]) cylinder(h=20,d=2.5,center=true);       
        }
        difference(){
            cylinder(h=2,d=22);
            cylinder(h=20,d=9,center=true);
        }
        difference(){
             cylinder(h=length,d=5);
             translate([-5/2, 4.5/2, 0]) cube([5,5,length]);
        }
    }
}
module bot()
{
    bot_r = 4;
    offset_xy = 42/2 - bot_r;
    union(){
        difference(){
            hull(){
                translate([offset_xy, offset_xy, -9.5]) cylinder(h=9.5, r=bot_r, $fn=4);
                translate([offset_xy, -offset_xy, -9.5]) cylinder(h=9.5, r=bot_r, $fn=4);
                translate([-offset_xy, offset_xy, -9.5]) cylinder(h=9.5, r=bot_r, $fn=4);
                translate([-offset_xy, -offset_xy, -9.5]) cylinder(h=9.5, r=bot_r, $fn=4);
            }
            translate([31/2, 31/2, 0]) cylinder(h=20,d=3.5,center=true); 
            translate([-31/2, 31/2, 0]) cylinder(h=20,d=3.5,center=true);
            translate([31/2, -31/2, 0]) cylinder(h=20,d=3.5,center=true);
            translate([-31/2, -31/2, 0]) cylinder(h=20,d=3.5,center=true);
            translate([31/2, 31/2, -2.5]) cylinder(h=20,d=6); 
            translate([-31/2, 31/2, -2.5]) cylinder(h=20,d=6);
            translate([31/2, -31/2, -2.5]) cylinder(h=20,d=6);
            translate([-31/2, -31/2, -2.5]) cylinder(h=20,d=6);
            translate([0, 0, -3.5]) cylinder(h=20,d=9);
        }
        translate([-16/2,42/2,-9.5])cube([16,6,9.5]);
    }
}







