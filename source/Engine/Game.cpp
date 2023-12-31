//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include <iostream>


//toreorganise
#include <fstream>

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace ImGui;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
	//const XMVECTORF32 SCENE_BOUNDS = { 100.f, 20.f, 100.f, 0.f };
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
#ifdef DXTK_AUDIO
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }
#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
	FILE* fp;
	AllocConsole();
	freopen_s(&fp, "CONIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);


	m_input.Initialise(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

	//setup imgui.  its up here cos we need the window handle too
	//pulled from imgui directx11 example
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window);		//tie to our window
	ImGui_ImplDX11_Init(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext());	//tie to directx

	m_fullscreenRect.left = 0;
	m_fullscreenRect.top = 0;
	m_fullscreenRect.right = 800;
	m_fullscreenRect.bottom = 600;

	m_CameraViewRect.left = 500;
	m_CameraViewRect.top = 0;
	m_CameraViewRect.right = 800;
	m_CameraViewRect.bottom = 240;

	//setup light
	m_Light.setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
	m_Light.setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	m_Light.setPosition(2.0f, 1.0f, 1.0f);
	m_Light.setDirection(-1.0f, -1.0f, 0.0f);

	//setup camera
	m_Camera01.setRotation(Vector3(90.0f, -300.0f, 0.0f));	//orientation is -90 becuase zero will be looking up at the sky straight up. 

	m_Player.SetPosition(m_Camera01.getPosition());
	m_Player.SetRadius(2.f);
	m_Player.SetCoinsCollected(0);
	m_Player.SetGrounded(true);
	m_Player.SetJumping(false);

	m_postProcess = std::make_unique<BasicPostProcess>(m_deviceResources->GetD3DDevice());
	m_postProcessMode = 0;

	GenerateCollectables();
	
#ifdef DXTK_AUDIO
    // Create DirectXTK for Audio objects
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    m_audEngine = std::make_unique<AudioEngine>(eflags);

    m_audioEvent = 0;
    m_audioTimerAcc = 10.f;
    m_retryDefault = false;

    m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

    m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"MusicMono_adpcm.wav");
    m_effect1 = m_soundEffect->CreateInstance();
    m_effect2 = m_waveBank->CreateInstance(10);

    m_effect1->Play(true);
    m_effect2->Play();
#endif
}

#pragma region Collectables
void Game::RenderCollectables(ID3D11DeviceContext* context)
{
	// Render coins 
	for (Collectable coin : m_Coins)
	{
		m_world = Matrix::Identity;

		// Applies a bobbing motion to the coins 
		float newY = sin(m_timer.GetTotalSeconds() * 3.f) * .25f;
		Matrix position = Matrix::CreateTranslation(Vector3(coin.GetPosition().x, coin.GetPosition().y + newY, coin.GetPosition().z));

		Matrix scale = Matrix::CreateScale(2.f);

		// Applies a constant rotation to the coins 
		Matrix rotate = Matrix::CreateRotationY(m_timer.GetTotalSeconds());
		m_world = m_world * rotate * scale * position;

		// Setup and draw cube
		m_BasicShaderPair.EnableShader(context);
		m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_goldTex.Get());

		// Only render the coin if it has not been collected by the player 
		if (!coin.GetCollected())
		{
			m_CoinModel.Render(context);
		}
	}
}

void Game::GenerateCollectables()
{
	// Generate coins if there are currently no coins (on initialisation or when player has collected all coins)
	if (m_Coins.size() == 0)
	{
		m_Player.SetCoinsCollected(0);
		for (int i = 0; i < 10; i++)
		{
			Collectable coin;
			// Generate a pseudo random position along the X and Z-axis'
			int randX = rand() % (120 - 5 + 1) + 5;
			int randZ = rand() % (120 - 5 + 1) + 5;
			coin.SetPosition(Vector3(randX, 1.f, randZ));
			// Set radius for sphere collider 
			coin.SetRadius(2.f);
			// Check for collisions with terrain
			m_Terrain.CheckCollectableCollision(coin);
			// Add to vector
			m_Coins.push_back(coin);
		}
	}
}

