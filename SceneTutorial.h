#pragma once

#include "framework.h"
#include "Scene.h"
#include <vector>
#include <memory>
#include "skinned_mesh.h"
#include <algorithm> // for min/max/clamp

// 定数定義 (SceneGameと同様)
#define P_ACCELE 2.5f

// チュートリアルの進行状態
enum class TutorialStep
{
	Welcome,    // ようこそ
	Move,       // 移動の説明
	Jump,       // ジャンプの説明
	Attack,     // 攻撃の説明
	Complete    // 完了
};

class SceneTutorial : public Scene
{
public:
	SceneTutorial(HWND hwnd, framework* fw);
	~SceneTutorial() override {}

	void Initialize() override;
	void Finalize() override;
	void Update(float elapsed_time) override;
	void Render() override;
	void DrawGUI() override;

	// フォント描画用
	void DrawNumber(int number, float x, float y, float scale, ID3D11DeviceContext* ctx);

private:
	CONST HWND hwnd;
	framework* fw_;
	enum class SAMPLER_STATE { POINT, LINEAR, ANISOTROPIC, LINEAR_BORDER_BLACK, LINEAR_BORDER_WHITE, LINEAR_CLAMP };
	enum class DEPTH_STATE { ZT_ON_ZW_ON, ZT_ON_ZW_OFF, ZT_OFF_ZW_ON, ZT_OFF_ZW_OFF };
	enum class BLEND_STATE { NONE, ALPHA, ADD, MULTIPLY };
	enum class RASTER_STATE { SOLID, WIREFRAME, CULL_NONE, WIREFRAME_CULL_NONE };
	std::unique_ptr<bloom> bloomer;
	// 進行状態管理
	TutorialStep currentStep = TutorialStep::Welcome;
	float stepTimer = 0.0f;
	bool actionDone = false; // アクションを行ったか判定フラグ

	// 共通リソース（ゲーム画面と同じ）
	struct scene_constants
	{
		DirectX::XMFLOAT4X4 view_projection;
		DirectX::XMFLOAT4 light_direction;
		float time = 0;
		float elapsed_time = 0;
		float pads[2];
	};
	scene_constants data{};

