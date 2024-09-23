$fa=6;
$fs=0.5;

size = 18.7;
offset = (size-0.1-4)/2;

module center(){
    hull(){
        translate([offset,offset,0]) cylinder(0.2, d=4, center=true);
        translate([offset,-offset,0]) cylinder(0.2, d=4, center=true);
        translate([-offset,offset,0]) cylinder(0.2, d=4, center=true);
        translate([-offset,-offset,0]) cylinder(0.2, d=4, center=true);
    }
}

module edge(){
    hull(){
        translate([offset,offset,0]) cube([4,4,0.2], center=true);
        translate([offset,-offset,0]) cube([4,4,0.2], center=true);
        translate([-offset,offset,0]) cylinder(0.2, d=4, center=true);
        translate([-offset,-offset,0]) cylinder(0.2, d=4, center=true);
    }
}

module corner(){
    hull(){
        translate([offset,offset,0]) cube([4,4,0.2], center=true);
        translate([offset,-offset,0]) cube([4,4,0.2], center=true);
        translate([-offset,offset,0]) cube([4,4,0.2], center=true);
        translate([-offset,-offset,0]) cylinder(0.2, d=4, center=true);
    }
}

module face(){
    center();
    for(i=[0,90,180,270]){
        rotate([0,0,i]) translate([size, 0, 0]) edge();
        rotate([0,0,i]) translate([size, size, 0]) corner();
    }
}

module rubik_cube(){
    color("white") cube(56,center=true);
    translate([0,0,3*size/2]) color("yellow") face();
    translate([0,0,-3*size/2]) color("white") face();
    rotate([90,0,0]){
        translate([0,0,3*size/2]) color("blue") face();
        translate([0,0,-3*size/2]) color("green") face();
    }
    rotate([0,90,0]){
        translate([0,0,3*size/2]) color("red") face();
        translate([0,0,-3*size/2]) color("orange") face();
    }
}
rubik_cube();