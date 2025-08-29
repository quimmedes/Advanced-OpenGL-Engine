#include "App.hpp"
#include "AdvancedMaterial.hpp" // Add this include for advanced materials
#include <iostream>

namespace Engine
{
	Engine::App* app = nullptr;
	
	bool Init(){
		app = new Engine::App();
		app->Init();
		return true;
	}
	void Release(){
		if (app) {
			delete app;
			app = nullptr;
		}
	
	}
}

Engine::App::App() : window(nullptr) {
}

Engine::App::~App() {
	// Clean up window if allocated
	delete window;
	window = nullptr;
}

bool Engine::App::Init() {
	window = new WindowWin();
	openGl = new OpenGL();

	if (!window->Init()) {
		return false;
	}
	if (!openGl->Init()) {
		return false;
	}
	
	if (!LoadAssets()) {
		return false;
	}
	
	// Start main loop
	window->Tick();
	return true;
}

bool Engine::App::LoadAssets() {
	// Create camera
	camera = std::make_unique<Camera>();
	
	// Load meshes (they will create their own materials)
	if (!LoadMeshes()) {
		return false;
	}
	
	// Setup lighting
	SetupLights();
	
	// Setup environmental systems (choose one approach)
	// Original systems:
	// SetupOcean();
	// SetupClouds();
	
	// Computer Graphics book-based systems:
	SetupOceanCG();
	SetupCloudsCG();
	
	// FFT-based ocean system:
	//SetupOceanFFT();
	
	return true;
}


bool Engine::App::LoadMeshes() {
	const std::vector<std::string> meshFiles = {
		"cavalry.glb",
		"scene.gltf"
	};
	
	const std::vector<glm::vec3> positions = {
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, -2.0f, 0.0f)
	};
	
	for (size_t i = 0; i < meshFiles.size(); ++i) {
		auto mesh = std::make_unique<Mesh>();
		if (mesh->loadFromFile(meshFiles[i])) {
			if (i < positions.size()) {
				mesh->position = positions[i];
				
			}
			
			// Only apply custom materials if explicitly requested
			// Comment out these lines to use materials loaded from 3D files
			if (meshFiles[i] == "scene.gltf") {
				ConfigureAdvancedSceneMaterial(mesh.get());
			}
			else if (meshFiles[i] == "cavalry.glb") {
				ConfigureAdvancedCavalryMaterial(mesh.get());
			}
			
			// Log material info from loaded file
			if (mesh->getMaterial()) {
				auto* material = mesh->getMaterial();
				std::cout << "=== MATERIAL LOADED FROM " << meshFiles[i] << " ===" << std::endl;
				std::cout << "Albedo: (" << material->getAlbedo().r << ", " << material->getAlbedo().g << ", " << material->getAlbedo().b << ")" << std::endl;
				std::cout << "Metallic: " << material->getMetallic() << std::endl;
				std::cout << "Roughness: " << material->getRoughness() << std::endl;
				std::cout << "Has Diffuse Texture: " << (material->hasDiffuseTexture() ? "Yes" : "No") << std::endl;
				std::cout << "Has Normal Texture: " << (material->hasNormalTexture() ? "Yes" : "No") << std::endl;
				std::cout << "Has Specular Texture: " << (material->hasSpecularTexture() ? "Yes" : "No") << std::endl;
				std::cout << "Has Occlusion Texture: " << (material->hasOcclusionTexture() ? "Yes" : "No") << std::endl;
				std::cout << "===============================================" << std::endl;
			}
			
			meshes.push_back(std::move(mesh));
		}
	}
	
	return !meshes.empty();
}

