#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_set>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// It's just the Java Random class
class Random
{
    uint64_t seed = 0;

    static uint64_t seedUniquifier;

    constexpr static uint64_t multiplier = 0x5DEECE66D;
    constexpr static uint64_t addend = 0xBL;
    constexpr static uint64_t mask = (uint64_t(1) << 48) - 1;

    static uint64_t initialScramble(const uint64_t seed)
	{
        return (seed ^ multiplier) & mask;
    }

	static uint64_t uniqueSeed()
    {
        // L'Ecuyer, "Tables of Linear Congruential Generators of
        // Different Sizes and Good Lattice Structure", 1999
        for (;;) {
	        const uint64_t current = seedUniquifier;
            const uint64_t next = current * 181783497276652981L;
            if (seedUniquifier == current)
            {
                seedUniquifier = next;
                return next;
            }
        }
    }

    int next(const int bits) const
    {
        uint64_t nextseed;
        const uint64_t seed1 = seed;
        do {
            const uint64_t oldseed = seed1;
            nextseed = (oldseed * multiplier + addend) & mask;
            seedUniquifier = nextseed;
        } while (seedUniquifier != nextseed);
        return int(nextseed >> (48 - bits));
    }
	
public:
    Random(const long seed) : seed(uniqueSeed()) {}

	Random() : seed(uniqueSeed() ^ uint64_t(glfwGetTime())) {}

	float nextFloat() const
	{
        return next(24) / float(1 << 24);
    }

	int nextInt() const
	{
        return next(32);
    }

	int nextInt(const int bound) const
    {
        int r = next(31);
        const int m = bound - 1;
        if ((bound & m) == 0)  // i.e., bound is a power of 2
            r = int(bound * uint64_t(r) >> 31);
        else {
            for (int u = r;
                u - (r = u % bound) + m < 0;
                u = next(31));
        }
        return r;
    }

	void setSeed(const uint64_t newSeed)
    {
        seed = initialScramble(newSeed);
    }
};

uint64_t Random::seedUniquifier = 8682522807148012;

constexpr bool classic = false;

std::unordered_set<int> input = std::unordered_set<int>();

glm::vec2 mouseDelta;

volatile bool needsResUpdate = true;

constexpr float PI = 3.14159265359f;

constexpr int MOUSE_RIGHT = 0;
constexpr int MOUSE_LEFT = 1;

int SCR_DETAIL = 1;

int SCR_RES_X = 107 * pow(2, SCR_DETAIL);
int SCR_RES_Y = 60 * pow(2, SCR_DETAIL);

constexpr float RENDER_DIST = classic ? 20.0f : 80.0f;
constexpr float PLAYER_REACH = 5.0f;

constexpr int WINDOW_WIDTH = 856;
constexpr int WINDOW_HEIGHT = 480;

constexpr int TEXTURE_RES = 16;

constexpr int WORLD_SIZE = 64;
constexpr int WORLD_HEIGHT = 64;

constexpr int AXIS_X = 0;
constexpr int AXIS_Y = 1;
constexpr int AXIS_Z = 2;

constexpr unsigned char BLOCK_AIR = 0;
constexpr unsigned char BLOCK_GRASS = 1;
constexpr unsigned char BLOCK_DEFAULT_DIRT = 2;
constexpr unsigned char BLOCK_STONE = 4;
constexpr unsigned char BLOCK_BRICKS = 5;
constexpr unsigned char BLOCK_WOOD = 7;
constexpr unsigned char BLOCK_LEAVES = 8;

constexpr int PERLIN_RES = 1024;

constexpr int CROSS_SIZE = 32;

// COLORS
constexpr glm::vec3 FOG_COLOR = glm::vec3(1);

// S = Sun, A = Amb, Y = skY
constexpr glm::vec3 SC_DAY = glm::vec3(1);
constexpr glm::vec3 AC_DAY = glm::vec3(0.5f, 0.5f, 0.5f);
constexpr glm::vec3 YC_DAY = glm::vec3(0x51BAF7);

constexpr glm::vec3 SC_TWILIGHT = glm::vec3(1, 0.5f, 0.01f);
constexpr glm::vec3 AC_TWILIGHT = glm::vec3(0.6f, 0.5f, 0.5f);
constexpr glm::vec3 YC_TWILIGHT = glm::vec3(0.27f, 0.24f, 0.33f);

