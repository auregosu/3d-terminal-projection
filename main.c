#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#define CUB_NUM 1

int width = 30;
int height = 30;
int scale = 100;

int keyIsPressed(KeySym ks) {
  Display *dpy = XOpenDisplay(":0");
  char keysReturn[32];
  XQueryKeymap(dpy, keysReturn);
  KeyCode kc2 = XKeysymToKeycode(dpy, ks);
  int isPressed = !!(keysReturn[kc2 >> 3] & (1 << (kc2 & 7)));
  XCloseDisplay(dpy);
  return isPressed;
}

typedef struct v2 {
  float x,y;
} v2;

typedef struct iv2 {
  int x, y;
} iv2;

typedef struct v3 {
  float x,y,z;
} v3;

typedef struct cube{
  struct v3 pos;
  struct v3 lP[8];
  struct v3 p[8];
  struct v3 r;
} cube;

struct v3 cam = {0, 0, 0};
struct v3 camR = {0, 0, 0};
float camSpeed = 0.1;
float camRSpeed = 0.01;

void populateCube(struct cube* cb) {
  cb->lP[0] = (v3){1, -1, 1};
  cb->lP[1] = (v3){-1, -1, 1};
  cb->lP[2] = (v3){1, -1, -1};
  cb->lP[3] = (v3){-1, -1, -1};

  cb->lP[4] = (v3){1, 1, 1};
  cb->lP[5] = (v3){-1, 1, 1};
  cb->lP[6] = (v3){1, 1, -1};
  cb->lP[7] = (v3){-1, 1, -1};
}

v3 rotationMatrix(struct v3 localP, struct v3 cubeP, struct v3 r) {
  // Bidimensional matrix
  /*
  float x = localP.x*cosf(r.x)-localP.z*sinf(r.x);
  float z = localP.x*sinf(r.x)+localP.z*cosf(r.x);
  */

  // 3D matrix
  float x = localP.x*(cosf(r.x)*cosf(r.y)) 
    + localP.y*(cosf(r.x)*sinf(r.y)*sinf(r.z)-sinf(r.x)*cosf(r.z)) 
    + localP.z*(cosf(r.x)*sinf(r.y)*cosf(r.z)+sinf(r.x)*sinf(r.z));
  float y = localP.x*(sinf(r.x)*cosf(r.y)) 
    + localP.y*(sinf(r.x)*sinf(r.y)*sinf(r.z)+cosf(r.x)*cosf(r.z)) 
    + localP.z*(sinf(r.x)*sinf(r.y)*cosf(r.z)-cosf(r.x)*sinf(r.z));
  float z = localP.x*-sinf(r.y) 
    + localP.y*cosf(r.y)*sinf(r.z) 
    + localP.z*cosf(r.y)*cosf(r.z);

  struct v3 newPos = {x-cubeP.x, localP.y-cubeP.y, z-cubeP.z};
  return newPos;
}

iv2 cameraMatrix(struct v3 a, float aspectRatio) {
  float x = a.x - cam.x;
  float y = a.y - cam.y;
  float z = a.z - cam.z;
  float dx = cosf(camR.y)*(sinf(camR.z)*y+cosf(camR.z)*x) - sinf(camR.y)*z;
  float dy = sinf(camR.x)*(cosf(camR.y)*z+sinf(camR.y)*(sinf(camR.z)*y+cosf(camR.z)*x)) + cosf(camR.x)*(cosf(camR.z)*y-sinf(camR.z)*x);
  float dz = cosf(camR.x)*(cosf(camR.y)*z+sinf(camR.y)*(sinf(camR.z)*y+cosf(camR.z)*x)) - cosf(camR.x)*(cosf(camR.z)*y-sinf(camR.z)*x);
  struct iv2 b;
  b = (iv2){aspectRatio*scale*dx/dz + width/2, scale*dy/dz + height/2};
  //struct iv2 b = {scale*x/z + width/2, scale*y/z + height/4};
  return b;
}

float  distanceBetween(struct v3 a, struct v3 b) {
  struct v3 dif = (v3){a.x-b.x, a.y-b.y, a.z-b.z};
  return sqrtf(dif.x*dif.x + dif.y*dif.y + dif.z*dif.z);
}

int main() {
  float aspectRatio;
  aspectRatio = width/height;
  char **buffer;
  buffer = malloc(width*sizeof(char*));
  for (int i = 0; i < width; i++)
    buffer[i] = malloc(height*sizeof(char));

  struct cube *cubes;
  cubes = malloc(CUB_NUM * sizeof(struct cube));

  for (int i = 0; i < CUB_NUM; i++) {
    cubes[i].pos = (v3){i*4, 0, 10};
    populateCube(cubes+i);
  }

  printf("\e[1;1H\e[2J");
  float time = 0;
  // Main loop
  while (cam.y <= 10) {
    //memset(buffer, '\'', sizeof(buffer));
    for (int x = 0; x < width; x++) {
      for (int y = 0; y < height; y++)
        buffer[x][y] = ' ';
    }

    for (int i = 0; i < CUB_NUM; i++) {
      for (int j = 0; j < 8; j++) {
      cubes[i].p[j] = rotationMatrix(cubes[i].lP[j], cubes[i].pos, cubes[i].r);
      }
    }

    for (int i = 0; i < CUB_NUM; i++) {
      for (int j = 0; j < 8; j++) {
        struct iv2 point = cameraMatrix(cubes[i].p[j], aspectRatio);
        printf("\033[%d;%dH [%d, %d] <- [%.1f, %.1f, %.1f]", j+1, width, point.x, point.y, cubes[i].p[j].x, cubes[i].p[j].y, cubes[i].p[j].z);
        if (point.x < width && point.y < height && point.x > 0 && point.y > 0) {
          if (distanceBetween(cam, cubes[i].p[j]) <= 10)
            buffer[point.x][point.y/2] = '#';
          else
            buffer[point.x][point.y/2] = '+';
        }
      }
    }

    //printf("\033[%d;%dH [%.2f, %.2f, %.2f]", height, width, cam.x, cam.y, cam.z);
    printf("\x1b[H");
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        //printf("\033[%d;%dH%c", y, x, buffer[x][y]);
        putchar(buffer[x][y]);
      }
      putchar('\n');
    }
    if (keyIsPressed(XK_W))
      cam.z -= camSpeed;
    if (keyIsPressed(XK_S))
      cam.z += camSpeed;
    if (keyIsPressed(XK_A))
      cam.x += camSpeed;
    if (keyIsPressed(XK_D))
      cam.x -= camSpeed;
    if (keyIsPressed(XK_Q))
      cam.y += camSpeed;
    if (keyIsPressed(XK_E))
      cam.y -= camSpeed;

    if (keyIsPressed(XK_Up))
      camR.x += camRSpeed;
    if (keyIsPressed(XK_Down))
      camR.x -= camRSpeed;
    if (keyIsPressed(XK_Right))
      camR.y += camRSpeed;
    if (keyIsPressed(XK_Left))
      camR.y -= camRSpeed;
    if (keyIsPressed(XK_O))
      camR.z += camRSpeed;
    if (keyIsPressed(XK_P))
      camR.z -= camRSpeed;
    cubes[0].r.y = sinf(time)*M_PI;
    //cubes[0].r.z+=0.005;

    time += 0.02;
  }

  printf("\e[1;1H\e[2J\n");
  printf("freeing cubes' array\n");
  free(cubes);
  printf("freeing buffer\n");
  for (int x = 0; x < width; x++) {
    free(buffer[x]);
  }
  printf("freeing buffers' array\n");
  free(buffer);
  printf("successful exit!\n");
  return 0;
}
