// =============================================================================
// 7.5" E-PAPER DESK CLOCK ENCLOSURE (70° TILT ANGLE)
// Parametric 3D Printable Enclosure with Snap-Fit Latches
// =============================================================================
// Designed for:
//   - Waveshare 7.5" V2 E-Paper Display (800x480)
//   - DFRobot FireBeetle 2 ESP32-E (USB-C)
//   - Waveshare e-Paper Driver HAT (Rev 2.3)
//   - ENS160 + AHT21 Air Quality & Temp/Humidity Sensor Module
//   - Flat LiPo Pouch Battery (1000mAh - 2500mAh)
// =============================================================================

/* [Render Selection] */
// Which part to render
part = "assembly"; // [assembly: Complete Preview, front: Front Bezel (Print Orientation), rear: Rear Stand (Print Orientation), exploded: Exploded View]

/* [Enclosure Dimensions] */
wall_thick        = 2.4;   // Outer shell wall thickness (mm)
bezel_margin      = 6.0;   // Margin around glass panel (mm)
corner_radius     = 6.0;   // Outer corner rounding radius (mm)
tilt_angle        = 70;    // Desktop viewing angle from horizontal (deg)

/* [Display Specifications (Waveshare 7.5" V2)] */
panel_w           = 170.2; // Glass panel width (mm)
panel_h           = 111.0; // Glass panel height (mm)
panel_t           = 1.35;  // Glass panel thickness with foam backing (mm)
active_w          = 163.2; // Viewable screen width (mm)
active_h          = 97.92; // Viewable screen height (mm)
active_y_offset   = 3.2;   // Offset of active area from center (mm)
fpc_w             = 28.0;  // Ribbon cable width (mm)
fpc_t             = 2.0;   // Ribbon cable relief depth (mm)

/* [Component Dimensions] */
firebeetle_w      = 25.8;  // FireBeetle 2 width (mm)
firebeetle_l      = 60.5;  // FireBeetle 2 length (mm)
firebeetle_t      = 7.5;   // FireBeetle 2 thickness with headers (mm)

hat_w             = 65.5;  // Waveshare HAT width (mm)
hat_l             = 56.5;  // Waveshare HAT length (mm)
hat_t             = 9.0;   // Waveshare HAT thickness (mm)

battery_w         = 55.0;  // LiPo pouch bay width (mm)
battery_l         = 75.0;  // LiPo pouch bay length (mm)
battery_d         = 9.0;   // LiPo pouch bay depth (mm)

sensor_w          = 16.0;  // ENS160+AHT21 width (mm)
sensor_l          = 21.0;  // ENS160+AHT21 length (mm)

/* [Snap-Fit Latch Geometry] */
snap_w            = 12.0;  // Width of each cantilever tab (mm)
snap_t            = 1.5;   // Tab thickness (mm)
snap_h            = 6.5;   // Tab cantilever height (mm)
snap_catch        = 0.75;  // Latch overhang undercut (mm)
joint_clearance   = 0.25;  // Fit tolerance between front and rear (mm)

// Calculated overall dimensions
outer_w = panel_w + (bezel_margin + wall_thick) * 2; // ~187 mm
outer_h = panel_h + (bezel_margin + wall_thick) * 2; // ~128 mm
stand_depth = 65.0; // Depth of desktop wedge foot for stability

$fn = 48;

// =============================================================================
// Top-Level Part Selector
// =============================================================================
if (part == "assembly") {
    // Show assembled model in standing orientation
    rotate([90 - tilt_angle, 0, 0]) {
        color([0.2, 0.2, 0.25]) front_bezel();
        color([0.85, 0.85, 0.9]) translate([0, 0, -18]) rear_housing();
        
        // Ghost display panel in place
        %translate([0, 0, -wall_thick - panel_t/2])
            cube([panel_w, panel_h, panel_t], center=true);
    }
} else if (part == "exploded") {
    rotate([90 - tilt_angle, 0, 0]) {
        color([0.2, 0.2, 0.25]) front_bezel();
        color([0.85, 0.85, 0.9]) translate([0, 0, -45]) rear_housing();
        
        // Display floating between parts
        color([0.95, 0.95, 0.95, 0.8]) translate([0, 0, -22])
            cube([panel_w, panel_h, panel_t], center=true);
    }
} else if (part == "front") {
    // Flat on build plate for printing without supports
    front_bezel();
} else if (part == "rear") {
    // Flat base on build plate for printing
    rotate([0, 0, 0]) rear_housing();
}