void Game::CheckCoinsCollection()
{
	for (auto it = m_Coins.begin(); it != m_Coins.end(); ++it)
	{
		// If the coin has not been collected and the collider of it intersects the collider of the player
		if (!it->GetCollected() && it->GetCollider().Intersects(m_Player.GetCollider()))
		{
			it->SetCollected(true);
			m_Player.SetCoinsCollected(m_Player.GetCoinsCollected() + 1);
			// Remove the coin from the vector of coins 
			m_Coins.erase(it);
			return;
		}
	}
}
#pragma endregion

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
	//take in input
	m_input.Update();								//update the hardware
	m_gameInputCommands = m_input.getGameInput();	//retrieve the input for our game
	
	//Update all game objects
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

	//Render all game content. 
    Render();

#ifdef DXTK_AUDIO
    // Only update audio engine once per frame
    if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
#endif	
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{	
	//this is hacky,  i dont like this here.  
	auto device = m_deviceResources->GetD3DDevice();

	if (m_gameInputCommands.left)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.y = rotation.y += m_Camera01.getRotationSpeed() * m_timer.GetElapsedSeconds();
		m_Camera01.setRotation(rotation);
	}
	if (m_gameInputCommands.right)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.y = rotation.y -= m_Camera01.getRotationSpeed() * m_timer.GetElapsedSeconds();
		m_Camera01.setRotation(rotation);
	}
	if (m_gameInputCommands.forward)
	{
		Vector3 position = m_Camera01.getPosition(); //get the position
		position += (m_Camera01.getForward() * m_Camera01.getMoveSpeed()) * m_timer.GetElapsedSeconds(); //add the forward vector
		m_Player.SetPosition(position);
		m_Camera01.setPosition(m_Player.GetPosition());
	}
	if (m_gameInputCommands.back)
	{
		Vector3 position = m_Camera01.getPosition(); //get the position
		position -= (m_Camera01.getForward() * m_Camera01.getMoveSpeed()) * m_timer.GetElapsedSeconds(); //add the forward vector
		m_Player.SetPosition(position);
		m_Camera01.setPosition(m_Player.GetPosition());
	}

	// Commented out but used for testing - can uncomment to fly around the terrain
	//if (m_gameInputCommands.up)
	//{
	//	Vector3 position = m_Camera01.getPosition(); //get the position
	//	position += (DirectX::SimpleMath::Vector3::Up * m_Camera01.getMoveSpeed()) * m_timer.GetElapsedSeconds(); //add the forward vector
	//	m_Player.SetPosition(position);
	//	m_Camera01.setPosition(m_Player.GetPosition());
	//}
	//if (m_gameInputCommands.down)
	//{
	//	Vector3 position = m_Camera01.getPosition(); //get the position
	//	position -= (DirectX::SimpleMath::Vector3::Up * m_Camera01.getMoveSpeed()) * m_timer.GetElapsedSeconds(); //add the forward vector
	//	m_Player.SetPosition(position);
	//	m_Camera01.setPosition(m_Player.GetPosition());
	//}

	// Player input for jump
	if (m_gameInputCommands.jump)
	{
		// Only allow player to jump if they are not currently jumping and are on the ground
		if (!m_Player.GetJumping() && m_Player.GetGrounded())
		{
				m_Player.SetJumping(true);
				Vector3 position = m_Camera01.getPosition();

				// Apply upwards force to player
				position.y += 65.f * m_timer.GetElapsedSeconds();

				m_Player.SetPosition(position);
				m_Camera01.setPosition(m_Player.GetPosition());
		}
	}

	// Apply gravity to the player/camera 
	m_Camera01.setPosition(Vector3(m_Camera01.getPosition().x, m_Camera01.getPosition().y - 3.f * m_timer.GetElapsedSeconds(), m_Camera01.getPosition().z)); 
	m_Player.SetPosition(m_Camera01.getPosition());

	// Player input for terrain generation
	if (m_gameInputCommands.generate)
	{
		// Set the camera and player to above the new amplitude of the terrain so they don't end up underneath the terrain
		m_Camera01.setPosition(Vector3(m_Camera01.getPosition().x, m_Camera01.getPosition().y + *m_Terrain.GetAmplitude() + 3.f, m_Camera01.getPosition().z));
		m_Player.SetPosition(m_Camera01.getPosition());

		// Set the coins positions to above the new amplitude of the terrain so they don't end up underneath the terrain
		for (Collectable coin : m_Coins)
		{
			coin.SetPosition(Vector3(coin.GetPosition().x, coin.GetPosition().y + *m_Terrain.GetAmplitude() + 6.f, coin.GetPosition().z));
		}

		// Generate the new heightmap
		m_Terrain.SimplexNoiseHeightMap(device);

		// Run collision detection between coins and terrain 
		for (Collectable coin : m_Coins)
		{
			m_Terrain.CheckCollectableCollision(coin);
		}		
	}	

	m_Player.Update();		//player update.   doesn't do anything at the moment.
	m_Terrain.Update();		//terrain update.  doesnt do anything at the moment. 

	m_Terrain.CheckCollisions(&m_Camera01, &m_Player);
	// Check for collisions between the player and coins for collecting them
	CheckCoinsCollection();

	m_view = m_Camera01.getCameraMatrix();
	m_world = Matrix::Identity;

	/*create our UI*/
	SetupGUI();

