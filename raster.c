#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>

#include <magenta/syscalls.h>
#include <sys/mman.h>
#include <mxio/io.h>
#include <magenta/device/display.h>
#include <magenta/device/ioctl.h>

#define MATH_3D_IMPLEMENTATION

#include "raster.h"


#define PI 3.14159265359
#define NUM_CUBES 6

typedef struct
{ 
  float r;
  float g;
  float b;
} color_t;

typedef struct
{
  unsigned int v1, v2, v3, v4;
  vec3_t n;
  vec3_t center;
  vec3_t n2;
  color_t *color;
  float diffuse;
  unsigned char culled;
} quad_t;

typedef struct
{
  vec3_t v[8];
  quad_t q[6];
  vec3_t n[6];
  vec3_t position;
  vec3_t rotation;
  vec3_t scale;  

  mat4_t modelview;
  mat4_t projection;
} cube_t;


const char kVirtualConsole[] = "/dev/class/console/vc";
int persist_fd;


vec3_t n2[6];

gfx_surface* setup_fb();
int flush(int fd);
void draw_rect(gfx_surface* surface, unsigned short x, unsigned short y, unsigned short width, unsigned short height, unsigned int color);

void draw_cube(gfx_surface* surface, cube_t *cube);
void calc_cube(gfx_surface* surface, cube_t *cube);
void draw_quad(gfx_surface* surface, quad_t *q); 

vec3_t light0;

quad_t create_quad(unsigned int v1, unsigned int v2, unsigned int v3, unsigned int v4, vec3_t n, float r, float g, float b) {

  quad_t q;
  color_t *color;
  // TODO free this somehwere
  color = malloc(sizeof(color_t));
  color->r = r; 
  color->g = g; 
  color->b = b;

  q.v1 = v1;
  q.v2 = v2;
  q.v3 = v3;
  q.v4 = v4;
  q.n = v3_norm(n);
  q.color = color;
  return q;

}

cube_t create_cube(vec3_t pos, vec3_t rot, vec3_t scale, vec3_t color) {

  cube_t c;

  c.position = pos;
  c.rotation = rot;
  c.scale = scale;

  // cube vertices
  c.v[0] = vec3(-1.0, 1.0, 1.0);
  c.v[1] = vec3(1.0,1.0,1.0);
  c.v[2] = vec3(1.0,-1.0,1.0);
  c.v[3] = vec3(-1.0,-1.0,1.0);
  c.v[4] = vec3(-1.0,1.0,-1.0);
  c.v[5] = vec3(1.0,1.0,-1.0);
  c.v[6] = vec3(1.0,-1.0,-1.0);
  c.v[7] = vec3(-1.0,-1.0,-1.0);

  // face front    0,1,2,3
  // face back     4,5,6,7
  // face top      4,5,1,0
  // face bottom   7,6,2,3
  // face left     4,0,3,7
  // face right    1,5,6,2
  
  c.q[0] = create_quad(0, 1, 2, 3, vec3(0.0,0.0,1.0), color.x, color.y, color.z);
  c.q[1] = create_quad(4, 5, 6, 7, vec3(0.0,0.0,-1.0), color.x, color.y, color.z);
  c.q[2] = create_quad(4, 5, 1, 0, vec3(0.0,1.0,0.0), color.x, color.y, color.z);
  c.q[3] = create_quad(7, 6, 2, 3, vec3(0.0,-1.0,0.0), color.x, color.y, color.z);
  c.q[4] = create_quad(4, 0, 3, 7, vec3(-1.0,0.0,0.0), color.x, color.y, color.z);
  c.q[5] = create_quad(1, 5, 6, 2, vec3(1.0,0.0,0.0), color.x, color.y, color.z);

  return c;
 
}

void print_fps(gfx_surface *surface, float fps_cnt) {
    char fps[10];
    snprintf(&fps[0], 10, "%.1f fps", fps_cnt);
    for (int i=0;i<10;i++) {
        if (fps[i] == 0) {
            break;
        }
        gfx_putchar(surface, &font18x32, fps[i], (18*i), 0, 0x00FF00FF, 0x00FFFFFF); 
    }
}