// =============================================================================
// Helper Module: Rounded Box
// =============================================================================
module rounded_box(w, h, d, r) {
    hull() {
        translate([-w/2 + r, -h/2 + r, 0]) cylinder(r=r, h=d);
        translate([ w/2 - r, -h/2 + r, 0]) cylinder(r=r, h=d);
        translate([-w/2 + r,  h/2 - r, 0]) cylinder(r=r, h=d);
        translate([ w/2 - r,  h/2 - r, 0]) cylinder(r=r, h=d);
    }
}

// =============================================================================
// 1. FRONT BEZEL
// =============================================================================
module front_bezel() {
    bezel_depth = wall_thick + panel_t + 1.2;

    difference() {
        union() {
            // Main outer rounded bezel body
            rounded_box(outer_w, outer_h, bezel_depth, corner_radius);

            // Perimeter mating rim that inserts into rear case
            translate([0, 0, bezel_depth])
                difference() {
                    rounded_box(outer_w - wall_thick*2 - joint_clearance*2, 
                                outer_h - wall_thick*2 - joint_clearance*2, 
                                3.5, corner_radius - wall_thick);
                    translate([0, 0, -0.1])
                        rounded_box(outer_w - wall_thick*4, outer_h - wall_thick*4, 4.0, 2);
                }
        }

        // 1. Active viewport window with aesthetic 45° chamfer
        translate([0, active_y_offset, -0.5]) {
            // Cutout through face
            cube([active_w, active_h, bezel_depth + 1], center=true);
            
            // Front chamfer
            hull() {
                translate([0, 0, -0.1])
                    cube([active_w + 3.0, active_h + 3.0, 0.1], center=true);
                translate([0, 0, 1.8])
                    cube([active_w, active_h, 0.1], center=true);
            }
        }

        // 2. Stepped recess for glass panel (0.3mm tolerance around glass)
        translate([0, 0, wall_thick])
            cube([panel_w + 0.6, panel_h + 0.6, panel_t + 10], center=true);

        // 3. FPC Ribbon cable clearance notch at bottom
        translate([0, -panel_h/2 - 2, wall_thick])
            cube([fpc_w + 4, 16, fpc_t + 4], center=true);

        // 4. Female snap-fit receiver slots (6 perimeter catches)
        snap_positions = [
            [-outer_w/4,  outer_h/2 - wall_thick - 1.0], // Top Left
            [ outer_w/4,  outer_h/2 - wall_thick - 1.0], // Top Right
            [-outer_w/4, -outer_h/2 + wall_thick + 1.0], // Bottom Left
            [ outer_w/4, -outer_h/2 + wall_thick + 1.0], // Bottom Right
            [-outer_w/2 + wall_thick + 1.0, 0],          // Side Left
            [ outer_w/2 - wall_thick - 1.0, 0]           // Side Right
        ];

        for (p = snap_positions) {
            translate([p[0], p[1], bezel_depth + 1.2]) {
                if (abs(p[0]) > outer_w/3) {
                    // Side slots
                    cube([snap_t*2 + 1.0, snap_w + 1.0, 3.2], center=true);
                } else {
                    // Top/Bottom slots
                    cube([snap_w + 1.0, snap_t*2 + 1.0, 3.2], center=true);
                }
            }
        }

        // 5. Coin pry notch on bottom edge for easy disassembly
        translate([0, -outer_h/2, 0])
            cube([18, 3.0, 2.0], center=true);
    }

    // Corner alignment pins
    for (cx = [-panel_w/2 - 1.5, panel_w/2 + 1.5]) {
        for (cy = [-panel_h/2 - 1.5, panel_h/2 + 1.5]) {
            translate([cx, cy, wall_thick])
                cylinder(r=1.5, h=panel_t + 1.0);
        }
    }
}

