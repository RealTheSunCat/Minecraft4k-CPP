#include "Util.h"


#include <chrono>
#include <complex>
#include <iostream>

long long currentTime()
{
    return std::chrono::system_clock::now().time_since_epoch().count();
}

uint64_t Random::seedUniquifier = 8682522807148012;
Random::Random() : seed(uniqueSeed() ^ uint64_t(currentTime())) {}
Random::Random(const long seed) : seed(initialScramble(seed)) {}

uint64_t Random::initialScramble(const uint64_t seed)
{
    return (seed ^ multiplier) & mask;
}

uint64_t Random::uniqueSeed()
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

int Random::next(const int bits)
{
    seed = (seed * multiplier + addend) & mask;
	
    return int(seed >> (48 - bits));
}

float Random::nextFloat()
{
    return next(24) / float(1 << 24);
}

int Random::nextInt()
{
    return next(32);
}

int Random::nextInt(const int bound)
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

void Random::setSeed(const uint64_t newSeed)
{
    seed = initialScramble(newSeed);
}

// Perlin noise

float scaled_cosine(const float i) {
    return 0.5f * (1.0f - std::cos(i * PI));
}

constexpr int PERLIN_RES = 1024;

constexpr float PERLIN_OCTAVES = 4; // default to medium smooth
constexpr float PERLIN_AMP_FALLOFF = 0.5f; // 50% reduction/octave

constexpr int PERLIN_YWRAPB = 4;
constexpr int PERLIN_YWRAP = 1 << PERLIN_YWRAPB;
constexpr int PERLIN_ZWRAPB = 8;
constexpr int PERLIN_ZWRAP = 1 << PERLIN_ZWRAPB;

float perlin[PERLIN_RES + 1];

float Perlin::noise(float x, float y) { // stolen from Processing
    if (perlin[0] == 0) {
	    Random r = Random(18295169L);

        for (float& i : perlin)
	        i = r.nextFloat();
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

        const float rxf = scaled_cosine(xf);
        const float ryf = scaled_cosine(yf);

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

float clamp(float val, const float min, const float max)
{
    if (min >= max)
    {
	    std::cout << "Min (" << min << ") is not less than max (" << max << ")!" << std::endl;
        return val;
	}
	
    if (val < min)
        val = min;
    else if (val > max)
        val = max;

    return val;
}

bool glError()
{
    const GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error " << err << std::endl;
        return true;
    }

    return false;
}

void GLAPIENTRY error_callback(GLenum source,
			    const GLenum type,
			    GLuint id,
                const GLenum severity,
			    GLsizei length,
			    const GLchar* message,
				const void* userParam) {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);
}

std::ostream& operator<<(std::ostream& os, const glm::vec3& vec3)
{
    os << vec3.x << ", " << vec3.y << ", " << vec3.z;
    return os;
}