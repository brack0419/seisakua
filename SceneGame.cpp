#include "shader.h"
#include "texture.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneEnd.h"
#include "framework.h"
#include "SceneGame.h"
#include <algorithm>
#include <cmath>
#undef min
#undef max
#include "Leaderboard.h"

float saveSpeed = 0.0f;

int globalMaxKills = 0;       // 最高キル数
float globalBestTime = 0.0f;  // 最速タイム

extern int saveNum;

SceneGame::SceneGame(HWND hwnd, framework* fw) : hwnd(hwnd), fw_(fw)
{
}

// 初期化
void SceneGame::Initialize()
{
	HRESULT hr{ S_OK };
	ShowCursor(FALSE);
	// ---------------------------------------------------------
	// モデルの読み込み
	// ---------------------------------------------------------

	skinned_meshes[0] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\player_run.cereal");
	skinned_meshes[1] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\enemy_red_run.cereal");
	skinned_meshes[2] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\main_stage.cereal");
	skinned_meshes[4] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\Obstacles.cereal");
	skinned_meshes[5] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\enemy_blue_run.cereal");
	skinned_meshes[6] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\player_slide.cereal");
	skinned_meshes[7] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\player_attack.cereal");
	skinned_meshes[8] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\player_knocked.cereal");
	skinned_meshes[9] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\player_up.cereal");
	skinned_meshes[10] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\player_jump.cereal");
	skinned_meshes[11] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\enemy_red_kick.cereal");

	sprite_batches[0] = std::make_unique<sprite_batch>(fw_->device.Get(), L".\\resources\\fontN.png", 1);
	sprite_batches[1] = std::make_unique<sprite_batch>(fw_->device.Get(), L".\\resources\\Sprspeed.png", 1);
	sprite_batches[2] = std::make_unique<sprite_batch>(fw_->device.Get(), L".\\resources\\SprM.png", 1);

	auto& audio = Audio::Instance();
	SE_PANCHI = audio.LoadAudioSource(".\\resources\\panchi.wav");
	SE_KICK = audio.LoadAudioSource(".\\resources\\kick.wav");
	SE_MISS = audio.LoadAudioSource(".\\resources\\miss.wav");
	bgmGame[0] = audio.LoadAudioSource(".\\resources\\スティックマンの冒険.wav");
	bgmGame[1] = audio.LoadAudioSource(".\\resources\\スティックマンの伝説.wav");
	bgmGame[2] = audio.LoadAudioSource(".\\resources\\Legends of Stickman.wav");
	if (bgmGame) bgmGame[saveNum]->Play(true);

	// スケール設定
	playerScale = { 0.02f, 0.02f, 0.02f };
	enemyScale = { 0.02f, 0.02f, 0.02f };
	stageScale = { 0.3f, 0.3f, 0.3f };
	rockScale = { 0.12f, 0.12f, 0.12f };

	// プレイヤー初期化
	player.position = { 0, 0, 0 };
	player.currentLane = 1; // 中央
	player.isGround = true;
	player.knockbackTimer = 0.0f;
	player.knockbackVelocityZ = 0.0f;
	player.moveSpeed = (P_ACCELE * 3);

	gameTime = 0.0f;

	fw_->light_direction = DirectX::XMFLOAT4{ 0.0f, -1.0f, 1.0f, 0.0f };

	//fw_->radial_blur_data.blur_strength = 0.1f;
	//fw_->radial_blur_data.blur_radius = 1.0f;
	//fw_->radial_blur_data.blur_decay = 0.0f;

	fw_->radial_blur_data.blur_strength = 0.0f;
	fw_->radial_blur_data.blur_radius = 0.0f;
	fw_->radial_blur_data.blur_decay = 0.0f;

	// ---------------------------------------------------------
	// ステージ・敵の初期配置
	// ---------------------------------------------------------
	stages.clear();
	enemies.clear();

	for (int i = 0; i < MAX_STAGE_TILES; ++i)
	{
		StageObject s;
		s.position = { 0, 0, i * STAGE_TILE_LENGTH };
		stages.push_back(s);

		// ★変更点: i > 1 の条件を外し、最初のタイルから敵生成を試みる
		// ただしSpawnEnemy内でスタート地点付近(z < 25)は弾くようにする
		SpawnEnemy(s.position.z);
	}
}

// 終了化
void SceneGame::Finalize()
{
	// カーソルを再表示
	ShowCursor(TRUE);

	// メッシュの解放
	for (auto& m : skinned_meshes)
		m.reset();

	// スプライトバッチの解放
	for (auto& batch : sprite_batches)
		batch.reset();

	// オーディオの解放
	if (bgmGame[saveNum])
	{
		bgmGame[saveNum]->Stop();  // BGM停止
		delete bgmGame[saveNum];   // メモリ解放
		bgmGame[saveNum] = nullptr;  // ポインタをNULLに設定
	}

	// ステージオブジェクトや敵オブジェクトなどの解放
	stages.clear();  // ステージオブジェクトの解放
	enemies.clear(); // 敵オブジェクトの解放
}

