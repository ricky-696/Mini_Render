#pragma once

#include "geometry.h"
#include <assert.h>
#include <stdlib.h>
#include <algorithm>


//typedef unsigned int IUINT32;
typedef unsigned int UI32;


// 基礎繪製
class Device
{
public:
    int width;                  // 寬度
    int height;                 // 高度
    UI32** framebuffer;			// pixelbuffer：framebuffer[y] 代表第 y行
    float** zbuffer;            // 深度緩衝：zbuffer[y] 為第 y行指針
    UI32** texture;             // 紋理：同樣是每行索引
    int tex_width;              // 紋理寬度
    int tex_height;             // 紋理高度
    float max_u;                // 紋理最大寬度：tex_width - 1
    float max_v;                // 紋理最大高度：tex_height - 1
    int render_state;           // 渲染狀態
    UI32 background;            // 背景顏色

    void init(int width, int height, void* fb) {
        int need = sizeof(void*) * (height * 2 + 1024) + width * height * 8;
        char* ptr = (char*)malloc(need + 64);
        char* framebuf, * zbuf;
        int j;
        assert(ptr);
        framebuffer = (UI32**)ptr;
        zbuffer = (float**)(ptr + sizeof(void*) * height);
        ptr += sizeof(void*) * height * 2;
        texture = (UI32**)ptr;
        ptr += sizeof(void*) * 1024;
        framebuf = (char*)ptr;
        zbuf = (char*)ptr + width * height * 4;
        ptr += width * height * 8;
        if (fb != NULL) framebuf = (char*)fb;
        for (j = 0; j < height; j++) {
            framebuffer[j] = (UI32*)(framebuf + width * 4 * j);
            zbuffer[j] = (float*)(zbuf + width * 4 * j);
        }
        texture[0] = (UI32*)ptr;
        texture[1] = (UI32*)(ptr + 16);
        memset(texture[0], 0, 64);
        tex_width = 2;
        tex_height = 2;
        max_u = 1.0f;
        max_v = 1.0f;
        this->width = width;
        this->height = height;
        background = 0xFFFFFF;
    }
    void destroy() {
        if (framebuffer)
            free(framebuffer);
        framebuffer = NULL;
        zbuffer = NULL;
        texture = NULL;
    }
    void clear(int mode = 0) {
        int y, x;
        for (y = 0; y < height; y++) {
            UI32* dst = framebuffer[y];
            UI32 cc = (height - 1 - y) * 230 / (height - 1);
            cc = (cc << 16) | (cc << 8) | cc;
            if (mode == 0) cc = background;
            for (x = width; x > 0; dst++, x--) dst[0] = cc;
        }
        for (y = 0; y < height; y++) {
            float* dst = zbuffer[y];
            for (x = width; x > 0; dst++, x--) dst[0] = 0.0f;
        }
    }
    void setPixel(int x, int y, UI32 color) {
        if (((UI32)x) < (UI32)width && ((UI32)y) < (UI32)height) {
            framebuffer[y][x] = color;
        }
    }
    void drawLine(int x1, int y1, int x2, int y2, UI32 c) {
        int x, y, rem = 0;
        if (x1 == x2 && y1 == y2) {
            setPixel(x1, y1, c);
        }
        else if (x1 == x2) {
            int inc = (y1 <= y2) ? 1 : -1;
            for (y = y1; y != y2; y += inc) setPixel(x1, y, c);
            setPixel(x2, y2, c);
        }
        else if (y1 == y2) {
            int inc = (x1 <= x2) ? 1 : -1;
            for (x = x1; x != x2; x += inc) setPixel(x, y1, c);
            setPixel(x2, y2, c);
        }
        else {
            int dx = (x1 < x2) ? x2 - x1 : x1 - x2;
            int dy = (y1 < y2) ? y2 - y1 : y1 - y2;
            if (dx >= dy) {
                if (x2 < x1) x = x1, y = y1, x1 = x2, y1 = y2, x2 = x, y2 = y;
                for (x = x1, y = y1; x <= x2; x++) {
                    setPixel(x, y, c);
                    rem += dy;
                    if (rem >= dx) {
                        rem -= dx;
                        y += (y2 >= y1) ? 1 : -1;
                        setPixel(x, y, c);
                    }
                }
                setPixel(x2, y2, c);
            }
            else {
                if (y2 < y1) x = x1, y = y1, x1 = x2, y1 = y2, x2 = x, y2 = y;
                for (x = x1, y = y1; y <= y2; y++) {
                    setPixel(x, y, c);
                    rem += dx;
                    if (rem >= dy) {
                        rem -= dy;
                        x += (x2 >= x1) ? 1 : -1;
                        setPixel(x, y, c);
                    }
                }
                setPixel(x2, y2, c);
            }
        }
    }
    void fillBottomFlatTriangle(vec3 v1, vec3 v2, vec3 v3, UI32 c) {
        float invslope1 = (v2.x - v1.x) / (v2.y - v1.y);
        float invslope2 = (v3.x - v1.x) / (v3.y - v1.y);

        float curx1 = v1.x;
        float curx2 = v1.x;

        for (int scanlineY = v1.y; scanlineY <= v2.y; scanlineY++)
        {
            drawLine((int)curx1, scanlineY, (int)curx2, scanlineY, c);
            curx1 += invslope1;
            curx2 += invslope2;
        }
    }
    void fillTopFlatTriangle(vec3 v1, vec3 v2, vec3 v3, UI32 c) {
        float invslope1 = (v3.x - v1.x) / (v3.y - v1.y);
        float invslope2 = (v3.x - v2.x) / (v3.y - v2.y);

        float curx1 = v3.x;
        float curx2 = v3.x;

        for (int scanlineY = v3.y; scanlineY > v1.y; scanlineY--)
        {
            drawLine((int)curx1, scanlineY, (int)curx2, scanlineY, c);
            curx1 -= invslope1;
            curx2 -= invslope2;
        }
    }
    void drawTriangle(vec3 v1, vec3 v2, vec3 v3, UI32 c);
    void fillTriangle(vec3 v1, vec3 v2, vec3 v3, UI32 c);
};