void Engine::App::ConfigureAdvancedSceneMaterial(Mesh* mesh) {
	if (!mesh || !mesh->getMaterial()) return;
	
	Material* material = mesh->getMaterial();
	
	std::cout << "Configuring Advanced Scene Material..." << std::endl;
	
	// Use advanced PBR shader for realistic rendering with material type support
	material->InitWithShader("shaders/pbr_shadows.vert", "shaders/pbr_shadows_reflections.frag");
	
	// CHOOSE MATERIAL TYPE: Comment/uncomment to switch between different materials
	// Option 1: Disney BRDF Material (currently active)
	//ConfigureDisneyTerrainMaterial(material);
	
	// Option 2: Subsurface Material (for organic terrain)
	// ConfigureSubsurfaceTerrainMaterial(material);
	
	// Option 3: Basic PBR Material
	 ConfigureBasicPBRTerrainMaterial(material);
	
	std::cout << "\n=== TERRAIN MATERIAL OPTIONS ===" << std::endl;
	std::cout << "To switch materials, edit App.cpp lines 115-121:" << std::endl;
	std::cout << "1. Disney BRDF - Advanced subsurface scattering (ACTIVE)" << std::endl;
	std::cout << "2. Subsurface - Realistic organic material" << std::endl;
	std::cout << "3. Basic PBR - Standard metallic/roughness workflow" << std::endl;
	
	// Load ALL available textures for the scene
	std::cout << "Loading ALL available scene textures..." << std::endl;
	LoadAllSceneTextures(material);
	
	std::cout << "Advanced Scene material configuration complete." << std::endl;
	std::cout << "- Material Type: " << GetMaterialTypeName(material->getMaterialType()) << std::endl;
	std::cout << "- Has Advanced Material: " << (material->hasAdvancedMaterial() ? "Yes" : "No") << std::endl;
	std::cout << "Has diffuse texture: " << (material->hasDiffuseTexture() ? "Yes" : "No") << std::endl;
	
	if (!material->hasDiffuseTexture()) {
		std::cout << "WARNING: Diffuse texture failed to load - using debug patterns" << std::endl;
		std::cout << "This is expected if actual image loading failed and fallback textures are being used." << std::endl;
	}
}

void Engine::App::ConfigureAdvancedCavalryMaterial(Mesh* mesh) {
	if (!mesh || !mesh->getMaterial()) return;
	
	Material* material = mesh->getMaterial();
	
	std::cout << "Configuring Advanced Cavalry Material..." << std::endl;
	
	// Use advanced PBR shader for realistic rendering with material type support
	material->InitWithShader("shaders/pbr.vert", "shaders/pbr_shadows_reflections.frag");
	
	// CHOOSE MATERIAL TYPE: Comment/uncomment to switch between different materials
	// Option 1: Metal Material (Realistic metal properties) (currently active)
	//ConfigureMetalArmorMaterial(material);
	
	// Option 2: Disney BRDF Material  
	// ConfigureDisneyArmorMaterial(material);
	
	// Option 3: Basic PBR Material
	 ConfigureBasicPBRArmorMaterial(material);
	
	std::cout << "\n=== CAVALRY ARMOR MATERIAL OPTIONS ===" << std::endl;
	std::cout << "To switch materials, edit App.cpp lines 152-158:" << std::endl;
	std::cout << "1. Metal Material - Realistic gold with complex IOR (ACTIVE)" << std::endl;
	std::cout << "2. Disney BRDF - Advanced clearcoat system" << std::endl;
	std::cout << "3. Basic PBR - Standard metallic workflow" << std::endl;
	
	std::cout << "Advanced Cavalry material configuration complete." << std::endl;
	std::cout << "- Material Type: " << GetMaterialTypeName(material->getMaterialType()) << std::endl;
	std::cout << "- Has Advanced Material: " << (material->hasAdvancedMaterial() ? "Yes" : "No") << std::endl;
}

// Material preset creation utilities
std::shared_ptr<AdvancedMaterial> Engine::App::CreateTerrainMaterial() {
	auto terrainColor = Spectrum::FromRGB(glm::vec3(0.6f, 0.5f, 0.4f));
	return std::make_shared<DisneyMaterial>(
		terrainColor, 0.0f, 0.8f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.2f, 0.0f, 1.5f
	);
}

std::shared_ptr<AdvancedMaterial> Engine::App::CreateArmorMaterial() {
	auto armorColor = Spectrum::FromRGB(glm::vec3(0.7f, 0.6f, 0.5f));
	return std::make_shared<DisneyMaterial>(
		armorColor, 0.9f, 0.1f, 1.0f, 0.0f, 0.0f, 0.5f, 0.1f, 0.9f, 0.0f, 0.0f, 2.5f
	);
}

