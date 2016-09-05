Simple cube rasterizer for Fuchsia running in QEMU




Build instructions for QEMU x86-64

1. Build fuchsia for QEMU according to instructions here: 
- https://fuchsia.googlesource.com/manifest/
- https://fuchsia.googlesource.com/magenta/+/HEAD/docs/getting\_started.md

2. Make sure it runs properly in QEMU.

3. Extract cubes in the ./apps directory of your main fuchsia directory.

4. Move the file ./apps/cubes/cubes to ./packages/gn/
mv ./apps/cubes/cubes ./packages/gn/

5. Run the command 
./packages/gn/gen.py --target=x86-64 --modules=cubes

6. Run ninja to build the program
./buildtools/ninja -C out/debug-x86-64

7. Boot magenta with the userboot.fs and GUI
./magenta/scripts/run-magenta-x86-64 -x out/debug-x86-64/user.bootfs -g

8. When the graphics shell appears, switch to the console with F2 and type /boot/apps/cubes <enter>

9. Switch to the correct framebuffer by pressing F2 a few times until you get the cubes.






This project includes software from the Fuchsia project under BSD license (see gfx/ and gfx/LICENSE).

This project includes software by Stephan Soller and Tobias Malmsheimer under MIT license (see 3d\_math.h).

This project contains algorithms based on code from http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html (see util.c). 

