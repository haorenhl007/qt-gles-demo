uniform sampler2D uTexture;

varying vec2 vTextureCoord;

void main() {
    vec4 color = texture2D(uTexture, vTextureCoord);
    if(color.a == 0.0) discard;
    gl_FragColor = color;
}