// =============================================================================
// 2. REAR DESK STAND HOUSING
// =============================================================================
module rear_housing() {
    base_depth = 16.0; // Main chassis depth
    
    difference() {
        union() {
            // Main housing body
            rounded_box(outer_w, outer_h, base_depth, corner_radius);

            // Integrated 70° Desktop Wedge Stand
            stand_h = outer_h * 0.75;
            stand_w = outer_w * 0.65;
            
            translate([0, -outer_h/2 + stand_h/2 + 6, base_depth]) {
                hull() {
                    // Top of wedge (tapered)
                    translate([0, stand_h/2 - 10, 0])
                        rounded_box(stand_w * 0.7, 12, 1.0, 4);
                    // Bottom footing of wedge (wide desktop contact surface)
                    translate([0, -stand_h/2 + 6, stand_depth * cos(90 - tilt_angle)])
                        rounded_box(stand_w, 20, 3.0, 6);
                    // Base junction
                    translate([0, -stand_h/2 + 6, 0])
                        rounded_box(stand_w, 20, 1.0, 6);
                }
            }
        }

        // 1. Main Internal Hollow Cavity
        translate([0, 0, wall_thick])
            rounded_box(outer_w - wall_thick*2, outer_h - wall_thick*2, base_depth + stand_depth + 1, corner_radius - 1);

        // 2. Front mating rim recess
        translate([0, 0, base_depth - 3.2])
            difference() {
                rounded_box(outer_w - wall_thick*2 + 0.2, outer_h - wall_thick*2 + 0.2, 3.5, corner_radius - wall_thick);
                translate([0, 0, -0.5])
                    rounded_box(outer_w - wall_thick*4, outer_h - wall_thick*4, 4.5, 2);
            }

        // 3. Side USB-C cutout for FireBeetle 2 ESP32-E
        // Positioned on the left side (standard desktop layout)
        translate([-outer_w/2 - 1, -outer_h/4, wall_thick + 4])
            hull() {
                translate([0, -4.5, 0]) rotate([0, 90, 0]) cylinder(r=2.0, h=wall_thick*3);
                translate([0,  4.5, 0]) rotate([0, 90, 0]) cylinder(r=2.0, h=wall_thick*3);
                translate([0, -4.5, 3.5]) rotate([0, 90, 0]) cylinder(r=2.0, h=wall_thick*3);
                translate([0,  4.5, 3.5]) rotate([0, 90, 0]) cylinder(r=2.0, h=wall_thick*3);
            }

        // 4. Sensor ventilation louvers (Lower rear isolated bay)
        for (i = [-3 : 3]) {
            translate([outer_w/3 + i * 4.5, -outer_h/2 + 14, -0.5])
                hull() {
                    cylinder(r=1.0, h=wall_thick*2);
                    translate([0, 12, 0]) cylinder(r=1.0, h=wall_thick*2);
                }
        }

        // Top passive convection exhaust slots
        for (i = [-5 : 5]) {
            translate([i * 6.0, outer_h/2 - 12, -0.5])
                hull() {
                    cylinder(r=1.0, h=wall_thick*2);
                    translate([0, 6, 0]) cylinder(r=1.0, h=wall_thick*2);
                }
        }
    }

    // =========================================================================
    // Internal Mounting Posts & Retention Features
    // =========================================================================

    // 1. FireBeetle 2 ESP32-E Retention Cradle (Left Bay)
    fb_x = -outer_w/4;
    fb_y = 6.0;
    translate([fb_x, fb_y, wall_thick]) {
        // Support rails
        difference() {
            cube([firebeetle_w + 3.0, firebeetle_l + 3.0, 6.0], center=true);
            translate([0, 0, 1.0])
                cube([firebeetle_w + 0.4, firebeetle_l + 0.4, 7.0], center=true);
            // Clearance for bottom SMD components
            translate([0, 0, -2.5])
                cube([firebeetle_w - 4.0, firebeetle_l - 4.0, 3.0], center=true);
        }
        // Retention clip tabs
        translate([-firebeetle_w/2 - 1.2, 0, 3.0])
            cube([1.2, 10.0, 1.5], center=true);
        translate([ firebeetle_w/2 + 1.2, 0, 3.0])
            cube([1.2, 10.0, 1.5], center=true);
    }

