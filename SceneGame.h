#pragma once

#include "framework.h"
#include "Scene.h"
#include <vector>
#include <memory>
#include "skinned_mesh.h"

#define P_ACCELE 5.0f

class SceneGame : public Scene
{
public:
	AudioSource* bgmGame[5] = {};

	float kariN = 7.0f;
	float kariScale = 0.2f;
	DirectX::XMFLOAT2 kariPos = { 130.0f,930.0f };;

	// =========================
	// 定数・状態
	// =========================
	const float GOAL_DISTANCE = 4000.0f;
	float speedScale = 0.17f;
	DirectX::XMFLOAT2 speedPos = { 137.0f,942.0f };;

	float MScale = 0.17f;
	DirectX::XMFLOAT2 MPos = { 1569.0f,945.0f };;

	DirectX::XMFLOAT4 attack_c = { 1.0f, 1.0f, 1.0f, 1.0f };


	bool isGoal = false;
	float goalTimer = 0.0f;

	bool miss_played = false;
	int defeatedCount = 0;
	float gameTime = 0.0f;


	// =========================
	// 入力・攻撃
	// =========================
	bool prevLButton = false;
	bool prevRButton = false;

	bool attack_state = false;
	bool attack_hit_enable = false;
	int  attack_timer = 0;

	enum class AttackType { None, Left, Right };
	AttackType attack_type = AttackType::None;

	// =========================
	// カメラ
	// =========================
	enum class CameraMode { Follow, Goal };
	CameraMode cameraMode = CameraMode::Follow;

