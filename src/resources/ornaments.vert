uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

attribute vec3 aPosition;
attribute vec2 aTextureCoord;

varying vec2 vTextureCoord;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
    vTextureCoord = aTextureCoord;
}