// 更新処理
void SceneGame::Update(float elapsed_time)
{
	gameTime += elapsed_time;

	if (saveSpeed <= player.moveSpeed)
	{
		saveSpeed = player.moveSpeed;
	}

	UpdatePlayer(elapsed_time);

	// ゴール判定
	if (player.position.z >= GOAL_DISTANCE)
	{
		isGoal = true;

		// カメラを固定
		cameraMode = CameraMode::Goal;

		goalCameraFocus = player.position;
		goalCameraFocus.y += 2.0f;

		goalCameraPosition = camera_position; // 現在の位置を保存

		goalTimer += elapsed_time;

		if (goalTimer >= 5.0f)
		{
			SceneManager::instance().ChangeScene(new SceneEnd(fw_, gameTime, defeatedCount));
		}
		return;
	}

	// 攻撃処理
	InputAttack();
	for (auto& enemy : enemies)
	{
		if (enemy.isBlownAway)
		{
			// 重力で落下させる
			enemy.blowVelocity.y -= 40.0f * elapsed_time;

			// 速度分移動させる
			enemy.position.x += enemy.blowVelocity.x * elapsed_time;
			enemy.position.y += enemy.blowVelocity.y * elapsed_time;
			enemy.position.z += enemy.blowVelocity.z * elapsed_time;

			// クルクル回転させる
			enemy.blowAngleX += 720.0f * elapsed_time;
			enemy.blowAngleY += 360.0f * elapsed_time;

			// 地面よりだいぶ下（-20）まで落ちたら完全に消滅させる
			if (enemy.position.y < -20.0f)
			{
				enemy.isAlive = false;
			}
		}
	}

	// ステージ管理（無限生成）
	UpdateStages();

	// カメラ更新
	UpdateCamera(elapsed_time);

	// 衝突判定
	CheckCollisions();
}

void SceneGame::UpdatePlayer(float elapsed_time)
{
	if (player.knockbackTimer > 0.0f)
	{
		player.knockbackTimer -= elapsed_time;
		player.position.z += player.knockbackVelocityZ * elapsed_time;
		player.knockbackVelocityZ += 20.0f * elapsed_time;
		if (player.knockbackVelocityZ > 0.0f) player.knockbackVelocityZ = 0.0f;
	}

	if (player.knockbackTimer <= 0.0f)
	{
		bool isLeftDown = (GetAsyncKeyState('A') & 0x8000) || (GetAsyncKeyState(VK_LEFT) & 0x8000);
		bool isRightDown = (GetAsyncKeyState('D') & 0x8000) || (GetAsyncKeyState(VK_RIGHT) & 0x8000);

		bool onLeftTrigger = isLeftDown && !player.wasLeftPressed;
		bool onRightTrigger = isRightDown && !player.wasRightPressed;

		player.wasLeftPressed = isLeftDown;
		player.wasRightPressed = isRightDown;

		if (onLeftTrigger && player.currentLane > 0) player.currentLane--;
		if (onRightTrigger && player.currentLane < 2) player.currentLane++;

		float targetX = (player.currentLane - 1) * player.laneWidth;
		float diff = targetX - player.position.x;
		if (fabsf(diff) > 0.01f)
		{
			float moveStep = (diff > 0 ? 1.0f : -1.0f) * player.laneChangeSpeed * elapsed_time;
			if (fabsf(moveStep) > fabsf(diff)) player.position.x = targetX;
			else player.position.x += moveStep;
		}
		else
		{
			player.position.x = targetX;
		}

		player.position.z += player.moveSpeed * elapsed_time;

		if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && player.isGround && !attack_state)
		{
			player.velocity.y = player.jumpPower;
			player.isGround = false;

			isJumpAnim = true;
			jump_anim_time = 0.0f;
		}
	}
	else
	{
		player.wasLeftPressed = false;
		player.wasRightPressed = false;
	}

	player.velocity.y += player.gravity * elapsed_time;
	player.position.y += player.velocity.y * elapsed_time;

	// 接地判定
	if (player.position.y <= 0.0f)
	{
		player.position.y = 0.0f;
		player.velocity.y = 0.0f;
		player.isGround = true;
	}

	// アニメーション更新
	if (!skinned_meshes[0]->animation_clips.empty())
	{
		animation& anim = skinned_meshes[0]->animation_clips[player_anim_index];

		player_anim_time += elapsed_time;

		int frame =
			static_cast<int>(player_anim_time * anim.sampling_rate);

		// ループ
		if (frame >= static_cast<int>(anim.sequence.size()))
		{
			frame = 0;
			player_anim_time = 0.0f;
		}

		player.keyframe = anim.sequence[frame];
	}
	if (skinned_meshes[1]->animation_clips.empty()) return;

	animation& anim = skinned_meshes[1]->animation_clips[0];

	for (auto& enemy : enemies)
	{
		if (!enemy.isAlive || enemy.type == 2) continue;

		skinned_mesh* mesh = nullptr;
		float* time = nullptr;

		// -------------------------
		// 状態ごとに完全分離
		// -------------------------
		if (enemy.animState == EnemyAnimState::Kick)
		{
			mesh = skinned_meshes[11].get(); // キック
			time = &enemy.kickAnimTime;
		}
		else
		{
			mesh = skinned_meshes[enemy.type == 0 ? 1 : 5].get(); // 通常走り
			time = &enemy.runAnimTime;
		}

		if (!mesh || mesh->animation_clips.empty()) continue;

		animation& anim = mesh->animation_clips[0];

		// -----------------------------
		// 時間更新
		// -----------------------------
		*time += elapsed_time;
		int frame = static_cast<int>(*time * anim.sampling_rate);
		// -----------------------------
		// 再生終了処理
		// -----------------------------
		if (frame >= (int)anim.sequence.size())
		{
			if (enemy.animState == EnemyAnimState::Kick)
			{
				// ★ キック終了 → 走りへ
				enemy.animState = EnemyAnimState::Run;
				enemy.kickAnimTime = 0.0f;
				enemy.runAnimTime = 0.0f;

				// 【修正】キーフレームを即座に「走りモーション」の初期状態に更新する
				// これをしないと、Renderで「走りモデル」に対して「キックのキーフレーム」を使ってしまい、
				// ボーン数が合わずに out_of_range エラーになる
				int runMeshIndex = (enemy.type == 0 ? 1 : 5);
				if (skinned_meshes[runMeshIndex] && !skinned_meshes[runMeshIndex]->animation_clips.empty())
				{
					enemy.keyframe = skinned_meshes[runMeshIndex]->animation_clips[0].sequence[0];
				}

				continue;
			}
			else
			{
				*time = 0.0f;
				frame = 0;
			}
		}

		// -----------------------------
		// keyframe は1回だけ
		// -----------------------------
		enemy.keyframe = anim.sequence[frame];
	}

	// ===============================
	// スライドアニメーション + ブラーフェード
	// ===============================
	if (isSlide && skinned_meshes[6] && !skinned_meshes[6]->animation_clips.empty())
	{
		// --- ブラー最大 ---
		blurFade = 1.0f;

		fw_->radial_blur_data.blur_strength = 0.1f * blurFade;
		fw_->radial_blur_data.blur_radius = 1.0f * blurFade;
		fw_->radial_blur_data.blur_decay = 0.0f;

		slideTimer += elapsed_time;

		animation& anim = skinned_meshes[6]->animation_clips[0];
		slide_anim_time += elapsed_time;

		int frame = static_cast<int>(slide_anim_time * anim.sampling_rate);
		if (frame >= static_cast<int>(anim.sequence.size()))
		{
			frame = static_cast<int>(anim.sequence.size()) - 1;
		}

		player.keyframe = anim.sequence[frame];

		// ★変更点2: スライド状態の継続時間を短くする (例: 1.5f → 0.75f)
		const float SLIDE_DURATION = 0.75f;

		if (slideTimer >= SLIDE_DURATION)
		{
			isSlide = false;
			slideTimer = 0.0f;
			slide_anim_time = 0.0f;
		}

		return; // ★ Runアニメを更新させない
	}

	// --- スライドしていない時：ブラーをなめらかに消す ---
	blurFade -= elapsed_time * BLUR_FADE_SPEED;
	blurFade = std::clamp(blurFade, 0.0f, 1.0f);

	// ★ ease-out を適用
	float eased = 1.0f - powf(1.0f - blurFade, 2.0f);

	fw_->radial_blur_data.blur_strength = 0.2f * eased;
	fw_->radial_blur_data.blur_radius = 2.0f * eased;
	fw_->radial_blur_data.blur_decay = 0.0f;

	// ===============================
