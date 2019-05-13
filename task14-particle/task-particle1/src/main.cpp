/***
* Task ����ϵͳ 1
*/
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../include/Shader.h"
#include "../include/camera.h"
#include "../include/texture.h"

#include <iostream>
#include <string>
#include <algorithm> //����
using namespace std;

// ���ڴ�С�����Ļص�����(�����ڴ�С�ı�ʱ���ӿ�ҲҪ�ı�)
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
// �����ƻص�
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// ���ֿ��ƻص�
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
// ���̿��ƻص�
void processInput(GLFWwindow *window);

// ��Ļ����
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 5.0f)); //�����λ��

float lastX = (float)SCR_WIDTH / 2.0, lastY = (float)SCR_HEIGHT / 2.0; // ��������ʼλ��Ϊ��Ļ����
bool firstMouse = true;

float deltaTime = 0.0f; // ��ǰ֡����һ֡��ʱ���
float lastFrame = 0.0f; // ��һ֡��ʱ��

// ����
struct Particle {
	glm::vec3 pos, speed;
	unsigned char r, g, b, a; // ��ɫ
	float size; //���Ӵ�С
	float life; // ���ӵ�ʣ��������С��0��ʾ����.
	float cameradistance; // ���Ӻ�����ͷ�ľ���

	bool operator<(const Particle& that) const {
		// �������� Զ����������ǰ��
		return this->cameradistance > that.cameradistance;
	}
};

const int MaxParticles = 200; //���������
const float life = 2.0; //���ӵĴ��ʱ��
glm::vec3 startPos = glm::vec3(0.0f, 0, 0.0f); //�������
glm::vec3 endPos = glm::vec3(0.0f, 0, 4.8f); //�����յ�
Particle ParticlesContainer[MaxParticles];

int LastUsedParticle = 0;
// �����������У��ҵ���������������
int FindUnusedParticle() {
	for (int i = LastUsedParticle; i<MaxParticles; i++) {
		if (ParticlesContainer[i].life < 0) {
			LastUsedParticle = i;
			return i;
		}
	}
	for (int i = 0; i<LastUsedParticle; i++) {
		if (ParticlesContainer[i].life < 0) {
			LastUsedParticle = i;
			return i;
		}
	}
	return 0;
}

// ����cameradistance����������
void SortParticles() {
	std::sort(&ParticlesContainer[0], &ParticlesContainer[MaxParticles]);
}