std::shared_ptr<AdvancedMaterial> Engine::App::CreateEnvironmentMaterial(const std::string& type) {
	if (type == "water") {
		return MaterialFactory::CreateWater(0.01f); // Very smooth water
	}
	else if (type == "metal_gold") {
		return std::shared_ptr<AdvancedMaterial>(MetalMaterial::CreateGold(0.05f));
	}
	else if (type == "metal_copper") {
		return std::shared_ptr<AdvancedMaterial>(MetalMaterial::CreateCopper(0.1f));
	}
	else if (type == "skin") {
		return std::shared_ptr<AdvancedMaterial>(SubsurfaceMaterial::CreateSkin());
	}
	else if (type == "glass") {
		return std::make_shared<GlassMaterial>(Spectrum(0.02f), Spectrum(0.98f), 1.5f, false);
	}
	else {
		// Default plastic material
		auto defaultColor = Spectrum::FromRGB(glm::vec3(0.5f, 0.5f, 0.5f));
		return MaterialFactory::CreatePlastic(defaultColor, 0.5f);
	}
}

// Material configuration methods for terrain
void Engine::App::ConfigureDisneyTerrainMaterial(Material* material) {
	std::cout << "Applying Disney BRDF Material to terrain..." << std::endl;
	
	// Create Disney Material for terrain
	auto terrainColor = Spectrum::FromRGB(glm::vec3(0.6f, 0.5f, 0.4f));
	auto disneyMaterial = std::make_shared<DisneyMaterial>(
		terrainColor, 0.0f, 0.8f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.2f, 0.0f, 1.5f
	);
	
	// Set basic material properties
	glm::vec3 disneyColor = terrainColor.toRGB();
	material->setAlbedo(disneyColor);
	material->setMetallic(0.0f);
	material->setRoughness(0.8f);
	material->setAO(1.0f);
	
	// Attach advanced material
	material->setMaterialType(MaterialType::DISNEY_BRDF);
	material->setAdvancedMaterial(disneyMaterial);
	
	std::cout << "- Disney BRDF with subsurface scattering for organic terrain look" << std::endl;
}

void Engine::App::ConfigureSubsurfaceTerrainMaterial(Material* material) {
	std::cout << "Applying Subsurface Material to terrain..." << std::endl;
	
	// Create subsurface material for organic terrain
	auto subsurfaceMaterial = std::shared_ptr<AdvancedMaterial>(SubsurfaceMaterial::CreateSkin());
	
	material->setAlbedo(glm::vec3(0.6f, 0.5f, 0.4f));
	material->setMetallic(0.0f);
	material->setRoughness(0.7f);
	material->setAO(1.0f);
	
	material->setMaterialType(MaterialType::SUBSURFACE_MATERIAL);
	material->setAdvancedMaterial(subsurfaceMaterial);
	
	std::cout << "- Subsurface scattering for realistic organic terrain" << std::endl;
}

void Engine::App::ConfigureBasicPBRTerrainMaterial(Material* material) {
	std::cout << "Applying Basic PBR Material to terrain..." << std::endl;
	
	material->setAlbedo(glm::vec3(0.6f, 0.5f, 0.4f));
	material->setMetallic(0.0f);
	material->setRoughness(0.8f);
	material->setAO(1.0f);
	
	material->setMaterialType(MaterialType::PBR_BASIC);
	
	std::cout << "- Standard PBR material" << std::endl;
}

// Material configuration methods for cavalry/armor
void Engine::App::ConfigureMetalArmorMaterial(Material* material) {
	std::cout << "Applying Metal Material to armor..." << std::endl;
	
	// Create realistic metal material
	auto metalMaterial = std::shared_ptr<AdvancedMaterial>(MetalMaterial::CreateGold(0.05f));
	
	material->setAlbedo(glm::vec3(0.7f, 0.6f, 0.5f));
	material->setMetallic(1.0f);
	material->setRoughness(0.05f);
	material->setAO(1.0f);
	
	material->setMaterialType(MaterialType::METAL_MATERIAL);
	material->setAdvancedMaterial(metalMaterial);
	
	std::cout << "- Realistic metal with complex IOR and absorption coefficients" << std::endl;
}

