#include "SceneTutorial.h"
#include "SceneTitle.h" // タイトルに戻るため
#include "SceneManager.h"
#include "shader.h"
#include "texture.h"
#include <cstdlib> // rand用

// ステージの間隔
static const float TUTORIAL_STAGE_LENGTH = 380.0f;

// 撃破カウンター
static int tutorialDefeatedRed = 0;
static int tutorialDefeatedBlue = 0;

SceneTutorial::SceneTutorial(HWND hwnd, framework* fw) : hwnd(hwnd), fw_(fw)
{
}

void SceneTutorial::Initialize()
{
	// リソース読み込み
	skinned_meshes[0] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\Run_main.cereal");
	skinned_meshes[1] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\enemy.cereal");
	skinned_meshes[2] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\main_stage.cereal");
	skinned_meshes[3] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\cube.000.fbx");
	skinned_meshes[4] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\Vegetable.fbx");
	sprite_batches[0] = std::make_unique<sprite_batch>(fw_->device.Get(), L".\\resources\\fontS.png", 1);

	SE_PANCHI = Audio::Instance().LoadAudioSource(".\\resources\\panchi.wav");
	SE_KICK = Audio::Instance().LoadAudioSource(".\\resources\\kick.wav");
	SE_MISS = Audio::Instance().LoadAudioSource(".\\resources\\miss.wav");

	// プレイヤー初期化
	player.position = { 0, 0, 0 };
	player.currentLane = 1;
	player.moveSpeed = 10.0f;

	// ステージ初期配置
	stages.clear();
	for (int i = 0; i < MAX_STAGE_TILES; ++i)
	{
		StageObject s;
		s.position = { 0, 0, i * TUTORIAL_STAGE_LENGTH };
		stages.push_back(s);
	}

	fw_->light_direction = DirectX::XMFLOAT4{ 1.0f, -1.0f, 1.0f, 0.0f };

	currentStep = TutorialStep::Welcome;
	stepTimer = 0.0f;
	actionDone = false;

	// カウンターリセット
	tutorialDefeatedRed = 0;
	tutorialDefeatedBlue = 0;

	// カメラの初期位置を確定させる
	UpdateCamera();
}

void SceneTutorial::Finalize()
{
	if (SE_PANCHI) { delete SE_PANCHI; SE_PANCHI = nullptr; }
	if (SE_KICK) { delete SE_KICK; SE_KICK = nullptr; }
	if (SE_MISS) { delete SE_MISS; SE_MISS = nullptr; }
}

void SceneTutorial::Update(float elapsedTime)
{
	// ステージ無限ループ処理
	float playerZ = player.position.z;
	if (!stages.empty())
	{
		StageObject& firstStage = stages.front();
		if (firstStage.position.z < playerZ - TUTORIAL_STAGE_LENGTH * 1.5f)
		{
			StageObject newStage = firstStage;
			stages.erase(stages.begin());

			StageObject& lastStage = stages.back();
			newStage.position.z = lastStage.position.z + TUTORIAL_STAGE_LENGTH;
			stages.push_back(newStage);
		}
	}

	UpdatePlayer(elapsedTime);
	InputAttack();

	//  吹き飛んでいる敵の更新処理
	for (auto& enemy : enemies)
	{
		if (enemy.isBlownAway)
		{
			// 重力で落下させる
			enemy.blowVelocity.y -= 40.0f * elapsedTime;

			// 速度分移動させる
			enemy.position.x += enemy.blowVelocity.x * elapsedTime;
			enemy.position.y += enemy.blowVelocity.y * elapsedTime;
			enemy.position.z += enemy.blowVelocity.z * elapsedTime;

			// クルクル回転させる
			enemy.blowAngleX += 720.0f * elapsedTime;
			enemy.blowAngleY += 360.0f * elapsedTime;

			// 地面よりだいぶ下（-20）まで落ちたら完全に消滅させる
			if (enemy.position.y < -20.0f)
			{
				enemy.isAlive = false;
			}
		}
	}

	UpdateTutorialFlow(elapsedTime);
	UpdateCamera();

	// 衝突判定
	if (player.knockbackTimer <= 0.0f)
	{
		for (auto& enemy : enemies)
		{
			//  吹き飛び中(isBlownAway)は当たり判定を無視する
			if (!enemy.isAlive || enemy.isBlownAway) continue;

			float dx = enemy.position.x - player.position.x;
			float dz = enemy.position.z - player.position.z;
			if (sqrtf(dx * dx + dz * dz) < 1.5f && fabsf(player.position.y - enemy.position.y) < 2.0f)
			{
				player.knockbackVelocityZ = -20.0f;
				player.knockbackTimer = 1.0f;
			}
		}
	}
}

