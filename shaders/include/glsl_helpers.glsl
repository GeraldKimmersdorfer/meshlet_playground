

ivec2 spiral(uint n) {
    if (n == 0) return ivec2(0, 0);

    int r = int(floor((sqrt(float(n)) - 1.0) / 2.0)) + 1;
    int p = 8 * r * (r - 1) / 2;
    int en = r * 2;
    int a = int(n - uint(p)) % (r * 8);

    switch (a / (r * 2)) {
    case 0: return ivec2(a - r, -r);
    case 1: return ivec2(r, (a % en) - r);
    case 2: return ivec2(r - (a % en), r);
    case 3: return ivec2(-r, r - (a % en));
    }

    return ivec2(0, 0);
}

// Returns vector a but with the components sorted from high to low
vec4 sortVec4HighLow(vec4 a) {
    float temp;
    
    if (a.x < a.y) { temp = a.x; a.x = a.y; a.y = temp; }
    if (a.y < a.z) { temp = a.y; a.y = a.z; a.z = temp; }
    if (a.z < a.w) { temp = a.z; a.z = a.w; a.w = temp; }
    if (a.x < a.y) { temp = a.x; a.x = a.y; a.y = temp; }
    if (a.y < a.z) { temp = a.y; a.y = a.z; a.z = temp; }
    if (a.x < a.y) { temp = a.x; a.x = a.y; a.y = temp; }
    
    return a;
}

uint compute_hash(uint a)
{
   uint b = (a+2127912214u) + (a<<12u); b = (b^3345072700u) ^ (b>>19u); b = (b+374761393u) + (b<<5u); 
   b = (b+3551683692u) ^ (b<<9u); b = (b+4251993797u) + (b<<3u); b = (b^3042660105u) ^ (b>>16u);
   return b;
}

vec3 color_from_id_hash(uint a) {
    uint hash = compute_hash(a);
	return vec3(float(hash & 255u), float((hash >> 8u) & 255u), float((hash >> 16u) & 255u)) / 255.0;
}

// Cool ressource for a lot of different blend modes: https://github.com/jamieowen/glsl-blend/tree/master
vec3 color_from_id_hash(uint a, vec3 tint) { 
	// individual colors
	//return vec3(a & 255u, (a >> 8u) & 255u, (a >> 16u) & 255u) / 255.0;// * tint;
	return (color_from_id_hash(a + uint(dot(tint, vec3(1.0)) * 255)) * tint); 

}

vec3 uint32_log_scale_color(uint value) {
    // Ensure the value is within the expected range
	float fvalue = float(value);
    fvalue = clamp(fvalue, 0.0, float(4294967296.0)); // 2^32

    // Convert value to log scale
    float logValue = log2(fvalue + 1.0); // Add 1 to avoid log(0)

    // Normalize logValue to the range [0, 1]
    float normalizedLogValue = logValue / 32.0; // log2(2^32) = 32

    // Map normalized log value to color
    // For example, using a gradient from blue to red
    vec3 startColor = vec3(0.0, 0.0, 1.0); // Blue
    vec3 endColor = vec3(1.0, 0.0, 0.0);   // Red

    vec3 color = mix(startColor, endColor, normalizedLogValue);

    return color;
}

vec3 uint_log_color(uint value, uint maxValue) {
    // Ensure the value is within the expected range
    float fvalue = float(value);
    float fmaxValue = float(maxValue);
    fvalue = clamp(fvalue, 0.0, fmaxValue);

    // Convert value to log scale
    float logValue = log2(fvalue + 1.0); // Add 1 to avoid log(0)

    // Normalize logValue to the range [0, 1]
    float maxLogValue = log2(fmaxValue + 1.0);
    float normalizedLogValue = logValue / maxLogValue;

    // Map normalized log value to color
    // For example, using a gradient from blue to red
    vec3 startColor = vec3(0.0, 0.0, 1.0); // Blue
    vec3 endColor = vec3(1.0, 0.0, 0.0);   // Red

    vec3 color = mix(startColor, endColor, normalizedLogValue);

    return color;
}

vec3 uint_linear_color(uint value, uint max) {
    // Ensure max is greater than 0
    if (max == 0) {
        return vec3(0.0, 0.0, 1.0); // Return blue if invalid range
    }

    // Map value to the range [0, 1]
    float t = clamp(float(value) / float(max), 0.0, 1.0);

    // Define the gradient colors
    vec3 colorStart = vec3(0.0, 0.0, 1.0); // Blue
    vec3 colorEnd = vec3(1.0, 0.0, 0.0);   // Red

    // Interpolate between the gradient colors
    return mix(colorStart, colorEnd, t);
}

vec3 colors[20] = vec3[20](
    vec3(0.0, 1.0, 0.0),  // Green
    vec3(1.0, 1.0, 0.0),  // Yellow
    vec3(1.0, 0.0, 0.0),  // Red
    vec3(0.0, 0.0, 1.0),  // Blue
    vec3(1.0, 0.0, 1.0),  // Magenta
    vec3(0.0, 1.0, 1.0),  // Cyan
    vec3(1.0, 0.5, 0.0),  // Orange
    vec3(0.5, 0.0, 1.0),  // Purple
    vec3(0.5, 1.0, 0.0),  // Chartreuse
    vec3(1.0, 0.0, 0.5),  // Rose
    vec3(0.0, 0.5, 1.0),  // Azure
    vec3(0.0, 1.0, 0.5),  // Spring Green
    vec3(1.0, 0.5, 1.0),  // Pink
    vec3(0.5, 1.0, 1.0),  // Aquamarine
    vec3(0.5, 0.5, 0.5),  // Grey
    vec3(1.0, 1.0, 0.5),  // Light Yellow
    vec3(0.5, 0.0, 0.5),  // Dark Magenta
    vec3(0.0, 0.5, 0.5),  // Teal
    vec3(0.5, 0.5, 1.0),  // Light Blue
    vec3(1.0, 0.5, 0.5)   // Light Red
);

vec3 uint_fixed_colors(uint value) {
    uint index = value % 20u;
    // Return the color
    return colors[index];
}

bool isApproximately(float a, float b, float epsilon) {
    return abs(a - b) < epsilon;
}

bool isApproximately(vec2 a, vec2 b, float epsilon) {
    return all(lessThan(abs(a - b), vec2(epsilon)));
}

bool isApproximately(vec3 a, vec3 b, float epsilon) {
    return all(lessThan(abs(a - b), vec3(epsilon)));
}

bool isApproximately(vec4 a, vec4 b, float epsilon) {
    return all(lessThan(abs(a - b), vec4(epsilon)));
}
