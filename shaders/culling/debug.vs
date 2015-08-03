#version 430 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

layout(location=0) in vec4 in_vertex;
layout(location=2) in vec3 in_normal;

out vec3 normals;
out vec3 light_dir;

void main()
{
  mat3 m3view = mat3(view[0].xyz, view[1].xyz, view[2].xyz);
  normals = m3view * in_normal;
  light_dir = m3view * normalize( vec3( 1, 1, 0 ) );
  gl_Position = (proj * view * model) * in_vertex;
}
