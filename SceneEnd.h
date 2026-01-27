#pragma once

#include"System/Sprite.h"
#include "Scene.h"
#include "framework.h"
#include "sprite.h" 
#include "sprite_batch.h"


//タイトルシーン
class SceneEnd : public Scene
{
public:
	SceneEnd(framework* fw, float time, int enemyCount);
	~SceneEnd()override {}

	//初期化
	void Initialize()override;

	//終了化
	void Finalize()override;

	//更新処理
	void Update(float elaspedTime)override;

	//描画処理
	void Render()override;

	//GUI描画
	void DrawGUI()override;

	std::unique_ptr<skinned_mesh> skinned_meshes[8];

	std::unique_ptr<sprite> sprite_end;

	std::unique_ptr<sprite_batch> font_batch;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> effect_shaders[2];

	enum class SAMPLER_STATE { POINT, LINEAR, ANISOTROPIC, LINEAR_BORDER_BLACK, LINEAR_BORDER_WHITE, LINEAR_CLAMP };
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_states[6];

	enum class DEPTH_STATE { ZT_ON_ZW_ON, ZT_ON_ZW_OFF, ZT_OFF_ZW_ON, ZT_OFF_ZW_OFF };
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_stencil_states[4];

	enum class BLEND_STATE { NONE, ALPHA, ADD, MULTIPLY };
	Microsoft::WRL::ComPtr<ID3D11BlendState> blend_states[4];

	enum class RASTER_STATE { SOLID, WIREFRAME, CULL_NONE, WIREFRAME_CULL_NONE };
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_states[4];

	std::unique_ptr<bloom> bloomer;

	bool flat_shading = false;

	DirectX::XMFLOAT2 timePosition = { 1325.0f,404.0f };

	float timeScale = 0.22f;
	int defeatedEnemies = 0;

	float speedScale = 0.27f;
	DirectX::XMFLOAT2 speedPos = { 288.0f,624.0f };;

	DirectX::XMFLOAT2 scorePosition = { 332.0f, 386.0f };
	float scoreScale = 0.33f;

	DirectX::XMFLOAT4 camera_position{ 0.0f, 0.0f, -10.0f, 1.0f };
	DirectX::XMFLOAT4 light_direction{ 0.0f, 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 camera_focus{ 0.0f, 0.283f, -10.0f };
	float rotateX{ DirectX::XMConvertToRadians(5) };
	float rotateY{ DirectX::XMConvertToRadians(90) };
	POINT cursor_position;
	float wheel{ 0 };
	float distance{ 0.1f };

	DirectX::XMFLOAT3 center_of_rotation;
	DirectX::XMFLOAT3 scaling{ 1, 1, 1 };

	DirectX::XMFLOAT4 material_color{ 1 ,1, 1, 1 };
	DirectX::XMFLOAT3 rotation{ 0.0f, 3.115f, 0.0f };
	DirectX::XMFLOAT3 translation{ -0.254f, 0.0f, -9.391f };

	// End_building
	DirectX::XMFLOAT4 material_color1{ 1 ,1, 1, 1 };
	DirectX::XMFLOAT3 rotation_object3{ 0.0f, 1.4f, 0.0f };
	DirectX::XMFLOAT3 translation_object3{ -18.5f, -32.68f, 26.0f };

	// End_building2
	DirectX::XMFLOAT4 material_color2{ 1 ,1, 1, 1 };
	DirectX::XMFLOAT3 rotation_object4{ 0.0f, 1.4f, 0.0f };
	DirectX::XMFLOAT3 translation_object4{ -17.0f, -20.0f, 26.0f };


	DirectX::XMFLOAT3 model_position_man{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 model_position_walk{ 0.0f, 0.0f, 0.0f };

	void DrawNumber(int number, float x, float y, float scale);

private:
	framework* fw_;
	high_resolution_timer tictoc;
	uint32_t frames{ 0 };
	float elapsed_time{ 0.0f };
	float current_fps{ 0.0f };
	float current_frame_time{ 0.0f };

	float camera_distance = 5.0f;

	// クリアタイム保存用
	float clearTime = 0.0f;
};