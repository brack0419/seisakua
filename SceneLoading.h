#pragma once

#include "framework.h"
#include "System/Sprite.h"
#include "Scene.h"
#include<thread>

//ローディングシーン
class SceneLoading : public Scene
{
public:
	SceneLoading(Scene* nextScene, framework* fw);
	~SceneLoading()override {}

	//初期化
	void Initialize()override;

	//終了化
	void Finalize()override;

	//更新処理
	void Update(float elapsedTime)override;

	//描画処理
	void Render()override;

	//GUI描画
	void DrawGUI()override;

	std::unique_ptr<skinned_mesh> skinned_meshes[8];

	Microsoft::WRL::ComPtr<ID3D11PixelShader> effect_shaders[2];

	enum class SAMPLER_STATE { POINT, LINEAR, ANISOTROPIC, LINEAR_BORDER_BLACK, LINEAR_BORDER_WHITE, LINEAR_CLAMP };
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_states[6];

	enum class DEPTH_STATE { ZT_ON_ZW_ON, ZT_ON_ZW_OFF, ZT_OFF_ZW_ON, ZT_OFF_ZW_OFF };
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_stencil_states[4];

	enum class BLEND_STATE { NONE, ALPHA, ADD, MULTIPLY };
	Microsoft::WRL::ComPtr<ID3D11BlendState> blend_states[4];

	enum class RASTER_STATE { SOLID, WIREFRAME, CULL_NONE, WIREFRAME_CULL_NONE };
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_states[4];

private:
	//ローディングスレッド
	static void LoadingThread(SceneLoading* scene);

private:
	framework* fw_;
	Scene* nextScene = nullptr;
	Sprite* sprite = nullptr;
	high_resolution_timer tictoc;
	uint32_t frames{ 0 };
	float angle = 0.0f;
	std::thread* thread = nullptr;
	float elapsed_time{ 0.0f };
	float current_fps{ 0.0f };
	float current_frame_time{ 0.0f };
	float object1_anim_time = 0.0f;
	int object1_anim_index = 0;
	animation::keyframe object1_keyframe{};
};