// チュートリアルのメインロジック
void SceneTutorial::UpdateTutorialFlow(float elapsedTime)
{
	stepTimer += elapsedTime;

	switch (currentStep)
	{
	case TutorialStep::Welcome:

		if (stepTimer > 0.5f)
		{
			if (GetAsyncKeyState(VK_RETURN) & 0x8000)
			{
				currentStep = TutorialStep::Move;
				stepTimer = 0.0f;
				actionDone = false;
			}
		}
		break;

	case TutorialStep::Move:
		// 移動（レーン変更）を検知した瞬間にタイマーをリセット
		if (!actionDone && player.currentLane != 1)
		{
			actionDone = true;
			stepTimer = 0.0f;
		}

		// 移動してから3秒経過したら次へ
		if (actionDone && stepTimer > 3.0f)
		{
			currentStep = TutorialStep::Jump;
			stepTimer = 0.0f;
			actionDone = false;
			SpawnObstacle();
		}
		break;

	case TutorialStep::Jump:
		if (player.position.y > 1.0f) actionDone = true;

		// 障害物がプレイヤーの後ろに行ったら再生成
		// 岩は吹き飛ばないので enemies[0] のチェックで問題ありません
		if (enemies.empty() || enemies[0].position.z < player.position.z - 5.0f)
		{
			if (!enemies.empty()) enemies.clear();

			if (!actionDone) SpawnObstacle();
			else
			{
				currentStep = TutorialStep::Attack;
				stepTimer = 0.0f;
				actionDone = false;
				SpawnDummyEnemy();
			}
		}
		break;

	case TutorialStep::Attack:
		// 赤と青をそれぞれ3体以上倒したらOK
		if (tutorialDefeatedRed >= 3 && tutorialDefeatedBlue >= 3)
		{
			actionDone = true;
		}

		// まだ条件未達成で、敵が通り過ぎてしまったら（またはいなくなったら）再生成
		if (!actionDone)
		{
			//  「生きている」かつ「吹き飛んでいない」敵がまだ手前にいるかチェック
			bool existsActiveEnemy = false;
			for (const auto& e : enemies)
			{
				if (e.isAlive && !e.isBlownAway)
				{
					// プレイヤーより手前（あるいは少し後ろまで）にいるなら「まだウェーブ中」とみなす
					if (e.position.z > player.position.z - 5.0f)
					{
						existsActiveEnemy = true;
						break;
					}
				}
			}

			// 有効な敵がいない（全滅させた or 全員通り過ぎた）場合は再生成
			if (!existsActiveEnemy)
			{
				enemies.clear();
				SpawnDummyEnemy();
			}
		}

		if (actionDone && stepTimer > 2.0f)
		{
			currentStep = TutorialStep::Complete;
			stepTimer = 0.0f;
		}
		break;

	case TutorialStep::Complete:
		if (stepTimer > 4.0f)
		{
			SceneManager::instance().ChangeScene(new SceneTitle(fw_));
		}
		break;
	}
}

void SceneTutorial::SpawnObstacle()
{
	// 3つのレーンすべてに岩を配置
	for (int i = -1; i <= 1; ++i)
	{
		Enemy e;
		e.position = { i * player.laneWidth, 0, player.position.z + 40.0f };
		e.type = 2; // 岩
		e.isAlive = true;
		enemies.push_back(e);
	}
}

void SceneTutorial::SpawnDummyEnemy()
{
	// 3つのレーンすべてに敵を配置（ランダムタイプ）
	for (int i = -1; i <= 1; ++i)
	{
		Enemy e;
		e.position = { i * player.laneWidth, 0, player.position.z + 40.0f };

		// 0(赤) か 1(青) をランダム設定
		e.type = rand() % 2;

		e.isAlive = true;
		if (skinned_meshes[1] && !skinned_meshes[1]->animation_clips.empty()) {
			e.keyframe = skinned_meshes[1]->animation_clips[0].sequence[0];
		}
		enemies.push_back(e);
	}
}