// Attackアニメーション（左クリック）
// ===============================
	if (isAttackAnim && skinned_meshes[7] && !skinned_meshes[7]->animation_clips.empty())
	{
		attackAnimTimer += elapsed_time;

		animation& anim = skinned_meshes[7]->animation_clips[0];

		// ★ ここだけ変更
		const float ATTACK_ANIM_SPEED = 1.5f;
		attack_anim_time += elapsed_time * ATTACK_ANIM_SPEED;

		int frame = static_cast<int>(attack_anim_time * anim.sampling_rate);
		if (frame >= static_cast<int>(anim.sequence.size()))
		{
			frame = static_cast<int>(anim.sequence.size()) - 1;
		}

		player.keyframe = anim.sequence[frame];

		const float ATTACK_DURATION = 0.7f;

		if (attackAnimTimer >= ATTACK_DURATION)
		{
			isAttackAnim = false;
			attackAnimTimer = 0.0f;
			attack_anim_time = 0.0f;
		}

		return;
	}
	// ===============================
	// Knocked / Recover アニメーション
	// ===============================
	if (knockState != KnockState::None)
	{
		// ---------- Knocked ----------
		if (knockState == KnockState::Knocked &&
			skinned_meshes[8] &&
			!skinned_meshes[8]->animation_clips.empty())
		{
			animation& anim = skinned_meshes[8]->animation_clips[0];

			knocked_anim_time += elapsed_time * KNOCKED_ANIM_SPEED;
			int frame = static_cast<int>(knocked_anim_time * anim.sampling_rate);

			if (frame >= static_cast<int>(anim.sequence.size()) - 1)
			{
				// ★ 即 Recover に移行（止めない）
				knockState = KnockState::Recover;
				knocked_anim_time =
					(knocked_anim_time * anim.sampling_rate -
						(anim.sequence.size() - 1)) / anim.sampling_rate;
			}
			else
			{
				player.keyframe = anim.sequence[frame];
				return;
			}
		}

		// ---------- Recover ----------
		if (knockState == KnockState::Recover &&
			skinned_meshes[9] &&
			!skinned_meshes[9]->animation_clips.empty())
		{
			animation& anim = skinned_meshes[9]->animation_clips[0];

			knocked_anim_time += elapsed_time * RECOVER_ANIM_SPEED;
			int frame = static_cast<int>(knocked_anim_time * anim.sampling_rate);

			if (frame >= static_cast<int>(anim.sequence.size()))
			{
				knockState = KnockState::None;
				knocked_anim_time =
					(knocked_anim_time * anim.sampling_rate -
						(anim.sequence.size() - 1)) / anim.sampling_rate;
				return;
			}

			player.keyframe = anim.sequence[frame];
			return;
		}
	}
	if (knockBlurTimer > 0.0f)
	{
		knockBlurTimer -= elapsed_time;

		float k = knockBlurTimer / 0.3f; // 1→0
		fw_->radial_blur_data.blur_strength = 0.35f * k;
		fw_->radial_blur_data.blur_radius = 1.5f * k;
		fw_->radial_blur_data.blur_decay = 0.0f;
	}
	// ===============================
	// Jump アニメーション（時間指定）
	// ===============================
	if (isJumpAnim && skinned_meshes[10] && !skinned_meshes[10]->animation_clips.empty())
	{
		animation& anim = skinned_meshes[10]->animation_clips[0];

		const float JUMP_ANIM_DURATION = 1.0f; // ← ここで伸ばす！

		jump_anim_time += elapsed_time;

		// 0〜1 に正規化
		float t = jump_anim_time / JUMP_ANIM_DURATION;
		t = std::clamp(t, 0.0f, 1.0f);

		int frame = static_cast<int>(t * anim.sequence.size());
		if (frame >= static_cast<int>(anim.sequence.size()))
		{
			frame = static_cast<int>(anim.sequence.size()) - 1;
		}

		player.keyframe = anim.sequence[frame];

		// 着地 or 再生完了
		if (player.isGround || t >= 1.0f)
		{
			isJumpAnim = false;
			jump_anim_time = 0.0f;
		}

		return;
	}
}

