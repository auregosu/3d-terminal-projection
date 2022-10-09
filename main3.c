#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#define bool unsigned char
#define false 0
#define true !false
#define CUB_NUM 1

int width = 50;
int height = 50;
float heightScale = 0.45f;
char **buffer;
float **projMat;

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

v3 v3sum(struct v3 a, struct v3 b, char sign) {
  struct v3 c;
  if (sign == '-') {
    return (v3){
      a.x - b.x,
      a.y - b.y,
      a.z - b.z};
  } else {
    return (v3){
      a.x + b.x,
      a.y + b.y,
      a.z + b.z};
  }
}

v3 v3sumf(struct v3 a, float b) {
  return (v3){
    a.x + b,
    a.y + b,
    a.z + b};
}

typedef struct triangle {
  struct v3 p[3];
} triangle;

typedef struct cubeMesh {
  struct triangle triangles[12];
} cubeMesh;

typedef struct cube{
  struct v3 pos;
  struct cubeMesh mesh;
  struct v3 r;
} cube;

struct v3 cam = {0, 0, 0};
struct v3 camR = {0, 0, 0};
float camSpeed = 0.1;
float camRSpeed = 0.1;

void populateCube(struct cube* cb) {
  cb->mesh = (cubeMesh){
    // South
    (triangle){(v3){0, 0, 0}, (v3){0, 1, 0}, (v3){1, 1, 0}},
    (triangle){(v3){0, 0, 0}, (v3){1, 1, 0}, (v3){1, 0, 0}},

    // East
    (triangle){(v3){1, 0, 0}, (v3){1, 1, 0}, (v3){1, 1, 1}},
    (triangle){(v3){1, 0, 0}, (v3){1, 1, 1}, (v3){1, 0, 1}},

    // North
    (triangle){(v3){1, 0, 1}, (v3){1, 1, 1}, (v3){0, 1, 1}},
    (triangle){(v3){1, 0, 1}, (v3){0, 1, 1}, (v3){0, 0, 1}},

    // West
    (triangle){(v3){0, 0, 1}, (v3){0, 1, 1}, (v3){0, 1, 0}},
    (triangle){(v3){0, 0, 1}, (v3){0, 1, 0}, (v3){0, 0, 0}},

    // Top
    (triangle){(v3){0, 1, 0}, (v3){0, 1, 1}, (v3){1, 1, 1}},
    (triangle){(v3){0, 1, 0}, (v3){1, 1, 1}, (v3){1, 1, 0}},

    // Bottom
    (triangle){(v3){1, 0, 1}, (v3){0, 0, 1}, (v3){0, 0, 0}},
    (triangle){(v3){1, 0, 1}, (v3){0, 0, 0}, (v3){1, 0, 0}}
  };
  for (int i = 0; i < 12; i++) {
    cb->mesh.triangles[i].p[0] = v3sumf(cb->mesh.triangles[i].p[0], -0.5f);
    cb->mesh.triangles[i].p[1] = v3sumf(cb->mesh.triangles[i].p[1], -0.5f);
    cb->mesh.triangles[i].p[2] = v3sumf(cb->mesh.triangles[i].p[2], -0.5f);
  }
}

void rotationMatrix(struct v3 i, struct v3 *o, struct v3 r) {
  o->x = i.x*(cosf(r.x)*cosf(r.y)) 
    + i.y*(cosf(r.x)*sinf(r.y)*sinf(r.z)-sinf(r.x)*cosf(r.z)) 
    + i.z*(cosf(r.x)*sinf(r.y)*cosf(r.z)+sinf(r.x)*sinf(r.z));
  o->y = i.x*(sinf(r.x)*cosf(r.y)) 
    + i.y*(sinf(r.x)*sinf(r.y)*sinf(r.z)+cosf(r.x)*cosf(r.z)) 
    + i.z*(sinf(r.x)*sinf(r.y)*cosf(r.z)-cosf(r.x)*sinf(r.z));
  o->z = i.x*-sinf(r.y) 
    + i.y*cosf(r.y)*sinf(r.z) 
    + i.z*cosf(r.y)*cosf(r.z);
}

void projectionMatrix(struct v3 i, struct v3 *o, float **m) {
  o->x = i.x * m[0][0] + i.y * m[1][0] + i.z * m[2][0] + m[3][0];
  o->y = i.x * m[0][1] + i.y * m[1][1] + i.z * m[2][1] + m[3][1];
  o->z = i.x * m[0][2] + i.y * m[1][2] + i.z * m[2][2] + m[3][2];
  float w = i.x * m[0][3] + i.y * m[1][3] + i.z * m[2][3] + m[3][3];
  if (w != 0.0f) {
    o->x /= w;
    o->y /= w;
    o->z /= w;
  }
}

float  distanceBetween(struct v3 a, struct v3 b) {
  struct v3 dif = (v3){a.x-b.x, a.y-b.y, a.z-b.z};
  return sqrtf(dif.x*dif.x + dif.y*dif.y + dif.z*dif.z);
}