constexpr glm::vec3 SC_NIGHT = glm::vec3(0.3f, 0.3f, 0.5f);
constexpr glm::vec3 AC_NIGHT = glm::vec3(0.3f, 0.3f, 0.5f);
constexpr glm::vec3 YC_NIGHT = glm::vec3(0.004f, 0.004f, 0.008f);

long deltaTime = 0;

constexpr float PERLIN_OCTAVES = 4; // default to medium smooth
constexpr float PERLIN_AMP_FALLOFF = 0.5f; // 50% reduction/octave

constexpr int PERLIN_YWRAPB = 4;
constexpr int PERLIN_YWRAP = 1 << PERLIN_YWRAPB;
constexpr int PERLIN_ZWRAPB = 8;
constexpr int PERLIN_ZWRAP = 1 << PERLIN_ZWRAPB;

float perlin[PERLIN_RES + 1];

float scaled_cosine(const float i) {
    return 0.5f * (1.0f - cos(i * PI));
}

float noise(float x, float y) { // stolen from Processing
    if (perlin[0] == 0) {
        Random r = Random(18295169L);
    	
        for (int i = 0; i < PERLIN_RES + 1; i++)
            perlin[i] = r.nextFloat();
    }

    if (x < 0)
        x = -x;
    if (y < 0)
        y = -y;

    int xi = int(x);
    int yi = int(y);

    float xf = x - xi;
    float yf = y - yi;

    float r = 0;
    float ampl = 0.5f;

    for (int i = 0; i < PERLIN_OCTAVES; i++) {
        int of = xi + (yi << PERLIN_YWRAPB);

        float rxf = scaled_cosine(xf);
        float ryf = scaled_cosine(yf);

        float n1 = perlin[of % PERLIN_RES];
        n1 += rxf * (perlin[(of + 1) % PERLIN_RES] - n1);
        float n2 = perlin[(of + PERLIN_YWRAP) % PERLIN_RES];
        n2 += rxf * (perlin[(of + PERLIN_YWRAP + 1) % PERLIN_RES] - n2);
        n1 += ryf * (n2 - n1);

        of += PERLIN_ZWRAP;
        n2 = perlin[of % PERLIN_RES];
        n2 += rxf * (perlin[(of + 1) % PERLIN_RES] - n2);
        float n3 = perlin[(of + PERLIN_YWRAP) % PERLIN_RES];
        n3 += rxf * (perlin[(of + PERLIN_YWRAP + 1) % PERLIN_RES] - n3);
        n2 += ryf * (n3 - n2);

        n1 += scaled_cosine(0) * (n2 - n1);

        r += n1 * ampl;
        ampl *= PERLIN_AMP_FALLOFF;
        xi <<= 1;
        xf *= 2;
        yi <<= 1;
        yf *= 2;

        if (xf >= 1.0) {
            xi++;
            xf--;
        }

        if (yf >= 1.0) {
            yi++;
            yf--;
        }
    }

    return r;
}

glm::vec3 playerPos = glm::vec3(WORLD_SIZE + WORLD_SIZE / 2.0f + 0.5f, 
							    WORLD_HEIGHT + 1, 
							    WORLD_SIZE + WORLD_SIZE / 2.0f + 0.5f);

glm::vec3 velocity(0);

// mouse movement stuff
volatile bool hovered = false;

volatile int hoveredBlockPosX = -1;
volatile int hoveredBlockPosY = -1;
volatile int hoveredBlockPosZ = -1;

volatile int placeBlockPosX = -1;
volatile int placeBlockPosY = -1;
volatile int placeBlockPosZ = -1;

volatile int newHoverBlockPosX = -1;
volatile int newHoverBlockPosY = -1;
volatile int newHoverBlockPosZ = -1;

glm::vec3 lightDirection = glm::vec3(0.866025404f, -0.866025404f, 0.866025404f);

float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
float FOV = 90.0f;

float sinYaw, sinPitch;
float cosYaw, cosPitch;

glm::vec3 sunColor = glm::vec3();
glm::vec3 ambColor = glm::vec3();
glm::vec3 skyColor = glm::vec3();

