

struct Plane
{
	float a, b, c, d;
};

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

// <0 ... pt lies on the negative halfspace
//  0 ... pt lies on the plane
// >0 ... pt lies on the positive halfspace
float ClassifyPoint(Plane plane, vec3 pt)
{
	float d;
	d = plane.a * pt.x + plane.b * pt.y + plane.c * pt.z + plane.d;
	return d;
}

struct bounding_box
{
	vec4 mMin;
	vec4 mMax;
};

vec3[8] bounding_box_corners(bounding_box bb)
{
	vec3 corners[8] = vec3[8](
		vec3(bb.mMin.x, bb.mMin.y, bb.mMin.z),
		vec3(bb.mMin.x, bb.mMin.y, bb.mMax.z),
		vec3(bb.mMin.x, bb.mMax.y, bb.mMin.z),
		vec3(bb.mMin.x, bb.mMax.y, bb.mMax.z),
		vec3(bb.mMax.x, bb.mMin.y, bb.mMin.z),
		vec3(bb.mMax.x, bb.mMin.y, bb.mMax.z),
		vec3(bb.mMax.x, bb.mMax.y, bb.mMin.z),
		vec3(bb.mMax.x, bb.mMax.y, bb.mMax.z)
	);
	return corners;
}

bounding_box transform(bounding_box bbIn, mat4 M)
{
	vec3[8] corners = bounding_box_corners(bbIn);
	// Transform all the corners:
	for (int i = 0; i < 8; ++i) {
		vec4 transformed = (M * vec4(corners[i], 1.0));
		corners[i] = transformed.xyz / transformed.w;
	}

	bounding_box bbOut;
	bbOut.mMin = vec4(corners[0], 0.0);
	bbOut.mMax = vec4(corners[0], 0.0);
	for (int i = 1; i < 8; ++i) {
		bbOut.mMin.xyz = min(bbOut.mMin.xyz, corners[i]);
		bbOut.mMax.xyz = max(bbOut.mMax.xyz, corners[i]);
	}
	return bbOut;
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
	return (color_from_id_hash(a + uint(dot(tint, vec3(1.0)) * 255)) * tint); 

}