void Engine::App::ConfigureDisneyArmorMaterial(Material* material) {
	std::cout << "Applying Disney BRDF Material to armor..." << std::endl;
	
	auto armorColor = Spectrum::FromRGB(glm::vec3(0.7f, 0.6f, 0.5f));
	auto disneyMaterial = std::make_shared<DisneyMaterial>(
		armorColor, 0.9f, 0.1f, 1.0f, 0.0f, 0.0f, 0.5f, 0.1f, 0.9f, 0.0f, 0.0f, 2.5f
	);
	
	glm::vec3 metalColor = armorColor.toRGB();
	material->setAlbedo(metalColor);
	material->setMetallic(0.9f);
	material->setRoughness(0.1f);
	material->setAO(1.0f);
	
	material->setMaterialType(MaterialType::DISNEY_BRDF);
	material->setAdvancedMaterial(disneyMaterial);
	
	std::cout << "- Disney BRDF with clearcoat for protective armor coating" << std::endl;
}

void Engine::App::ConfigureBasicPBRArmorMaterial(Material* material) {
	std::cout << "Applying Basic PBR Material to armor..." << std::endl;
	
	material->setAlbedo(glm::vec3(0.7f, 0.6f, 0.5f));
	material->setMetallic(0.9f);
	material->setRoughness(0.1f);
	material->setAO(1.0f);
	
	material->setMaterialType(MaterialType::PBR_BASIC);
	
	std::cout << "- Standard PBR metallic material" << std::endl;
}

void Engine::App::LoadAllSceneTextures(Material* material) {
	if (!material) return;
	
	std::cout << "\n=== LOADING ALL SCENE TEXTURES ===" << std::endl;
	
	// Terrain textures (primary)
	std::cout << "📁 Loading Terrain texture set..." << std::endl;
	material->setDiffuseTexture("textures_scene/Terrain_diffuse.jpeg");
	material->setNormalTexture("textures_scene/Terrain_normal.png");
	material->setSpecularTexture("textures_scene/Terrain_specularGlossiness.png");
	material->setOcclusionTexture("textures_scene/Terrain_occlusion.png");
	
	// Log texture loading results
	std::cout << "✅ Terrain Diffuse: " << (material->hasDiffuseTexture() ? "LOADED" : "FAILED") << std::endl;
	std::cout << "✅ Terrain Normal: " << (material->hasNormalTexture() ? "LOADED" : "FAILED") << std::endl;
	std::cout << "✅ Terrain Specular: " << (material->hasSpecularTexture() ? "LOADED" : "FAILED") << std::endl;
	std::cout << "✅ Terrain Occlusion: " << (material->hasOcclusionTexture() ? "LOADED" : "FAILED") << std::endl;
	
	// Additional texture sets available (for future use or layered materials)
	std::cout << "\n📋 Additional texture sets detected:" << std::endl;
	std::cout << "   • Bush_Mediteranean_Frond_Mat (diffuse, normal, specularGlossiness)" << std::endl;
	std::cout << "   • Rock (diffuse, normal, occlusion, specularGlossiness)" << std::endl;
	std::cout << "   Note: These can be used for multi-material or layered setups" << std::endl;
	
	// Summary
	int loadedCount = 0;
	if (material->hasDiffuseTexture()) loadedCount++;
	if (material->hasNormalTexture()) loadedCount++;
	if (material->hasSpecularTexture()) loadedCount++;
	if (material->hasOcclusionTexture()) loadedCount++;
	
	std::cout << "\n🎨 TEXTURE LOADING COMPLETE: " << loadedCount << "/4 textures loaded successfully" << std::endl;
	
	if (loadedCount < 4) {
		std::cout << "⚠️  Some textures failed to load - this may be due to file format or path issues" << std::endl;
		std::cout << "   The renderer will use fallback/procedural textures for missing maps" << std::endl;
	} else {
		std::cout << "🌟 All scene textures loaded successfully! Enhanced PBR rendering active." << std::endl;
	}
}

