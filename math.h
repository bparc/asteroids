#pragma once

typedef struct
{
	float x;
	float y;
} v2_t;

static v2_t V2(float x, float y)
{
	v2_t result = { x, y };
	return result;
}

typedef union
{
	struct
	{
		float x;
		float y;
		float z;
		float w;
	};
	struct
	{
		float r;
		float g;
		float b;
		float a;
	};
	struct
	{
		v2_t xy;
		v2_t zw;
	};
} v4_t;

static v4_t V4(float x, float y, float z, float w)
{
	v4_t result = { x,y,z,w };
	return result;
}

static v4_t RGBU8(uint8_t R, uint8_t G, uint8_t B)
{
	v4_t result =
	{
		(float)R / 255.0f,
		(float)G / 255.0f,
		(float)B / 255.0f,
		1.0f,
	};
	return result;
}

static int32_t TruncateFloat(float X)
{
	int32_t result = (int32_t)floorf(X);
	return result;
}

static float RandomFloat(void)
{
	float result = ((float)rand() / (float)RAND_MAX);
	return result;
}

static float Radians(float degrees)
{
	float result = (float)((double)degrees * M_PI / 180.0);
	return result;
}

static float Sin(float degrees)
{
	float result = sinf(Radians(degrees));
	return result;
}

static float Cos(float degrees)
{
	float result = cosf(Radians(degrees));
	return result;
}

static float Lerp1(float A, float B, float t)
{
	float result = A;
	if (t > 0.0f && t < 1.0f)
	{
		result = (A * (1.0f - t) + B * t);
	}
	return result;
}

static v2_t Lerp2(v2_t A, v2_t B,float t)
{
	v2_t result = { Lerp1(A.x,B.x,t),
		Lerp1(A.y,B.y,t) };
	return result;
}

static v4_t Lerp4(v4_t A, v4_t B, float t)
{
	v4_t result;
	result.xy = Lerp2(A.xy, B.xy, t);
	result.zw = Lerp2(A.zw, B.zw, t);
	return result;
}

static float Clamp1(float X, float min, float max)
{
	float result = X;
	if (X < min)
	{
		X = min;
	}
	if (X > max)
	{
		X = max;
	}
	return result;
}

static float Dot(v2_t A, v2_t B)
{
	float result = (A.x * B.x + A.y * B.y);
	return result;
}

static float Dot1(v2_t A)
{
	float result = Dot(A, A);
	return result;
}

static float DistanceSquared(v2_t A, v2_t B)
{
	float result = Dot1(V2(B.x - A.x, B.y - A.y));
	return result;
}

static float Distance(v2_t A, v2_t B)
{
	float result = sqrtf(DistanceSquared(A, B));
	return result;
}

static float Blink(double T)
{
	float result = (fmodf(sinf((float)(T * 2.0f)), 1.0f) + 1.0f) / 2.0f;
	return result;
}