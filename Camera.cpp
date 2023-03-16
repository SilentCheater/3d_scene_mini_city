#include "Camera.hpp"
#include <iostream>


namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraUpDirection = glm::normalize(cameraUp);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection));
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);
    }
    glm::vec3 Camera::getCameraPosition() {
        return this->cameraPosition;
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        //TODO
        glm::vec3 translationVec;
        switch (direction) {
        case MOVE_FORWARD:
            translationVec = cameraFrontDirection * speed;
            break;
        case MOVE_LEFT:
            translationVec = -cameraRightDirection * speed;
            break;
        case MOVE_RIGHT:
            translationVec = cameraRightDirection * speed;
            break;
        case MOVE_BACKWARD:
            translationVec = -cameraFrontDirection * speed;
            break;
        case MOVE_UP:
            translationVec = cameraUpDirection * speed;
            break;
        case MOVE_DOWN:
            translationVec = -cameraUpDirection * speed;
            break;
        }
        cameraPosition = cameraPosition + translationVec;
        cameraTarget = cameraTarget + translationVec;

    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        //TODO
        glm::mat4 rotationMat = glm::translate(cameraPosition);

        rotationMat = glm::rotate(rotationMat, glm::radians(yaw), cameraUpDirection);
        rotationMat = glm::rotate(rotationMat, glm::radians(pitch), cameraRightDirection);
        rotationMat = glm::translate(rotationMat, -cameraPosition);
        cameraTarget = rotationMat * glm::vec4(cameraTarget, 1.0f);

        // keep the front direction fixed at a certain height
        cameraFrontDirection = glm::rotate(glm::radians(yaw), cameraUpDirection) * glm::vec4(cameraFrontDirection, 1.0f);
        // enable the front direction to point upward or downward

        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }
    void Camera::scenePreview(float angle) {
        // set the camera
        this->cameraPosition = glm::vec3(30.0, 30.0, 140.0);

        // rotate with specific angle around Y axis
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));

        // compute the new position of the camera 
        // previous position * rotation matrix
        this->cameraPosition = glm::vec4(rotation * glm::vec4(this->cameraPosition, 1));
        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
    }
}