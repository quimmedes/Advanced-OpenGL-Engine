#pragma once
#include "WindowWin.hpp"
#include "OpenGL.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "Light.hpp"
#include "Ocean.hpp"
#include "CloudSystem.hpp"
#include "OceanCG.hpp"
#include "CloudsCG.hpp"
#include "OceanFFT.hpp"
#include <vector>
#include <memory>

// Forward declaration for AdvancedMaterial to avoid circular dependencies
class AdvancedMaterial;

namespace Engine {

	class App {
		WindowWin* window;
		OpenGL* openGl;
		std::unique_ptr<Camera> camera;
		std::vector<std::unique_ptr<Mesh>> meshes;
		std::vector<std::unique_ptr<Light>> lights;
		
		// Environmental systems
		std::unique_ptr<Ocean> ocean;
		std::unique_ptr<CloudSystem> cloudSystem;
		
		// Computer Graphics book-based systems
		std::unique_ptr<OceanCG> oceanCG;
		std::unique_ptr<CloudsCG> cloudsCG;
		
		// FFT-based ocean system
		std::unique_ptr<OceanFFT> oceanFFT;

	public:
		App();
		~App();
		bool Init();
		bool Tick();

	private:
		bool Render();
		bool LoadAssets();
		bool LoadMeshes();
		void SetupLights();
		
		// Original material configuration methods (for compatibility)
		void ConfigureSceneMaterial(Mesh* mesh);
		void ConfigureCavalryMaterial(Mesh* mesh);
		
		// Advanced material configuration methods
		void ConfigureAdvancedSceneMaterial(Mesh* mesh);
		void ConfigureAdvancedCavalryMaterial(Mesh* mesh);
		
		// Material preset creation utilities
		std::shared_ptr<AdvancedMaterial> CreateTerrainMaterial();
		std::shared_ptr<AdvancedMaterial> CreateArmorMaterial();
		std::shared_ptr<AdvancedMaterial> CreateEnvironmentMaterial(const std::string& type);
		
		// Material configuration methods for terrain
		void ConfigureDisneyTerrainMaterial(Material* material);
		void ConfigureSubsurfaceTerrainMaterial(Material* material);
		void ConfigureBasicPBRTerrainMaterial(Material* material);
		
		// Material configuration methods for armor
		void ConfigureMetalArmorMaterial(Material* material);
		void ConfigureDisneyArmorMaterial(Material* material);
		void ConfigureBasicPBRArmorMaterial(Material* material);
		
		// Environmental system setup
		void SetupOcean();
		void SetupOceanFFT();
		void SetupClouds();
		void UpdateEnvironmentalSystems(float deltaTime);
		
		// Computer Graphics book-based setup
		void SetupOceanCG();
		void SetupCloudsCG();
		void UpdateCGSystems(float deltaTime);
		
		// Helper methods
		std::string GetMaterialTypeName(MaterialType type);
		void LoadAllSceneTextures(Material* material);
		
	public:
		Camera* getCamera() const { return camera.get(); }
		const std::vector<std::unique_ptr<Mesh>>& getMeshes() const { return meshes; }
		const std::vector<std::unique_ptr<Light>>& getLights() const { return lights; }
	};

	extern Engine::App* app;

	bool Init();
	void Release();
}

