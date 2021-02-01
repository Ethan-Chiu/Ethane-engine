#include <Ethane.h>
//-----EntryPoint-----
#include <Ethane/Core/EntryPoint.h>

#include "Platform/OpenGL/OpenGLShader.h"

#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Sandbox2D.h"

class ExampleLayer : public Ethane::Layer
{
public:
	ExampleLayer()
		:Layer("Example"), m_CameraController(1280.0f / 720.0f)
	{
		m_VertexArray = Ethane::VertexArray::Create();

		float vertices[3 * 7] = {
			-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
			 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
			 0.0f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
		};

		Ethane::Ref<Ethane::VertexBuffer> vertexBuffer;
		vertexBuffer.reset(Ethane::VertexBuffer::Create(vertices, sizeof(vertices)));

		Ethane::BufferLayout layout = {
			{ Ethane::ShaderDataType::Float3, "a_Position" },
			{ Ethane::ShaderDataType::Float4, "a_Color" }
		};
		vertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(vertexBuffer);

		uint32_t indices[3] = { 0, 1, 2 };
		Ethane::Ref<Ethane::IndexBuffer> indexBuffer;
		indexBuffer.reset(Ethane::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
		m_VertexArray->SetIndexBuffer(indexBuffer);



		m_SquareVA = Ethane::VertexArray::Create();

		float squareVertices[5 * 4] = {
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 0.0f, 1.0f
		};

		Ethane::Ref<Ethane::VertexBuffer> squareVB;
		squareVB.reset(Ethane::VertexBuffer::Create(squareVertices, sizeof(squareVertices)));

		squareVB->SetLayout({
			{ Ethane::ShaderDataType::Float3, "a_Position" },
			{ Ethane::ShaderDataType::Float2, "a_TexCoord" }
		});
		m_SquareVA->AddVertexBuffer(squareVB);

		uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };
		Ethane::Ref<Ethane::IndexBuffer> squareIB;
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

		m_Shader = Ethane::Shader::Create("VertexPosColor", vertexSrc, fragmentSrc);

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

		m_FlatColorShader = Ethane::Shader::Create("FlatColor", flatColorShaderVertexSrc, flatColorShaderfragmentSrc);

		auto textureShader = m_ShaderLibrary.Load("assets/shaders/Texture.glsl");

		m_Texture = Ethane::Texture2D::Create("assets/textures/test.png");
		m_AlphaTexture = Ethane::Texture2D::Create("assets/textures/test3_rgba.png");

		std::dynamic_pointer_cast<Ethane::OpenGLShader>(textureShader)->Bind();
		std::dynamic_pointer_cast<Ethane::OpenGLShader>(textureShader)->UploadUniformInt("u_Texture", 0);
	}

	void OnUpdate(Ethane::Timestep ts) override
	{
		//ETH_TRACE("Delta time: {0}s ({1}ms)", ts.GetSeconds(), ts.GetMillisecond());

		/*if (Ethane::Input::IsKeyPressed(ETH_KEY_J))
			m_SquarePosition.x -= m_SquareMoveSpeed * ts;
		else if (Ethane::Input::IsKeyPressed(ETH_KEY_L))
			m_SquarePosition.x += m_SquareMoveSpeed * ts;
		if (Ethane::Input::IsKeyPressed(ETH_KEY_I))
			m_SquarePosition.y -= m_SquareMoveSpeed * ts;
		else if (Ethane::Input::IsKeyPressed(ETH_KEY_K))
			m_SquarePosition.y += m_SquareMoveSpeed * ts;*/

		//Update
		m_CameraController.OnUpdate(ts);

		//Render
		Ethane::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
		Ethane::RenderCommand::Clear();

		Ethane::Renderer::BeginScene(m_CameraController.GetCamera());
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

		auto textureShader = m_ShaderLibrary.Get("Texture");

		m_Texture->Bind();
		Ethane::Renderer::Submit(textureShader, m_SquareVA, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));
		m_AlphaTexture->Bind();
		Ethane::Renderer::Submit(textureShader, m_SquareVA, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));

		//triangle
		//Ethane::Renderer::Submit(m_Shader, m_VertexArray);

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

	void OnEvent(Ethane::Event& e) override
	{
		// ETH_TRACE("{0}", event);
		m_CameraController.OnEvent(e);
	}

private:
	Ethane::ShaderLibrary m_ShaderLibrary;

	Ethane::Ref<Ethane::Shader> m_Shader;
	Ethane::Ref<Ethane::VertexArray> m_VertexArray;

	Ethane::Ref<Ethane::Shader> m_FlatColorShader;
	Ethane::Ref<Ethane::VertexArray> m_SquareVA;

	Ethane::Ref<Ethane::Texture2D> m_Texture, m_AlphaTexture;

	Ethane::OrthographicCameraController m_CameraController;

	// glm::vec3 m_SquarePosition;
	// float m_SquareMoveSpeed = 3.0f;

	glm::vec3 m_SquareColor = { 0.2f, 0.3f, 0.8f };
};

class Sandbox : public Ethane::Application
{
public:
	Sandbox()
	{
		// PushLayer(new ExampleLayer());
		PushLayer(new Sandbox2D());
	}
	~Sandbox()
	{

	}
};

Ethane::Application* Ethane::CreateApplication()
{
	return new Sandbox();
}