unsigned char world[WORLD_SIZE][WORLD_HEIGHT][WORLD_SIZE];

unsigned char hotbar[] { BLOCK_GRASS, BLOCK_DEFAULT_DIRT, BLOCK_STONE, BLOCK_BRICKS, BLOCK_WOOD, BLOCK_LEAVES };
int heldBlockIndex = 0;

static void fillBox(const unsigned char blockId, const glm::vec3& pos0,
    const glm::vec3& pos1, const bool replace)
{
    for (int x = pos0.x; x < pos1.x; x++)
    {
        for (int y = pos0.y; y < pos1.y; y++)
        {
            for (int z = pos0.z; z < pos1.z; z++)
            {
                if (!replace) {
                    if (world[x][y][z] != BLOCK_AIR)
                        continue;
                }

                world[x][y][z] = blockId;
            }
        }
    }
}

static glm::vec3 lerp(const glm::vec3& start, const glm::vec3& end, const float t)
{
    return glm::vec3(start + (start - end) * t);
}

void updateScreenResolution()
{
    SCR_RES_X = 107 * pow(2, SCR_DETAIL);
    SCR_RES_Y = 60 * pow(2, SCR_DETAIL);

    // auto generated code - do not delete
    std::string title = "Minecraft4k";

    switch (SCR_RES_X) {
    case 6:
        title += " on battery-saving mode";
        break;
    case 13:
        title += " on a potato";
        break;
    case 26:
        title += " on an undocked switch";
        break;
    case 53:
        title += " on a TI-84";
        break;
    case 107:
        title += " on an Atari 2600";
        break;
    case 428:
        title += " at SD";
        break;
    case 856:
        title += " at HD";
        break;
    case 1712:
        title += " at Full HD";
        break;
    case 3424:
        title += " at 4K";
        break;
    case 6848:
        title += " on a NASA supercomputer";
        break;
    }

    //frame.setTitle(title);
}

