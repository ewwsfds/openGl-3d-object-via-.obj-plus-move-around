#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <iostream>

struct Vertex {
    glm::vec3 position;
};

struct ModelRange {
    unsigned int indexOffset;
    unsigned int indexCount;
};

unsigned int VAO, VBO, EBO;
unsigned int shaderProgram;

std::vector<Vertex> vertices;
std::vector<unsigned int> indices;

// Store ranges for each model
std::vector<ModelRange> modelRanges;

// ---------- Shader ----------
const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0);
}
)";

// ---------- Compile shader ----------
unsigned int compileShader(unsigned int type, const char* src)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    return shader;
}

unsigned int createShaderProgram()
{
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

// ---------- Load OBJ and append ----------
void loadModel(const std::string& path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate | aiProcess_FlipUVs
    );

    if (!scene || !scene->mRootNode)
    {
        std::cout << "Assimp error\n";
        return;
    }

    aiMesh* mesh = scene->mMeshes[0];

    ModelRange range;
    range.indexOffset = indices.size();

    unsigned int vertexOffset = vertices.size();

    // Vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex v;
        v.position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );
        vertices.push_back(v);
    }

    // Indices (offset them!)
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j] + vertexOffset);
        }
    }

    range.indexCount = indices.size() - range.indexOffset;
    modelRanges.push_back(range);
}

// ---------- Setup buffers ----------
void setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// ---------- Main ----------
int main()
{
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "Two Models One VAO", NULL, NULL);
    glfwMakeContextCurrent(window);

    gladLoadGL();

    shaderProgram = createShaderProgram();

    // Load TWO models into same buffers
    loadModel("model1.obj");
    loadModel("model2.obj");

    setupMesh();

    glEnable(GL_DEPTH_TEST);

    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -5));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    float offsetx = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float speed = 0.02f;

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            offsetx -= speed;

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            offsetx += speed;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);

        // ---------- Draw Model 1 (static) ----------
        glm::mat4 model1 = glm::mat4(1.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model1));

        glDrawElements(
            GL_TRIANGLES,
            modelRanges[0].indexCount,
            GL_UNSIGNED_INT,
            (void*)(modelRanges[0].indexOffset * sizeof(unsigned int))
        );

        // ---------- Draw Model 2 (moving) ----------
        glm::mat4 model2 = glm::translate(glm::mat4(1.0f), glm::vec3(offsetx, 0.0f, 0.0f));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model2));

        glDrawElements(
            GL_TRIANGLES,
            modelRanges[1].indexCount,
            GL_UNSIGNED_INT,
            (void*)(modelRanges[1].indexOffset * sizeof(unsigned int))
        );

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
