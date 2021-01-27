#include <Ethane.h>

#include "Platform/OpenGL/OpenGLShader.h"

#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class ExampleLayer : public Ethane::Layer
{
public:
	ExampleLayer()
		:Layer("Example"), m_Camera(-1.6f, 1.6f, -0.9f, 0.9f), m_CameraPosition(0.0f), m_SquarePosition(0.0f)
	{
		m_VertexArray.reset(Ethane::VertexArray::Create());

		float vertices[3 * 7] = {
			-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
			 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
			 0.0f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
		};

		std::shared_ptr<Ethane::VertexBuffer> vertexBuffer;
		vertexBuffer.reset(Ethane::VertexBuffer::Create(vertices, sizeof(vertices)));

		Ethane::BufferLayout layout = {
			{ Ethane::ShaderDataType::Float3, "a_Position" },
			{ Ethane::ShaderDataType::Float4, "a_Color" }
		};
		vertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(vertexBuffer);

		uint32_t indices[3] = { 0, 1, 2 };
		std::shared_ptr<Ethane::IndexBuffer> indexBuffer;
		indexBuffer.reset(Ethane::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
		m_VertexArray->SetIndexBuffer(indexBuffer);



		m_SquareVA.reset(Ethane::VertexArray::Create());

		float squareVertices[3 * 4] = {
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			 0.5f,  0.5f, 0.0f,
			-0.5f,  0.5f, 0.0f
		};

		std::shared_ptr<Ethane::VertexBuffer> squareVB;
		squareVB.reset(Ethane::VertexBuffer::Create(squareVertices, sizeof(squareVertices)));

		squareVB->SetLayout({
			{ Ethane::ShaderDataType::Float3, "a_Position" }
			});
		m_SquareVA->AddVertexBuffer(squareVB);

		uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };
		std::shared_ptr<Ethane::IndexBuffer> squareIB;
		squareIB.reset(Ethane::IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
		m_SquareVA->SetIndexBuffer(squareIB);


		std::string vertexSrc = R"(
			#version 330 core

			layout(location = 0) in vec3 a_Position;
			layout(location = 1) in vec4 a_Color;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;

			out vec3 v_Position;
			out vec4 v_Color;

			void main()
			{
				v_Position = a_Position;
				v_Color = a_Color;
				gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0); 
			}
		)";

		std::string fragmentSrc = R"(
			#version 330 core

			layout(location = 0) out vec4 color;

			in vec3 v_Position;
			in vec4 v_Color;    

			void main()
			{
				color = vec4(v_Position*0.5 + 0.25, 1.0);
				color = v_Color;
			}
		)";

		m_Shader.reset(Ethane::Shader::Create(vertexSrc, fragmentSrc));

		std::string flatColorShaderVertexSrc = R"(
			#version 330 core

			layout(location = 0) in vec3 a_Position;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;

			out vec3 v_Position;

			void main()
			{
				v_Position = a_Position;
				gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0); 
			}
		)";

		std::string flatColorShaderfragmentSrc = R"(
			#version 330 core

			layout(location = 0) out vec4 color;

			in vec3 v_Position;

			uniform vec3 u_Color;   

			void main()
			{
				color = vec4(u_Color, 1.0);
			}
		)";

		m_FlatColorShader.reset(Ethane::Shader::Create(flatColorShaderVertexSrc, flatColorShaderfragmentSrc));
	}

	void OnUpdate(Ethane::Timestep ts) override
	{
		//ETH_TRACE("Delta time: {0}s ({1}ms)", ts.GetSeconds(), ts.GetMillisecond());

		if (Ethane::Input::IsKeyPressed(ETH_KEY_LEFT))
			m_CameraPosition.x -= m_CameraMoveSpeed * ts;
		else if (Ethane::Input::IsKeyPressed(ETH_KEY_RIGHT))
			m_CameraPosition.x += m_CameraMoveSpeed * ts;
		if (Ethane::Input::IsKeyPressed(ETH_KEY_DOWN))
			m_CameraPosition.y -= m_CameraMoveSpeed * ts;
		else if (Ethane::Input::IsKeyPressed(ETH_KEY_UP))
			m_CameraPosition.y += m_CameraMoveSpeed * ts;

		/*if (Ethane::Input::IsKeyPressed(ETH_KEY_J))
			m_SquarePosition.x -= m_SquareMoveSpeed * ts;
		else if (Ethane::Input::IsKeyPressed(ETH_KEY_L))
			m_SquarePosition.x += m_SquareMoveSpeed * ts;
		if (Ethane::Input::IsKeyPressed(ETH_KEY_I))
			m_SquarePosition.y -= m_SquareMoveSpeed * ts;
		else if (Ethane::Input::IsKeyPressed(ETH_KEY_K))
			m_SquarePosition.y += m_SquareMoveSpeed * ts;*/

		if (Ethane::Input::IsKeyPressed(ETH_KEY_A))
			m_CameraRotation += m_CameraRotationSpeed * ts;
		else if (Ethane::Input::IsKeyPressed(ETH_KEY_D))
			m_CameraRotation -= m_CameraRotationSpeed * ts;

		Ethane::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
		Ethane::RenderCommand::Clear();

		m_Camera.SetPosition(m_CameraPosition);
		m_Camera.SetRotation(m_CameraRotation);

		Ethane::Renderer::BeginScene(m_Camera);
		//
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));

		std::dynamic_pointer_cast<Ethane::OpenGLShader>(m_FlatColorShader)->Bind();
		std::dynamic_pointer_cast<Ethane::OpenGLShader>(m_FlatColorShader)->UploadUniformFloat3("u_Color", m_SquareColor);

		for (int y = 0; y < 20; y++)
		{
			for (int x = 0; x < 20; x++)
			{
				glm::vec3 pos(x * 0.11f, y * 0.11f, 0.0f);
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
				Ethane::Renderer::Submit(m_FlatColorShader, m_SquareVA, transform);
			}
		}

		Ethane::Renderer::Submit(m_Shader, m_VertexArray);
		//
		Ethane::Renderer::EndScene();
	}

	virtual void OnImGuiRender() override
	{
		ImGui::Begin("Settings");
		//ImGui::Text("Hello world");
		ImGui::ColorEdit3("Square Color", glm::value_ptr(m_SquareColor));
		ImGui::End();
	}

	void OnEvent(Ethane::Event& event) override
	{
		// ETH_TRACE("{0}", event);
	}

private:
	std::shared_ptr<Ethane::Shader> m_Shader;
	std::shared_ptr<Ethane::VertexArray> m_VertexArray;

	std::shared_ptr<Ethane::Shader> m_FlatColorShader;
	std::shared_ptr<Ethane::VertexArray> m_SquareVA;

	Ethane::OrthographicCamera m_Camera;
	glm::vec3 m_CameraPosition;
	float m_CameraMoveSpeed = 5.0f;

	float m_CameraRotation = 0.0f;
	float m_CameraRotationSpeed = 30.0f;

	glm::vec3 m_SquarePosition;
	float m_SquareMoveSpeed = 3.0f;

	glm::vec3 m_SquareColor = { 0.2f, 0.3f, 0.8f };
};


class Sandbox : public Ethane::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}
	~Sandbox()
	{

	}
};

Ethane::Application* Ethane::CreateApplication()
{
	return new Sandbox();
}