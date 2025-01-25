#version 460
#define PI 3.1415926535897932384626433832795
// Originally 3.14, GLSL doesn't have fixed, half, etc and I also couldn't be
// bothered to make floats of varying precisions so, here, fully precise PI as joar intended

const vec4 WHITE = vec4(1);

// Comments that start with 'Joar:' are comments or code carried from the original file

layout (binding = 0, set = 0) uniform SceneInfo {
    mat4x4 transform;
} scene;

layout (binding = 0, set = 1) uniform LevelInfo {
    // Joar stuff
    ivec2 texelSize;

    float rain;
    float light;
    vec2  screenOffset;

    vec4  lightDirAndPixelSize;
    float fogAmount;
    float waterLevel;
    float grime;
    float swarmRoom;
    float wetTerrain;
    float cloudsSpeed;
    float darkness;
    float contrast;
    float saturation;
    float hue;
    float brightness;
    vec4  aboveCloudsAtmosphereColor;
    float rimFix;
} level;

layout (binding = 1, set = 1) uniform sampler2D lTex; // level texture                          | 1400x800
layout (binding = 2, set = 1) uniform sampler2D pTex; // palette texture                        | 32x16
layout (binding = 3, set = 1) uniform sampler2D nTex; // noise texture                          | 64x64
layout (binding = 4, set = 1) uniform sampler2D sTex; // shadow grabpass (originally _GrabPass) | 1400x800

layout(location = 0) in vec2 f_uv;
layout(location = 1) in vec2 f_uv2;

layout(location = 0) out vec4 out_color;

// shorthand for texelFetch
vec4 texel(sampler2D smpl, ivec2 pos) {
	return texelFetch(smpl, pos, 0);
}

vec3 applyHue(vec3 aColor, float aHue) {
	float angle = radians(aHue);
	vec3 k = vec3(0.57735);
	float cosAngle = cos(angle);

	//Rodrigues' rotation formula
	return aColor * cosAngle + cross(k, aColor) * sin(angle) + k * dot(k, aColor) * (1 - cosAngle);
}

void maskShadows(vec2 pos) {
    vec4 grabTexCol = texture(sTex, pos);

    /* Joar:
    if (int(round(grabTexCol.r * 255)) > 1 || grabTexCol.g != 0.0 || grabTexCol.b != 0.0) {
        discard;
    }
    */
    if (grabTexCol.g + grabTexCol.b > 0 || int(round(grabTexCol.r * 255)) > 1) discard;
}

void colorParams(int red, int not_floor_dark, vec2 screen_pos) {
    /* Wack, modified from Joar:
    float clamp_red = 1.0;
    if (red < 10) {
        clamp_red = mix(not_floor_dark, 1.0, .5);
    }
    */
    float clamp_red = int(red < 10) * mix(not_floor_dark, 1.0, .5) + int(red >= 10); // Added for clarity

    out_color = mix(
        out_color,
        texel(pTex, ivec2(1.5, 7.5)),
        clamp(red * clamp_red * level.fogAmount / 30.0, 0, 1)
    );

    if (red >= 5) {
        // maskShadows(screen_pos);
    }

    // Joar: Color Adjustment params
    out_color.rgb *= level.darkness;
    out_color.rgb = ((out_color.rgb - 0.5) * level.contrast) + 0.5;
    float greyscale = dot(out_color.rgb, vec3(.222, .707, .071));  // Joar: Convert to greyscale numbers with magic luminance numbers
    out_color.rgb = mix(vec3(greyscale, greyscale, greyscale), out_color.rgb, level.saturation);
    out_color.rgb = applyHue(out_color.rgb, level.hue);
    out_color.rgb += level.brightness;
}