void SceneGame::InputAttack()
{
	if (player.knockbackTimer > 0.0f) return;

	bool nowLButton = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
	bool nowRButton = (GetKeyState(VK_RBUTTON) & 0x8000) != 0;

	if (!attack_state && player.isGround)
	{
		if (nowLButton && !prevLButton)
		{
			attack_state = true;
			attack_hit_enable = true;
			attack_timer = 0;
			attack_type = AttackType::Left;
			miss_played = false;
			attack_c = { 1.0f, 0.0f, 0.0f, 1.0f };

			isAttackAnim = true;
			attackAnimTimer = 0.0f;
			attack_anim_time = 0.0f;
		}
		else if (nowRButton && !prevRButton)
		{
			attack_state = true;
			attack_hit_enable = true;
			attack_timer = 0;
			attack_type = AttackType::Right;
			miss_played = false;
			attack_c = { 0.0f, 0.0f, 1.0f, 1.0f };

			// スライド開始
			isSlide = true;
			slideTimer = 0.0f;
			slide_anim_time = 0.0f;
		}
	}
	if (attack_hit_enable)
	{
		const float attackRange = 8.0f;
		const float laneThreshold = 2.5f;
		Boxes.length = attackRange;

		for (auto& enemy : enemies)
		{
			// 死んでいる、または既に吹き飛んでいる敵は無視
			if (!enemy.isAlive || enemy.isBlownAway) continue;

			// 岩(Type 2)は攻撃不可
			if (enemy.type == 2) continue;

			if ((attack_type == AttackType::Left && enemy.type != 0) && (fabsf(enemy.position.x - player.position.x) < laneThreshold) ||
				(attack_type == AttackType::Right && enemy.type != 1) && (fabsf(enemy.position.x - player.position.x) < laneThreshold))
			{
				if (fabsf(enemy.position.x - player.position.x) < laneThreshold)
				{
					float dZ = enemy.position.z - player.position.z;
					if (dZ > 0.0f && dZ < attackRange)
					{
						if (!miss_played)
						{
							if (SE_MISS) SE_MISS->Play(false);
							miss_played = true;
						}
					}
				}
			}

			if (attack_type == AttackType::Left && enemy.type != 0) continue;
			if (attack_type == AttackType::Right && enemy.type != 1) continue;

			if (fabsf(enemy.position.x - player.position.x) < laneThreshold)
			{
				float distZ = enemy.position.z - player.position.z;
				if (distZ > 0.0f && distZ < attackRange)
				{
					// ここで吹き飛び処理を開始
					enemy.isBlownAway = true;

					// 上方向と奥方向へ飛ばす
					float blowUp = 20.0f;
					float blowForward = 55.0f;
					// 殴った方向（左右）へも少し飛ばす
					float blowSide = (attack_type == AttackType::Right) ? 15.0f : -15.0f;

					enemy.blowVelocity = { blowSide, blowUp, blowForward };

					player.moveSpeed += P_ACCELE;
					defeatedCount++;
					if (attack_type == AttackType::Right)
					{
						if (SE_KICK) SE_KICK->Play(false);
					}
					if (attack_type == AttackType::Left)
					{
						if (SE_PANCHI) SE_PANCHI->Play(false);
					}

					break;
				}
			}
		}
	}

	if (attack_state)
	{
		attack_timer++;
		if (attack_timer == 20) attack_hit_enable = false;
		if (attack_timer >= 30)
		{
			attack_state = false;
			attack_type = AttackType::None;
			attack_timer = 0;
		}
	}

	prevLButton = nowLButton;
	prevRButton = nowRButton;
}