#ifdef DXTK_AUDIO
    m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
    if (m_audioTimerAcc < 0)
    {
        if (m_retryDefault)
        {
            m_retryDefault = false;
            if (m_audEngine->Reset())
            {
                // Restart looping audio
                m_effect1->Play(true);
            }
        }
        else
        {
            m_audioTimerAcc = 4.f;

            m_waveBank->Play(m_audioEvent++);

            if (m_audioEvent >= 11)
                m_audioEvent = 0;
        }
    }
#endif

	if (m_input.Quit())
	{
		ExitGame();
	}
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{	
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();

    // Draw Text to the screen
    m_sprites->Begin();
		m_font->DrawString(m_sprites.get(), L"Procedural Methods", XMFLOAT2(10, 10), Colors::Yellow);
    m_sprites->End();

	//Set Rendering states. 
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->RSSetState(m_states->CullClockwise());
	//context->RSSetState(m_states->Wireframe());
	RenderTexturePass();

	//prepare transform for floor object. 
	m_world = SimpleMath::Matrix::Identity; //set world back to identity
	SimpleMath::Matrix newPosition3 = SimpleMath::Matrix::CreateTranslation(0.0f, -1.6f, 0.0f);
	m_world = m_world * newPosition3;

	//setup and draw terrain
	m_TerrainShaderPair.EnableShader(context);
	m_TerrainShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get(), m_texture2.Get(), m_texture3.Get());
	m_Terrain.Render(context);

	RenderCollectables(context);
	
	// Handle post processing effects based on GUI button presses
	if (m_postProcessMode == 0)
	{
		context->OMSetRenderTargets(1, &renderTargetView, nullptr);
		m_postProcess->SetSourceTexture(m_FirstRenderPass->getShaderResourceView());
		m_postProcess->SetEffect(BasicPostProcess::Copy);
		m_postProcess->Process(context);
	}
	else if (m_postProcessMode == 1)
	{
		context->OMSetRenderTargets(1, &renderTargetView, nullptr);
		m_postProcess->SetSourceTexture(m_FirstRenderPass->getShaderResourceView());
		m_postProcess->SetBloomBlurParameters(true, 12.f, 5.f);
		m_postProcess->SetEffect(BasicPostProcess::BloomBlur);
		m_postProcess->Process(context);
	}
	else if (m_postProcessMode == 2)
	{
		context->OMSetRenderTargets(1, &renderTargetView, nullptr);
		m_postProcess->SetSourceTexture(m_FirstRenderPass->getShaderResourceView());
		m_postProcess->SetEffect(BasicPostProcess::Monochrome);
		m_postProcess->Process(context);
	}
	else if (m_postProcessMode == 3)
	{
		context->OMSetRenderTargets(1, &renderTargetView, nullptr);
		m_postProcess->SetSourceTexture(m_FirstRenderPass->getShaderResourceView());
		m_postProcess->SetEffect(BasicPostProcess::Sepia);
		m_postProcess->Process(context);
	}
	else if (m_postProcessMode == 4)
	{
		context->OMSetRenderTargets(1, &renderTargetView, nullptr);
		m_postProcess->SetSourceTexture(m_FirstRenderPass->getShaderResourceView());
		m_postProcess->SetGaussianParameter(1.3f);
		m_postProcess->SetEffect(BasicPostProcess::GaussianBlur_5x5);
		m_postProcess->Process(context);
	}

	// Camera update (added here for 'late update' - smoothing)
	m_Camera01.Update();	

	//render our GUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());	

    // Show the new frame.
    m_deviceResources->Present();
}