std::string Engine::App::GetMaterialTypeName(MaterialType type) {
	switch (type) {
		case MaterialType::PBR_BASIC: return "Basic PBR";
		case MaterialType::PBR_ADVANCED: return "Advanced PBR";
		case MaterialType::DISNEY_BRDF: return "Disney BRDF";
		case MaterialType::METAL_MATERIAL: return "Realistic Metal";
		case MaterialType::GLASS_MATERIAL: return "Glass";
		case MaterialType::SUBSURFACE_MATERIAL: return "Subsurface Scattering";
		case MaterialType::EMISSIVE_MATERIAL: return "Emissive";
		default: return "Unknown";
	}
}

void Engine::App::SetupLights() {
	std::cout << "Setting up realistic daytime sunny lighting..." << std::endl;
	
	// REALISTIC SUN - Main directional light for bright daytime scene
	auto sunLight = std::make_unique<DirectionalLight>(
		glm::vec3(-0.3f, -0.7f, -0.2f),  // Sun angle from southwest (realistic afternoon sun)
		glm::vec3(1.0f, 0.95f, 0.8f),    // Bright warm sunlight color
		3.0f                              // Moderate intensity for daytime
	);
	sunLight->position = glm::vec3(10.0f, 10.0f, 2.0f);
	lights.push_back(std::move(sunLight));
	
	// SKY LIGHT - Subtle fill light to simulate sky bounce lighting
	auto skyLight = std::make_unique<PointLight>(
		glm::vec3(0.0f, 20.0f, 0.0f),    // High above scene center
		glm::vec3(0.7f, 0.85f, 1.0f),    // Cool sky blue color
		2.0f                              // Moderate sky illumination
	);
	// Very wide spread for sky lighting
	skyLight->constant = 1.0f;
	skyLight->linear = 0.005f;   // Very low attenuation 
	skyLight->quadratic = 0.0001f;
	lights.push_back(std::move(skyLight));
	
	std::cout << "\n=== REALISTIC DAYTIME LIGHTING ===" << std::endl;
	std::cout << "🌞 Sun Direction: (-0.3, -0.7, -0.2) - Southwest afternoon sun" << std::endl;
	std::cout << "🌞 Sun Intensity: 15.0 - Bright daylight" << std::endl;
	std::cout << "🌤️  Sky Light: High ambient fill from above (12.0 intensity)" << std::endl;
	std::cout << "🎨 Background: Bright sky blue (0.6, 0.8, 1.0)" << std::endl;
	std::cout << "💡 Shader Ambient: 0.4 (40% bright daytime ambient)" << std::endl;
	std::cout << "📸 Exposure: 2.5x for outdoor brightness" << std::endl;
	std::cout << "\n✅ Target: Bright sunny outdoor daytime appearance" << std::endl;
	std::cout << "✅ Lighting setup complete - " << lights.size() << " lights configured" << std::endl;
}

bool Engine::App::Tick() {
	// Get actual delta time from OpenGL for frame-rate independent animation
	float deltaTime = openGl->getDeltaTime();
	// UpdateEnvironmentalSystems(deltaTime);  // Original systems
	UpdateCGSystems(deltaTime);  // Book-based systems
	
	return Render();
}

bool Engine::App::Render() {
	// Choose rendering approach
	// Original systems:
	// openGl->Render(camera.get(), meshes, lights, ocean.get(), cloudSystem.get());
	
	// Book-based systems with FFT ocean:
	openGl->Render(camera.get(), meshes, lights, nullptr, nullptr, oceanCG.get(), cloudsCG.get(), oceanFFT.get());
	return true;
}

