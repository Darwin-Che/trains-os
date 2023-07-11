Group: Zhaocheng Che(20818599) and Saksham Bajaj(20820438)

## Building program

Step 0: In the student environment, clone the repository and checkout the hash.

Step 1: Run `make` in the root of the project directory will produce a `trainsos.img`.This will build with configuration of ‘-O3’ and both caches enabled.
(On this branch, MMU can be enabled at the kernel boot.) The default compile option allows the program to run on the PI connected to the track.

Step 2: Load the image on a Raspberry Pi by (replace X for a valid barcode) `/u/cs452/public/tools/setupTFTP.sh CS01754X trainsos.img`

Step 3: Make sure the track is in good condition and reset the box.

Step 4: Boot the Raspberry Pi.

Step 5: Dashboard start running automatically after booting. Please don't reset the box while the program is running.

## Compile Option

The default compile option doesn't include CTS. If you want to run the program with CTS, change the compiler option `WAIT_FOR_CTS` located in `user/include/uart.h`.

## Design Document
The dashboard navigation section at the end of the design document shows the available commands, and a screenshot of a sample dashboard with available commands.
The most recent design document can be found at `doc/final.pdf`.

## Raw Experiment Data

Located at `user/src/calib/calib.c`.
