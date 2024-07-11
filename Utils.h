#ifndef UTILS_H
#define UTILS_H

// this does not seem to work for small values like we use in the envelope
inline float fastPow(float a, float b)
{
    union {
        float    f;
        uint32_t u;
    } temp;
    temp.f = a;
    temp.u = (uint32_t)(b * (float)(temp.u - 1064866808) + 1064866808);
    return temp.f;
}

static inline float fast_exp2f(float x)
{
	float i;
	float f = modff(x, &i);
	f *= 0.693147f / 256.0f;
	f += 1.0f;
	f *= f;
	f *= f;
	f *= f;
	f *= f;
	f *= f;
	f *= f;
	f *= f;
	f *= f;
	f = ldexpf(f, i);
	return f;
}

static inline float fast_tanh(float x)
{
	if (x > 3.0f) return 1.0f;
	if (x < -3.0f) return -1.0f;
	float x2 = x * x;
	return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}



#endif