struct v3 crossProduct(struct v3 a, struct v3 b, struct v3 c) {
  struct v3 line1, line2, d;
  line1 = v3sum(a, c, '-');
  line2 = v3sum(b, c, '-');
  return (v3){
    line1.y * line2.z - line1.z * line2.y,
    line1.z * line2.x - line1.x * line2.z,
    line1.x * line2.y - line1.y * line2.x};
}

void putPixel (int x, int y, char c) {
  y = (int)((float)y*heightScale);
  if (x >= 0 && x < width && y >= 0 && y < height)
    buffer[x][y] = c;
}
void drawLine(struct v2 a, struct v2 b, char c) {
  // Bresenham's algorithm
  struct v2 dif = (v2){abs(b.x - a.x), -abs(b.y - a.y)};
  float sx = a.x < b.x ? 1 : -1;
  float sy = a.y < b.y ? 1 : -1;
  float error = dif.x + dif.y;

  float hypothenuse = sqrtf(dif.x*dif.x + dif.y*dif.y);
  unsigned int step = (int)(hypothenuse*0.8f);
  while (step) {
    putPixel(a.x, a.y, c);
    if (a.x == b.x && a.y == b.y)
      break;
    float e2 = 2 * error;
    if (e2 >= dif.y) {
        if (a.x == b.x)
          break;
        error += dif.y;
        a.x += sx;
    }
    if (e2 <= dif.x) {
        if (a.y == b.y)
          break;
        error += dif.x;
        a.y += sy;
    }
    step--;
  }
}

float crossProduct2(struct v2 a, struct v2 b) {
  return a.x * b.y - a.y * b.x;
}

void drawTriangle(struct v2 p1, struct v2 p2, struct v2 p3, char c) {
  int maxX = (int)fmax(p1.x, fmax(p2.x, p3.x));
  int minX = (int)fmin(p1.x, fmin(p2.x, p3.x));
  int maxY = (int)fmax(p1.y, fmax(p2.y, p3.y));
  int minY = (int)fmin(p1.y, fmin(p2.y, p3.y));

  struct v2 vs1 = (v2){p2.x - p1.x, p2.y - p1.y};
  struct v2 vs2 = (v2){p3.x - p1.x, p3.y - p1.y};

  for (int x = minX; x <= maxX; x++)
  {
    for (int y = minY; y <= maxY; y++)
    {
      struct v2 q = (v2){x - p1.x, y - p1.y};

      float s = (float)crossProduct2(q, vs2) / crossProduct2(vs1, vs2);
      float t = (float)crossProduct2(vs1, q) / crossProduct2(vs1, vs2);

      if ( (s >= 0) && (t >= 0) && (s + t <= 1))
      {
        putPixel(x, y, c);
      }
    }
  }
}