// 敵生成ヘルパー関数
void SceneGame::SpawnEnemy(float zPosition)
{
	// 1つのステージパネル(長さ約60)の中に、間隔を詰めて配置判定を行う
	float interval = 23.0f;

	int count = static_cast<int>(STAGE_TILE_LENGTH / interval);

	for (int i = 0; i < count; ++i)
	{
		if (rand() % 10 < 8)
		{
			Enemy e;
			int lane = (rand() % 3) - 1;

			// Z位置を計算
			float zOffset = (i * interval) + (static_cast<float>(rand() % 40) / 10.0f);
			e.position = { lane * player.laneWidth, 0, zPosition + zOffset };

			if (e.position.z < 25.0f) continue;

			e.isAlive = true;

			// アニメーション設定
			if (e.type == 0 && skinned_meshes[1] && !skinned_meshes[1]->animation_clips.empty())
			{
				e.keyframe = skinned_meshes[1]->animation_clips[0].sequence[0];
			}
			else if (e.type == 1 && skinned_meshes[5] && !skinned_meshes[5]->animation_clips.empty())
			{
				e.keyframe = skinned_meshes[5]->animation_clips[0].sequence[0];
			}

			// 0,1:敵, 2:岩
			e.type = rand() % 3;

			enemies.push_back(e);
		}
	}
}

