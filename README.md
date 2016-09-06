## Simple cube rasterizer for Fuchsia running in QEMU

### Build instructions for QEMU x86-64

1. Build Fuchsia and Magenta for QEMU x86-64 according to instructions here:
  - https://fuchsia.googlesource.com/manifest/
  - https://fuchsia.googlesource.com/magenta/+/HEAD/docs/getting_started.md

2. Make sure it runs properly in QEMU.
3. Switch to the Fuchsia root dir.

4. Clone the cubes repo in **$FUCHSIAROOT/apps**:

    `cd apps`
    
    `git clone https://github.com/j-tetteroo/cubes/`
    
    `cd ..`

5. Copy the file **./apps/cubes/cubes** to directory **./packages/gn/**

    `cp ./apps/cubes/cubes ./packages/gn/`


6. Run the command to generate the ninja files:

    `./packages/gn/gen.py --target=x86-64 --modules=cubes`

7. Run Ninja to build the program:

    `./buildtools/ninja -C out/debug-x86-64`

8. Boot the kernel with the userboot.fs and GUI:
 
    `./magenta/scripts/run-magenta-x86-64 -x out/debug-x86-64/user.bootfs -g`

9. When the graphics shell is done booting, switch to the console with F2 and type **/boot/apps/cubes** `<enter>`

10. Switch to the correct framebuffer by pressing F2 a few times until you get the cubes.
 



See NOTICE and LICENSE file for copyright information.