/*
* Method to render scene as a texture for post processing
*/
void Game::RenderTexturePass()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();

	m_FirstRenderPass->setRenderTarget(context);
	m_FirstRenderPass->clearRenderTarget(context, 0.39f, 0.58f, 0.93f, 1.f);	// cornflour blue colour for sky

	// Draw Text to the screen
	m_sprites->Begin();
	m_font->DrawString(m_sprites.get(), L"Procedural Methods", XMFLOAT2(10, 10), Colors::Yellow);
	m_sprites->End();

	//Set Rendering states. 
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->RSSetState(m_states->CullClockwise());
	//context->RSSetState(m_states->Wireframe());

	//prepare transform for floor object. 
	m_world = SimpleMath::Matrix::Identity; //set world back to identity
	SimpleMath::Matrix newPosition3 = SimpleMath::Matrix::CreateTranslation(0.0f, -1.6f, 0.0f);
	m_world = m_world * newPosition3;

	//setup and draw cube
	m_TerrainShaderPair.EnableShader(context);
	m_TerrainShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get(), m_texture2.Get(), m_texture3.Get());
	m_Terrain.Render(context);

	RenderCollectables(context);

	//render our GUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	context->OMSetRenderTargets(1, &renderTargetView, depthTargetView);
}


// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}

#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
#ifdef DXTK_AUDIO
    m_audEngine->Suspend();
#endif
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

#ifdef DXTK_AUDIO
    m_audEngine->Resume();
#endif
}

void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
    if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
}
#endif

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 800;
    height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

    m_states = std::make_unique<CommonStates>(device);
    m_fxFactory = std::make_unique<EffectFactory>(device);
    m_sprites = std::make_unique<SpriteBatch>(context);
    m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");
	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

	//setup our terrain
	m_Terrain.Initialize(device, 129, 129);

	// Set up coin model
	m_CoinModel.InitializeModel(device, "Models/coin.obj");

	//load and set up our Vertex and Pixel Shaders 
	m_BasicShaderPair.InitStandard(device, L"light_vs.cso", L"light_ps.cso");
	m_TerrainShaderPair.InitStandard(device, L"terrain_vs.cso", L"terrain_ps.cso");

	//load Textures
	CreateDDSTextureFromFile(device, L"Textures/dirt_tex.dds", nullptr, m_texture1.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Textures/grass_tex_1.dds",		nullptr,	m_texture2.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Textures/snow_tex.dds", nullptr,	m_texture3.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Textures/gold_tex.dds", nullptr, m_goldTex.ReleaseAndGetAddressOf());

	//Initialise Render to texture
	m_FirstRenderPass = new RenderTexture(device, 800, 600, 1, 2);	//for our rendering, We dont use the last two properties. but.  they cant be zero and they cant be the same. 
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    float aspectRatio = float(size.right) / float(size.bottom);
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    m_projection = Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        0.01f,
        100.0f
    );

	m_FirstRenderPass = new RenderTexture(m_deviceResources->GetD3DDevice(), size.right - size.left, size.bottom - size.top, 1, 2);
}

void Game::SetupGUI()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("CMP505 Assignment");
		ImGui::Text("Terrain Values");
		ImGui::SliderFloat("Wave Amplitude", m_Terrain.GetAmplitude(), 0.0f, 3.0f);
		ImGui::SliderFloat("Wavelength", m_Terrain.GetWavelength(), 0.0f, 1.0f);
		ImGui::Text("Post Processing Effects");
		if (ImGui::Button("No Effect"))
		{
			m_postProcessMode = 0;
		}
		if (ImGui::Button("Bloom"))
		{
			m_postProcessMode = 1;
		}
		if (ImGui::Button("Greyscale"))
		{
			m_postProcessMode = 2;
		}
		if (ImGui::Button("Sepia"))
		{
			m_postProcessMode = 3;
		}
		if (ImGui::Button("Blur"))
		{
			m_postProcessMode = 4;
		}
		ImGui::Text("Controls");
		ImGui::Indent();
			ImGui::BulletText("Press 'Enter' to generate the terrain");
			ImGui::BulletText("Use 'W' and 'E' to move forwards and backwards");
			ImGui::BulletText("Use 'A' and 'D' to rotate left and right");
			ImGui::BulletText("Press 'Space' to jump");
			ImGui::BulletText("Press 'Esc' to quit");
		ImGui::Unindent();
	ImGui::End();

	ImGui::Begin("Coins");
	ImGui::Text("Coins collected: %u", m_Player.GetCoinsCollected());
	if (ImGui::Button("Generate Coins"))
	{
		GenerateCollectables();
	}
	ImGui::End();
}

void Game::OnDeviceLost()
{
    m_states.reset();
    m_fxFactory.reset();
    m_sprites.reset();
    m_font.reset();
	m_batch.reset();
	m_testmodel.reset();
    m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion
