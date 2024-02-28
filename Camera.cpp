#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;

        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUp));
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraTarget, this->cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        if (direction == MOVE_FORWARD)
        {
            cameraPosition += speed * cameraFrontDirection;
        }
        else if (direction == MOVE_BACKWARD)
        {
            cameraPosition -= speed * cameraFrontDirection;
        }
        else if (direction == MOVE_RIGHT)
        {
            cameraPosition += cameraRightDirection * speed;
        }
        else if (direction == MOVE_LEFT)
        {
            cameraPosition -= cameraRightDirection * speed;
        }
        else if (direction == MOVE_UP)
        {
            cameraPosition += cameraUpDirection * speed;
        }
        else if (direction == MOVE_DOWN)
        {
            cameraPosition -= cameraUpDirection * speed;
        }
        cameraTarget = cameraPosition + cameraFrontDirection;
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFrontDirection = glm::normalize(direction);
        
        cameraTarget = cameraPosition + cameraFrontDirection;
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }
}
