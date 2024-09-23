$fa=6;
$fs=0.5;
use <main.scad>



// 以下分别打印1个
//base_board();
//cube_spin();
rotate([180,0,0]) fan_holder();

// 以下分别打印2个
//claw_3d_print();
//stepper_shelf();

// 以下分别打印4个
//spring();


// 垫柱，打印不同尺寸的备用
/*
for(i=[0:7]){
    translate([10*0,i*10,0])cyl(2);
    translate([10*1,i*10,0])cyl(3);
    translate([10*2,i*10,0])cyl(4);
    translate([10*3,i*10,0])cyl(5);
}
*/