void SceneGame::UpdateStages()
{
	float playerZ = player.position.z;

	if (!stages.empty())
	{
		StageObject& firstStage = stages.front();

		// プレイヤーがステージ1枚分以上進んだら、一番後ろのステージを一番前に持ってくる
		if (firstStage.position.z < playerZ - STAGE_TILE_LENGTH * 1.5f)
		{
			StageObject& lastStage = stages.back();

			float nextZ = lastStage.position.z + STAGE_TILE_LENGTH;

			firstStage.position.z = nextZ;

			stages.push_back(firstStage);
			stages.erase(stages.begin());

			// 新しいステージに合わせて敵・障害物を生成
			SpawnEnemy(nextZ);
		}
	}

	// プレイヤーより遥か後ろに行った敵の削除
	auto it = enemies.begin();
	while (it != enemies.end())
	{
		if (it->position.z < playerZ - 20.0f || !it->isAlive)
		{
			it = enemies.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void SceneGame::UpdateCamera(float elapsed_time)
{
	camera_focus = player.position;
	camera_focus.y += 2.0f;

	DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&camera_focus);
	float sx = sinf(rotateX), cx = cosf(rotateX);
	float sy = sinf(rotateY), cy = cosf(rotateY);

	// 背後からのベクトル
	DirectX::XMVECTOR Offset = DirectX::XMVectorSet(cx * sy, -sx, cx * cy, 0.0f);
	Offset = DirectX::XMVectorScale(Offset, distance);

	DirectX::XMVECTOR Eye = DirectX::XMVectorSubtract(Focus, Offset);
	DirectX::XMStoreFloat3(&camera_position, Eye);
	// --- Camera Shake ---
	if (shakeTimer > 0.0f)
	{
		shakeTimer -= elapsed_time;

		float t = shakeTimer * 20.0f;
		float shakeX = (sinf(t * 12.3f)) * shakePower;
		float shakeY = (cosf(t * 9.7f)) * shakePower;

		camera_position.x += shakeX;
		camera_position.y += shakeY;
	}
	// カメラ距離キック
	if (cameraKickBack > 0.0f)
	{
		cameraKickBack -= elapsed_time * 8.0f;
	}
	cameraKickBack = (cameraKickBack < 0.0f) ? 0.0f : cameraKickBack;

	Offset = DirectX::XMVectorScale(Offset, distance + cameraKickBack);
}

void SceneGame::CheckCollisions()
{
	if (player.knockbackTimer > 0.0f) return;

	const float hitRadius = 1.0f;
	const float hitHeight = 2.0f;

	for (auto& enemy : enemies)
	{
		// 死んでいる、または吹き飛んでいる敵との衝突は無視する
		if (!enemy.isAlive || enemy.isBlownAway) continue;

		float dx = enemy.position.x - player.position.x;
		float dz = enemy.position.z - player.position.z;
		float distXZ = sqrtf(dx * dx + dz * dz);
		float dy = fabsf(player.position.y - enemy.position.y);

		if (distXZ < hitRadius && dy < hitHeight)
		{
			// プレイヤー側ノックバック（既存）
			player.knockbackVelocityZ = -30.0f;
			player.knockbackTimer = 2.5f;
			player.moveSpeed = (P_ACCELE * 3);
			player.moveSpeed = 15.0f;

			// ★ 敵が赤(mesh[1])ならキック再生
			if (enemy.type == 0)
			{
				enemy.animState = EnemyAnimState::Kick;
				enemy.kickAnimTime = 0.0f;
			}
			if (skinned_meshes[11] && !skinned_meshes[11]->animation_clips.empty())
			{
				const auto& anim = skinned_meshes[11]->animation_clips[0];
				if (!anim.sequence.empty())
				{
					enemy.keyframe = anim.sequence[0];
				}
			}

			knockState = KnockState::Knocked;
			knocked_anim_time = 0.0f;
		}
	}
}

// 描画処理
void SceneGame::Render()
{
	fw_->framebuffers[0]->clear(fw_->immediate_context.Get());
	fw_->framebuffers[0]->activate(fw_->immediate_context.Get());

	fw_->immediate_context->OMSetBlendState(fw_->blend_states[static_cast<size_t>(BLEND_STATE::ALPHA)].Get(), nullptr, 0xFFFFFFFF);
	fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_ON_ZW_ON)].Get(), 0);
	fw_->immediate_context->RSSetState(fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::SOLID)].Get());

	// ビューポート・プロジェクション
	D3D11_VIEWPORT viewport{};
	UINT num_viewports = 1;
	fw_->immediate_context->RSGetViewports(&num_viewports, &viewport);

	assert(viewport.Height != 0);

	float aspect_ratio = viewport.Width / viewport.Height;

	DirectX::XMMATRIX P{ DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45), aspect_ratio, 0.1f, 300.0f) };

	DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&camera_position),
		DirectX::XMLoadFloat3(&camera_focus),
		DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

	DirectX::XMStoreFloat4x4(&fw_->data.view_projection, V * P);
	fw_->data.light_direction = fw_->light_direction;

	fw_->immediate_context->UpdateSubresource(fw_->constant_buffers[0].Get(), 0, 0, &fw_->data, 0, 0);
	fw_->immediate_context->VSSetConstantBuffers(1, 1, fw_->constant_buffers[0].GetAddressOf());
	fw_->immediate_context->PSSetConstantBuffers(1, 1, fw_->constant_buffers[0].GetAddressOf());

	const DirectX::XMFLOAT4X4 coordinate_system_transforms[]{
{ -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },	// 0:RHS Y-UP
{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },		// 1:LHS Y-UP
{ -1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },	// 2:RHS Z-UP
{ 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },		// 3:LHS Z-UP
	};
	const float scale_factor = 1.0f;

	DirectX::XMMATRIX C{ DirectX::XMLoadFloat4x4(&coordinate_system_transforms[0]) * DirectX::XMMatrixScaling(scale_factor, scale_factor, scale_factor) };

	// -----------------------------------------------------------
	// プレイヤー描画
	// -----------------------------------------------------------
	if (skinned_meshes[0])
	{
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
			player.position.x,
			player.position.y,
			player.position.z
		);

		DirectX::XMMATRIX R =
			DirectX::XMMatrixRotationX(stageRotation.x) *
			DirectX::XMMatrixRotationY(stageRotation.y) *
			DirectX::XMMatrixRotationZ(stageRotation.z);

		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(
			playerScale.x,
			playerScale.y,
			playerScale.z
		);

		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, C * S * R * T);
		int meshIndex = 0;

		if (knockState == KnockState::Knocked) meshIndex = 8;
		else if (knockState == KnockState::Recover) meshIndex = 9;
		else if (isSlide)                           meshIndex = 6;
		else if (isAttackAnim)                      meshIndex = 7;
		else if (isJumpAnim)                        meshIndex = 10;

		auto* playerMesh = skinned_meshes[meshIndex].get();

		playerMesh->render(
			fw_->immediate_context.Get(),
			world,
			playerColor,
			&player.keyframe,
			false
		);
	}

	// -----------------------------------------------------------
	// 敵・障害物描画
	// -----------------------------------------------------------
	if (skinned_meshes[1] && skinned_meshes[4] && skinned_meshes[5])
		for (const auto& enemy : enemies)
		{
			if (!enemy.isAlive) continue;

			DirectX::XMMATRIX T =
				DirectX::XMMatrixTranslation(
					enemy.position.x,
					enemy.position.y,
					enemy.position.z
				);

			// -----------------------------
			// 岩 (type == 2)
			// -----------------------------
			if (enemy.type == 2)
			{
				DirectX::XMMATRIX S =
					DirectX::XMMatrixScaling(
						rockScale.x,
						rockScale.y,
						rockScale.z
					);

				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, C * S * T);

				skinned_meshes[4]->render(
					fw_->immediate_context.Get(),
					world,
					{ 0.4f, 0.4f, 0.4f, 1.0f },
					nullptr,
					false
				);
			}
			// -----------------------------
			// 敵 (type == 0,1)
			// -----------------------------
			else
			{
				DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(180));
				if (enemy.isBlownAway)
				{
					R *= DirectX::XMMatrixRotationRollPitchYaw(
						DirectX::XMConvertToRadians(enemy.blowAngleX),
						DirectX::XMConvertToRadians(enemy.blowAngleY),
						0.0f
					);
				}

				DirectX::XMMATRIX S =
					DirectX::XMMatrixScaling(
						enemyScale.x,
						enemyScale.y,
						enemyScale.z
					);

				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, C * S * R * T);

				// ★ ここが重要：タイプでメッシュ切り替え
				auto* enemyMesh =
					(enemy.animState == EnemyAnimState::Kick)
					? skinned_meshes[11].get()
					: skinned_meshes[enemy.type == 0 ? 1 : 5].get();

				enemyMesh->render(
					fw_->immediate_context.Get(),
					world,
					{ 1,1,1,1 },
					&enemy.keyframe,
					true
				);
			}
		}

	// -----------------------------------------------------------
	// ステージ描画
	// -----------------------------------------------------------
	if (skinned_meshes[2])
	{
		for (const auto& stage : stages)
		{
			DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(stage.position.x, stage.position.y, stage.position.z);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(stageScale.x, stageScale.y, stageScale.z);

			DirectX::XMFLOAT4X4 world;
			DirectX::XMStoreFloat4x4(&world, C * S * T);

			skinned_meshes[2]->render(fw_->immediate_context.Get(), world, stageColor, nullptr, false);
		}
	}

	// 攻撃ヒットボックス
	//if (skinned_meshes[3])
	//{
	//	if (attack_state)
	//	{
	//		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(player.position.x, player.position.y, player.position.z + 0.1);
	//		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(0.5f, 0.1f, Boxes.length);
	//		DirectX::XMFLOAT4X4 world;
	//		DirectX::XMStoreFloat4x4(&world, C * S * T);
	//		skinned_meshes[3]->render(fw_->immediate_context.Get(), world, attack_c, nullptr, true);
	//	}
	//}

	fw_->framebuffers[0]->deactivate(fw_->immediate_context.Get());
	if (fw_->enable_bloom)
	{
		fw_->bloomer->make(fw_->immediate_context.Get(), fw_->framebuffers[0]->shader_resource_views[0].Get());

		fw_->framebuffers[1]->clear(fw_->immediate_context.Get());
		fw_->framebuffers[1]->activate(fw_->immediate_context.Get());

		fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
		fw_->immediate_context->RSSetState(fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get());
		fw_->immediate_context->OMSetBlendState(fw_->blend_states[static_cast<size_t>(BLEND_STATE::ALPHA)].Get(), nullptr, 0xFFFFFFFF);
		ID3D11ShaderResourceView* bloom_srvs[] =
		{
			fw_->framebuffers[0]->shader_resource_views[0].Get(),
			fw_->bloomer->shader_resource_view(),
		};

		fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), bloom_srvs, 0, 2, fw_->pixel_shaders[2].Get());
		fw_->framebuffers[1]->deactivate(fw_->immediate_context.Get());
	}

	// Radial Blur
	if (fw_->enable_radial_blur)
	{
		fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
		fw_->immediate_context->RSSetState(fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get());

		ID3D11ShaderResourceView* blur_srvs[] =
		{
			fw_->framebuffers[1]->shader_resource_views[0].Get()
		};

		fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), blur_srvs, 0, 1, fw_->pixel_shaders[0].Get());
	}
	else
	{
		fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), fw_->framebuffers[1]->shader_resource_views[0].GetAddressOf(), 0, 1, fw_->pixel_shaders[2].Get());
	}

	int speed = static_cast<int>(player.moveSpeed / P_ACCELE);
	DrawNumber(speed, speedPos.x, speedPos.y, speedScale, 8, fw_->immediate_context.Get());

	int remainDistance = static_cast<int>(std::max(0.0f, GOAL_DISTANCE - player.position.z));

	// 画面右上に距離を表示

	DrawNumber(remainDistance, MPos.x, MPos.y, MScale, 9, fw_->immediate_context.Get());

	sprite_batches[1]->begin(fw_->immediate_context.Get());
	sprite_batches[1]->render(
		fw_->immediate_context.Get(),
		-10.0f, 893.0f,
		1700 * 0.25f, 700 * 0.25f,
		1, 1, 1, 1,
		0.0f
	);
	sprite_batches[1]->end(fw_->immediate_context.Get());

	sprite_batches[2]->begin(fw_->immediate_context.Get());
	sprite_batches[2]->render(
		fw_->immediate_context.Get(),
		1412.0f, 887.0f,
		2082 * 0.24f, 790 * 0.24f,
		1, 1, 1, 1,
		0.0f
	);
	sprite_batches[2]->end(fw_->immediate_context.Get());
}