int main(int argc, char** argv) {

  gfx_surface* surface;

  struct timeval t1, t2, t3;
  unsigned int frame_cnt = 0;
  float framerate = 999;


  double delta_t = 0.0;
  float angle = 0.0;
  float rot = 0.0;

  cube_t cube;
  cube_t cube2;
  cube_t cube3;
  cube_t cube4;
  cube_t cube5;
  cube_t cube6;

  cube_t *ca[NUM_CUBES];
  cube_t *ctemp;


  surface = setup_fb();
  if (surface == NULL) {
    printf("Cannot create framebuffer surface\n");
    exit(-1);
  }

  // projection matrix
  mat4_t projection = m4_perspective(90, surface->width / ((float)surface->height), 1.0, 10.0);
  mat4_t screenscale = m4_scaling(vec3(surface->width/2.0, -1.0 * surface->height/2.0,1.0));
  mat4_t screentranslate = m4_translation(vec3(surface->width/2.0, surface->height/2.0, 0.0));
  screenscale = m4_mul(screentranslate, screenscale);  
  projection = m4_mul(screenscale, projection);

  // modelview matrix
  mat4_t transform;

  // lights
  light0 = vec3(0.0,2.0,-2.0);

  // cubes
  cube = create_cube(vec3(0.0, 0.0, -4.0), vec3(0.0, 0.0, 0.0), vec3(0.6, 0.6, 0.6), vec3(1.0,0.0,1.0));
  cube2 = create_cube(vec3(-2.0, 0.0, -4.0), vec3(0.0, 0.0, 0.0), vec3(0.4, 0.4, 0.4), vec3(1.0,0.0,0.0));
  cube3 = create_cube(vec3(2.0, 0.0, -4.0), vec3(0.0, 0.0, 0.0), vec3(0.4, 0.4, 0.4), vec3(0.0,0.0,1.0));
  cube4 = create_cube(vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 0.0), vec3(0.15, 0.15, 0.15), vec3(0.0,1.0,1.0));
  cube5 = create_cube(vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 0.0), vec3(0.15, 0.15, 0.15), vec3(1.0,1.0,0.0));
  cube6 = create_cube(vec3(2.0, 0.0, -4.0), vec3(0.0, 0.0, 0.0), vec3(0.4, 0.4, 0.4), vec3(0.0,1.0,0.0));
  ca[0] = &cube;
  ca[1] = &cube2;
  ca[2] = &cube3;
  ca[3] = &cube4;
  ca[4] = &cube5;
  ca[5] = &cube6;
  
  //draw_rect(surface, 50,50, 200, 300, 0x00FF00FF);
 
  gettimeofday(&t3, NULL);

  while(1) {
    gettimeofday(&t1, NULL);

    // clear screen
    gfx_fillrect(surface, 0,0, surface->width, surface->height, 0x00FFFFFF); 


    // set up cubes
    float offset = (2.0/3.0)*PI;

    cube2.position = vec3(2.0*cos(rot), cos(rot), -4.0+(1.0*sin(rot)));
    cube3.position = vec3( 2.0*cos(rot+offset), cos(rot+PI), -4.0+(1.0*sin(rot+offset)));
    cube6.position = vec3( 2.0*cos(rot+(offset*2)), cos(rot*2.0), -4.0+(1.0*sin(rot+(offset*2.0))));
    

    float angle4 = (rot*0.3) + (PI*0.3);
    cube4.position = vec3(2.0*cos(angle4), 2.0*cos(angle4) , -4.0+(2.0*sin(angle4)));
    angle4 += PI;
    cube5.position = vec3(-2.0*cos(angle4), 2.0*cos(angle4) , -4.0+(2.0*sin(angle4)));


    cube.rotation = vec3(angle*2.0, angle*0.33, angle*0.5);
    cube2.rotation = vec3(angle, angle, angle);
    cube3.rotation = vec3(angle, angle, angle);
    cube6.rotation = vec3(angle, angle, angle);
    cube4.rotation = vec3(angle*0.4, angle*0.3, angle*0.5);
    cube5.rotation = vec3(angle*0.2, angle*0.5, angle*0.7);


    // modelview transform
    for (int i=0;i<NUM_CUBES;i++) {
        transform = m4_identity();
        transform = m4_mul(m4_scaling(ca[i]->scale), transform);
        transform = m4_mul(m4_rotation_x(ca[i]->rotation.x), transform);
        transform = m4_mul(m4_rotation_y(ca[i]->rotation.y), transform);
        transform = m4_mul(m4_rotation_z(ca[i]->rotation.z), transform);
        ca[i]->modelview = m4_mul(m4_translation(ca[i]->position), transform);
        ca[i]->projection = projection;
      
    }

    // z-sort cubes
    /*
    for (int i=0;i<NUM_CUBES;i++) {
        for(int j=0;j<NUM_CUBES;j++) {
            if (ca[i]->position.z < ca[j]->position.z) {
                ctemp = ca[i];
                ca[i] = ca[j];
                ca[j] = ctemp;
            }
        }
    }
    */
    for (int i=1;i<NUM_CUBES;i++) {
        for(int j=0;j<NUM_CUBES-1;j++) {
            if (ca[j]->position.z > ca[j+1]->position.z) {
                ctemp = ca[j];
                ca[j] = ca[j+1];
                ca[j+1] = ctemp;
            }
        }
    }
    

    // render
    for (int i=0;i<NUM_CUBES;i++) {
        draw_cube(surface, ca[i]);
    }
    
  /*
    gfx_fillrect(surface, v1.x-50,v1.y-50,100,100, 0x00FF0000);
    gfx_fillrect(surface, v2.x-50,v2.y-50,100,100, 0x0000FF00);
    gfx_fillrect(surface, v3.x-50,v3.y-50,100,100, 0x000000FF);
    gfx_fillrect(surface, v4.x-50,v4.y-50,100,100, 0x0000FFFF);
    */

    print_fps(surface, framerate);

    flush(persist_fd);
    gettimeofday(&t2, NULL);
    delta_t = ((t2.tv_sec - t1.tv_sec) * 1000.0) + ((t2.tv_usec - t1.tv_usec) * 0.001);
   

    if ((t1.tv_sec - t3.tv_sec) >= 1) {
      float time = ((t1.tv_sec - t3.tv_sec) * 1000.0) + ((t1.tv_usec - t3.tv_usec) * 0.001);
      gettimeofday(&t3, NULL);
      framerate = frame_cnt / (time*0.001);
      frame_cnt = 0;
    } else {
      frame_cnt++;
    }

    angle += (delta_t / (0.5*PI*1000.0));
    rot += (delta_t / (0.25*PI*1000.0));
  }
  sleep(5);
  gfx_surface_destroy(surface);
  // TODO mx_handle_close(device->gfx_vmo);
  close(persist_fd);

  return(0);

}

