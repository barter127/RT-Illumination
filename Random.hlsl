static uint state = 27689287542;

// I "stole" these functions for Sebastian Lague and he stole these functions from stack overflow.

float RandomValue(inout uint state)
{
    state = state * 747796405 + 2891336453;
    uint result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
    result = (result >> 22) ^ result;
    return result / 4294967295.0f;
}

float RandomValueNormalDistribution(inout uint state)
{
    float theta = 2 * 3.1415926;
    float rho = sqrt(-2 * log(RandomValue(state)));
    return rho * cos(theta);
}

float3 RandomDirection(inout uint state)
{
    float x = RandomValueNormalDistribution(state);
    float y = RandomValueNormalDistribution(state);
    float z = RandomValueNormalDistribution(state);
    
    return normalize(float3(x, y, z));
}