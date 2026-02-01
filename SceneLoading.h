#pragma once

#include "framework.h"
#include "System/Sprite.h"
#include "Scene.h"
#include<thread>
#include "skinned_mesh.h"

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

	std::unique_ptr<skinned_mesh> skinned_meshes[15];

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
	static constexpr int MESH_COUNT = 7;
	float dist;
	float anim_times[MESH_COUNT]{};
	int anim_indices[MESH_COUNT]{};
	animation::keyframe keyframes[MESH_COUNT];
	float modelScale = 0.1f;   // ← 全モデル共通スケール
	// 位置・速度・浮遊用
	DirectX::XMFLOAT3 modelPos[MESH_COUNT];
	DirectX::XMFLOAT3 modelVel[MESH_COUNT];
	float floatPhase[MESH_COUNT];
	static constexpr int INSTANCE_COUNT = 33;

	struct FloatingInstance
	{
		int meshIndex;                 // どのFBXを使うか
		DirectX::XMFLOAT3 centerPos;   // 浮遊の中心
		DirectX::XMFLOAT3 velocity;    // ゆっくり動く用
		float phase;                   // sin波用
	};

	FloatingInstance instances[INSTANCE_COUNT];
	// mesh[8] 専用
	DirectX::XMFLOAT3 mesh8Pos{ -12.5f, 0.0f, 15.0f };
	DirectX::XMFLOAT3 mesh8Rotation = { 0.0f, 3.0f, 0.0f };  // 初期値：回転なし (Pitch, Yaw, Roll)

	float mesh8Scale = 5.5f;

	// mesh[8] 専用（ローディングロゴ）
	int   mesh8AnimFrame = 0;            // 常に0でOK
	animation::keyframe mesh8Keyframe;              // 現在のフレームデータ格納
	float mesh8AnimTime = 0.0f;          // アニメーション再生時間

	// 演出用（任意）
	float mesh8FloatPhase = 0.0f;
	float loading_bloom_boost = 3.0f;
};