	DirectX::XMFLOAT3 camera_position{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 camera_focus{ 0.0f, 0.283f, -10.0f };

	DirectX::XMFLOAT3 goalCameraPosition{};
	DirectX::XMFLOAT3 goalCameraFocus{};

	float rotateX = DirectX::XMConvertToRadians(20);
	float rotateY = DirectX::XMConvertToRadians(0);
	float distance = 8.0f;

	DirectX::XMFLOAT4 light_direction{ 0.0, -0.8f, 1.0f, 0.0f };

	// =========================
	// シーン定数バッファ
	// =========================
	struct scene_constants
	{
		DirectX::XMFLOAT4X4 view_projection;
		DirectX::XMFLOAT4   light_direction;
		float time = 0;
		float elapsed_time = 0;
		float pads[2];
	};
	scene_constants data{};

	// =========================
	// サウンド
	// =========================
	AudioSource* SE_PANCHI = nullptr;
	AudioSource* SE_KICK = nullptr;
	AudioSource* SE_MISS = nullptr;

	// =========================
	// 描画・リソース
	// =========================
	enum class SAMPLER_STATE { POINT, LINEAR, ANISOTROPIC, LINEAR_BORDER_BLACK, LINEAR_BORDER_WHITE, LINEAR_CLAMP };
	enum class DEPTH_STATE { ZT_ON_ZW_ON, ZT_ON_ZW_OFF, ZT_OFF_ZW_ON, ZT_OFF_ZW_OFF };
	enum class BLEND_STATE { NONE, ALPHA, ADD, MULTIPLY };
	enum class RASTER_STATE { SOLID, WIREFRAME, CULL_NONE, WIREFRAME_CULL_NONE };

	Microsoft::WRL::ComPtr<ID3D11PixelShader> effect_shaders[2];
	std::unique_ptr<bloom> bloomer;

	// 0:Player 1:Enemy 2:Stage 3:Box 4:Obstacle
	std::unique_ptr<skinned_mesh>  skinned_meshes[15];
	std::unique_ptr<sprite_batch>  sprite_batches[8];

	// =========================
	// スケール
	// =========================
	DirectX::XMFLOAT3 playerScale{ 0.01f,0.01f,0.01f };
	DirectX::XMFLOAT3 enemyScale{ 0.01f,0.01f,0.01f };
	DirectX::XMFLOAT3 stageScale{ 1,1,1 };
	DirectX::XMFLOAT3 rockScale{ 0.02f,0.02f,0.02f };

	// =========================
	// ライフサイクル
	// =========================
	SceneGame(HWND hwnd, framework* fw);
	~SceneGame() override {}

	void Initialize() override;
	void Finalize() override;
	void Update(float elapsedTime) override;
	void Render() override;
	void DrawGUI() override;

	void DrawNumber(int number, float x, float y, float scale, int sukima, ID3D11DeviceContext* ctx);

private:
	// =========================
	// 基本
	// =========================
	const HWND hwnd;
	framework* fw_ = nullptr;

	high_resolution_timer tictoc;
	uint32_t frames = 0;
	float elapsed_time = 0.0f;
	float current_fps = 0.0f;
	float current_frame_time = 0.0f;

	// =========================
	// プレイヤー
	// =========================
	struct Player
	{
		DirectX::XMFLOAT3 position{ 0,0,0 };
		DirectX::XMFLOAT3 velocity{ 0,0,0 };

		int currentLane = 1;
		float laneWidth = 5.0f;
		float laneChangeSpeed = 20.0f;
		float moveSpeed = 15.0f;

		bool isGround = true;
		float gravity = -50.0f;
		float jumpPower = 20.0f;

		animation::keyframe keyframe;
		float animationTime = 0.0f;
		int animationIndex = 0;

		bool wasLeftPressed = false;
		bool wasRightPressed = false;

		float knockbackTimer = 0.0f;
		float knockbackVelocityZ = 0.0f;
	};
	Player player;

	// =========================
	// 敵・ステージ
	// =========================
	enum class EnemyAnimState
	{
		Run,
		Kick,
		Hit
	};

	struct Enemy
	{
		DirectX::XMFLOAT3 position{};

		bool isAlive = true;
		int type = 0;

		// ===== アニメ =====
		EnemyAnimState animState = EnemyAnimState::Run;

		float runAnimTime = 0.0f;
		float kickAnimTime = 0.0f;		
		float hitAnimTime = 0.0f;

		animation::keyframe keyframe;

		// ===== 吹き飛び =====
		bool isBlownAway = false;
		DirectX::XMFLOAT3 blowVelocity{ 0,0,0 };
		float blowAngleX = 0.0f;
		float blowAngleY = 0.0f;
	};

	std::vector<Enemy> enemies;

	struct StageObject { DirectX::XMFLOAT3 position{}; };
	std::vector<StageObject> stages;

	struct Box { float length = 0.0f; };
	Box Boxes;

	const float STAGE_TILE_LENGTH = 260.0f;
	const int   MAX_STAGE_TILES = 3;

	DirectX::XMFLOAT3 stageRotation{ 0,0,0 };

	// =========================
	// アニメーション・演出
	// =========================
	float player_anim_time = 0.0f;
	int   player_anim_index = 0;
	float enemy_anim_time = 0.0f;
	int   enemy_anim_index = 0;

	DirectX::XMFLOAT4 playerColor{ 1,1,1,1 };
	DirectX::XMFLOAT4 enemyColor{ 1,1,1,1 };
	DirectX::XMFLOAT4 stageColor{ 1,1,1,1 };

	bool isSlide = false;
	float slideTimer = 0.0f;
	const float SLIDE_DURATION = 0.6f;

	int slide_anim_index = 0;
	float slide_anim_time = 0.0f;

	float blurFade = 0.0f;
	const float BLUR_FADE_SPEED = 4.0f;

	// Attack / Knock
	bool isAttackAnim = false;
	float attackAnimTimer = 0.0f;
	float attack_anim_time = 0.0f;

	bool isKnockedAnim = false;
	float knockedAnimTimer = 0.0f;
	float knocked_anim_time = 0.0f;

	enum class KnockState { None, Knocked, Recover };
	KnockState knockState = KnockState::None;

	const float KNOCKED_ANIM_SPEED = 1.0f;
	const float RECOVER_ANIM_SPEED = 0.9f;

	float shakeTimer = 0.0f;
	float shakePower = 0.0f;
	float knockBlurTimer = 0.0f;
	float cameraKickBack = 0.0f;

	bool isJumpAnim = false;
	float jump_anim_time = 0.0f;
	float jumpAnimSpeed = 1.0f;

	

	bool isScoreSent = false; // ★追加: スコア送信済みフラグ

	// =========================
	// ヘルパー
	// =========================
	void UpdatePlayer(float elapsedTime);
	void UpdateCamera(float elapsedTime);
	void UpdateStages();
	void CheckCollisions();
	void InputAttack();
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
