#version 330 core
uniform int useWhite;
out vec4 FragColor;
void main() {
    FragColor = (useWhite == 1) ? vec4(1.0) : vec4(0.0, 0.0, 0.0, 1.0);
}