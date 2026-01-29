#pragma once

#include "framework.h"
#include "Scene.h"
#include <vector>
#include <memory>
#include "skinned_mesh.h"

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
	void Update(float elapsedTime) override;
	void Render() override;
	void DrawGUI() override;

	// フォント描画用
	void DrawNumber(int number, float x, float y, float scale, ID3D11DeviceContext* ctx);

private:
	CONST HWND hwnd;
	framework* fw_;

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
	std::unique_ptr<skinned_mesh> skinned_meshes[6];
	std::unique_ptr<sprite_batch> sprite_batches[1]; // 数字用

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
		float animationTime = 0.0f;

		bool wasLeftPressed = false;
		bool wasRightPressed = false;
		float knockbackTimer = 0.0f;
		float knockbackVelocityZ = 0.0f;
	};
	Player player;

	// 敵・障害物
	struct Enemy
	{
		DirectX::XMFLOAT3 position;
		bool isAlive = true;
		int type = 0; // 0:赤, 1:青, 2:岩
		animation::keyframe keyframe;
		float animationTime = 0.0f;
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
	const float STAGE_TILE_LENGTH = 60.0f;
	const int MAX_STAGE_TILES = 5;

	// アニメーション管理
	float player_anim_time = 0.0f;

	// 関数
	void UpdatePlayer(float elapsedTime);
	void UpdateCamera();
	void UpdateTutorialFlow(float elapsedTime); // チュートリアル進行管理
	void InputAttack();
	void SpawnDummyEnemy();
	void SpawnObstacle();
};