#version 450

layout(constant_id = 0) const float inPosition0Xyz = 0.0;
layout(constant_id = 1) const float inPosition0xYz = -0.5;
layout(constant_id = 2) const float inPosition0xyZ = 0.0;
layout(constant_id = 3) const float inPosition1Xyz = 0.5;
layout(constant_id = 4) const float inPosition1xYz = 0.5;
layout(constant_id = 5) const float inPosition1xyZ = 0.0;
layout(constant_id = 6) const float inPosition2Xyz = -0.5;
layout(constant_id = 7) const float inPosition2xYz = 0.5;
layout(constant_id = 8) const float inPosition2xyZ = 0.0;

vec3 inPosition[] = {{inPosition0Xyz, inPosition0xYz, inPosition0xyZ}
 , {inPosition1Xyz, inPosition1xYz, inPosition1xyZ}
 , {inPosition2Xyz, inPosition2xYz, inPosition2xyZ}};

void main() {
    gl_Position = vec4(inPosition[gl_VertexIndex], 1.0);
}