void SceneTutorial::UpdatePlayer(float elapsedTime)
{
	if (player.knockbackTimer > 0.0f)
	{
		player.knockbackTimer -= elapsedTime;
		player.position.z += player.knockbackVelocityZ * elapsedTime;
		player.knockbackVelocityZ += 20.0f * elapsedTime;
		if (player.knockbackVelocityZ > 0.0f) player.knockbackVelocityZ = 0.0f;
	}

	if (player.knockbackTimer <= 0.0f)
	{
		// レーン移動処理
		bool isLeftDown = (GetAsyncKeyState('A') & 0x8000) || (GetAsyncKeyState(VK_LEFT) & 0x8000);
		bool isRightDown = (GetAsyncKeyState('D') & 0x8000) || (GetAsyncKeyState(VK_RIGHT) & 0x8000);
		bool onLeftTrigger = isLeftDown && !player.wasLeftPressed;
		bool onRightTrigger = isRightDown && !player.wasRightPressed;
		player.wasLeftPressed = isLeftDown;
		player.wasRightPressed = isRightDown;

		if (onLeftTrigger && player.currentLane > 0) player.currentLane--;
		if (onRightTrigger && player.currentLane < 2) player.currentLane++;

		// 座標計算
		float targetX = (player.currentLane - 1) * player.laneWidth;
		float diff = targetX - player.position.x;
		if (fabsf(diff) > 0.01f)
		{
			float moveStep = (diff > 0 ? 1.0f : -1.0f) * player.laneChangeSpeed * elapsedTime;
			if (fabsf(moveStep) > fabsf(diff)) player.position.x = targetX;
			else player.position.x += moveStep;
		}
		else player.position.x = targetX;

		player.position.z += player.moveSpeed * elapsedTime;

		if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && player.isGround && !attack_state)
		{
			player.velocity.y = player.jumpPower;
			player.isGround = false;
		}
	}

	player.velocity.y += player.gravity * elapsedTime;
	player.position.y += player.velocity.y * elapsedTime;
	if (player.position.y <= 0.0f)
	{
		player.position.y = 0.0f;
		player.velocity.y = 0.0f;
		player.isGround = true;
	}

	// アニメーション更新
	if (!skinned_meshes[0]->animation_clips.empty())
	{
		animation& anim = skinned_meshes[0]->animation_clips[0];
		player_anim_time += elapsedTime;
		int frame = static_cast<int>(player_anim_time * anim.sampling_rate);
		if (frame >= static_cast<int>(anim.sequence.size())) { frame = 0; player_anim_time = 0.0f; }
		player.keyframe = anim.sequence[frame];
	}
}

void SceneTutorial::InputAttack()
{
	bool nowLButton = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
	bool nowRButton = (GetKeyState(VK_RBUTTON) & 0x8000) != 0;

	if (!attack_state && player.isGround)
	{
		if (nowLButton && !prevLButton) { attack_state = true; attack_hit_enable = true; attack_timer = 0; attack_type = 1; attack_c = { 1,0,0,1 }; }
		else if (nowRButton && !prevRButton) { attack_state = true; attack_hit_enable = true; attack_timer = 0; attack_type = 2; attack_c = { 0,0,1,1 }; }
	}

	if (attack_hit_enable)
	{
		Boxes.length = 20.0f;
		for (auto& enemy : enemies)
		{
			//  吹き飛び中も除外
			if (!enemy.isAlive || enemy.isBlownAway || enemy.type == 2) continue; // 岩は壊せない

			float distZ = enemy.position.z - player.position.z;
			if (distZ > 0.0f && distZ < 20.0f && fabsf(enemy.position.x - player.position.x) < 2.5f)
			{
				if ((attack_type == 1 && enemy.type == 0) || (attack_type == 2 && enemy.type == 1))
				{
					// ここで吹き飛び処理を開始
					enemy.isBlownAway = true;

					// 上方向と奥方向へ飛ばす
					float blowUp = 15.0f;
					float blowForward = 25.0f;
					// 殴った方向（左右）へも少し飛ばす (attack_type 2=Right(Blue Kick))
					float blowSide = (attack_type == 2) ? 5.0f : -5.0f;

					enemy.blowVelocity = { blowSide, blowUp, blowForward };

					// 倒した敵の種類をカウント
					if (enemy.type == 0) tutorialDefeatedRed++;
					if (enemy.type == 1) tutorialDefeatedBlue++;

					// 敵を倒すたびに速度アップ
					player.moveSpeed += 5.0f;

					if (attack_type == 1 && SE_PANCHI) SE_PANCHI->Play(false);
					if (attack_type == 2 && SE_KICK) SE_KICK->Play(false);
				}
			}
		}
	}

	if (attack_state)
	{
		attack_timer++;
		if (attack_timer == 30) attack_hit_enable = false;
		if (attack_timer >= 60) { attack_state = false; attack_timer = 0; }
	}
	prevLButton = nowLButton;
	prevRButton = nowRButton;
}