int main()
{
	// ---------------------��ʼ��--------------------------
	// glfw��ʼ�������õ�GL�汾Ϊ3.3���İ汾
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// ����GL����
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "particle", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// ָ��������Ϊ��ǰ����
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // ���ع�꣬���ͣ���ڴ�����

	// ��ʼ��glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	static GLfloat* particle_position_size_data = new GLfloat[5000];
	static GLubyte* particle_color_data = new GLubyte[5000];
	for (int i = 0; i<MaxParticles; i++) 
	{
		ParticlesContainer[i].life = -1.0f;
		ParticlesContainer[i].cameradistance = -1.0f;
	}

	GLuint particleTexture = Texture::LoadTextureFromFile("res/texture/circle.png");

	// ���Ӷ���λ��
	static const GLfloat g_vertex_buffer_data[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f,
		0.5f,  0.5f, 0.0f,
	};

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	//  ���ӵĶ������� ��ÿ�����Ӷ�һ����
	GLuint billboard_vertex_buffer;
	glGenBuffers(1, &billboard_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

	// ��ɫ��
	Shader shader("res/shader/particle_system.vs", "res/shader/particle_system.fs");

	lastFrame = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		// ��ÿһ֡�м�����µ�deltaTime
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		// �������
		processInput(window);

		// �����ɫ����Ȼ���
		glClearColor(0.0f, 57.0f / 255, 92.0f / 255, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// ���ڴ���������ɫ�������ù���弼����������
		glm::mat4 ProjectionMatrix = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 ViewMatrix = camera.GetViewMatrix();
		glm::vec3 CameraPosition = camera.Position;
		glm::mat4 ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

		// �����������ӣ�������������
		int newparticles = deltaTime / life * MaxParticles;

		for (int i = 0; i<newparticles; i++) {
			int particleIndex = FindUnusedParticle();
			ParticlesContainer[particleIndex].life = life;
			glm::vec3 maindir = glm::vec3(0.0f, 10.0f, 0.0f); //��Ҫ����
			//���������λ��ƫ��
			glm::vec3 randomdOffset = glm::vec3(
				(rand() % 2000 - 1000.0f) / 1500.0f, //[-0.333, 0.333]
				(rand() % 2000 - 1000.0f) / 1500.0f,
				(rand() % 2000 - 1000.0f) / 1500.0f
			);
			ParticlesContainer[particleIndex].pos = startPos + randomdOffset; //�������
			ParticlesContainer[particleIndex].speed = (endPos - startPos) / life;

			// �����������ɫ��͸���ȡ���С
			ParticlesContainer[particleIndex].r = rand() % 256; 
			ParticlesContainer[particleIndex].g = rand() % 256;
			ParticlesContainer[particleIndex].b = rand() % 256;
			ParticlesContainer[particleIndex].a = (rand() % 100) + 50;
			ParticlesContainer[particleIndex].size = (rand() % 1000) / 50000 + 0.04f; //[0.04, 0.06]
		}

		// ģ�����е�����
		int ParticlesCount = 0;
		for (int i = 0; i<MaxParticles; i++) {
			Particle& p = ParticlesContainer[i]; // ����
			if (p.life > 0.0f) {
				p.life -= deltaTime;
				if (p.life > 0.0f) {
					p.pos += p.speed * (float)deltaTime;
					p.cameradistance = glm::length(p.pos - CameraPosition);
					//�������
					particle_position_size_data[4 * ParticlesCount + 0] = p.pos.x;
					particle_position_size_data[4 * ParticlesCount + 1] = p.pos.y;
					particle_position_size_data[4 * ParticlesCount + 2] = p.pos.z;
					particle_position_size_data[4 * ParticlesCount + 3] = p.size;
					particle_color_data[4 * ParticlesCount + 0] = p.r;
					particle_color_data[4 * ParticlesCount + 1] = p.g;
					particle_color_data[4 * ParticlesCount + 2] = p.b;
					particle_color_data[4 * ParticlesCount + 3] = p.a;
				}
				else {
					//�Ѿ����������ӣ��ڵ���SortParticles()֮�󣬻ᱻ������������
					p.cameradistance = -1.0f;
				}
				ParticlesCount++;
			}
		}

		SortParticles();

		//�������
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		shader.Use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, particleTexture);
		shader.SetInt("myTextureSampler", 0);

		// ����ͷ���ҷ���
		shader.SetVec3("CameraRight_worldspace", ViewMatrix[0][0], ViewMatrix[1][0], ViewMatrix[2][0]);
		// ����ͷ���Ϸ���
		shader.SetVec3("CameraUp_worldspace", ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);
		shader.SetMat4("VP", ViewProjectionMatrix);

		glBindVertexArray(VertexArrayID);
		for (int i = 0; i < ParticlesCount; i++)
		{
			shader.SetVec4("xyzs", particle_position_size_data[4 * i + 0],
				particle_position_size_data[4 * i + 1],
				particle_position_size_data[4 * i + 2],
				particle_position_size_data[4 * i + 3]);
			shader.SetVec4("color", particle_color_data[4 * i + 0],
				particle_color_data[4 * i + 1],
				particle_color_data[4 * i + 2],
				particle_color_data[4 * i + 3]);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	delete[] particle_position_size_data;

	glDeleteBuffers(1, &billboard_vertex_buffer);
	glDeleteProgram(shader.ID);
	glDeleteTextures(1, &particleTexture);
	glDeleteVertexArrays(1, &VertexArrayID);

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; 

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