    // 2. Waveshare Driver HAT Retaining Stand (Right Upper Bay)
    hat_x = outer_w/4 - 4;
    hat_y = 18.0;
    translate([hat_x, hat_y, wall_thick]) {
        // Corner mounting standoffs (M2.5 compatible)
        for (dx = [-hat_w/2 + 3.5, hat_w/2 - 3.5]) {
            for (dy = [-hat_l/2 + 3.5, hat_l/2 - 3.5]) {
                translate([dx, dy, 0])
                    difference() {
                        cylinder(r=3.0, h=6.5);
                        translate([0, 0, 1.5]) cylinder(r=1.1, h=6.0); // Pilot hole
                    }
            }
        }
    }

    // 3. Flat LiPo Battery Recessed Compartment (Center Bay)
    translate([0, -8, wall_thick]) {
        difference() {
            cube([battery_w + 4.0, battery_l + 4.0, 4.5], center=true);
            translate([0, 0, 1.0])
                cube([battery_w + 0.5, battery_l + 0.5, 5.0], center=true);
            // Wire pass-through channels
            translate([0, battery_l/2, 0]) cube([10, 8, 5], center=true);
            translate([0, -battery_l/2, 0]) cube([10, 8, 5], center=true);
        }
    }

    // 4. Isolated Thermal Partition for ENS160+AHT21 Sensor (Lower Right)
    sensor_x = outer_w/3;
    sensor_y = -outer_h/2 + 20;
    translate([sensor_x, sensor_y, wall_thick]) {
        // Thermal isolation wall separating sensor from battery and ESP32
        difference() {
            cube([sensor_w + 6.0, sensor_l + 6.0, 8.0], center=true);
            translate([0, 0, 1.0])
                cube([sensor_w + 0.5, sensor_l + 0.5, 9.0], center=true);
            // Wire notch through top wall
            translate([0, sensor_l/2 + 2, 2.0]) cube([8, 4, 5], center=true);
        }
    }

    // 5. 6x Male Cantilever Snap-Fit Tabs
    snap_positions = [
        [-outer_w/4,  outer_h/2 - wall_thick - 0.7], // Top Left
        [ outer_w/4,  outer_h/2 - wall_thick - 0.7], // Top Right
        [-outer_w/4, -outer_h/2 + wall_thick + 0.7], // Bottom Left
        [ outer_w/4, -outer_h/2 + wall_thick + 0.7], // Bottom Right
        [-outer_w/2 + wall_thick + 0.7, 0],          // Side Left
        [ outer_w/2 - wall_thick - 0.7, 0]           // Side Right
    ];

    for (p = snap_positions) {
        translate([p[0], p[1], base_depth - 1.0]) {
            if (abs(p[0]) > outer_w/3) {
                // Side snaps
                cantilever_snap(snap_w, snap_t, snap_h, snap_catch, 90);
            } else {
                // Top/Bottom snaps
                rot = (p[1] > 0) ? 0 : 180;
                cantilever_snap(snap_w, snap_t, snap_h, snap_catch, rot);
            }
        }
    }
}

// =============================================================================
// Snap-Fit Cantilever Tab Module
// =============================================================================
module cantilever_snap(w, t, h, catch_depth, rotation) {
    rotate([0, 0, rotation]) {
        translate([0, 0, h/2]) {
            // Flexible vertical stem
            cube([w, t, h], center=true);
            // Locking barb with 45° lead-in ramp
            translate([0, t/2 + catch_depth/2, h/2 - 1.2]) {
                hull() {
                    cube([w, catch_depth, 0.4], center=true);
                    translate([0, -catch_depth/2, 1.2])
                        cube([w, 0.1, 0.1], center=true);
                }
            }
        }
    }
}
