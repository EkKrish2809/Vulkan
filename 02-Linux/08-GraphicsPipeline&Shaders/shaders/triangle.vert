#version 460 core

// layout(location=0) in vec3 a_position;

// void main()
// {
//     gl_Position = vec4(a_position, 1.0);
// }

vec2 pos[3] = vec2[3]( vec2(-0.7, 0.7), vec2(0.7, 0.7), vec2(0.0, -0.7) ); 

void main() 
{
	gl_Position = vec4( pos[gl_VertexIndex], 0.0, 1.0 );
}