void run(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window))
    {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return;
	
    Random rand = Random(18295169L);

    // generate world

    float maxTerrainHeight = WORLD_HEIGHT / 2.0f;
    if (classic) {
        for (int x = WORLD_SIZE; x >= 0; x--) {
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                for (int z = 0; z < WORLD_SIZE; z++) {
                    unsigned char block;

                    if (y > maxTerrainHeight + rand.nextInt(8))
                        block = rand.nextInt(8) + 1;
                    else
                        block = BLOCK_AIR;

                    if (x == WORLD_SIZE)
                        continue;

                    world[x][y][z] = block;
                }
            }
        }
    }
    else {
        float halfWorldSize = WORLD_SIZE / 2.0f;

        constexpr int stoneDepth = 5;

        for (int x = 0; x < WORLD_SIZE; x++) {
            for (int z = 0; z < WORLD_SIZE; z++) {
                int terrainHeight = round(maxTerrainHeight + noise(x / halfWorldSize, z / halfWorldSize) * 10.0f);

                for (int y = terrainHeight; y < WORLD_HEIGHT; y++)
                {
                    unsigned char block;

                    if (y > terrainHeight + stoneDepth)
                        block = BLOCK_STONE;
                    else if (y > terrainHeight)
                        block = 2; // dirt
                    else // (y == terrainHeight)
                        block = BLOCK_GRASS;

                    world[x][y][z] = block;
                }
            }
        }

        // populate trees
        for (int x = 4; x < WORLD_SIZE - 4; x += 8) {
            for (int z = 4; z < WORLD_SIZE - 4; z += 8) {
                if (rand.nextInt(4) == 0) // spawn tree
                {
                    int treeX = x + (rand.nextInt(4) - 2);
                    int treeZ = z + (rand.nextInt(4) - 2);

                    int terrainHeight = round(maxTerrainHeight + noise(treeX / halfWorldSize, treeZ / halfWorldSize) * 10.0f) - 1;

                    int treeHeight = 4 + rand.nextInt(2); // min 4 max 5

                    for (int y = terrainHeight; y >= terrainHeight - treeHeight; y--)
                    {
                        unsigned char block = BLOCK_WOOD;

                        world[treeX][y][treeZ] = block;
                    }

                    // foliage
                    fillBox(BLOCK_LEAVES, 
                        glm::vec3(treeX - 2, terrainHeight - treeHeight + 1, treeZ - 2),
                        glm::vec3(treeX + 3, terrainHeight - treeHeight + 3, treeZ + 3), false);

                    // crown
                    fillBox(BLOCK_LEAVES,
                        glm::vec3(treeX - 1, terrainHeight - treeHeight - 1, treeZ - 1),
                        glm::vec3(treeX + 2, terrainHeight - treeHeight + 1, treeZ + 2), false);


                    int foliageXList[] = { treeX - 2, treeX - 2, treeX + 2, treeX + 2 };
                    int foliageZList[] = { treeZ - 2, treeZ + 2, treeZ + 2, treeZ - 2 };

                    int crownXList[] = { treeX - 1, treeX - 1, treeX + 1, treeX + 1 };
                    int crownZList[] = { treeZ - 1, treeZ + 1, treeZ + 1, treeZ - 1 };

                    for (int i = 0; i < 4; i++)
                    {
                        int foliageX = foliageXList[i];
                        int foliageZ = foliageZList[i];

                        int foliageCut = rand.nextInt(10);

                        switch (foliageCut) {
                        case 0: // cut out top
                            world[foliageX][terrainHeight - treeHeight + 1][foliageZ] = BLOCK_AIR;
                            break;
                        case 1: // cut out bottom
                            world[foliageX][terrainHeight - treeHeight + 2][foliageZ] = BLOCK_AIR;
                            break;
                        case 2: // cut out both
                            world[foliageX][terrainHeight - treeHeight + 1][foliageZ] = BLOCK_AIR;
                            world[foliageX][terrainHeight - treeHeight + 2][foliageZ] = BLOCK_AIR;
                            break;
                        default: // do nothing
                            break;
                        }


                        int crownX = crownXList[i];
                        int crownZ = crownZList[i];

                        int crownCut = rand.nextInt(10);

                        switch (crownCut) {
                        case 0: // cut out both
                            world[crownX][terrainHeight - treeHeight - 1][crownZ] = BLOCK_AIR;
                            world[crownX][terrainHeight - treeHeight][crownZ] = BLOCK_AIR;
                            break;
                        default: // do nothing
                            world[crownX][terrainHeight - treeHeight - 1][crownZ] = BLOCK_AIR;
                            break;
                        }
                    }
                }
            }
        }
    }

    // set random seed to generate textures
    rand.setSeed(151910774187927L);

    // procedurally generates the 16x3 textureAtlas
    // gsd = grayscale detail
    for (int blockType = 1; blockType < 16; blockType++) {
        int gsd_tempA = 0xFF - rand.nextInt(0x60);

        for (int y = 0; y < TEXTURE_RES * 3; y++) {
            for (int x = 0; x < TEXTURE_RES; x++) {
                // gets executed per pixel/texel

                int gsd_constexpr;
                int tint;

                if (classic) {
                    if (blockType != BLOCK_STONE || rand.nextInt(3) == 0) // if the block type is stone, update the noise value less often to get a stretched out look
                        gsd_tempA = 0xFF - rand.nextInt(0x60);

                    tint = 0x966C4A; // brown (dirt)
                    switch (blockType)
                    {
                    case BLOCK_STONE:
                        tint = 0x7F7F7F; // grey
                        break;
                    case BLOCK_GRASS:
                        if (y < (x * x * 3 + x * 81 >> 2 & 0x3) + (TEXTURE_RES * 1.125f)) // grass + grass edge
                            tint = 0x6AAA40; // green
                        else if (y < (x * x * 3 + x * 81 >> 2 & 0x3) + (TEXTURE_RES * 1.1875f)) // grass edge shadow
                            gsd_tempA = gsd_tempA * 2 / 3;
                        break;
                    case BLOCK_WOOD:
                        tint = 0x675231; // brown (bark)
                        if (!(y >= TEXTURE_RES && y < TEXTURE_RES * 2) && // second row = stripes
                            x > 0 && x < TEXTURE_RES - 1 &&
                            ((y > 0 && y < TEXTURE_RES - 1) || (y > TEXTURE_RES * 2 && y < TEXTURE_RES * 3 - 1))) { // wood side area
                            tint = 0xBC9862; // light brown

                            // the following code repurposes 2 gsd variables making it a bit hard to read
                            // but in short it gets the absolute distance from the tile's center in x and y direction 
                            // finds the max of it
                            // uses that to make the gray scale detail darker if the current pixel is part of an annual ring
                            // and adds some noise as a finishing touch
                            int woodCenter = TEXTURE_RES / 2 - 1;

                            int dx = x - woodCenter;
                            int dy = (y % TEXTURE_RES) - woodCenter;

                            if (dx < 0)
                                dx = 1 - dx;

                            if (dy < 0)
                                dy = 1 - dy;

                            if (dy > dx)
                                dx = dy;

                            gsd_tempA = 196 - rand.nextInt(32) + dx % 3 * 32;
                        }
                        else if (rand.nextInt(2) == 0) {
                            // make the gsd 50% brighter on random pixels of the bark
                            // and 50% darker if x happens to be odd
                            gsd_tempA = gsd_tempA * (150 - (x & 1) * 100) / 100;
                        }
                        break;
                    case BLOCK_BRICKS:
                        tint = 0xB53A15; // red
                        if ((x + y / 4 * 4) % 8 == 0 || y % 4 == 0) // gap between bricks
                            tint = 0xBCAFA5; // reddish light grey
                        break;
                    }

                    gsd_constexpr = gsd_tempA;
                    if (y >= TEXTURE_RES * 2) // bottom side of the block
                        gsd_constexpr /= 2; // make it darker, baked "shading"

                    if (blockType == BLOCK_LEAVES) {
                        tint = 0x50D937; // green
                        if (rand.nextInt(2) == 0) {
                            tint = 0;
                            gsd_constexpr = 0xFF;
                        }
                    }
                }
                else {
                    float pNoise = noise(x, y);

                    tint = 0x966C4A; // brown (dirt)

                    gsd_tempA = (1 - pNoise * 0.5f) * 255;
                    switch (blockType) {
                    case BLOCK_STONE:
                        tint = 0x7F7F7F; // grey
                        gsd_tempA = (0.75 + round(abs(noise(x * 0.5f, y * 2))) * 0.125f) * 255;
                        break;
                    case BLOCK_GRASS:
                        if (y < (((x * x * 3 + x * 81) / 2) % 4) + 18) // grass + grass edge
                            tint = 0x7AFF40; //green
                        else if (y < (((x * x * 3 + x * 81) / 2) % 4) + 19)
                            gsd_tempA = gsd_tempA * 1 / 3;
                        break;
                    case BLOCK_WOOD:
                        tint = 0x776644; // brown (bark)

                        int woodCenter = TEXTURE_RES / 2 - 1;
                        int dx = x - woodCenter;
                        int dy = (y % TEXTURE_RES) - woodCenter;

                        if (dx < 0)
                            dx = 1 - dx;

                        if (dy < 0)
                            dy = 1 - dy;

                        if (dy > dx)
                            dx = dy;

                        double distFromCenter = (sqrt(dx * dx + dy * dy) * .25f + std::max(dx, dy) * .75f);

                        if (y < 16 || y > 32) { // top/bottom
                            if (distFromCenter < float(TEXTURE_RES) / 2.0f)
                            {
                                tint = 0xCCAA77; // light brown

                                gsd_tempA = 196 - rand.nextInt(32) + dx % 3 * 32;
                            }
                            else if (dx > dy) {
                                gsd_tempA = noise(y, x * .25f) * 255 * (180 - sin(x * PI) * 50) / 100;
                            }
                            else {
                                gsd_tempA = noise(x, y * .25f) * 255 * (180 - sin(x * PI) * 50) / 100;
                            }
                        }
                        else { // side texture
                            gsd_tempA = noise(x, y * .25f) * 255 * (180 - sin(x * PI) * 50) / 100;
                        }
                        break;
                    case BLOCK_BRICKS:
                        tint = 0x444444; // red

                        float brickDX = abs(x % 8 - 4);
                        float brickDY = abs((y % 4) - 2) * 2;

                        if ((y / 4) % 2 == 1)
                            brickDX = abs((x + 4) % 8 - 4);

                        float d = sqrt(brickDX * brickDX + brickDY * brickDY) * .5f
                    				+ std::max(brickDX, brickDY) * .5f;

                        if (d > 4) // gap between bricks
                            tint = 0xAAAAAA; // light grey
                        break;
                    }

                    gsd_constexpr = gsd_tempA;

                    if (blockType == BLOCK_LEAVES)
                    {
                        tint = 0;

                        float dx = abs(x % 4 - 2) * 2;
                        float dy = (y % 8) - 4;

                        if ((y / 8) % 2 == 1)
                            dx = abs((x + 2) % 4 - 2) * 2;

                        dx += pNoise;

                        float d = dx + abs(dy);

                        if (dy < 0)
                            d = sqrt(dx * dx + dy * dy);

                        if (d < 3.5f)
                            tint = 0xFFCCDD;
                        else if (d < 4)
                            tint = 0xCCAABB;
                    }
                }

                // multiply tint by the grayscale detail
                int col = ((tint & 0xFFFFFF) == 0 ? 0 : 0xFF) << 24 |
                    (tint >> 16 & 0xFF) * gsd_constexpr / 0xFF << 16 |
                    (tint >> 8 & 0xFF) * gsd_constexpr / 0xFF << 8 |
                    (tint & 0xFF) * gsd_constexpr / 0xFF;

                // write pixel to the texture atlas
                textureAtlas[x + y * TEXTURE_RES + blockType * (TEXTURE_RES * TEXTURE_RES) * 3] = col;
            }
        }
    }

    long startTime = glfwGetTime();


    while (true) {
        long time = glfwGetTime();

        if (needsResUpdate) {
            needsResUpdate = false;
            updateScreenResolution();
        }

        if (input.contains(KeyEvent.VK_Q))
        {
	        std::cout << "DEBUG::BREAK\n";
        }

        sinYaw = sin(cameraYaw);
        cosYaw = cos(cameraYaw);
        sinPitch = sin(cameraPitch);
        cosPitch = cos(cameraPitch);

        lightDirection.y = sin(time / 10000.0);

        lightDirection.x = 0; //lightDirection.y * 0.5f;
        lightDirection.z = cos(time / 10000.0);


        if (lightDirection.y < 0.0f)
        {
            sunColor = lerp(SC_TWILIGHT, SC_DAY, -lightDirection.y);
            ambColor = lerp(AC_TWILIGHT, AC_DAY, -lightDirection.y);
            skyColor = lerp(YC_TWILIGHT, YC_DAY, -lightDirection.y);
        }
        else {
            sunColor = lerp(SC_TWILIGHT, SC_NIGHT, lightDirection.y);
            ambColor = lerp(AC_TWILIGHT, AC_NIGHT, lightDirection.y);
            skyColor = lerp(YC_TWILIGHT, YC_NIGHT, lightDirection.y);
        }

        while (glfwGetTime() - startTime > 10L) {
            // adjust camera
            cameraYaw += mouseDelta.x / 400.0F;
            cameraPitch -= mouseDelta.y / 400.0F;

            if (cameraPitch < -1.57F)
                cameraPitch = -1.57F;

            if (cameraPitch > 1.57F)
                cameraPitch = 1.57F;


            startTime += 10L;
            float inputX = (input.contains(KeyEvent.VK_D) - input.contains(KeyEvent.VK_A)) * 0.02F;
            float inputZ = (input.contains(KeyEvent.VK_W) - input.contains(KeyEvent.VK_S)) * 0.02F;
        	
            velocity.x *= 0.5F;
            velocity.y *= 0.99F;
            velocity.z *= 0.5F;
        	
            velocity.x += sinYaw * inputZ + cosYaw * inputX;
            velocity.z += cosYaw * inputZ - sinYaw * inputX;
            velocity.y += 0.003F; // gravity


            //check for movement on each axis individually
			OUTER:
            for (int axisIndex = 0; axisIndex < 3; axisIndex++) {
                float newPlayerX = playerPos.x + velocity.x * ((axisIndex + AXIS_Y) % 3 / 2);
                float newPlayerY = playerPos.y + velocity.y * ((axisIndex + AXIS_X) % 3 / 2);
                float newPlayerZ = playerPos.z + velocity.z * ((axisIndex + AXIS_Z) % 3 / 2);

                for (int colliderIndex = 0; colliderIndex < 12; colliderIndex++) {
                    // magic
                    int colliderBlockX = (newPlayerX + (colliderIndex & 1) * 0.6F - 0.3F) - WORLD_SIZE;
                    int colliderBlockY = (newPlayerY + ((colliderIndex >> 2) - 1) * 0.8F + 0.65F) - WORLD_HEIGHT;
                    int colliderBlockZ = (newPlayerZ + (colliderIndex >> 1 & 1) * 0.6F - 0.3F) - WORLD_SIZE;

                    if (colliderBlockY < 0)
                        continue;

                    // check collision with world bounds and world blocks
                    if (colliderBlockX < 0 || colliderBlockZ < 0
                        || colliderBlockX >= WORLD_SIZE || colliderBlockY >= WORLD_HEIGHT || colliderBlockZ >= WORLD_SIZE
                        || world[colliderBlockX][colliderBlockY][colliderBlockZ] != BLOCK_AIR) {

                        if (axisIndex != AXIS_Z) // not checking for vertical movement
                            continue OUTER; // movement is invalid

                        // if we're falling, colliding, and we press space
                        if (input.contains(KeyEvent.VK_SPACE) == true && velocity.y > 0.0F) {
                            velocity.y = -0.1F; // jump
                            break OUTER;
                        }

                        // stop vertical movement
                        velocity.y = 0.0F;
                        break OUTER;
                    }
                }

                playerPos.x = newPlayerX;
                playerPos.y = newPlayerY;
                playerPos.z = newPlayerZ;
            }
        }

        if (hoveredBlockPosX > -1) { // all axes will be -1 if nothing hovered
            // break block
            if (input.contains(MOUSE_LEFT) == true) {
                world[hoveredBlockPosX][hoveredBlockPosY][hoveredBlockPosZ] = BLOCK_AIR;
                input.remove(MOUSE_LEFT);
            }


            if (placeBlockPosY > 0) {
                // place block
                if (input.contains(MOUSE_RIGHT)) {
                    world[placeBlockPosX][placeBlockPosY][placeBlockPosZ] = hotbar[heldBlockIndex];
                    input.remove(MOUSE_RIGHT);
                }
            }
        }

        for (int colliderIndex = 0; colliderIndex < 12; colliderIndex++) {
            int magicX = (int)(playerPos.x + (colliderIndex & 1) * 0.6F - 0.3F) - WORLD_SIZE;
            int magicY = (int)(playerPos.y + ((colliderIndex >> 2) - 1) * 0.8F + 0.65F) - WORLD_HEIGHT;
            int magicZ = (int)(playerPos.z + (colliderIndex >> 1 & 1) * 0.6F - 0.3F) - WORLD_SIZE;

            // check if hovered block is within world boundaries
            if (magicX >= 0 && magicY >= 0 && magicZ >= 0 && magicX < WORLD_SIZE && magicY < WORLD_HEIGHT && magicZ < WORLD_SIZE)
                world[magicX][magicY][magicZ] = BLOCK_AIR;
        }

        // render the screen
        newHoverBlockPosX = -1;
        newHoverBlockPosY = -1;
        newHoverBlockPosZ = -1;

    	// RENDER AAA

        hoveredBlockPosX = newHoverBlockPosX;
        hoveredBlockPosY = newHoverBlockPosY;
        hoveredBlockPosZ = newHoverBlockPosZ;

        placeBlockPosX += hoveredBlockPosX;
        placeBlockPosY += hoveredBlockPosY;
        placeBlockPosZ += hoveredBlockPosZ;

        deltaTime = glfwGetTime() - time;

    	
        if (deltaTime < 16)
            std::this_thread::yield();
    }
}

int main(int argc, char** argv)
{
    if (!glfwInit())
    {
        // Initialization failed
        std::cout << "Failed to init GLFW!\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Minecraft4k", nullptr, nullptr);
    if (!window)
    {
        // Window or OpenGL context creation failed
        std::cout << "Failed to create window!\n";
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cout << "Failed to initialize GLAD!\n" << std::endl;
        return -1;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    run(window);
}