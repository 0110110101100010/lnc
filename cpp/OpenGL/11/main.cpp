#include <iostream>
#include <vector>
#include <string>
#include <chrono>
typedef std::chrono::high_resolution_clock Clock;

#include <GL/glew.h>  
#include <GLFW/glfw3.h>
#include <GLEnv.h>

#include <Vec3.h>
#include <Mat4.h>
#include <Rand.h>

#include "Octree.h"

#include <AbstractParticleSystem.h>

class SimpleStaticParticleSystem : public AbstractParticleSystem {
public:
    SimpleStaticParticleSystem(const std::vector<Vec3> data, float pointSize) :
        AbstractParticleSystem(pointSize),
        color{1.0f,1.0f,1.0f}
    {
        setData(data);
    }
    
    virtual void update(float t) {}
    virtual void setColor(const Vec3& color) {this->color = color;}

    void setData(const std::vector<float> data) {
        floatParticleData.resize(data.size()/3*7);
        size_t j = 0;
        for (size_t i = 0;i<data.size();i+=3) {
            floatParticleData[j*7+0] = data[i+0];
            floatParticleData[j*7+1] = data[i+1];
            floatParticleData[j*7+2] = data[i+2];
            
            Vec3 c = color == RAINBOW_COLOR ? Vec3::hsvToRgb({float(i)/data.size()*360,1.0,1.0}) : computeColor(color);
            
            floatParticleData[j*7+3] = c.x();
            floatParticleData[j*7+4] = c.y();
            floatParticleData[j*7+5] = c.z();
            floatParticleData[j*7+6] = 1.0f;
            j+=1;
        }
    }
    
    void setData(const std::vector<Vec3> data) {
        floatParticleData.resize(data.size()*7);
        for (size_t i = 0;i<data.size();++i) {
            floatParticleData[i*7+0] = data[i].x();
            floatParticleData[i*7+1] = data[i].y();
            floatParticleData[i*7+2] = data[i].z();
            
            Vec3 c = color == RAINBOW_COLOR ? Vec3::hsvToRgb({float(i)/data.size()*360,1.0,1.0}) : computeColor(color);
            
            floatParticleData[i*7+3] = c.x();
            floatParticleData[i*7+4] = c.y();
            floatParticleData[i*7+5] = c.z();
            floatParticleData[i*7+6] = 1.0f;
        }
    }
    
    virtual std::vector<float> getData() const {return floatParticleData;}
    virtual size_t getParticleCount() const  {return floatParticleData.size()/7;}
private:
    Vec3 color;
    std::vector<float> floatParticleData;

};


std::vector<Vec3> fixedParticles{};
const float radius = 0.001f;
const float colDist = 2*radius;
const size_t particleCount = 5000;
Octree octree{1.0f, Vec3{0.0f,0.0f,0.0f},2};

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {  
    if (action == GLFW_REPEAT || action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE :
                glfwSetWindowShouldClose(window, GL_TRUE);
                break;
        }
    } 
}

float mindDist(const Vec3& pos) {
//    return octree.minDist(pos);
    float minDist = (pos - fixedParticles[0]).length();
    for (const Vec3& p : fixedParticles) {
        float currentDist = (pos - p).length();
        if (currentDist < minDist) minDist = currentDist;
    }
    return minDist;
}

bool checkCollision(const Vec3& pos) {
    return mindDist(pos) < colDist;
}

Vec3 randomWalk(const Vec3& pos) {
    return pos + Vec3::randomPointInSphere()*mindDist(pos);
}

Vec3 genRandomStartpoint() {
    Vec3 current{Rand::rand11(),Rand::rand11(),Rand::rand11()};
    while (checkCollision(current))
        current = genRandomStartpoint();
    return current;
}

void initParticles() {
    fixedParticles.push_back(Vec3(0.0f,0.0f,0.0f));
}

