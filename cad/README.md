# 3D Printable Enclosure - 7.5" E-Paper Desk Clock (70° Tilt)

A two-piece, toolless snap-fit desk stand enclosure tailored for the Waveshare 7.5" V2 E-Paper display, DFRobot FireBeetle 2 ESP32-E, ENS160+AHT21 air quality sensor, and a flat LiPo pouch battery.

---

## 📁 CAD Files in this Directory

| File | Description |
| :--- | :--- |
| [`front_bezel.stl`](file:///home/drewfus/.icons/eink_clock/cad/front_bezel.stl) | Front frame with active viewing window, glass retention shelf, and snap receivers |
| [`rear_stand.stl`](file:///home/drewfus/.icons/eink_clock/cad/rear_stand.stl) | Rear housing with integrated 70° desk stand, component bays, USB-C slot, and snap tabs |
| [`eink_clock_enclosure.scad`](file:///home/drewfus/.icons/eink_clock/cad/eink_clock_enclosure.scad) | Parametric OpenSCAD source file for modifying tolerances or tilt angles |
| [`generate_stl.py`](file:///home/drewfus/.icons/eink_clock/cad/generate_stl.py) | Standalone Python script to regenerate binary STLs directly |

---

## 🖨️ Recommended 3D Slicer Settings

| Setting | Recommended Value | Notes |
| :--- | :--- | :--- |
| **Material** | **PETG** or **PLA** | PETG is ideal for resilient snap-fit tabs that flex without breaking |
| **Layer Height** | **0.20 mm** | Good balance between surface finish and print speed |
| **Perimeters / Walls** | **3 to 4 walls** | Ensures robust snap tabs and rigid stand structure |
| **Infill** | **15% – 20%** | Gyroid or Cubic pattern |
| **Top / Bottom Layers** | **4 / 4 layers** | Solid finish on flat surfaces |

### Print Orientation & Supports:
1. **`front_bezel.stl`**:
   - Orient **flat face-down** on the build plate (window facing the bed).
   - A textured PEI sheet will give the front bezel a premium matte textured look!
   - **Supports**: **None required** (0% support).
2. **`rear_stand.stl`**:
   - Orient with the flat base on the build plate.
   - **Supports**: Tree supports (organic) enabled only for the small horizontal USB-C cutout bridge.

---

## 🛠️ Step-by-Step Assembly Instructions

### Step 1: Install the Waveshare 7.5" Display
1. Place the front bezel face-down on a soft, clean surface (e.g. microfiber cloth).
2. Drop the Waveshare 7.5" e-Paper panel into the rear stepped recess. The active screen area fits into the viewing window, and the glass edges rest against the inner retention ledge.
3. Pass the bottom flexible flat cable (FPC) through the relief notch.

### Step 2: Mount the Electronics into the Rear Housing
1. **FireBeetle 2 ESP32-E**: Slide the board into the left retention cradle with the USB-C connector facing the side port cutout.
2. **Waveshare e-Paper Driver HAT**: Seat the board on the upper right mounting standoffs and connect the 8-pin JST-PH cable from the HAT to the display FPC adapter.
3. **ENS160 + AHT21 Sensor**: Place the sensor module into the lower-right isolated chamber directly over the ventilation louvers. Run the 4 I2C wires (3.3V, GND, SDA IO21, SCL IO22) to the FireBeetle board.
4. **LiPo Pouch Battery**: Connect the battery lead to the FireBeetle PH2.0 battery port, and rest the battery into the center cavity (can be secured with a small strip of double-sided tape).

### Step 3: Snap Together
1. Align the front bezel with the rear housing.
2. Press gently along the perimeter until the **6 snap-fit latches** click into place (top, bottom, and sides).
3. The clock is now assembled and sits at an ergonomic 70° angle on your desk!

---

## 🔄 Disassembly (Opening the Case)
If you ever need to service the battery or rewire components:
- Insert a small flat tool, guitar pick, or coin into the **pry slot on the bottom center edge**.
- Gently twist to release the bottom snap tabs, then unlatch the top and sides. No screws or destructive force required!