void Engine::App::SetupOcean() {
	std::cout << "\n=== SETTING UP REALISTIC OCEAN SYSTEM ===" << std::endl;
	
	ocean = std::make_unique<Ocean>();
	
	// Configure ocean for tropical daytime scene
	auto oceanConfig = OceanFactory::CreateTropicalOcean();
	oceanConfig.size = 2000.0f; // Large ocean area
	oceanConfig.resolution = 200; // Good quality for performance balance
	
	// Position ocean below terrain
	oceanConfig.deepColor = glm::vec3(0.0f, 0.25f, 0.5f);    // Deep blue
	oceanConfig.shallowColor = glm::vec3(0.2f, 0.7f, 0.9f);  // Tropical turquoise
	
	if (ocean->Initialize(oceanConfig)) {
		std::cout << "🌊 Ocean system initialized successfully!" << std::endl;
		std::cout << "- Type: Tropical Ocean with Gerstner waves" << std::endl;
		std::cout << "- Wave count: " << oceanConfig.numWaves << std::endl;
		std::cout << "- Resolution: " << oceanConfig.resolution << "x" << oceanConfig.resolution << std::endl;
		std::cout << "- Realistic wave physics with Fresnel reflections" << std::endl;
	} else {
		std::cerr << "❌ Failed to initialize ocean system!" << std::endl;
	}
}

void Engine::App::SetupClouds() {
	std::cout << "\n=== SETTING UP VOLUMETRIC CLOUD SYSTEM ===" << std::endl;
	
	cloudSystem = std::make_unique<CloudSystem>();
	
	// Configure clouds for daytime scene  
	auto cloudConfig = CloudFactory::CreatePartlyCloudy();
	cloudConfig.cloudHeight = 2000.0f;        // High altitude clouds
	cloudConfig.cloudThickness = 600.0f;      // Reasonable thickness
	cloudConfig.cloudCoverage = 0.3f;         // Partly cloudy
	cloudConfig.numSteps = 48;                // Good quality vs performance
	cloudConfig.numLightSteps = 4;            // Faster light sampling
	
	// Set wind for realistic movement
	cloudConfig.windDirection = glm::vec3(1.0f, 0.1f, 0.5f);
	cloudConfig.cloudSpeed = 3.0f;
	
	if (cloudSystem->Initialize(cloudConfig)) {
		std::cout << "☁️ Volumetric cloud system initialized successfully!" << std::endl;
		std::cout << "- Type: Partly Cloudy with ray marching" << std::endl;
		std::cout << "- Ray march steps: " << cloudConfig.numSteps << std::endl;
		std::cout << "- Cloud coverage: " << (cloudConfig.cloudCoverage * 100.0f) << "%" << std::endl;
		std::cout << "- 3D noise textures for realistic cloud shapes" << std::endl;
		std::cout << "- Multiple scattering and realistic lighting" << std::endl;
	} else {
		std::cerr << "❌ Failed to initialize cloud system!" << std::endl;
	}
	
	std::cout << "\n🌅 ENVIRONMENTAL SYSTEMS READY" << std::endl;
	std::cout << "Scene now features realistic ocean waves and volumetric clouds!" << std::endl;
}

void Engine::App::UpdateEnvironmentalSystems(float deltaTime) {
	if (ocean && ocean->IsInitialized()) {
		ocean->Update(deltaTime);
	}
	
	if (cloudSystem && cloudSystem->IsInitialized()) {
		cloudSystem->Update(deltaTime);
	}
}

void Engine::App::SetupOceanCG() {
	std::cout << "\n=== SETTING UP OCEAN (Computer Graphics Book Method) ===" << std::endl;
	
	oceanCG = std::make_unique<OceanCG>();
	
	if (oceanCG->Initialize(80, 200.0f)) {  // 80x80 grid, 200 units size
		// Setup book-style lighting
		auto sunlight = OceanCGFactory::CreateSunlight();
		oceanCG->SetLighting(sunlight);
		
		// Setup water material
		auto waterMaterial = OceanCGFactory::CreateWaterMaterial();
		oceanCG->SetMaterial(waterMaterial);
		
		// Set wave parameters - use rough waves for more dynamic water effects
		auto waves = OceanCGFactory::CreateRoughWaves();
		oceanCG->SetWaveParameters(waves);
		
		// Set ocean colors
		oceanCG->SetOceanColors(
			glm::vec3(0.0f, 0.2f, 0.4f),  // Deep water
			glm::vec3(0.2f, 0.6f, 0.8f)   // Shallow water
		);
		
		oceanCG->SetFresnelPower(2.5f);
		oceanCG->SetGlobalAmbient(glm::vec4(0.3f, 0.4f, 0.5f, 1.0f));
		
		std::cout << "🌊 Ocean (CG Book) initialized successfully!" << std::endl;
		std::cout << "- Sinusoidal wave displacement" << std::endl;
		std::cout << "- Phong reflection model" << std::endl;
		std::cout << "- Procedural foam generation" << std::endl;
		std::cout << "- Fresnel-based water color mixing" << std::endl;
	} else {
		std::cerr << "❌ Failed to initialize Ocean (CG Book)!" << std::endl;
	}
}