void main() {
    out_color = vec4(0.0, 0.0, 0.0, 1.0);
    gl_FragDepth = 1;

    // I've no idea what 'ugh' does but here's what it's supposed to be:
    // sample the level texture and take its red value and multiply by 255 (to get to 0-255 color range)
    // round it (to get an integer 255 color value)
    // modulo it with 90 (remainder when divided by 90)
    // decrement by 1 and modulo with 30
    // divide by 300
    float ugh = mod(mod(round(texture(lTex, f_uv).r * 255), 90) - 1, 30) / 300.0;

    // I've got no damn idea
    float displace = texture(nTex, vec2((f_uv.x * 1.5) - ugh + (level.rain * .01), (f_uv.y * .25) - ugh + level.rain * 0.05)).x;

    // what?
    displace = clamp(
        (sin((displace + f_uv.x + f_uv.y + level.rain * .1) * 3 * PI) - 0.95) * 20,
        0,
        1
    );

    /* Joar:
    vec2 screenPos = vec2(mix(level.spriteRect.x + level.spriteRect.x, level.spriteRect.z + level.screenOffset.x, f_uv.x), mix(level.spriteRect.y + level.screenOffset.y, level.spriteRect.w + level.screenOffset.y, f_uv.y));
    */
    vec2 screenPos = level.screenOffset + gl_FragCoord.xy;

    // if we're not wet enough or the pixel is above the water, do not displace pixels down
    /* Joar:
        if (level.wetTerrain < .5 || 1 - screenPos.y > level.waterLevel) displace = 0;
    */
    displace *= int(!(level.wetTerrain < .5 || /*1 - */screenPos.y > level.waterLevel));

    vec4 texcol = texture(lTex, vec2(f_uv2.x, f_uv2.y + displace * .001));

    int red = int(round(texcol.r * 255));
    int green = int(round(texcol.g * 255));

    /* Wack, modified from Joar:
    int notFloorDark = 1;
    bool renderDecals = false;

    if (green >= 16)
    {
        notFloorDark = 0;
        green -= 16;
    }

    if (green >= 8)
    {
        renderDecals = true;
        green -= 8;
    }
    */
    int notFloorDark = int(!(green >= 16));
    green %= 16;

    bool renderDecals = green >= 8;
    green %= 8;

    // Early return for white (empty) pixels
    if (texcol == WHITE) {
        out_color = texel(pTex, ivec2(0)); /*Bg color*/

        /* Joar:
        if (level.rimFix > .5) {
            out_color = level.aboveCloudsAtmosphereColor;
        }
        */
        out_color = mix(
            out_color,
            level.aboveCloudsAtmosphereColor,
            level.rimFix /*ceil(level.rimFix)*/
        );

        // maskShadows(screenPos);
        colorParams(red, notFloorDark, screenPos);
        return;
    }

    float subtract_red = mod(float(red), 30.0) * 0.003; // Added for clarity
    float shadow = texture(
        nTex,
        vec2(
            (f_uv.x * .5) + (level.rain * .1 * level.cloudsSpeed) - subtract_red,
            1 - (f_uv.y * .5) + (level.rain * .2 * level.cloudsSpeed) - subtract_red
        )
    ).x;

    shadow = .5 + sin(
        mod(
            shadow + (level.rain * .1 * level.cloudsSpeed) - f_uv.y,
            1
        ) * PI * 2
    ) * .5;

    shadow = clamp(((shadow - .5) * 6) + .5 - (level.light * 4), 0, 1);

    /* Joar:
    if (red > 90) {
        red -= 90;
    }
    else {
        shadow = 1.0;
    }
    */
    shadow = shadow * int(red > 90) + int(red <= 90);
    red %= 90;

    int paletteColor = int(clamp(floor((red - 1) / 30.0), 0, 2)); // Joar: some distant objects want to get palette color 3, so we clamp it

    red = int(mod(float(red) - 1.0, 30.0));
    gl_FragDepth = (red * notFloorDark) / 30.0;

    if (shadow != 1.0 && red >= 5) {
        vec2 grabPos = vec2(screenPos.x + -level.lightDirAndPixelSize.x * level.lightDirAndPixelSize.z * (red - 5), /*1 - */screenPos.y + level.lightDirAndPixelSize.y * level.lightDirAndPixelSize.w * (red - 5));
        grabPos = ((grabPos - vec2(0.5, 0.3)) * (1 + (red - 5.0) / 460.0)) + vec2(0.5, 0.3);
        vec4 sColor = texture(sTex, grabPos); // shadow color

        /* Joar:
        if (sColor.x != 0.0 || sColor.y != 0.0 || sColor.z != 0.0) {
            shadow = 1.0;
        }
        */
        float shadowExists = sColor.x + sColor.y + sColor.z;
        shadow = shadow * int(shadowExists <= 0) + int(shadowExists > 0);
    }

    // NOW we're getting the color to output
    out_color = mix(
        texel(pTex, ivec2(red * notFloorDark, paletteColor + 6)),
        texel(pTex, ivec2(red * notFloorDark, paletteColor + 3)),
        shadow
    );

    float rbcol = (sin((level.rain + (texture(nTex, vec2(f_uv.x * 2, f_uv.y * 2) ).x * 4) + red / 12.0) * PI * 2) * 0.5) + 0.5;
    out_color = mix(out_color, texel(pTex, ivec2(5 + rbcol * 25, 6)), (green >= 4 ? 0.2 : 0.0) * level.grime);

    if (renderDecals) {
        vec4 decalCol = texel(lTex, ivec2(255 - round(texcol.b * 255.0), 799));
        if(paletteColor == 2) decalCol = mix(decalCol, vec4(1), 0.2 - shadow * 0.1);
        decalCol = mix(decalCol, texel(pTex, ivec2(1, 7)), red / 60.0); // mix decal with palette base color

        out_color = mix(
            mix(
                out_color,
                decalCol,
                .7
            ),
            out_color * decalCol * 1.5,
            mix(
                .9,
                .3 + .4 * shadow,
                clamp((red - 3.5) * .3, 0, 1)
            )
        );

        colorParams(red, notFloorDark, screenPos);
        return;
    }

    if (green > 0 && green < 3) {
        out_color = mix(
            out_color,
            mix(
                mix(
                    texel(pTex, ivec2(30, 5 - (green - 1) * 2)), // lit
                    texel(pTex, ivec2(31, 5 - (green - 1) * 2)), // unlit
                    shadow
                ),
                mix(
                    texel(pTex, ivec2(30, 4 - (green - 1) * 2)), // lit
                    texel(pTex, ivec2(31, 4 - (green - 1) * 2)), // unlit
                    shadow
                ),
                red / 30.0
            ),
            texcol.b
        );

        colorParams(red, notFloorDark, screenPos);
        return;
    }

    if (green == 3) {
        out_color = mix(out_color, vec4(1), texcol.b * level.swarmRoom);
    }

    colorParams(red, notFloorDark, screenPos);
}
