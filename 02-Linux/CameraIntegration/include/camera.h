#pragma once

#include <math.h>
// #include "../../includes/vmath.h"

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi

// This includes the global perspectiveProjectionMatrix
// #include "common.h"
// using namespace vmath;

const float YAW = -90.0;
const float PITCH = 0.0;
const float SPEED = 4.5;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0;

#define MOUSE_KEY_PRESS 4
#define MOUSE_KEY_RELEASE 5
#define LEFT_PRESS 1
// #define RIGHT _RELEASE

struct MouseInputs
{
    // int right_click;
    bool left_click = false;
    bool move = false;
    int action; // mouse key is pressed or not !
    float x;
    float y;
};

enum MOUSE_BUTTON {
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_BUTTON_NONE
};

enum MOUSE_ACTION {
    MOUSE_ACTION_PRESS,
    MOUSE_ACTION_RELEASE,
    MOUSE_ACTION_MOVE,
    MOUSE_ACTION_NONE
};


class Camera
{
public:
    float width;
    float height;

    glm::vec3 position;
    glm::vec3 front = glm::vec3(0.0, 0.0, -1.0);
    glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);
    glm::vec3 right;
    glm::vec3 worldUp = up;

    // euler angles
    float yaw = YAW;
    float pitch = PITCH;
    // Camera options
    float movementSpeed = SPEED;
    float mouseSensitivity = SENSITIVITY;
    float zoom = ZOOM;
    
    float lastX = 0.0;
    float lastY = 0.0;
    bool firstMouse = true;

    glm::mat4 perspectiveProjectionMatrix;
    
    // mouse data
    struct MouseInputs m_data;

    Camera() {}

    Camera(float _width, float _height, float *_position)
    {
        width = _width;
        height = _height;

        // Camera attributes
        position = glm::vec3(_position[0], _position[1], _position[2]);

        updateCameraVectors();
    }

    glm::mat4 getViewMatrix(void)
    {
        glm::mat4 tempViewMatrix = glm::mat4(1.0f);
        glm::vec3 center;
        center = position + front;

        tempViewMatrix = glm::lookAt(position, center, up);

        return tempViewMatrix;
    }

    glm::vec3 getEye(void)
    {
        return position;
    }

    glm::vec3 getFront(void)
    {
        return front;
    }

    glm::vec3 getCenter(void)
    {
        return (position + front);
    }

    glm::vec3 getUp()
    {
        return up;
    }

    glm::mat4 getProjectionMatrix()
    {
        return perspectiveProjectionMatrix;
    }

    void invertPitch(float height)
    {
        pitch = -pitch;

        position = glm::vec3(position[0],
                               -position[1] + (height * 2.0f),
                               position[2]);

        updateCameraVectors();
    }

    void updateCameraVectors(void)
    {

        glm::vec3 front_ = glm::vec3(
            cos(degToRad(yaw)) * cos(degToRad(pitch)),
            sin(degToRad(pitch)),
            sin(degToRad(yaw)) * cos(degToRad(pitch)));

        front = glm::normalize(front_);

        right = glm::normalize(glm::cross(front, worldUp));
    }

    void updateResolution(float _width, float _height)
    {
        width = _width;
        height = _height;

        perspectiveProjectionMatrix = glm::perspective(glm::radians(60.0f) + degToRad(zoom), (float)width / (float)height, 0.1f, 1000.0f);
    }

    void keyboardInputs(char keyPressed)
    {
        // in
        // printf("KeyPressed: %c\n", keyPressed);
        float velocity = movementSpeed * 0.1f;
        if (keyPressed == 'W' || keyPressed == 'w')
        {
            position = position + (front * velocity);
        }

        // left
        if (keyPressed == 'A' || keyPressed == 'a')
        {
            position = position - (right * velocity);
        }

        // out
        if (keyPressed == 'S' || keyPressed == 's')
        {
            position = position - (front * velocity);
        }

        // right
        if (keyPressed == 'D' || keyPressed == 'd')
        {
            position = position + (right * velocity);
        }

        // up
        if (keyPressed == 'V' || keyPressed == 'v')
        {
            position = position + (up * velocity);
        }

        // down
        if (keyPressed == ' ')
        {
            position = position - (up * velocity);
        }
        updateCameraVectors();
    }

    void mouseInputs(int button, int action, float mouseX, float mouseY)
    {
        if (button == MOUSE_BUTTON_LEFT)
        {
            if (action == MOUSE_ACTION_RELEASE)
            {
                lastX = 0.0f;
				lastY = 0.0f;
            }
            // if (firstMouse)
            else if (action == MOUSE_ACTION_PRESS)
            {
                lastX = mouseX;
                lastY = mouseY;
                firstMouse = false;
            }
            else if (action == MOUSE_ACTION_MOVE)
            {
                // PrintLog("Mouse X = %f, Mouse Y = %f\n", mouseX, mouseY);
                float xoffset = mouseX - lastX;
                float yoffset = lastY - mouseY;
                lastX = mouseX;
                lastY = mouseY;

                bool constrainPitch = true;
                xoffset *= mouseSensitivity;
                yoffset *= mouseSensitivity;

                yaw = fmod((yaw + xoffset), 360.0f);
                // yaw += xoffset;

                pitch += yoffset;

                if (constrainPitch)
                {
                    if (pitch > 89.0)
                        pitch = 89.0;
                    if (pitch < -89.0)
                        pitch = -89.0;
                }
            }
        }
        updateCameraVectors();
    }

    /*
    void mouseInputs(float mouseX, float mouseY)
    {
        // if (m_data.left_click == true)
        // {
            if (firstMouse)
            // if (m_data.action == MOUSE_KEY_PRESS)
            {
                // printf("MouseX: %f\t MouseY: %f\n", m_data.x, m_data.y);
                printf("clicked\n");
                lastX = m_data.x;
                lastY = m_data.y;
                firstMouse = false;
            }
            // else if (m_data.action == MOUSE_KEY_RELEASE)
            // {
                // printf("released\n");
                // lastX = 0.0f;
                // lastY = 0.0f;
            // }
            // if (m_data.move == true)
            // {

                float xoffset = m_data.x - lastX;
                float yoffset = lastY - m_data.y;
                lastX = m_data.x;
                lastY = m_data.y;

                bool constrainPitch = true;
                xoffset *= mouseSensitivity;
                yoffset *= mouseSensitivity;

                yaw = fmod((yaw + xoffset), 360.0f);
                // yaw = yaw + xoffset;
                pitch += yoffset;

                if (constrainPitch)
                {
                    if (pitch > 89.0)
                        pitch = 89.0;
                    if (pitch < -89.0)
                        pitch = -89.0;
                }
                glm::vec3 front_ = glm::vec3(
                    cos(degToRad(yaw)) * cos(degToRad(pitch)),
                    sin(degToRad(pitch)),
                    sin(degToRad(yaw)) * cos(degToRad(pitch)));

                front = glm::normalize(front_);
            // }
        // }
        updateCameraVectors();
    }*/

    // Process mouse scroll
    void mouseScroll(float scrollDelta_)
    {
        zoom -= (float)scrollDelta_;

        // PrintLog("Zoom = %f\n", zoom);

        if (zoom < -10000.0f)
            zoom = -10000.0f;

        if (zoom > 10000.0f)
            zoom = 10000.0f;

        perspectiveProjectionMatrix = glm::perspective(45.0f + degToRad(zoom), (float)width / (float)height, 0.1f, 1000.0f);

        // printf("Zoom = %f \n", zoom);
    }

    float degToRad(float degrees)
    {
        return (degrees * M_PI / 180.0);
    }

    void UIControls()
    {
        // ImGui::Begin("Camera Controls");

        // ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", position[0], position[1], position[2]);
        // ImGui::Text("Camera Front: (%.2f, %.2f, %.2f)", front[0], front[1], front[2]);
        // ImGui::Text("Camera Up: (%.2f, %.2f, %.2f)", up[0], up[1], up[2]);
        // ImGui::Text("Yaw: %.2f", yaw);
        // ImGui::Text("Pitch: %.2f", pitch);

        // // control movement speed and mouse sensitivity
        // ImGui::SliderFloat("Movement Speed", &movementSpeed, 0.1f, 20.0f);
        // ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.01f, 1.0f);
        // ImGui::SliderFloat("Zoom", &zoom, -10000.0f, 10000.0f);
        // // ImGui::Text("Movement Speed: %.2f", movementSpeed);
        // // ImGui::Text("Mouse Sensitivity: %.2f", mouseSensitivity);
        // // ImGui::Text("Zoom: %.2f", zoom);

        // if (ImGui::Button("Invert Pitch"))
        //     invertPitch(0.0f);

        // ImGui::End();
    }
};

// Current camera functionality
// - Mouse inputs for camera movement
// - Keyboard inputs for camera movement
// - Mouse scroll for zooming
// - UI controls for camera settings (movement speed, mouse sensitivity, zoom)
// - Get view matrix for rendering
// - Get camera position, front, up vectors
// - Update camera vectors based on yaw and pitch
// - Update resolution for perspective projection matrix
// - Invert pitch for reflection rendering
// - Convert degrees to radians
// - Handle mouse inputs for left click, move, and release actions
// - Handle mouse scroll for zooming in and out

// New functionality to be added
// - Add camera collission detection with the scene
// - Add camera collision detection with objects in the scene
// - Add camera collision detection with terrain