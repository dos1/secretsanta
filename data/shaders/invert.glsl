#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D al_tex;
uniform float saturation;
varying vec2 varying_texcoord;
varying vec4 varying_color;

void main() {
	vec4 color = texture2D(al_tex, varying_texcoord);
	gl_FragColor = vec4((vec3(1.0) - color.rgb / color.a) * color.a, color.a) * varying_color;
}
