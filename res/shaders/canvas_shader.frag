#version 330 core

uniform sampler1D uDashTexture;     // 虚线纹理
uniform int uIsDashed;              // 0=实线, 1=虚线
uniform float uDashScale;           // 虚线缩放（控制密度）

in vec4 vColor;
in float vTexCoord;
out vec4 FragColor;

void main()
{
    if (uIsDashed == 0) {
        // 实线
        FragColor = vColor;
    } else {
        // 虚线：采样纹理
        float texPos = vTexCoord * uDashScale;
        float alpha = texture(uDashTexture, texPos).r;
        if (alpha < 0.5) discard;
        FragColor = vColor;
    }
}
