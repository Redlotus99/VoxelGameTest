#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 Position;
    float Yaw;
    float Pitch;
    
    // Camera vectors for lookAt
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f));

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;
    glm::vec3 getFront() const { return Front; }

    void processMouse(float dx, float dy);
    void processKeyboard(bool forward, bool back, bool left, bool right, bool up, bool down, float dt);

private:
    static constexpr float YAW = -90.0f;
    static constexpr float PITCH = 0.0f;
    static constexpr float SPEED = 6.0f;
    static constexpr float SENSITIVITY = 0.1f;
    static constexpr float FOV = 70.0f;
    static constexpr float NEAR_PLANE = 0.1f;
    static constexpr float FAR_PLANE = 500.0f;
    static constexpr float PITCH_MIN = -89.0f;
    static constexpr float PITCH_MAX = 89.0f;

    void updateCameraVectors();
};

#endif // CAMERA_H