void simulate(size_t particleCount) {
    for (size_t i = 0;i<particleCount;++i) {
        Vec3 current = genRandomStartpoint();
        while (!checkCollision(current)) {
            current = randomWalk(current);
            if (current.sqlength() > 5*5) {
                current = genRandomStartpoint();
            }
        }
        fixedParticles.push_back(current);
        octree.add(current);
        std::cout << i+1 << "/" << particleCount << "\r" << std::flush;
    }
}


void simulate(size_t maxParticleCount, uint32_t quota) {
    auto t1 = Clock::now();
    
    for (size_t i = fixedParticles.size();i<maxParticleCount;++i) {
        Vec3 current = genRandomStartpoint();
        while (!checkCollision(current)) {
            current = randomWalk(current);
            if (current.sqlength() > 5*5) {
                current = genRandomStartpoint();
            }
        }
        fixedParticles.push_back(current);
        octree.add(current);

        auto t2 = Clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() > quota) {
            std::cout << i+1 << "/" << particleCount << "\r" << std::flush;
            return;
        }
    }
}



void checkGLError(const std::string& id) {
    GLenum e = glGetError();
    if (e != GL_NO_ERROR) {
        std::cerr << "An openGL error occured:" << e << " at " << id << std::endl;
    }
}

int main(int agrc, char ** argv) {
    GLEnv gl{640,480,4,"Dendrite Growth Simulation", true, true, 4, 1, true};

    
    std::string vsString{
    "#version 410\n"
    "uniform mat4 MVP;\n"
    "layout (location = 0) in vec3 vPos;\n"
    "void main() {\n"
    "    gl_Position = MVP * vec4(vPos, 1.0);\n"
    "}\n"};

    std::string fsString{
    "#version 410\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(1.0,1.0,1.0,1.0)*0.03;\n"
    "}\n"};
    GLProgram prog{GLProgram::createFromString(vsString, fsString)};
    GLint mvpLocation{prog.getUniformLocation("MVP")};
    GLArray octreeArray{};
    GLBuffer vbOctreePos{GL_ARRAY_BUFFER};

    std::vector<float> empty;
    vbOctreePos.setData(empty,3,GL_DYNAMIC_DRAW);

    octreeArray.bind();
    octreeArray.connectVertexAttrib(vbOctreePos, prog, "vPos", 3);

    initParticles();
  //  simulate(10);
    SimpleStaticParticleSystem simplePS(fixedParticles, 5);
    simplePS.setColor(RAINBOW_COLOR);

    gl.setKeyCallback(keyCallback);
    glfwSetTime(0);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearDepth(1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    Vec3 lookAtVec{0,0,0};
    Vec3 lookFromVec{0,1,1};
    Vec3 upVec{0,1,0};
        
    checkGLError("init");

    size_t trisVertexCount = 0;
    do {
        Dimensions dim{gl.getFramebufferSize()};

        const Mat4 p{Mat4::perspective(3.0f, dim.aspect(), 0.0001f, 1000.0f)};
        const Mat4 v{Mat4::lookAt(lookFromVec,lookAtVec,upVec)};
        const Mat4 m{Mat4::rotationY(glfwGetTime()*27)*Mat4::rotationX(glfwGetTime()*17)};
        
        glViewport(0, 0, dim.width, dim.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                
        if (fixedParticles.size() < particleCount) {
            simulate(particleCount, 5);
            octreeArray.bind();
            std::vector<float> data{octree.toTriList()};
            trisVertexCount = data.size();
            vbOctreePos.setData(data,3,GL_DYNAMIC_DRAW);
            simplePS.setData(fixedParticles);
        }
        
        simplePS.setPointSize(dim.height/60.0f);
        simplePS.render(m*v,p);
        
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        
        octreeArray.bind();
        prog.enable();
        prog.setUniform(mvpLocation, m*v*p);
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(trisVertexCount));
        glEnable(GL_CULL_FACE);

        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);

        
        checkGLError("endOfFrame");
        gl.endOfFrame();
    } while (!gl.shouldClose());  
  
    return EXIT_SUCCESS;
}  