void draw_rect(gfx_surface* surface, unsigned short x, unsigned short y, unsigned short width, unsigned short height, unsigned int color) {
  
  gfx_line(surface, x, y+height, x, y, color);
  gfx_line(surface, x, y, x+width, y, color);
  gfx_line(surface, x+width, y, x+width, y+height, color);
  gfx_line(surface, x+width, y+height, x, y+height, color);

}

void draw_cube(gfx_surface* surface, cube_t *cube) {
/*
1--2
|  |
4--3
*/

    quad_t *q[6];
    quad_t *temp;
    
    vec3_t normal;
    vec3_t center;
    vec3_t v[8];
    vec3_t v1, v2, v3, v4;
    float avgzi, avgzj;

    float r,g,b;
    
    // modelview transform 
    
    for (int i=0;i<8;i++) {
       v[i] = m4_mul_pos(cube->modelview, cube->v[i]);
    }
    

    for (int i=0;i<6;i++) {
        q[i] = &cube->q[i];
        q[i]->culled = 0;
        q[i]->diffuse = 0.0;
        v1 = v[q[i]->v1];
        v2 = v[q[i]->v2];
        v3 = v[q[i]->v3];
        v4 = v[q[i]->v4];

        center = v3_divs(v3_add(v1, v3_add(v2, v3_add(v3, v4))), 4.0);
        normal = v3_norm(m4_mul_dir(cube->modelview, q[i]->n));
        //normal = v3_norm(v3_cross(v3_sub(v2,v1),v3_sub(v3,v1)));

        // backface culling
        if (v3_dot(v3_sub(vec3(0.0,0.0,0.0), center), normal) <= 0) {
             q[i]->culled = 1;
        } else {
             // lighting
             q[i]->diffuse = fmin(0.1+fmax(v3_dot(normal, v3_norm(v3_sub(light0, center))),0.0),1.0);
        }
 
    }

    // projection transform
    for (int i=0;i<8;i++) {
       v[i] = m4_mul_pos(cube->projection, v[i]);
    }


    // z-sort faces 
    // TODO O(n^2) bad..
    for (int i=0;i<6;i++) {
        for (int j=0;j<6;j++) {
            avgzi = (v[q[i]->v1].z + v[q[i]->v2].z + v[q[i]->v3].z + v[q[i]->v4].z) / 4.0;
            avgzj = (v[q[j]->v1].z + v[q[j]->v2].z + v[q[j]->v3].z + v[q[j]->v4].z) / 4.0;
            if (avgzi > avgzj) {
                temp = q[i];
                q[i] = q[j];
                q[j] = temp;
            }
        }
    }


    for (int i=0;i<6;i++) {
        if (q[i]->culled) {
          continue;
        }
        v1 = v[q[i]->v1];
        v2 = v[q[i]->v2];
        v3 = v[q[i]->v3];
        v4 = v[q[i]->v4];

        r = q[i]->diffuse * q[i]->color->r * 255.0;
        g = q[i]->diffuse * q[i]->color->g * 255.0;
        b = q[i]->diffuse * q[i]->color->b * 255.0;
        unsigned int c = ((unsigned int)r) << 16 | ((unsigned int)g) << 8 | ((unsigned int)b);

        fill_triangle(surface, &v1, &v2, &v4, c);  
        fill_triangle(surface, &v2, &v4, &v3, c);  
        gfx_line(surface, v4.x, v4.y, v2.x, v2.y, c);
        // debug normal
        //gfx_line(surface, q->center.x, q->center.y, q->n2.x, q->n2.y, 0x000000FF);
    }
}


