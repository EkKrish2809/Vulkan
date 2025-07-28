#version 460 core

layout(location=0) in vec2 a_position;
layout(location=1) in vec3 a_color;

layout(location=0) out vec3 o_color;

void main()
{
    gl_Position = vec4(a_position, 0.0, 1.0);
	o_color = a_color;
}

// vec2 pos[3] = vec2[3]( vec2(-0.7, 0.7), vec2(0.7, 0.7), vec2(0.0, -0.7) ); 

// void main() 
// {
// 	gl_Position = vec4( pos[gl_VertexIndex], 0.0, 1.0 );
// }