void SceneTutorial::UpdateCamera()
{
	camera_focus = player.position;
	camera_focus.y += 2.0f;
	DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&camera_focus);
	float sx = sinf(rotateX), cx = cosf(rotateX);
	float sy = sinf(rotateY), cy = cosf(rotateY);
	DirectX::XMVECTOR Offset = DirectX::XMVectorScale(DirectX::XMVectorSet(cx * sy, -sx, cx * cy, 0.0f), distance);
	DirectX::XMStoreFloat3(&camera_position, DirectX::XMVectorSubtract(Focus, Offset));
}

void SceneTutorial::Render()
{
	fw_->framebuffers[0]->clear(fw_->immediate_context.Get());
	fw_->framebuffers[0]->activate(fw_->immediate_context.Get());

	fw_->immediate_context->OMSetBlendState(fw_->blend_states[1].Get(), nullptr, 0xFFFFFFFF);
	fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[0].Get(), 0);
	fw_->immediate_context->RSSetState(fw_->rasterizer_states[0].Get());

	D3D11_VIEWPORT viewport{}; UINT num_viewports = 1;
	fw_->immediate_context->RSGetViewports(&num_viewports, &viewport);
	float aspect_ratio = viewport.Width / viewport.Height;

	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45), aspect_ratio, 0.1f, 300.0f);
	DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&camera_position), DirectX::XMLoadFloat3(&camera_focus), DirectX::XMVectorSet(0, 1, 0, 0));
	DirectX::XMStoreFloat4x4(&fw_->data.view_projection, V * P);
	fw_->data.light_direction = fw_->light_direction;

	fw_->immediate_context->UpdateSubresource(fw_->constant_buffers[0].Get(), 0, 0, &fw_->data, 0, 0);
	fw_->immediate_context->VSSetConstantBuffers(1, 1, fw_->constant_buffers[0].GetAddressOf());
	fw_->immediate_context->PSSetConstantBuffers(1, 1, fw_->constant_buffers[0].GetAddressOf());

	DirectX::XMMATRIX C = DirectX::XMMatrixIdentity();
	C.r[0].m128_f32[0] = -1.0f;

	// プレイヤー描画
	if (skinned_meshes[0])
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, C * DirectX::XMMatrixScaling(0.01f, 0.01f, 0.01f) * DirectX::XMMatrixTranslation(player.position.x, player.position.y, player.position.z));
		skinned_meshes[0]->render(fw_->immediate_context.Get(), world, { 1,1,1,1 }, &player.keyframe, true);
	}
	// 敵・障害物描画
	if (skinned_meshes[1] && skinned_meshes[4])
	{
		for (const auto& e : enemies)
		{
			if (!e.isAlive) continue;
			DirectX::XMFLOAT4X4 world;
			if (e.type == 2) // 岩
			{
				DirectX::XMStoreFloat4x4(&world, C * DirectX::XMMatrixScaling(0.12f, 0.12f, 0.12f) * DirectX::XMMatrixTranslation(e.position.x, e.position.y, e.position.z));
				skinned_meshes[4]->render(fw_->immediate_context.Get(), world, { 0.4f, 0.4f, 0.4f, 1 }, nullptr, true);
			}
			else // 敵
			{
				//  回転計算を追加
				DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(e.position.x, e.position.y, e.position.z);
				DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(DirectX::XM_PI);

				// 吹き飛び中はグルグル回転させる
				if (e.isBlownAway)
				{
					R *= DirectX::XMMatrixRotationRollPitchYaw(
						DirectX::XMConvertToRadians(e.blowAngleX),
						DirectX::XMConvertToRadians(e.blowAngleY),
						0.0f
					);
				}

				DirectX::XMMATRIX S = DirectX::XMMatrixScaling(0.01f, 0.01f, 0.01f);

				DirectX::XMStoreFloat4x4(&world, C * S * R * T);

				DirectX::XMFLOAT4 col = (e.type == 0) ? DirectX::XMFLOAT4(1, 0.5f, 0.5f, 1) : DirectX::XMFLOAT4(0.5f, 0.5f, 1, 1);
				skinned_meshes[1]->render(fw_->immediate_context.Get(), world, col, &e.keyframe);
			}
		}
	}
	// ステージ描画
	if (skinned_meshes[2])
	{
		for (const auto& s : stages)
		{
			DirectX::XMFLOAT4X4 world;
			DirectX::XMStoreFloat4x4(&world, C * DirectX::XMMatrixScaling(0.3f, 0.3f, 0.3f) * DirectX::XMMatrixTranslation(s.position.x, s.position.y, s.position.z));
			skinned_meshes[2]->render(fw_->immediate_context.Get(), world, { 1,1,1,1 }, nullptr, false);
		}
	}
	// 攻撃エフェクト
	if (attack_state && skinned_meshes[3])
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, C * DirectX::XMMatrixScaling(0.5f, 0.1f, Boxes.length) * DirectX::XMMatrixTranslation(player.position.x, player.position.y, player.position.z + 0.1f));
		skinned_meshes[3]->render(fw_->immediate_context.Get(), world, attack_c, nullptr, true);
	}

	fw_->framebuffers[0]->deactivate(fw_->immediate_context.Get());

	// ポストプロセス
	if (fw_->enable_bloom)
	{
		fw_->bloomer->make(fw_->immediate_context.Get(), fw_->framebuffers[0]->shader_resource_views[0].Get());
		fw_->framebuffers[1]->clear(fw_->immediate_context.Get());
		fw_->framebuffers[1]->activate(fw_->immediate_context.Get());
		fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[3].Get(), 0);
		fw_->immediate_context->RSSetState(fw_->rasterizer_states[2].Get());
		fw_->immediate_context->OMSetBlendState(fw_->blend_states[1].Get(), nullptr, 0xFFFFFFFF);
		ID3D11ShaderResourceView* bloom_srvs[] =
		{
			fw_->framebuffers[0]->shader_resource_views[0].Get(),
			fw_->bloomer->shader_resource_view(),
		};
		fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), bloom_srvs, 0, 2, fw_->pixel_shaders[2].Get());
		fw_->framebuffers[1]->deactivate(fw_->immediate_context.Get());
	}
	else
	{
		fw_->framebuffers[1]->clear(fw_->immediate_context.Get());
		fw_->framebuffers[1]->activate(fw_->immediate_context.Get());
		fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), fw_->framebuffers[0]->shader_resource_views[0].GetAddressOf(), 0, 1, fw_->pixel_shaders[2].Get());
		fw_->framebuffers[1]->deactivate(fw_->immediate_context.Get());
	}

	fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), fw_->framebuffers[1]->shader_resource_views[0].GetAddressOf(), 0, 1, fw_->pixel_shaders[2].Get());

	DrawNumber((int)currentStep + 1, 100, 30, 0.5f, fw_->immediate_context.Get());
}

