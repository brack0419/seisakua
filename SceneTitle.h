#pragma once

#include "System/Sprite.h"
#include "Scene.h"
#include "framework.h"

//==================================================
// タイトルシーン
//==================================================
class SceneTitle : public Scene
{
public:
	// -------------------------------------------------
	// ctor / dtor
	// -------------------------------------------------
	SceneTitle(framework* fw);
	~SceneTitle() override {}

	// -------------------------------------------------
	// Scene lifecycle
	// -------------------------------------------------
	void Initialize() override;
	void Finalize() override;
	void Update(float elaspedTime) override;
	void Render() override;
	void DrawGUI() override;

	// -------------------------------------------------
	// Graphics resources
	// -------------------------------------------------
	std::unique_ptr<skinned_mesh> skinned_meshes[8];
	Microsoft::WRL::ComPtr<ID3D11PixelShader> effect_shaders[2];

	enum class SAMPLER_STATE
	{
		POINT,
		LINEAR,
		ANISOTROPIC,
		LINEAR_BORDER_BLACK,
		LINEAR_BORDER_WHITE,
		LINEAR_CLAMP
	};
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_states[6];

	enum class DEPTH_STATE
	{
		ZT_ON_ZW_ON,
		ZT_ON_ZW_OFF,
		ZT_OFF_ZW_ON,
		ZT_OFF_ZW_OFF
	};
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_stencil_states[4];

	enum class BLEND_STATE
	{
		NONE,
		ALPHA,
		ADD,
		MULTIPLY
	};
	Microsoft::WRL::ComPtr<ID3D11BlendState> blend_states[4];

	enum class RASTER_STATE
	{
		SOLID,
		WIREFRAME,
		CULL_NONE,
		WIREFRAME_CULL_NONE
	};
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_states[4];

	std::unique_ptr<bloom> bloomer;

private:
	// -------------------------------------------------
	// Framework / utility
	// -------------------------------------------------
	framework* fw_;
	Sprite* sprite = nullptr;

	high_resolution_timer tictoc;
	uint32_t frames{ 0 };
	float elapsed_time{ 0.0f };
	float current_fps{ 0.0f };
	float current_frame_time{ 0.0f };

	// -------------------------------------------------
	// Camera
	// -------------------------------------------------
	float camera_distance = 5.0f;

	// -------------------------------------------------
	// Transform : text / objects
	// -------------------------------------------------
	DirectX::XMFLOAT3 scale_object4{ 1, 1, 1 };

	DirectX::XMFLOAT3 text_object{ 3.6f, 2.5f, -15 };
	DirectX::XMFLOAT3 text_object1{ 3.6f, 2.5f, -15 };
	DirectX::XMFLOAT3 text2_object3{ -0.254f, 0.0f, -9.391f };

	// skinned_meshes[3] 用
	DirectX::XMFLOAT3 translation_object4{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 rotation_object4{ 0.0f, 0.0f, 0.0f };
	float obj2_scale = 1.0f;

	// -------------------------------------------------
	// Animation control
	// -------------------------------------------------
	float sword_bloom_boost = 7.7f;

	float object_anim_time = 0.0f;
	float object2_anim_time = 0.0f;
	float player_anim_time = 0.0f;

	animation::keyframe object_keyframe{};
	animation::keyframe object2_keyframe{};

	int player_anim_index = 0;
	int object_anim_index = 0;
	int object2_anim_index = 0;

	// -------------------------------------------------
	// Player
	// -------------------------------------------------
	struct Player
	{
		DirectX::XMFLOAT3 position{ 0, 0, 0 };
		DirectX::XMFLOAT3 velocity{ 0, 0, 0 };

		animation::keyframe keyframe;
		float animationTime = 0.0f;
		int animationIndex = 0;

		bool wasLeftPressed = false;
		bool wasRightPressed = false;
	};
	Player player;

	// -------------------------------------------------
	// Static mesh transform
	// -------------------------------------------------
	DirectX::XMFLOAT3 static_pos{ 8.3f, -2.9f, -14.5f };
	DirectX::XMFLOAT3 static_rot{ 0.0f, 179, 0.0f };
	float static_scale = 0.033f;

	DirectX::XMFLOAT4 player_color{ 1.0f, 1.0f, 1.0f, 1.0f };

	// -------------------------------------------------
	// Effect (skinned_meshes[5])
	// -------------------------------------------------
	DirectX::XMFLOAT3 effect_pos{ 1.0f, -2.8f, -7.1f };
	DirectX::XMFLOAT3 effect_rot{ 0.0f, 3.115f, 0.0f };
	float effect_scale{ 1.0f };

	float text_start_y = 0.0f;   // 開始Y（上）
	float text_target_y = 0.0f;  // 最終Y（止まる位置）
	float text_drop_time = 0.0f;
	float text_drop_duration = 1.0f; // 落下にかかる秒数
	bool  text_dropped = false;

	float text_base_y = 0.0f;     // 基準Y
	float text_anim_time = 0.0f; // アニメ用時間
	float text_float_time = 0.0f;
	float text_float_base_y = 0.0f;

	DirectX::XMFLOAT3 camera_base_pos;
	bool screen_shake = false;
	float shake_time = 0.0f;
	// 例（ヘッダ or 初期値）
	float shake_power = 0.35f;   // ← 今より大きく（0.15?0.25くらいなら安全）
	float shake_duration = 0.4f;   // ← 少し長め

	float impact_time = 0.0f;
	float impact_duration = 0.08f; // 超短時間（約5フレ）

	// 保存用
	float bloom_base_threshold;
	float bloom_base_intensity;
	bool radial_blur_base;

	// フラッシュ
	float flash_alpha = 0.0f;
	DirectX::XMFLOAT4 material_color_backup;
	// ループ用
	float base_start_y = 0.0f;
	float fall_duration = 1.2f;
	float fall_time = 0.0f;

	float fall_loop_timer = 0.0f;
	const float fall_loop_interval = 4.0f;
	DirectX::XMFLOAT2 click_min = { 165.0f, 770.0f };
	DirectX::XMFLOAT2 click_max = { 304.0f, 895.0f };

	bool text_falling;
	bool landing_impact;
};
