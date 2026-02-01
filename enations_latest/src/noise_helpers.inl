// ----------------------------------------------------------------------------
// NOISE HELPERS for Badlands Generation
// ----------------------------------------------------------------------------

// Simple pseudo-random hash for 2D coordinates (returns 0.0 to 1.0)
static float Hash2D(int x, int y) {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    int t = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return (float)t / 1073741824.0f * 0.5f; // Normalized to 0..1
}

// Cubic smoothing for noise interpolation
static float SmoothStep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

// Basic Value Noise: Interpolates between random values on a grid
static float ValueNoise(float x, float y) {
    int ix = (int)floor(x);
    int iy = (int)floor(y);
    float fx = x - ix;
    float fy = y - iy;

    float v00 = Hash2D(ix, iy);
    float v10 = Hash2D(ix + 1, iy);
    float v01 = Hash2D(ix, iy + 1);
    float v11 = Hash2D(ix + 1, iy + 1);

    float sx = SmoothStep(fx);
    float sy = SmoothStep(fy);

    float val = (v00 * (1 - sx) + v10 * sx) * (1 - sy) + 
                (v01 * (1 - sx) + v11 * sx) * sy;
    
    return val * 2.0f - 1.0f; // Remap to -1.0..1.0
}

// RIDGED Noise: The secret sauce for badlands.
// Creates sharp peaks naturally by flipping negative values.
static float RidgedNoise(float x, float y) {
    float n = ValueNoise(x, y);
    return 1.0f - abs(n); // Sharp ridges at 1.0, valleys at 0.0
}
