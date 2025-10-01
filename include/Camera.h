#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    struct Transform {
        glm::vec3 pos{0.0f, 0.0f, 0.0f};
        float yaw{0.0f};
        float pitch{0.0f};
    };

    struct MovementSettings {
        float rotationSpeed{5.0f};
        float moveSpeed{0.5f};
    };

    struct ProjectionSettings {
        float fov{45.0f};
        float nearPlane{0.1f};
        float farPlane{100.0f};
    };

private:
    Transform cam;
    Transform ghostQuad;
    Transform savedCam;

    MovementSettings movement;
    ProjectionSettings projection;

    bool ghostMode{false};

public:
    Camera();

    // Getters
    float getFov() const;
    float getNearPlane() const;
    float getFarPlane() const;
    float getMoveSpeed() const;

    glm::vec3 getCamPos() const;
    float getCamYaw() const;
    float getCamPitch() const;

    glm::vec3 getGhostPos() const;
    float getGhostYaw() const;
    float getGhostPitch() const;

    bool getGhostMode() const;

    // Ghost mode toggle
    void toggleGhostMode();

    // Setters
    void setCamPos(glm::vec3);
    void setCamYaw(float);
    void setCamPitch(float);
    void setFov(float newFov);
    void setMoveSpeed(float newSpeed);
};

