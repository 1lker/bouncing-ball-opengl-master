#include "Angel.h"
#include "Globals.h"
#include "InitShader.h"
#include "input.h"
#include "objects.h"
#include "physics.h"
#include "render.h"
#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>

static void setupVAO(GLuint &vao, GLuint &vbo,
                     const std::vector<vec4>& verts,
                     const std::vector<vec3>& norms) {
    std::vector<GLfloat> data;
    data.reserve(verts.size() * 7);
    for (size_t i = 0; i < verts.size(); i++) {
        data.push_back(verts[i].x);
        data.push_back(verts[i].y);
        data.push_back(verts[i].z);
        data.push_back(verts[i].w);
        data.push_back(norms[i].x);
        data.push_back(norms[i].y);
        data.push_back(norms[i].z);
    }
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(GLfloat), data.data(), GL_STATIC_DRAW);
    
    GLuint posLoc = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (GLvoid*)0);
    
    GLuint normLoc = glGetAttribLocation(program, "vNormal");
    if (normLoc != (GLuint)-1) {
        glEnableVertexAttribArray(normLoc);
        glVertexAttribPointer(normLoc, 3, GL_FLOAT, GL_FALSE,
                              7*sizeof(GLfloat), (GLvoid*)(4*sizeof(GLfloat)));
    }
}

// Setup a separate VAO/VBO for the trajectory
static void setupTrajectoryVAO() {
    glGenVertexArrays(1, &vaoTrajectory);
    glBindVertexArray(vaoTrajectory);
    
    glGenBuffers(1, &vboTrajectory);
    glBindBuffer(GL_ARRAY_BUFFER, vboTrajectory);
    // We'll upload data dynamically each frame
    glBufferData(GL_ARRAY_BUFFER, MAX_TRAJECTORY_POINTS * sizeof(vec4), nullptr, GL_DYNAMIC_DRAW);
    
    GLuint posLoc = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    
    // Provide a default empty normal for trajectory points
    GLuint normLoc = glGetAttribLocation(program, "vNormal");
    if (normLoc != (GLuint)-1) {
        // Create a separate VBO for normals
        GLuint normalVBO;
        glGenBuffers(1, &normalVBO);
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        
        // Default normals pointing toward camera (z-axis)
        std::vector<vec3> defaultNormals(MAX_TRAJECTORY_POINTS, vec3(0.0, 0.0, 1.0));
        glBufferData(GL_ARRAY_BUFFER, MAX_TRAJECTORY_POINTS * sizeof(vec3), defaultNormals.data(), GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(normLoc);
        glVertexAttribPointer(normLoc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    }
    
    // Reset to default state
    glBindVertexArray(0);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,2);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE,GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Enhanced Bouncing Ball", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
#ifndef __APPLE__
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW error: " << glewGetErrorString(err) << "\n";
        glfwTerminate();
        return -1;
    }
#endif

    // Very important: Disable face culling so all faces are always rendered
    glDisable(GL_CULL_FACE);
    registerCallbacks(window);
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); // Change from GL_LESS to GL_LEQUAL for better z-fighting handling
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    program = Angel::InitShader("vshader.glsl", "fshader.glsl");
    modelLoc = glGetUniformLocation(program, "model");
    projectionLoc = glGetUniformLocation(program, "projection");
    objColorLoc = glGetUniformLocation(program, "objColor");
    lightDirLoc = glGetUniformLocation(program, "lightDir");
    viewPosLoc  = glGetUniformLocation(program, "viewPos");
    
    glUseProgram(program);
    
    // Use multiple light sources for better illumination
    vec3 lightDir(0.5f, 1.0f, 0.75f); // Adjusted for better all-around lighting
    glUniform3fv(lightDirLoc, 1, &lightDir[0]);
    
    // Camera position (view position) - move a bit to improve lighting angles
    vec3 viewPos(windowWidth/2.0f, windowHeight/2.0f, 300.0f);
    glUniform3fv(viewPosLoc, 1, &viewPos[0]);
    
    initCube();
    
    // Reduced subdivision level for better wireframe visualization
    // Changed from 5 to 2 subdivisions for more visible wireframe
    initSphere(2);
    
    if (loadBunnyModel("bunny.off")) {
        calculateBunnyNormals();
        bunnyLoaded = true;
    } else {
        bunnyLoaded = false;
    }
    
    // Create VAOs for each object
    setupVAO(vaoCube, vboCube, cubeVertices, cubeNormals);
    setupVAO(vaoSphere, vboSphere, sphereVertices, sphereNormals);
    setupVAO(vaoBunny, vboBunny, bunnyVertices, bunnyNormals);
    // Create VAO/VBO for trajectory
    setupTrajectoryVAO();
    
    glViewport(0, 0, windowWidth, windowHeight);
    mat4 proj = Ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f, -1000.0f, 1000.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_TRUE, proj);
    
    initBall();
    printHelp();
    
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentT = glfwGetTime();
        double dt = currentT - lastTime;
        lastTime = currentT;
        
        if (multipleObjects && (currentT - lastLaunchTime)>launchInterval) {
            launchBall();
            lastLaunchTime = currentT;
        }
        updateBall(dt);
        if (showParticles) updateParticles(dt);
        
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    glDeleteVertexArrays(1, &vaoCube);
    glDeleteBuffers(1, &vboCube);
    glDeleteVertexArrays(1, &vaoSphere);
    glDeleteBuffers(1, &vboSphere);
    glDeleteVertexArrays(1, &vaoBunny);
    glDeleteBuffers(1, &vboBunny);
    glDeleteVertexArrays(1, &vaoTrajectory);
    glDeleteBuffers(1, &vboTrajectory);
    
    glfwTerminate();
    return 0;
}