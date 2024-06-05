#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <SFML/Graphics.h>
#include <SFML/Audio.h>
#include <SFML/Window.h>
#include <SFML/System.h>
#include <SFML/Config.h>
#include <SFML/Network.h>
#include <SFML/OpenGL.h>
#include <time.h>
#include <math.h>

#define MAX_OCTAVES 16
#define PERSISTENCE 0.5f
#define CHUNK_SIZE 500
#define MAX_HEIGHT 100

typedef enum {
    GRASS,
    DIAMOND_ORE,
    BEDROCK,
    STONE,
    DIRT,
    AIR,
} block_type;

float fade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float grad(int hash, float x, float y, float z)
{
    int h = hash & 15;
    float u = h<8 ? x : y;
    float v = h<4 ? y : h==12||h==14 ? x : z;
    return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}


block_type ***alloc_noise_3d(void)
{
    block_type ***blocks = malloc(MAX_HEIGHT * sizeof(block_type **));
    for (int i = 0; i < MAX_HEIGHT; i++) {
        blocks[i] = malloc(CHUNK_SIZE * sizeof(block_type *));
        for (int j = 0; j < CHUNK_SIZE; j++) {
            blocks[i][j] = malloc(CHUNK_SIZE * sizeof(int));
        }
    }
    return blocks;
}

void free_noise_3d(block_type*** blocks)
{
    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            free(blocks[i][j]);
        }
        free(blocks[i]);
    }
    free(blocks);
}

float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float smoothstep(float t)
{
    return t * t * (3 - 2 * t);
}

int* create_seed(void)
{
    int* tab = malloc(sizeof(int) * 512);
    int i = 0;
    int j = 0;
    int tmp = 0;
    for (i = 0; i < 256; i++)
    {
        tab[i] = i;
    }
    for (i = 0; i < 256; i++)
    {
        j = rand() % 256;
        tmp = tab[i];
        tab[i] = tab[j];
        tab[j] = tmp;
    }
    for (i = 0; i < 256; i++)
    {
        tab[i + 256] = tab[i];
    }
    return tab;
}

float perlinNoise3D(float x, float y, float z, int* tab)
{
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    int Z = (int)floor(z) & 255;
    x -= floor(x);
    y -= floor(y);
    z -= floor(z);
    float u = smoothstep(x);
    float v = smoothstep(y);
    float w = smoothstep(z);
    int A = tab[X] + Y, AA = tab[A] + Z, AB = tab[A + 1] + Z, B = tab[X + 1] + Y, BA = tab[B] + Z, BB = tab[B + 1] + Z;

    float lerp_uv_x = grad(tab[AA], x, y, z) * (1 - u) + grad(tab[BA], x - 1, y, z) * u;
    float lerp_uv_x_1 = grad(tab[AB], x, y - 1, z) * (1 - u) + grad(tab[BB], x - 1, y - 1, z) * u;
    float lerp_uvw_x = lerp(lerp_uv_x, lerp_uv_x_1, v);

    float lerp_uv_x_z_1 = grad(tab[AA + 1], x, y, z - 1) * (1 - u) + grad(tab[BA + 1], x - 1, y, z - 1) * u;
    float lerp_uv_x_1_z_1 = grad(tab[AB + 1], x, y - 1, z - 1) * (1 - u) + grad(tab[BB + 1], x - 1, y - 1, z - 1) * u;
    float lerp_uvw_x_z_1 = lerp(lerp_uv_x_z_1, lerp_uv_x_1_z_1, v);

    float res = lerp(lerp_uvw_x, lerp_uvw_x_z_1, w);
    return (res + 1.0f) / 2.0f;
}

void perlinNoise(block_type***cubes, int decalx, int decaly, int* seed)
{
    sfClock* clock = sfClock_create();
    float baseFrequency = .2f;
    int octaves = 1;
    float persistence = 1.0f;
    float baseAmplitude = 1.0f;
    for (int i = 0; i < MAX_HEIGHT; i++)
    {
        for (int j = 0; j < CHUNK_SIZE; j++)
        {
            for (int k = 0; k < CHUNK_SIZE; k++)
            {
                float x = (float) i * baseFrequency;
                float y = (float) (j + decalx + 16) * baseFrequency;
                float z = (float) (k + decaly + 16) * baseFrequency;
                float noise = 0.0f;
                float perlin = 0.0f;
                for (int o = 0; o < octaves; o++)
                {
                    perlin += perlinNoise3D(x, y, z, seed) * baseAmplitude;
                    x *= 2.0f;
                    y *= 2.0f;
                    z *= 2.0f;
                    baseAmplitude *= persistence;
                }
                noise = perlin;
                if (noise > 0.5f)
                {
                    cubes[i][j][k] = 1;
                }
                else
                {
                    cubes[i][j][k] = 0;
                }
            }
        }
    }
}

int main()
{
    sfClock* clock = sfClock_create();
    int width = CHUNK_SIZE;
    int height = CHUNK_SIZE;

    int* seed = create_seed();
    block_type ***cubes = alloc_noise_3d();
    printf("time : %d\n", sfClock_getElapsedTime(clock).microseconds / 1000);
    perlinNoise(cubes, 0, 0 , seed);
    printf("time : %d\n", sfClock_getElapsedTime(clock).microseconds / 1000);

    sfVideoMode mode = { width, height, 32 };
    sfRenderWindow* window = sfRenderWindow_create(mode, "Perlin Noise", sfResize | sfClose, NULL);
    sfRenderWindow_setFramerateLimit(window, 30);
    if (!window)
        return EXIT_FAILURE;

    sfImage* image = sfImage_create(CHUNK_SIZE, CHUNK_SIZE);
    sfTexture* texture = sfTexture_create(CHUNK_SIZE, CHUNK_SIZE);
    sfSprite* sprite = sfSprite_create();
    int zeub = 0; int zboun = 0;

    int layer = 0;
    while (sfRenderWindow_isOpen(window))
    {
        layer = (layer + 1) % MAX_HEIGHT;
        for (int i = 0; i < CHUNK_SIZE; i++)
        {
            for (int j = 0; j < CHUNK_SIZE; j++)
            {
                sfColor color = sfBlack;
                
                if (cubes[layer][i][j] == 1)
                {
                    color = (sfColor) sfWhite;
                }
                sfImage_setPixel(image, i, j, color);
            }
                
        }
        sfTexture_updateFromImage(texture, image, 0, 0);
        sfSprite_setTexture(sprite, texture, sfTrue);
        sfRenderWindow_drawSprite(window, sprite, NULL);
        sfRenderWindow_display(window);
        sfSleep(sfMilliseconds(100));
    }

    free_noise_3d(cubes);

    sfRenderWindow_destroy(window);

    return EXIT_SUCCESS;
}