void Engine::App::SetupOceanFFT() {
	std::cout << "\n=== SETTING UP FFT OCEAN (Tessendorf Method) ===" << std::endl;
	
	oceanFFT = std::make_unique<OceanFFT>();
	
	// Create high-detail configuration for the FFT ocean
	auto config = OceanFFTFactory::CreateMediumDetailConfig();
	config.oceanSize = 1000.0f;  // Large ocean area
	config.enableChoppiness = true;
	config.enableFoam = true;
	config.foamThreshold = 0.6f;
	
	
	if (oceanFFT->Initialize(config)) {
		// Setup Tessendorf wave parameters - stormy sea for dynamic waves
		auto waveParams = OceanFFTFactory::CreateRoughSea();
		waveParams.windSpeed = glm::vec2(25.0f, 30.0f);
		waveParams.windDirection = glm::vec2(1.0f, 0.8f);
		waveParams.lambda = -1.2f;  // Strong choppiness
		waveParams.A = 0.0008f;     // Good wave amplitude
		
		oceanFFT->SetWaveParameters(waveParams);
		
		std::cout << "🌊 FFT Ocean initialized successfully!" << std::endl;
		std::cout << "- Phillips spectrum wave generation" << std::endl;
		std::cout << "- GPU-accelerated FFT computation" << std::endl;
		std::cout << "- Tessendorf displacement mapping" << std::endl;
		std::cout << "- Real-time foam generation based on wave folding" << std::endl;
		std::cout << "- Resolution: " << config.N << "x" << config.N << std::endl;
		std::cout << "- Ocean size: " << config.oceanSize << " meters" << std::endl;
	} else {
		std::cerr << "❌ Failed to initialize FFT Ocean!" << std::endl;
	}
}

void Engine::App::SetupCloudsCG() {
	std::cout << "\n=== SETTING UP CLOUDS (Computer Graphics Book Method) ===" << std::endl;
	
	cloudsCG = std::make_unique<CloudsCG>();
	
	if (cloudsCG->Initialize()) {
		// Set cloud parameters
		auto cloudParams = CloudsCGFactory::CreatePartlyCloudy();
		cloudsCG->SetCloudParameters(cloudParams);
		
		// Set daytime sky color
		cloudsCG->SetSkyColor(CloudsCGFactory::GetDaySkyColor());
		
		// Set sun direction (matching the lighting)
		cloudsCG->SetLightDirection(glm::vec3(-0.3f, -0.7f, -0.2f));
		
		std::cout << "☁️ Clouds (CG Book) initialized successfully!" << std::endl;
		std::cout << "- Skybox technique from the book" << std::endl;
		std::cout << "- 3D procedural noise (Perlin-like)" << std::endl;
		std::cout << "- Fractal Brownian Motion" << std::endl;
		std::cout << "- Proper depth testing for skybox" << std::endl;
	} else {
		std::cerr << "❌ Failed to initialize Clouds (CG Book)!" << std::endl;
	}
	
	std::cout << "\n🎓 COMPUTER GRAPHICS BOOK SYSTEMS READY" << std::endl;
	std::cout << "Implementing techniques from 'Computer Graphics Programming in OpenGL with C++'" << std::endl;
}

void Engine::App::UpdateCGSystems(float deltaTime) {
	if (oceanCG && oceanCG->IsInitialized()) {
		oceanCG->Update(deltaTime);
	}
	
	if (cloudsCG && cloudsCG->IsInitialized()) {
		cloudsCG->Update(deltaTime);
	}
	
	// Update FFT ocean system
	if (oceanFFT && oceanFFT->IsInitialized()) {
		oceanFFT->Update(deltaTime);
	}
}