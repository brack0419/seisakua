#pragma once

#include "framework.h"
#include "Scene.h"
#include <vector>
#include <memory>
#include "skinned_mesh.h"

#define P_ACCELE 2.5f

class SceneGame : public Scene
{
public:

	// ★変更: ゴールまでの距離を長く設定（ほぼ無限ランにするため）
	const float GOAL_DISTANCE = 5000.0f;

	DirectX::XMFLOAT4 attack_c = { 1.0f, 1.0f, 1.0f, 1.0f };

	bool miss_played = false;

	bool prevLButton = false;
	bool prevRButton = false;

	bool attack_state = false;
	bool attack_hit_enable = false;
	int attack_timer = 0;

	bool isGoal = false;
	float goalTimer = 0.0f;

	int defeatedCount = 0;
	// ゲーム内タイム計測用
	float gameTime = 0.0;

	enum class AttackType
	{
		None,
		Left,
		Right
	};

	enum class CameraMode
	{
		Follow,
		Goal
	};
	CameraMode cameraMode = CameraMode::Follow;
	DirectX::XMFLOAT3 goalCameraPosition{};
	DirectX::XMFLOAT3 goalCameraFocus{};

	struct scene_constants
	{
		DirectX::XMFLOAT4X4 view_projection;
		DirectX::XMFLOAT4 light_direction;

		// DYNAMIC_TEXTURE
		float time = 0;
		float elapsed_time = 0;
		float pads[2];
	};
	scene_constants data{};

	AttackType attack_type = AttackType::None;

	AudioSource* bgmGame = nullptr;
	AudioSource* SE_PANCHI = nullptr;
	AudioSource* SE_KICK = nullptr;
	AudioSource* SE_MISS = nullptr;

	SceneGame(HWND hwnd, framework* fw);
	~SceneGame() override {}

	void Initialize()override;

	void Finalize()override;

	void Update(float elapsedTime)override;

	void Render()override;

	void DrawGUI()override;

	void SceneGame::DrawNumber(int number, float x, float y, float scale, ID3D11DeviceContext* ctx);

	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11PixelShader> effect_shaders[2];

	enum class SAMPLER_STATE { POINT, LINEAR, ANISOTROPIC, LINEAR_BORDER_BLACK, LINEAR_BORDER_WHITE, LINEAR_CLAMP };
	enum class DEPTH_STATE { ZT_ON_ZW_ON, ZT_ON_ZW_OFF, ZT_OFF_ZW_ON, ZT_OFF_ZW_OFF };
	enum class BLEND_STATE { NONE, ALPHA, ADD, MULTIPLY };
	enum class RASTER_STATE { SOLID, WIREFRAME, CULL_NONE, WIREFRAME_CULL_NONE };

	// カメラパラメータ
	DirectX::XMFLOAT3 camera_position{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 light_direction{ 0.0, -0.8f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 camera_focus{ 0.0f, 0.283f, -10.0f };
	float rotateX{ DirectX::XMConvertToRadians(20) };
	float rotateY{ DirectX::XMConvertToRadians(0) };
	float distance{ 8.0f }; // プレイヤーとの距離

	std::unique_ptr<bloom> bloomer;

	// メッシュ管理
	// 0: Player, 1: Enemy, 2: Stage, 3: Box, 4: Obstacle
	std::unique_ptr<skinned_mesh> skinned_meshes[6];

	std::unique_ptr<sprite_batch> sprite_batches[8];

	DirectX::XMFLOAT3 playerScale{ 0.01f, 0.01f, 0.01f };
	DirectX::XMFLOAT3 enemyScale{ 0.01f, 0.01f, 0.01f };
	DirectX::XMFLOAT3 stageScale{ 3.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 rockScale{ 0.02f, 0.02f, 0.02f }; // 岩のサイズ

private:
	CONST HWND hwnd;
	framework* fw_;

	high_resolution_timer tictoc;
	uint32_t frames{ 0 };
	float elapsed_time{ 0.0f };
	float current_fps{ 0.0f };
	float current_frame_time{ 0.0f };

	// プレイヤー構造体
	struct Player
	{
		DirectX::XMFLOAT3 position{ 0, 0, 0 };
		DirectX::XMFLOAT3 velocity{ 0, 0, 0 };

		int currentLane = 1;      // 0:左, 1:中央, 2:右
		float laneWidth = 5.0f;   // レーン幅
		float laneChangeSpeed = 20.0f; // 横移動速度
		float moveSpeed = 15.0f;  // 前進速度

		// ジャンプ関連
		bool isGround = true;
		float gravity = -50.0f;
		float jumpPower = 20.0f;

		// アニメーション関連
		animation::keyframe keyframe;
		float animationTime = 0.0f;
		int animationIndex = 0;

		// 入力状態
		bool wasLeftPressed = false;
		bool wasRightPressed = false;

		float knockbackTimer = 0.0f;
		float knockbackVelocityZ = 0.0f;
	};
	Player player;

	// 敵構造体
	struct Enemy
	{
		DirectX::XMFLOAT3 position;
		bool isAlive = true;
		int type = 0; // 0:赤, 1:青, 2:岩(障害物)
		animation::keyframe keyframe;
		float animationTime = 0.0f;
	};
	std::vector<Enemy> enemies;

	// ステージ床構造体
	struct StageObject
	{
		DirectX::XMFLOAT3 position;
	};
	std::vector<StageObject> stages;

	struct Box
	{
		float length;
	};
	Box Boxes;

	// ゲーム内定数
	// ★変更: ここでステージモデル1個分の長さを定義します。
	// モデルのサイズに合わせて調整してください (例: 40.0f ~ 100.0f)
	const float STAGE_TILE_LENGTH = 260.0f;

	// ★変更: 表示するステージタイルの枚数を増やす（ループさせるため）
	const int MAX_STAGE_TILES = 3;

	// ステージ回転（ラジアン）
	DirectX::XMFLOAT3 stageRotation = { 0.0f, 0.0f, 0.0f };
	float player_anim_time = 0.0f;
	int   player_anim_index = 0;
	float enemy_anim_time = 0.0f;
	int   enemy_anim_index = 0;
	DirectX::XMFLOAT4 playerColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 enemyColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 stageColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	// ヘルパー関数
	void UpdatePlayer(float elaspedTime);
	void UpdateCamera();
	void UpdateStages();
	void CheckCollisions();
	void InputAttack();
	// 敵生成ヘルパー
	void SpawnEnemy(float zPosition);

	void calculate_frame_stats()
	{
		++frames;
		float time = tictoc.time_stamp();
		if (time - elapsed_time >= 1.0f)
		{
			current_fps = static_cast<float>(frames);
			current_frame_time = 1000.0f / current_fps;

			frames = 0;
			elapsed_time += 1.0f;
		}
	}
};