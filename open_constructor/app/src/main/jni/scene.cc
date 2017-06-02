#include "gl/opengl.h"
#include "scene.h"

std::string ColorFragmentShader() {
  return "varying vec4 f_color;\n"
         "void main() {\n"
         "  gl_FragColor = f_color;\n"
         "}\n";
}

std::string ColorVertexShader() {
  return "attribute vec4 v_vertex;\n"
         "attribute vec4 v_color;\n"
         "uniform mat4 MVP;\n"
         "varying vec4 f_color;\n"
         "void main() {\n"
         "  gl_Position = MVP * v_vertex;\n"
         "  f_color = v_color;\n"
         "}\n";
}

std::string TexturedFragmentShader() {
  return "uniform sampler2D u_texture;\n"
         "varying vec4 f_color;\n"
         "varying vec2 v_uv;\n"
         "void main() {\n"
         "  gl_FragColor = f_color + texture2D(u_texture, v_uv);\n"
         "}\n";
}

std::string TexturedVertexShader() {
  return "attribute vec4 v_vertex;\n"
         "attribute vec2 v_coord;\n"
         "attribute vec4 v_color;\n"
         "varying vec4 f_color;\n"
         "varying vec2 v_uv;\n"
         "uniform mat4 MVP;\n"
         "void main() {\n"
         "  f_color = v_color;\n"
         "  v_uv.x = v_coord.x;\n"
         "  v_uv.y = 1.0 - v_coord.y;\n"
         "  gl_Position = MVP * v_vertex;\n"
         "}\n";
}

namespace oc {

    Scene::Scene() { }

    Scene::~Scene() {
        delete color_vertex_shader;
        color_vertex_shader = nullptr;
        delete textured_shader;
        textured_shader = nullptr;
    }

    void Scene::InitGLContent() {
        renderer = new GLRenderer();
        color_vertex_shader = new GLSL(ColorVertexShader(), ColorFragmentShader());
        textured_shader = new GLSL(TexturedVertexShader(), TexturedFragmentShader());
    }

    void Scene::SetupViewPort(int w, int h) {
        glViewport(0, 0, w, h);
        renderer->Init(w, h, 1);
    }

    void Scene::Render(bool frustum) {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        long lastTexture = INT_MAX;
        for (Mesh& mesh : static_meshes_) {
            if (mesh.image && (mesh.texture == -1)) {
                GLuint textureID;
                glGenTextures(1, &textureID);
                mesh.texture = textureID;
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mesh.image->GetWidth(), mesh.image->GetHeight(),
                             0, GL_RGB, GL_UNSIGNED_BYTE, mesh.image->GetData());
            }
            if (mesh.texture == -1) {
                color_vertex_shader->Bind();
                renderer->Render(&mesh.vertices[0].x, 0, 0, mesh.colors.data(), mesh.vertices.size());
            } else {
                if (lastTexture != mesh.texture) {
                    lastTexture = (unsigned int)mesh.texture;
                    glBindTexture(GL_TEXTURE_2D, (unsigned int)mesh.texture);
                }
                textured_shader->Bind();
                renderer->Render(&mesh.vertices[0].x, 0, &mesh.uv[0].s, mesh.colors.data(), mesh.vertices.size());
            }
        }
        color_vertex_shader->Bind();
        if(!frustum_.vertices.empty() && frustum)
            renderer->Render(&frustum_.vertices[0].x, 0, 0, frustum_.colors.data(),
                             frustum_.indices.size(), frustum_.indices.data());

        for (long i : Mesh::TexturesToDelete())
            glDeleteTextures(1, (const GLuint *) &i);
    }

    void Scene::UpdateFrustum(glm::vec3 pos, float zoom) {
        if(frustum_.colors.empty()) {
            frustum_.colors.push_back(0xFFFFFF00);
            frustum_.colors.push_back(0xFFFFFF00);
            frustum_.colors.push_back(0xFFFFFF00);
            frustum_.colors.push_back(0xFFFFFF00);
            frustum_.colors.push_back(0xFFFFFF00);
            frustum_.colors.push_back(0xFFFFFF00);
            frustum_.indices.push_back(0);
            frustum_.indices.push_back(1);
            frustum_.indices.push_back(2);
            frustum_.indices.push_back(0);
            frustum_.indices.push_back(1);
            frustum_.indices.push_back(3);
            frustum_.indices.push_back(0);
            frustum_.indices.push_back(1);
            frustum_.indices.push_back(4);
            frustum_.indices.push_back(0);
            frustum_.indices.push_back(1);
            frustum_.indices.push_back(5);
            frustum_.indices.push_back(2);
            frustum_.indices.push_back(3);
            frustum_.indices.push_back(4);
            frustum_.indices.push_back(2);
            frustum_.indices.push_back(3);
            frustum_.indices.push_back(5);
        }
        if(!frustum_.vertices.empty())
            frustum_.vertices.clear();
        float f = zoom * 0.005f;
        frustum_.vertices.push_back(pos + glm::vec3(f, 0, 0));
        frustum_.vertices.push_back(pos + glm::vec3(-f, 0, 0));
        frustum_.vertices.push_back(pos + glm::vec3(0, 0, f));
        frustum_.vertices.push_back(pos + glm::vec3(0, 0, -f));
        frustum_.vertices.push_back(pos + glm::vec3(0, f, 0));
        frustum_.vertices.push_back(pos + glm::vec3(0, -f, 0));
    }
}
