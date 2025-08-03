#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 positions;
    glm::vec3 normals;
    glm::vec2 uv;
};

class Sphere
{
    public:
    Sphere() {}
    ~Sphere() {}
    
    


    void initSphere()
    {

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                Vertex vertex;

                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                
                vertex.positions = glm::vec3(xPos, yPos, zPos);
                vertex.uv = glm::vec2(xSegment, ySegment);
                vertex.normals = glm::vec3(xPos, yPos, zPos);

                vertices.emplace_back(vertex);
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.emplace_back(y * (X_SEGMENTS + 1) + x);
                    indices.emplace_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.emplace_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.emplace_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<int>(indices.size());
    }

    int getIndexCount() const
    {
        return indexCount;
    }

    std::vector<Vertex> getVertexData()
    {
        return vertices;
    }

    std::vector<uint16_t> getIndexData()
    {
        return indices;
    }

private:
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    int indexCount = 0;
};