void SceneGame::DrawGUI()
{
#ifdef USE_IMGUI
	ImGui::Begin("Game Settings");
	ImGui::Text("Anim Time: %.2f", player_anim_time);
	ImGui::Text("Anim Frames: %d",
		(int)skinned_meshes[0]->animation_clips[0].sequence.size());

	ImGui::ColorEdit4(
		"Player Color",
		&playerColor.x,
		ImGuiColorEditFlags_Float
	);
	ImGui::Text("--- kari Display ---");
	ImGui::DragFloat2("kari Pos", &kariPos.x, 1.0f);
	ImGui::DragFloat("kari Scale", &kariScale, 0.01f);
	ImGui::DragFloat("kari N", &kariN, 0.01f);

	ImGui::Text("--- speed Display ---");
	ImGui::DragFloat2("speed Pos", &speedPos.x, 1.0f);
	ImGui::DragFloat("speed Scale", &speedScale, 0.01f);

	ImGui::Text("--- M Display ---");
	ImGui::DragFloat2("M Pos", &MPos.x, 1.0f);
	ImGui::DragFloat("M Scale", &MScale, 0.01f);

	ImGui::SliderFloat("camera_position.x", &fw_->camera_position.x, -100.0f, +100.0f);
	ImGui::SliderFloat("camera_position.y", &fw_->camera_position.y, -100.0f, +100.0f);
	ImGui::SliderFloat("camera_position.z", &fw_->camera_position.z, -100.0f, -1.0f);

	ImGui::Checkbox("flat_shading", &fw_->flat_shading);
	ImGui::Checkbox("Enable Dynamic Shader", &fw_->enable_dynamic_shader);
	ImGui::Checkbox("Enable Dynamic Background", &fw_->enable_dynamic_background);
	ImGui::Checkbox("Enable RADIAL_BLUR", &fw_->enable_radial_blur);
	ImGui::Checkbox("Enable Bloom", &fw_->enable_bloom);

	ImGui::SliderFloat("light_direction.x", &fw_->light_direction.x, -1.0f, +1.0f);
	ImGui::SliderFloat("light_direction.y", &fw_->light_direction.y, -1.0f, +1.0f);
	ImGui::SliderFloat("light_direction.z", &fw_->light_direction.z, -1.0f, +1.0f);

	ImGui::SliderFloat("extraction_threshold", &fw_->parametric_constants.extraction_threshold, +0.0f, +5.0f);
	ImGui::SliderFloat("gaussian_sigma", &fw_->parametric_constants.gaussian_sigma, +0.0f, +10.0f);
	ImGui::SliderFloat("exposure", &fw_->parametric_constants.exposure, +0.0f, +10.0f);

	// RADIAL_BLUR
	ImGui::DragFloat2("blur_center", &fw_->radial_blur_data.blur_center.x, 0.01f);
	ImGui::SliderFloat("blur_strength", &fw_->radial_blur_data.blur_strength, +0.0f, +1.0f);
	ImGui::SliderFloat("blur_radius", &fw_->radial_blur_data.blur_radius, +0.0f, +1.0f);
	ImGui::SliderFloat("blur_decay", &fw_->radial_blur_data.blur_decay, +0.0f, +1.0f);

	// BLOOM
	ImGui::SliderFloat("bloom_extraction_threshold", &fw_->bloomer->bloom_extraction_threshold, +0.0f, +1.0f);
	ImGui::SliderFloat("bloom_intensity", &fw_->bloomer->bloom_intensity, +0.0f, +5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10)); // 内側の余白を増やす
	ImGui::SliderFloat("Camera Distance", &fw_->distance, 1.0f, 20.0f);
	ImGui::PopStyleVar();

	ImGui::Text("Stage Rotation");
	ImGui::SliderAngle("Rotate X", &stageRotation.x, -180.0f, 180.0f);
	ImGui::SliderAngle("Rotate Y", &stageRotation.y, -180.0f, 180.0f);
	ImGui::SliderAngle("Rotate Z", &stageRotation.z, -180.0f, 180.0f);

	if (ImGui::CollapsingHeader("Model Scales", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3("Player Scale", &playerScale.x, 0.001f, 0.0f, 10.0f);
		ImGui::DragFloat3("Enemy Scale", &enemyScale.x, 0.001f, 0.0f, 10.0f);
		ImGui::DragFloat3("Stage Scale", &stageScale.x, 0.01f, 0.0f, 100.0f);
	}

	if (ImGui::CollapsingHeader("Game Info"))
	{
		ImGui::Text("Lane: %d", player.currentLane);
		ImGui::Text("Pos: %.1f, %.1f, %.1f", player.position.x, player.position.y, player.position.z);
		ImGui::Text("Enemies: %d", (int)enemies.size());
		ImGui::Text("Stages: %d", (int)stages.size());
	}

	ImGui::Text("Controls: A/D to Move, Space to Jump, Click to Attack");
	ImGui::End();
#endif
}
void SceneGame::DrawNumber(int number, float x, float y, float scale, int sukima, ID3D11DeviceContext* ctx)
{
	const float cellW = 507.0f;
	const float cellH = 476.00f;

	std::string str = std::to_string(number);
	float posX = x;

	for (char c : str)
	{
		int digit = c - '0';

		int col = digit % 4;
		int row = digit / 4;

		float sx = col * cellW;
		float sy = row * cellH;
		float sw = cellW;
		float sh = cellH;

		sprite_batches[0]->begin(fw_->immediate_context.Get());
		sprite_batches[0]->render(
			ctx,
			posX, y,
			cellW * scale, cellH * scale,
			1, 1, 1, 1,
			0.0f,
			sx, sy,
			sw, sh
		);
		sprite_batches[0]->end(fw_->immediate_context.Get());

		posX += cellW / sukima;
	}
}