void SceneTutorial::DrawNumber(int number, float x, float y, float scale, ID3D11DeviceContext* ctx)
{
	if (!sprite_batches[0]) return;
	const float cellW = 256.0f;
	const float cellH = 303.33f;
	int digit = number % 10;
	float sx = (digit % 4) * cellW;
	float sy = (digit / 4) * cellH;
	sprite_batches[0]->begin(fw_->immediate_context.Get());
	sprite_batches[0]->render(ctx, x, y, cellW * scale, cellH * scale, 1, 1, 1, 1, 0.0f, sx, sy, cellW, cellH);
	sprite_batches[0]->end(fw_->immediate_context.Get());
}

void SceneTutorial::DrawGUI()
{
#ifdef USE_IMGUI
	ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_FirstUseEver);
	ImGui::Begin("Tutorial");

	switch (currentStep)
	{
	case TutorialStep::Welcome:
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "Welcome to Tutorial!");
		ImGui::Text("Let's learn how to play.");
		ImGui::Text("Press [ENTER] to Start");
		break;
	case TutorialStep::Move:
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Step 1: MOVE");
		ImGui::Text("Press 'A' or 'Left' to go Left.");
		ImGui::Text("Press 'D' or 'Right' to go Right.");
		break;
	case TutorialStep::Jump:
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Step 2: JUMP");
		ImGui::Text("Press 'SPACE' to Jump over rocks.");
		break;
	case TutorialStep::Attack:
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Step 3: ATTACK");
		ImGui::Text("Left Click: Red Punch");
		ImGui::Text("Right Click: Blue Kick");
		ImGui::Text("Defeat Red: %d/3, Blue: %d/3", tutorialDefeatedRed, tutorialDefeatedBlue);
		break;
	case TutorialStep::Complete:
		ImGui::TextColored(ImVec4(0, 1, 1, 1), "COMPLETED!");
		ImGui::Text("Good luck in the main game.");
		break;
	}
	ImGui::End();
#endif
}