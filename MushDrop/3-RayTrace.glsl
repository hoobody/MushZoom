// RayTrace.glsl: ray-tracing pixel shader

#version 130

// ----- Application Data

uniform vec4 spheres[3];				// sphere.xyz is center, sphere.w is radius
uniform vec4 planes[6];					// 'room' formed from six planes, facing outwards
uniform vec3 light;						// location of illumination
uniform float windowWidth;				// window size to normalize ray direction
uniform float windowHeight;
uniform vec3 viewPnt = vec3(0,0,-1);	// location of camera
uniform vec3 viewDir = vec3(0,0,1);		// camera looks at screen center
uniform vec3 up = vec3(0,1,0);			// direction vertically up wrt camera
uniform vec3 right = vec3(1,0,0);

// ----- Shader Output

out vec4 pColor;						// determined by ray-intersections, shadow, etc.

// ----- Ray Definition, Intersection Tests

vec3 RayDirection() {
	// compute 3D point on screen, then compute view direction
	float xf = 2*gl_FragCoord.x/windowWidth-1;	// +/-1
	float yf = 2*gl_FragCoord.y/windowHeight-1;	// +/-1
	return normalize(viewDir+xf*right+yf*up);
}

struct Ray { vec3 b, v; };				// origin, direction of primary or secondary ray, v presumed unit length

// The RayPlane and RaySphere procedures return parameter α that
// represents the intersection p on the ray; that is, p = r.b+α*r.v.

float RayPlane(Ray r, vec4 p) {
	// return parametric intersection of ray and plane (or -1 if no intersection)
	float a = dot(p.xyz, r.v);
	if (a == 0)
		return -1;						// ray is parallel to plane
	float d = dot(p.xyz, r.b)+p.w;
	if ((d > 0 && a > 0) || (d < 0 && a < 0))
		return -1; 						// ray faces away from plane
	return (-p.w-dot(r.b, p.xyz))/a;	// ray hits plane
}

float RaySphere(Ray r, vec4 s) {
	// return least positive alpha intersection of ray and sphere (or -1 if none)
	// quadratic formula, for ax**2+bx*c = 0, x = (-b +/- sqrt(b*b-4ac))/2a
	// a = 1, b = 2 (v • q), and c = q • q - r2
	vec3 q = r.b-s.xyz;
	float vDot = dot(r.v, q);
	float sq = vDot*vDot-dot(q, q)+s.w*s.w;
	if (sq < 0)
		return -1;						// ray misses sphere
	float root = sqrt(sq), a = -vDot-root;
	if (a < 0)
		a = -vDot+root;
	return a > 0? a : -1;
}

bool HitNearestPlane(Ray r, out vec3 hit, out int id) {
	// if ray hits an input plane, set intersection and return true
	hit = vec3(0);
	id = -1;
	float minA = 1000;
	for (int i = 0; i < 6; i++) {
		float a = RayPlane(r, planes[i]);
		if (a > 0 && a < minA) {
			minA = a;
			id = i;
		}
	}
	if (id >= 0)
		hit = r.b+minA*r.v;
	return id >= 0;
}

bool HitNearestSphere(Ray r, out vec3 intersection, out int id) {
	// if r hits light or sphere, set nearest intersection and return true
	// id >= 0: sphere index, id = -1: light, id = -2: no hit
	intersection = vec3(0);
	id = -2;
	float minA = 1000;
	// test light
	float a = RaySphere(r, vec4(light, .07));
	if (a > 0) {
		minA = a;
		id = -1;
	}
	// test spheres
	for (int i = 0; i < 3; i++) {
		float a = RaySphere(r, spheres[i]);
		if (a > 0 && a < minA) {
			minA = a;
			id = i;
		}
	}
	if (id > -2)
		intersection = r.b+minA*r.v;
	return id > -2;
}

// ----- Shading Operations

// Using the intersection routines, the test for a point in shadow is straightforward:

bool InShadow(vec3 pos) {
	// return true if pos not blocked from light
	int id;
	vec3 v = normalize(light-pos);
	vec3 p = pos+.0001*v, hit;						// .0001 offset: avoid self-intersection
	HitNearestSphere(Ray(p, v), hit, id);
	return id != -1;
}

// Phong shading: a simple shading method for a partly specular, partly diffuse surface.

void Phong(vec3 p, vec3 n, vec3 e, out float dif, out float spc) {
	// in: p: position, n: normal, e: view dir
	// out: dif: diffuse coeff, spc: specular coeff
	vec3 l = normalize(light-p);					// light vector
	vec3 r = reflect(l, n);							// highlight vector
	dif = max(0, dot(n, l));						// one-sided diffuse
	spc = pow(max(0, dot(e, r)), 50);				// one-sided specular
}

// ----- Main

// ----- Colors for 3 Spheres, 6 Walls, Light

vec3 sphereCols[] = vec3[3](vec3(1,1,0), vec3(0,1,0), vec3(0,0,1)), lightColor = vec3(1);
vec3 planeCols[] = vec3[6](vec3(0,.7,0), vec3(1,1,1), vec3(0,1,1), vec3(1,0,1), vec3(1,0,0), vec3(1,.6,0));

// the pixel shader is given a pixel location (glFragCoord.x,y) through which a ray
// passes and is tested for intersection with the spheres; the implementation
// can be arbitrarily complex with respect to the interaction of light and matter

void main() {
	// create ray
	vec3 hit;
	Ray ray = Ray(viewPnt, RayDirection());
	// set default lighting coefficients
	float amb = .1f, dif = 1, spec = 1, tmp;
	int id = -2;
	HitNearestSphere(ray, hit, id);					// does ray hit a sphere?
	// set pColor based on id >= 0: sphere index, -1: light, -2: no hit
	if (id == 1) {
		// sphere[1] is chrome, so recompute id after single bounce
		vec3 n = normalize(hit-spheres[id].xyz);	// sphere normal
		Phong(hit, n, viewDir, dif, tmp);			// reduce dif, spec from defaults
		dif = max(.7, dif); 						// max is GLSL function
		ray = Ray(hit+.0001*n, reflect(ray.v, n));	// note built-in GLSL reflect function
		HitNearestSphere(ray, hit, id);				// reflected ray hit another sphere?
	}
	if (id == -1) {
		// diffuse shaded light bulb
		vec3 n = normalize(hit-light);				// sphere surface normal easy
		float ad = min(.8+.3*abs(dot(n, viewDir)), 1);
		pColor = vec4(ad*lightColor, 1);
	}
	if (id >= 0) {
		// Phong-shaded sphere
		vec3 nrm = normalize(hit-spheres[id].xyz);
		float df, sp;
		Phong(hit, nrm, ray.v, df, sp);
		float ad = clamp(amb+df*dif, 0, 1);
		vec3 color = ad*sphereCols[id]+sp*spec*lightColor;
		pColor = vec4((InShadow(hit)? .5 : 1)*color, 1);
	}
	if (id == -2 && HitNearestPlane(ray, hit, id)) {
		// diffuse-shaded wall
		vec4 plane = planes[id];
		vec3 l = normalize(light-hit);
		dif *= abs(dot(plane.xyz, l));
		float ad = clamp(amb+dif, 0, 1);
		pColor = vec4(ad*planeCols[id], 1);
	}
}
