#version 460 core

in vec2 uv;

layout(location = 0) out vec4 color;

float H = 0.7;
float lacunarity= 4.0;
float octaves = 5.0;
float offset = 0.9;
float gain = 1.0;
float density = 7.5;

const float PI = 3.1415926535;

float adapt(float x){
    return (x+1.0)/2.0;
}

vec4 permute(vec4 y){
    vec4 x = y;
    return mod(((x*34.0)+1.0)*x, 289.0);
}

vec2 fade(vec2 t) {
    return t*t*t*(t*(t*6.0-15.0)+10.0);
}


// Used instead of the gradient-based algorithm proposed in the slides
// delivers better results without the need to precompute a gradiant lookup table
// https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
float perlinNoise(vec2 P){
    vec4 Pi = floor(vec4(P,P)) + vec4(0.0, 0.0, 1.0, 1.0);
    vec4 Pf = fract(vec4(P,P)) - vec4(0.0, 0.0, 1.0, 1.0);
    Pi = mod(Pi, 289.0); // To avoid truncation effects in permutation
    vec4 ix = Pi.xzxz;
    vec4 iy = Pi.yyww;
    vec4 fx = Pf.xzxz;
    vec4 fy = Pf.yyww;
    vec4 i = permute(permute(ix) + iy);
    vec4 gx = 2.0 * fract(i * 0.0243902439) - 1.0; // 1/41 = 0.024...
    vec4 gy = abs(gx) - 0.5;
    vec4 tx = floor(gx + 0.5);
    gx = gx - tx;
    vec2 g00 = vec2(gx.x,gy.x);
    vec2 g10 = vec2(gx.y,gy.y);
    vec2 g01 = vec2(gx.z,gy.z);
    vec2 g11 = vec2(gx.w,gy.w);
    vec4 norm = 1.79284291400159 - 0.85373472095314 *
            vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11));
    g00 *= norm.x;
    g01 *= norm.y;
    g10 *= norm.z;
    g11 *= norm.w;
    float n00 = dot(g00, vec2(fx.x, fy.x));
    float n10 = dot(g10, vec2(fx.y, fy.y));
    float n01 = dot(g01, vec2(fx.z, fy.z));
    float n11 = dot(g11, vec2(fx.w, fy.w));
    vec2 fade_xy = fade(Pf.xy);
    vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
    float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
    return 2.3 *  n_xy;
}

float fbm(vec2 x) {
    float v = 0.0;
    float a = 0.1;
    vec2 shift = vec2(100);
    mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.50));
    for (int i = 0; i < 8; ++i) {
        v += a * perlinNoise(x);
        x =  x * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}


// with the help of https://www.classes.cs.uchicago.edu/archive/2015/fall/23700-1/final-project/MusgraveTerrain00.pdf
float hyrbidMultifractal(vec2 point, float H, float lacunarity, float octaves, float offset, float gain){
    vec2 p =  point;
    float frequency, result, signal, weight;
    int i;
    float exponent_array[100];
    frequency = 1.0;
    //filling the exponent array
    for(i=0; i<octaves; ++i){
        exponent_array[i] = pow(frequency, -H);
        frequency *= lacunarity;
    }

    signal = offset - abs(perlinNoise(p));
    signal *= signal;
    result = signal;
    weight = 1.0;

    for( i=1; i<octaves; i++ ) {
        p = p * lacunarity;
        weight = signal * gain;
        clamp(weight, 0.0, 1.0);
        signal = offset - abs(perlinNoise(p));
        signal *= signal;
        signal *= weight;
        result += signal * exponent_array[i];
    }
    return result;

}

float getTerrainHeight(vec2 pos){
    vec2 p = pos*density;
    float b2 = fbm(p*10)*0.2;
    float h1 = hyrbidMultifractal(p/8.0, H, lacunarity, octaves, offset, gain);
    float h2 = hyrbidMultifractal(p/3.0, H, lacunarity, octaves, offset, gain/2.0)*2.0;
    float h3 = hyrbidMultifractal(p*2.0, H, lacunarity, octaves, offset, gain)*0.3;
    return b2+h1+h2+h3-0.8;
}

void main(){
    color = vec4(vec3(getTerrainHeight(uv)), 1);
    //color = vec4(gl_FragCoord.xy, 1, 1);
    color = vec4(1, 0, 1, 1);
}