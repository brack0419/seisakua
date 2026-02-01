#pragma once

#include "framework.h"
#include "Scene.h"
#include "System/Sprite.h"
#include "sprite_batch.h"
#include "Leaderboard.h" // 追加
#include <mutex> // 追加
//==================================================
// タイトルシーン
//==================================================
class SceneTitle : public Scene
{
public:
	int music_Num = 0;

	float kariScale = 0.2f;
	DirectX::XMFLOAT2 kariPos = { 130.0f,930.0f };
	// =========================
	// ctor / dtor
	// =========================
	SceneTitle(framework* fw);
	~SceneTitle() override {}

	// =========================
	// Scene lifecycle
	// =========================
	void Initialize() override;
	void Finalize() override;
	void Update(float elaspedTime) override;
	void Render() override;
	void DrawGUI() override;

	void DrawNumber(int number, float x, float y, float scale, int sukima, ID3D11DeviceContext* ctx);

	// =========================
	// Graphics resources
	// =========================
	AudioSource* bgmGame[5] = {};


	std::unique_ptr<skinned_mesh>  skinned_meshes[15];
	std::unique_ptr<sprite_batch>  sprite_batches[30];
	std::unique_ptr<sprite_batch>  Spr_botan[3];
	std::unique_ptr<sprite_batch>  Spr_music[10];

	Microsoft::WRL::ComPtr<ID3D11PixelShader> effect_shaders[2];

	enum class SAMPLER_STATE
	{
		POINT, LINEAR, ANISOTROPIC,
		LINEAR_BORDER_BLACK, LINEAR_BORDER_WHITE, LINEAR_CLAMP
	};
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_states[6];

	enum class DEPTH_STATE
	{
		ZT_ON_ZW_ON, ZT_ON_ZW_OFF,
		ZT_OFF_ZW_ON, ZT_OFF_ZW_OFF
	};
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_stencil_states[4];

	enum class BLEND_STATE { NONE, ALPHA, ADD, MULTIPLY };
	Microsoft::WRL::ComPtr<ID3D11BlendState> blend_states[4];

	enum class RASTER_STATE
	{
		SOLID, WIREFRAME,
		CULL_NONE, WIREFRAME_CULL_NONE
	};
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_states[4];

	std::unique_ptr<bloom> bloomer;

	//  数字描画用のバッチと関数
	std::unique_ptr<sprite_batch> font_batch;
	void DrawNumber(int number, float x, float y, float scale);

	//  表示位置とサイズ調整用
	DirectX::XMFLOAT2 bestScorePos = { 1500.0f, 100.0f }; // 画面右上あたり
	float bestScoreScale = 0.4f;
	DirectX::XMFLOAT2 bestTimePos = { 1500.0f, 180.0f };
	float bestTimeScale = 0.3f;

	float gameTime = 0.0;
	int defeatedCount = 0;
private:
	// =========================
	// Framework / utility
	// =========================
	framework* fw_ = nullptr;
	Sprite* sprite = nullptr;

	high_resolution_timer tictoc;
	uint32_t frames = 0;
	float elapsed_time = 0.0f;
	float current_fps = 0.0f;
	float current_frame_time = 0.0f;

	// =========================
	// Camera
	// =========================
	float camera_distance = 5.0f;

	// =========================
	// Transform (text / objects)
	// =========================
	DirectX::XMFLOAT3 scale_object4{ 1,1,1 };

	DirectX::XMFLOAT3 text_object{ 3.6f, 1.5f, -15 };
	DirectX::XMFLOAT3 text_object1{ 3.6f, 2.5f, -15 };
	DirectX::XMFLOAT3 text2_object3{ -0.254f, 0.0f, -9.391f };

	DirectX::XMFLOAT3 translation_object4{ 0,0,0 };
	DirectX::XMFLOAT3 rotation_object4{ 0,0,0 };
	float obj2_scale = 1.0f;

	// =========================
	// Animation control
	// =========================
	float sword_bloom_boost = 15.0f;