	// カメラ
	DirectX::XMFLOAT3 camera_position{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 camera_focus{ 0.0f, 0.0f, 0.0f };
	float rotateX{ DirectX::XMConvertToRadians(20) };
	float rotateY{ DirectX::XMConvertToRadians(0) };
	float distance{ 8.0f };

	// メッシュ
	// 0:PlayerRun 1:EnemyRed 2:Stage 3:Box 4:Obstacle 
	// 5:EnemyBlue 6:Slide 7:Attack 8:Knocked 9:Recover 10:Jump 11:EnemyKick
	std::unique_ptr<skinned_mesh> skinned_meshes[15];
	std::unique_ptr<sprite_batch> sprite_batches[15]; // 数字用

	// 音声
	AudioSource* SE_PANCHI = nullptr;
	AudioSource* SE_KICK = nullptr;
	AudioSource* SE_MISS = nullptr;

	// プレイヤー
	struct Player
	{
		DirectX::XMFLOAT3 position{ 0, 0, 0 };
		DirectX::XMFLOAT3 velocity{ 0, 0, 0 };
		int currentLane = 1;      // 0:Left, 1:Center, 2:Right
		float laneWidth = 5.0f;
		float laneChangeSpeed = 20.0f;
		float moveSpeed = 10.0f; // チュートリアルなので少し遅め

		bool isGround = true;
		float gravity = -50.0f;
		float jumpPower = 20.0f;

		animation::keyframe keyframe;

		bool wasLeftPressed = false;
		bool wasRightPressed = false;
		float knockbackTimer = 0.0f;
		float knockbackVelocityZ = 0.0f;
	};
	Player player;

	// アニメーション・演出用変数 (SceneGameから移植)
	float player_anim_time = 0.0f;
	int   player_anim_index = 0;

	// スライド
	bool isSlide = false;
	float slideTimer = 0.0f;
	int slide_anim_index = 0;
	float slide_anim_time = 0.0f;

	// 攻撃アニメ
	bool isAttackAnim = false;
	float attackAnimTimer = 0.0f;
	float attack_anim_time = 0.0f;

	// ジャンプアニメ
	bool isJumpAnim = false;
	float jump_anim_time = 0.0f;

	// ダメージ・復帰アニメ
	bool isKnockedAnim = false;
	float knockedAnimTimer = 0.0f;
	float knocked_anim_time = 0.0f;
	enum class KnockState { None, Knocked, Recover };
	KnockState knockState = KnockState::None;

	// 定数 (SceneGameから移植)
	const float KNOCKED_ANIM_SPEED = 1.0f;
	const float RECOVER_ANIM_SPEED = 0.9f;

	// 演出用
	float shakeTimer = 0.0f;
	float shakePower = 0.0f;
	float knockBlurTimer = 0.0f;
	float cameraKickBack = 0.0f;
	float blurFade = 0.0f;
	const float BLUR_FADE_SPEED = 4.0f;
	// 敵・障害物
	enum class EnemyAnimState
	{
		Run,
		Kick
	};

	struct Enemy
	{
		DirectX::XMFLOAT3 position;
		bool isAlive = true;
		int type = 0; // 0:赤, 1:青, 2:岩

		animation::keyframe keyframe;
		float animationTime = 0.0f;
		EnemyAnimState animState = EnemyAnimState::Run;
		bool isHitAnim = false;
		float runAnimTime = 0.0f;
		float kickAnimTime = 0.0f;
		float hitAnimTime = 0.0f;

		bool isBlownAway = false;                 // 吹き飛び中フラグ
		DirectX::XMFLOAT3 blowVelocity{ 0, 0, 0 }; // 吹き飛び速度
		float blowAngleX = 0.0f;                  // 回転角度
		float blowAngleY = 0.0f;
	};
	std::vector<Enemy> enemies;

	// ステージ
	struct StageObject
	{
		DirectX::XMFLOAT3 position;
	};
	std::vector<StageObject> stages;

	struct Box { float length; };
	Box Boxes;

	// 攻撃関連
	bool prevLButton = false;
	bool prevRButton = false;
	bool attack_state = false;
	bool attack_hit_enable = false;
	int attack_timer = 0;
	int attack_type = 0; // 1:Left, 2:Right
	DirectX::XMFLOAT4 attack_c = { 1,1,1,1 };
	bool miss_played = false;

	// 定数
	const float STAGE_TILE_LENGTH = 180.0f;
	const int MAX_STAGE_TILES = 4;

	// 色
	DirectX::XMFLOAT4 playerColor{ 1,1,1,1 };
	DirectX::XMFLOAT3 playerScale{ 0.02f, 0.02f, 0.02f };
	DirectX::XMFLOAT3 enemyScale{ 0.02f,0.02f,0.02f };
	DirectX::XMFLOAT3 rockScale{ 0.12f, 0.12f, 0.12f };
	// スプライト位置（ImGuiで調整用）
	DirectX::XMFLOAT2 spritePos[5] =
	{
		{ -91.0f,   -212.0f },   // sprite_batches[1]
		{1011.0f, -203.0f },   // sprite_batches[2]
		{ -91.0f,   -212.0f}, // sprite_batches[3]
		{1011.0f, -203.0f },  // sprite_batches[4]
		{400, 0 }  // sprite_batches[4]
	};
	float tutorialSpriteScale = 0.5f;
	float tutorialTextTimer = 0.0f;
	float tutorialTextScale = 0.8f;
	TutorialStep prevStep;

	// 関数
	void UpdatePlayer(float elapsed_time);
	void UpdateCamera(float elapsed_time);
	void UpdateTutorialFlow(float elapsed_time); // チュートリアル進行管理
	void InputAttack();
	void SpawnDummyEnemy();
	void SpawnObstacle();
};