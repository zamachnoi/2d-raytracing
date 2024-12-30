#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>

#define WIDTH 1200
#define HEIGHT 800
#define COLOR_WHITE 0xffffffff
#define NUM_RAYS 120
#define M_PI 3.14159265358979323846

enum ObstacleType {
    CIRCLE,
    PIXEL,
    LINE
};

struct Circle {
    int32_t x, y, r;
};

struct Ray {
    int32_t x, y;
    double angle_rad;
};

struct LightSource {
    struct Circle circle;
    struct Ray rays[NUM_RAYS];
};

struct Pixel {
    int32_t x, y;
};

struct Line {
    int32_t x1, y1, x2, y2;
};

struct Obstacle {
    enum ObstacleType type;
    union {
        struct Circle circle;
        struct Pixel pixel;
        struct Line line;
    };
};

struct ObstacleData {
    struct Obstacle *obstacles;
    int num_obstacles;
};

void PutPixel(SDL_Surface *surface, int x, int y, uint32_t color) {
    if (x >= 0 && x < surface->w && y >= 0 && y < surface->h) {
        uint32_t *pixels = (uint32_t *)surface->pixels;
        pixels[(y * surface->w) + x] = color;
    }
}

void DrawHorizontalLine(SDL_Surface *surface, int x1, int x2, int y, uint32_t color) {
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    for (int x = x1; x <= x2; x++) {
        PutPixel(surface, x, y, color);
    }
}

int DrawLine(SDL_Surface *surface, int x0, int y0, int x1, int y1, Uint32 color, int (*callback)(int, int, void*), void *data) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {        
        if (x0 < 0 || x0 >= surface->w || y0 < 0 || y0 >= surface->h) break;
        if (callback && callback(x0, y0, data)) break; // Call the callback and break if it returns 1
        PutPixel(surface, x0, y0, color);

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
    return 1;
}

void DrawFilledCircle(SDL_Surface *surface, struct Circle circle) {
    int x = circle.r;
    int y = 0;
    int err = 0;

    while (x >= y) {
        DrawHorizontalLine(surface, circle.x - x, circle.x + x, circle.y + y, COLOR_WHITE);
        DrawHorizontalLine(surface, circle.x - y, circle.x + y, circle.y + x, COLOR_WHITE);
        DrawHorizontalLine(surface, circle.x - x, circle.x + x, circle.y - y, COLOR_WHITE);
        DrawHorizontalLine(surface, circle.x - y, circle.x + y, circle.y - x, COLOR_WHITE);

        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }

        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

void DrawObstacles(SDL_Surface *surface, struct Obstacle obstacles[], int num_obstacles) {
    for (int i = 0; i < num_obstacles; i++) {
        struct Obstacle obstacle = obstacles[i];
        switch (obstacle.type) {
            case CIRCLE: {
                DrawFilledCircle(surface, obstacle.circle);
            } break;
            case PIXEL: {
                PutPixel(surface, obstacle.pixel.x, obstacle.pixel.y, COLOR_WHITE);
            } break;
            case LINE: {
                DrawLine(surface, obstacle.line.x1, obstacle.line.y1, obstacle.line.x2, obstacle.line.y2, COLOR_WHITE, NULL, NULL);
            } break;
        }
    }
}

int IsObstacle(int x, int y, struct Obstacle obstacles[], int num_obstacles) {
    for (int i = 0; i < num_obstacles; i++) {
        struct Obstacle obstacle = obstacles[i];
        switch (obstacle.type) {
            case CIRCLE: {
                int dx = x - obstacle.circle.x;
                int dy = y - obstacle.circle.y;
                if (dx * dx + dy * dy < obstacle.circle.r * obstacle.circle.r) {
                    return 1;
                }
            } break;
            case PIXEL: {
                if (x == obstacle.pixel.x && y == obstacle.pixel.y) {
                    return 1;
                }
            } break;
            case LINE: {
                int x1 = obstacle.line.x1;
                int y1 = obstacle.line.y1;
                int x2 = obstacle.line.x2;
                int y2 = obstacle.line.y2;
                int dx = x2 - x1;
                int dy = y2 - y1;
                int d = dx * (y - y1) - (x - x1) * dy;
                if (d == 0) {
                    if (x >= x1 && x <= x2 && y >= y1 && y <= y2) {
                        return 1;
                    }
                }
            } break;
        }
    }
    return 0;
}

int ObstacleCallback(int x, int y, void *data) {
    struct ObstacleData *obstacleData = (struct ObstacleData *)data;
    return IsObstacle(x, y, obstacleData->obstacles, obstacleData->num_obstacles);
}

void DrawRays(SDL_Surface *surface, struct Ray rays[], struct Obstacle obstacles[], int num_obstacles) {
    struct ObstacleData obstacleData = {obstacles, num_obstacles};
    for (int i = 0; i < NUM_RAYS; i++) {
        struct Ray ray = rays[i];
        int32_t x0 = ray.x;
        int32_t y0 = ray.y;
        double angle_rad = ray.angle_rad;

        int x1 = x0 + cos(angle_rad) * 10000;
        int y1 = y0 + sin(angle_rad) * 10000;

        DrawLine(surface, x0, y0, x1, y1, COLOR_WHITE, ObstacleCallback, &obstacleData);
    }
}


int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Raytracing", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);

    SDL_Surface *surface = SDL_GetWindowSurface(window);
    SDL_Rect rect = (SDL_Rect) {200,200,200,200};

    
    struct LightSource lightSource = {
        .circle = {400, 300, 50},
        .rays = {}
    };

    for (int i = 0; i < NUM_RAYS; i++) {
        double angle_rad = (2 * M_PI / NUM_RAYS) * i;
        lightSource.rays[i].x = lightSource.circle.x + lightSource.circle.r * cos(angle_rad);
        lightSource.rays[i].y = lightSource.circle.y + lightSource.circle.r * sin(angle_rad);
        lightSource.rays[i].angle_rad = angle_rad;
    }

    struct Obstacle obstacles[] = {
        {CIRCLE, .circle = {600, 400, 50}},
        {PIXEL, .pixel = {700, 400}},
        {LINE, .line = {800, 400, 900, 400}}
    };

    int num_obstacles = sizeof(obstacles) / sizeof(obstacles[0]);

    SDL_Event e;
    int quit = 0;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            } else if (e.type == SDL_MOUSEMOTION && e.motion.state != 0) {
                lightSource.circle.x = e.motion.x;
                lightSource.circle.y = e.motion.y;
                for (int i = 0; i < NUM_RAYS; i++){
                    lightSource.rays[i].x = lightSource.circle.x + lightSource.circle.r * cos(lightSource.rays[i].angle_rad);
                    lightSource.rays[i].y = lightSource.circle.y + lightSource.circle.r * sin(lightSource.rays[i].angle_rad);
                }
            }
        }

        SDL_FillRect(surface, NULL, 0x000000);
        DrawFilledCircle(surface, lightSource.circle);
        DrawObstacles(surface, obstacles, num_obstacles);
        DrawRays(surface, lightSource.rays, obstacles, num_obstacles);

        SDL_UpdateWindowSurface(window);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}