gfx_surface* setup_fb() {

  int fd;

  ioctl_display_get_fb_t frame_buffer;
  ssize_t result;

  fd = open(kVirtualConsole, O_RDWR);
  if (fd < 0) {
    printf("Failed to open frame buffer");
    return NULL;
  }
  persist_fd = fd;

  result = mxio_ioctl(fd, IOCTL_DISPLAY_GET_FB, NULL, 0, &frame_buffer, sizeof(frame_buffer));

  if (result < 0) {
    printf("IOCTL_DISPLAY_GET_FB failed.");
    close(fd);
    return NULL;
  }

  printf("Dimensions: %ix%i\n", frame_buffer.info.width, frame_buffer.info.height);
  printf("Format: %i\n", frame_buffer.info.format);
  printf("Pixelsize: %i\n", frame_buffer.info.pixelsize);
  printf("Framebuffer: 0x%x\n", frame_buffer.vmo);

  size_t fb_size = frame_buffer.info.pixelsize * frame_buffer.info.stride * frame_buffer.info.height;
  uintptr_t fb_ptr = 0;

  mx_status_t status = mx_process_map_vm(0, frame_buffer.vmo, 0, fb_size, &fb_ptr,
                                         MX_VM_FLAG_PERM_READ | MX_VM_FLAG_PERM_WRITE);
 
  if (status < 0) {
    printf("Fb vm_map failed: %d.", status);
    return NULL;
  }

  return gfx_create_surface((void *)fb_ptr, frame_buffer.info.width, frame_buffer.info.height, frame_buffer.info.stride, frame_buffer.info.format, 0);

}

int flush(int fd) {

  ssize_t result = mxio_ioctl(fd, IOCTL_DISPLAY_FLUSH_FB, NULL, 0, NULL, 0);
  if (result < 0) {
    perror("IOCTL_DISPLAY_FLUSH_FB failed.");
    return -1;
  }

  return 0;

}

