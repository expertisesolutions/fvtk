#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in mat4 transformMatrix;

layout(location = 0) out vec2 fragTexCoord;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec2 positions2[3] = vec2[](
    vec2(0.0, -0.25),
    vec2(0.25, 0.25),
    vec2(-0.25, 0.25)
);

void main() {
  // int i = 0;
  //   if (
  //       transformMatrix[i].r == 1.0
  //       // transformMatrix[i].r == 1.0
  //       // || transformMatrix[i].g == 1.0
  //       // || transformMatrix[i].b == 1.0
  //       // || transformMatrix[i].a == 1.0
  //       )
  //     gl_Position = vec4(inPosition.xy*0.5, 0.0, 1.0) * mat4 (vec4 (0.0, -1.0, 0.0, 0.0)
  //                                                         , vec4 (1.0, 0.0, 0.0, 0.0)
  //                                                         , vec4 (0.0, 0.0, 1.0, 0.0)
  //                                                         , vec4 (0.0, 0.0, 0.0, 1.0)
  //                                                         );
  //   else
  gl_Position = vec4(inPosition, 0.0, 1.0) * transformMatrix;
  // if (transformMatrix[0].r == 0.0)
  //   gl_Position = vec4 (positions[gl_VertexIndex], 0.0, 1.0);
  // else
  //   gl_Position = vec4 (positions2[gl_VertexIndex], 0.0, 1.0);
  fragTexCoord = inTexCoord;
}
