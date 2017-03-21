#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D scene;
uniform sampler2D rays;


void main()
{

    vec4 render = texture(scene, TexCoords);
    vec4 godRays = texture(rays, TexCoords);

    color = godRays;
    //color = vec4(0.5f, 0.5f, 0.5f, 1.0f);
}