	float object_anim_time = 0.0f;
	float object2_anim_time = 0.0f;
	float player_anim_time = 0.0f;

	animation::keyframe object_keyframe{};
	animation::keyframe object2_keyframe{};

	int player_anim_index = 0;
	int object_anim_index = 0;
	int object2_anim_index = 0;

	// =========================
	// Player
	// =========================
	struct Player
	{
		DirectX::XMFLOAT3 position{ 0,0,0 };
		DirectX::XMFLOAT3 velocity{ 0,0,0 };

		animation::keyframe keyframe;
		float animationTime = 0.0f;
		int animationIndex = 0;

		bool wasLeftPressed = false;
		bool wasRightPressed = false;
	};
	Player player;

	// =========================
	// Static mesh
	// =========================
	DirectX::XMFLOAT3 static_pos{ 8.3f, -2.9f, -14.5f };
	DirectX::XMFLOAT3 static_rot{ 0.0f, 179, 0.0f };
	float static_scale = 0.033f;

	DirectX::XMFLOAT4 player_color{ 1,1,1,1 };

	// =========================
	// Effect (skinned_meshes[5])
	// =========================
	DirectX::XMFLOAT3 effect_pos{ 1.0f, -2.8f, -7.1f };
	DirectX::XMFLOAT3 effect_rot{ 0.0f, 3.115f, 0.0f };
	float effect_scale = 1.0f;

	// =========================
	// Text animation
	// =========================
	float text_start_y = 0.0f;
	float text_target_y = 0.0f;
	float text_drop_time = 0.0f;
	float text_drop_duration = 1.0f;
	bool  text_dropped = false;

	float text_base_y = 0.0f;
	float text_anim_time = 0.0f;
	float text_float_time = 0.0f;
	float text_float_base_y = 0.0f;

	// =========================
	// Camera shake / impact
	// =========================
	DirectX::XMFLOAT3 camera_base_pos{};
	bool  screen_shake = false;
	float shake_time = 0.0f;
	float shake_power = 0.35f;
	float shake_duration = 0.4f;

	float impact_time = 0.0f;
	float impact_duration = 0.08f;

	// =========================
	// Bloom / flash backup
	// =========================
	float bloom_base_threshold = 0.0f;
	float bloom_base_intensity = 0.0f;
	bool  radial_blur_base = false;

	float flash_alpha = 0.0f;
	DirectX::XMFLOAT4 material_color_backup{};

	// =========================
	// Falling loop
	// =========================
	float base_start_y = 0.0f;
	float fall_duration = 1.2f;
	float fall_time = 0.0f;

	float fall_loop_timer = 0.0f;
	const float fall_loop_interval = 4.0f;

	DirectX::XMFLOAT2 click_min{ 165.0f, 770.0f };
	DirectX::XMFLOAT2 click_max{ 304.0f, 895.0f };

	bool text_falling = false;
	bool landing_impact = false;
	bool title_clicked = false;

	// =========================
	// Click effect
	// =========================
	bool  click_flash = false;
	float click_flash_time = 0.0f;
	const float click_flash_duration = 0.2f;

	float bloom_flash_threshold = 0.0f;
	float bloom_flash_intensity = 0.0f;
	float titleGlowIntensity = 5.0f;
	float base_saturation = 1.0f;

	// =========================
	// Direction / model swap
	// =========================
	bool play_direction = false;
	int  direction_frame = 0;
	int  direction_count = 0;
	const int frameSpan = 7;

	int player_model_indices[3]{ 3,7,8 };
	int current_model_index = 3;

	int play_target_count = 0;
	int play_current_count = 0;

	int model_order[3]{ 0,1,2 };
	int model_order_index = 0;

	// ランキングデータ保持用
	std::vector<RankingData> rankingData;
	std::mutex rankingMutex; // スレッド競合防止用

	// 数字描画用 (SceneGameから持ってくる必要がある)
	void DrawNumber(int number, float x, float y, float scale, ID3D11DeviceContext* ctx);
};