int main() {
  float fNear = 0.1f, fFar = 1000.0f, fov = 90.0f, aspectRatio = (float)height/(float)width;
  float fovTan = 1.0f / tanf(fov * 0.5f / 180.0f * M_PI);

  // Create projection matrix
  projMat = malloc(4*sizeof(float*));
  for (int i = 0; i < 4; i++)
    projMat[i] = malloc(4*sizeof(float));
  projMat[0][0] = aspectRatio * fovTan;
  projMat[1][1] = fovTan;
  projMat[2][2] = fFar / (fFar - fNear);
  projMat[3][2] = (-fFar * fNear) / (fFar - fNear);
  projMat[2][3] = 1.0f;
  projMat[3][3] = 0.0f;

  buffer = malloc(width*sizeof(char*));
  for (int i = 0; i < width; i++)
    buffer[i] = malloc(height*sizeof(char));

  struct cube *cubes;
  cubes = malloc(CUB_NUM * sizeof(struct cube));

  for (int i = 0; i < CUB_NUM; i++) {
    cubes[i].pos = (v3){i*4, 0, 1.5f};
    populateCube(cubes+i);
  }

  printf("\e[1;1H\e[2J");
  float time = 0;
  // Main loop
  while (time >= 0) {
    for (int x = 0; x < width; x++) {
      for (int y = 0; y < height; y++)
        buffer[x][y] = ' ';
    }

    printf("\033[%d;%dH cam:position = [%.2f, %.2f, %.2f]", 1, width+2, cam.x, cam.y, cam.z);
    printf("\033[%d;%dH cam:rotation = [%.2f, %.2f, %.2f]", 2, width+2, camR.x, camR.y, camR.z);

    int k = 0;
    for (int i = 0; i < CUB_NUM; i++) {
      for (int j = 0; j < 12; j++) {
        struct triangle triProjected, triTranslated, triRotated;

        rotationMatrix(cubes[i].mesh.triangles[j].p[0], triRotated.p, cubes[i].r);
        rotationMatrix(cubes[i].mesh.triangles[j].p[1], triRotated.p+1, cubes[i].r);
        rotationMatrix(cubes[i].mesh.triangles[j].p[2], triRotated.p+2, cubes[i].r);

        triTranslated.p[0] = v3sum(triRotated.p[0], v3sum(cam, cubes[i].pos, '+'), '+');
        triTranslated.p[1] = v3sum(triRotated.p[1], v3sum(cam, cubes[i].pos, '+'), '+');
        triTranslated.p[2] = v3sum(triRotated.p[2], v3sum(cam, cubes[i].pos, '+'), '+');

        struct v3 normal = crossProduct(triTranslated.p[0], triTranslated.p[1], triTranslated.p[2]);
        float length = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
        normal.x /= length; normal.y /= length; normal.z /= length;
        float dotProduct = normal.x * triTranslated.p[0].x +
                          normal.y * triTranslated.p[0].y +
                          normal.z * triTranslated.p[0].z;

        if (dotProduct < 0.0f) {
          projectionMatrix(triTranslated.p[0], triProjected.p, projMat);
          projectionMatrix(triTranslated.p[1], triProjected.p+1, projMat);
          projectionMatrix(triTranslated.p[2], triProjected.p+2, projMat);

          struct v2 p0 = (v2){(triProjected.p[0].x+1.0f)*0.5f*width, (triProjected.p[0].y+1.0f)*0.5f*height};
          struct v2 p1 = (v2){(triProjected.p[1].x+1.0f)*0.5f*width, (triProjected.p[1].y+1.0f)*0.5f*height};
          struct v2 p2 = (v2){(triProjected.p[2].x+1.0f)*0.5f*width, (triProjected.p[2].y+1.0f)*0.5f*height};

          //printf("\033[%d;%dH [%f, %f, %f] -> [%f, %f]", j+1, width+1, triTranslated.p[0].x, triTranslated.p[0].y, triTranslated.p[0].z, p0.x, p0.y);

          short light = (int)(dotProduct*-11.0f);
          switch (light) {
            case 0: drawTriangle(p0, p1, p2, '@'); break;
            case 1: drawTriangle(p0, p1, p2, '$'); break;
            case 2: drawTriangle(p0, p1, p2, 'M'); break;
            case 3: drawTriangle(p0, p1, p2, 'X'); break;
            case 4: drawTriangle(p0, p1, p2, '?'); break;
            case 5: drawTriangle(p0, p1, p2, 'x'); break;
            case 6: drawTriangle(p0, p1, p2, '+'); break;
            case 7: drawTriangle(p0, p1, p2, '='); break;
            case 8: drawTriangle(p0, p1, p2, '-'); break;
            case 9: drawTriangle(p0, p1, p2, ';'); break;
            case 10:drawTriangle(p0, p1, p2, ':'); break;
            case 11:drawTriangle(p0, p1, p2, ','); break;
            default:drawTriangle(p0, p1, p2, '.'); break;
          }
          //drawLine(p0, p1, '#');
          //drawLine(p1, p2, '#');
          //drawLine(p2, p0, '#');
          putPixel(p0.x, p0.y, '#');
          putPixel(p1.x, p1.y, '#');
          putPixel(p2.x, p2.y, '#');
        }
      }
      k = 0;
      printf("\033[%d;%dH %d:position = [%.2f, %.2f, %.2f]", 2*i+4, width+2, i, cubes[i].pos.x, cubes[i].pos.y, cubes[i].pos.z);
      printf("\033[%d;%dH %d:rotation = [%.2f, %.2f, %.2f]", 2*i+5, width+2, i, cubes[i].r.x, cubes[i].r.y, cubes[i].r.z);
    }

    // Drawing
    int scaledHeight = (int)((float)height*heightScale);
    printf("\x1b[H");
    for (int y = 0; y <= scaledHeight; y++) {
      for (int x = 0; x <= width; x++) {
        if (x == width)
          putchar('#');
        else
          putchar(buffer[x][y]);
      }
      putchar('\n');
      if (y == scaledHeight)
        for (int i = 0; i < width-1; i++)
          putchar('#');
    }
    putchar('\n');

    // Input
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

    // Showoff
    if (CUB_NUM >= 5) {
      cubes[1].r.x = time*M_PI;
      cubes[2].r.y = time*M_PI;
      cubes[3].r.z = time*M_PI;

      cubes[4].r.x = time*M_PI;
      cubes[4].r.y = time*M_PI;
      cubes[4].r.z = time*M_PI;
    } else if (CUB_NUM == 1) {
      cubes[0].r.x = time*M_PI;
      cubes[0].r.y = time*M_PI;
      cubes[0].r.z = time*M_PI;
    }

    if (keyIsPressed(XK_Escape))
      time = -1.0f;
    time += 0.01f;
  }

  printf("\e[1;1H\e[2J\n");
  printf("freeing memory\n");
  free(cubes);
  for (int x = 0; x < width; x++) {
    free(buffer[x]);
  }
  free(buffer);
  for (int x = 0; x < 4; x++) {
    free(projMat[x]);
  }
  free(projMat);
  printf("successful exit!\n");
  return 0;
}
