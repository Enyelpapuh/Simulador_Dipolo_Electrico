#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Imgui/imgui.h>
#include <Imgui/imgui_impl_glfw.h>
#include <Imgui/imgui_impl_opengl3.h>

#include <iostream>
#include <vector>
#include <cmath>
#define _CRT_SECURE_NO_WARNINGS


// Shader loader (usa tus funciones favoritas o un sistema propio)
GLuint compileShader(const char* source, GLenum type);
GLuint createShaderProgram(const char* vertPath, const char* fragPath);

// Calcular campo eléctrico de un dipolo
glm::vec2 calcularCampo(glm::vec2 r, glm::vec2 p) {
    float r_norm = glm::length(r);
    if (r_norm < 0.01f) return glm::vec2(0.0f);

    glm::vec2 r_hat = glm::normalize(r);

    // Factor escalar
    float factor = 3.0f * glm::dot(p, r_hat);

    // (3 (p . r_hat) r_hat - p)
    glm::vec2 term = factor * r_hat - p;

    // Multiplicar por (1 / r_norm^3)
    return (1.0f / (r_norm * r_norm * r_norm)) * term;
}

int main() {
    // Inicializar GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Campo Dipolar", nullptr, nullptr);
    if (!window) { std::cerr << "Error!" << std::endl; return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Configurar ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark();

    // Cargar shaders
    GLuint shaderProgram = createShaderProgram("source/shader/vertex.glsl", "source/shader/fragment.glsl");

    // Geometría de la flecha (línea + triángulo)
    float flecha[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.9f,  0.05f,
        1.0f,  0.0f,
        0.9f, -0.05f
    };
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(flecha), flecha, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Proyección ortográfica
    glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -1.5f, 1.5f, -1.0f, 1.0f);

    // Momento dipolar inicial
    glm::vec2 momento_p(1.0f, 0.0f);

    // Puntos donde se evaluará el campo
    std::vector<glm::vec2> puntos;
    for (float y = -1.4f; y <= 1.4f; y += 0.3f) {
        for (float x = -1.9f; x <= 1.9f; x += 0.3f) {
            puntos.push_back(glm::vec2(x, y));
        }
    }

    // Loop principal
    // Loop principal
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Interfaz
        static bool soloDireccion = true;
        ImGui::Begin("Parametros del Dipolo");
        ImGui::SliderFloat2("Momento p", &momento_p.x, -5.0f, 5.0f);
        ImGui::Checkbox("Mostrar solo direccion", &soloDireccion);
        ImGui::End();

        // Renderizar
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Fondo negro como en la imagen de referencia
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

        for (auto& punto : puntos) {
            glm::vec2 E = calcularCampo(punto, momento_p);
            float magnitud = glm::length(E);
            if (magnitud < 0.001f) continue; // Ignorar campos muy pequeños

            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(punto, 0.0f));
            float angulo = atan2(E.y, E.x);
            model = glm::rotate(model, angulo, glm::vec3(0.0f, 0.0f, 1.0f));

            // Ajustar la longitud de la flecha según la opción
            if (soloDireccion) {
                model = glm::scale(model, glm::vec3(0.1f, 0.1f, 1.0f)); // Longitud fija
            }
            else {
                model = glm::scale(model, glm::vec3(0.1f * magnitud, 0.1f, 1.0f)); // Escalado por magnitud
            }

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
            glDrawArrays(GL_LINES, 0, 2);
            glDrawArrays(GL_TRIANGLES, 2, 3);
        }

        // ImGui render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }


    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// Shader loader simplificado
GLuint compileShader(const char* source, GLenum type) {
    FILE* file;
    if (fopen_s(&file, source, "rb") != 0 || !file) {
        std::cerr << "No se pudo abrir el archivo: " << source << std::endl;
        exit(-1);
    }
    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    rewind(file);
    char* data = new char[len + 1];
    fread(data, 1, len, file);
    data[len] = '\0';
    fclose(file);

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &data, nullptr);
    glCompileShader(shader);
    delete[] data;

    // Check
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        std::cerr << "Error shader: " << info << std::endl;
    }
    return shader;
}

GLuint createShaderProgram(const char* vertPath, const char* fragPath) {
    GLuint vertShader = compileShader(vertPath, GL_VERTEX_SHADER);
    GLuint fragShader = compileShader(fragPath, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    return program;
}
