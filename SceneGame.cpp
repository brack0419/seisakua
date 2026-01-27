#include "shader.h"
#include "texture.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneEnd.h"
#include "framework.h"
#include "SceneGame.h"
#include <algorithm> // std::max, std::min
#include <cmath>

float saveSpeed = 0.0f;

SceneGame::SceneGame(HWND hwnd, framework* fw) : hwnd(hwnd), fw_(fw)
{
}

// 初期化
void SceneGame::Initialize()
{
	HRESULT hr{ S_OK };

	// ---------------------------------------------------------
	// モデルの読み込み
	// ---------------------------------------------------------

	skinned_meshes[0] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\Run_main.cereal");

	// 1: 敵 (Slime)

	skinned_meshes[1] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\enemy.cereal");

	// 2: ステージ床 (Stage)

	skinned_meshes[2] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\main_stage.cereal");

	// 3:  当たり判定BOX(BOX)

	skinned_meshes[3] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\cube.000.fbx");

	// 3:  障害物

	skinned_meshes[4] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\Vegetable.fbx");

	sprite_batches[0] = std::make_unique<sprite_batch>(fw_->device.Get(), L".\\resources\\fontS.png", 1);

	SE_PANCHI = Audio::Instance().LoadAudioSource(".\\resources\\panchi.wav");
	SE_KICK = Audio::Instance().LoadAudioSource(".\\resources\\kick.wav");
	SE_MISS = Audio::Instance().LoadAudioSource(".\\resources\\miss.wav");

	bgmGame = Audio::Instance().LoadAudioSource(".\\resources\\スティックマンの冒険.wav");

	playerScale = { 0.01f, 0.01f, 0.01f };
	enemyScale = { 0.01f, 0.01f, 0.01f };
	stageScale = { 0.3f, 0.3f, 0.3f };
	rockScale = { 0.12f, 0.12f, 0.12f };

	// プレイヤー初期化
	player.position = { 0, 0, 0 };
	player.currentLane = 1; // 中央
	player.isGround = true;
	player.knockbackTimer = 0.0f;
	player.knockbackVelocityZ = 0.0f;
	player.moveSpeed = 15.0f;

	gameTime = 0.0f;

	fw_->light_direction = DirectX::XMFLOAT4{ 1.0f, -1.0f, 1.0f, 0.0f };

	fw_->radial_blur_data.blur_strength = 0.1f;
	fw_->radial_blur_data.blur_radius = 1.0f;
	fw_->radial_blur_data.blur_decay = 0.0f;

	// ---------------------------------------------------------
	// ステージ・敵の初期配置
	// ---------------------------------------------------------
	for (int i = 0; i < MAX_STAGE_TILES; ++i)
	{
		StageObject s;
		s.position = { 0, 0, i * STAGE_TILE_LENGTH };
		stages.push_back(s);

		if (i > 5)
		{
			// 確率で敵か岩を配置
			if (rand() % 3 == 0)
			{
				Enemy e;
				int lane = (rand() % 3) - 1;
				e.position = { lane * player.laneWidth, 0, i * STAGE_TILE_LENGTH };
				e.isAlive = true;

				// 0,1:敵, 2:岩
				e.type = rand() % 3;

				enemies.push_back(e);
			}
		}
	}
}

// 終了化
void SceneGame::Finalize()
{
	if (bgmGame) {
		bgmGame->Stop();
		delete bgmGame;
		bgmGame = nullptr;
	}
}

// 更新処理
void SceneGame::Update(float elapsedTime)
{
	gameTime += elapsedTime;

	if (saveSpeed <= player.moveSpeed)
	{
		saveSpeed = player.moveSpeed;
	}

	UpdatePlayer(elapsedTime);

	// ゴール判定
	if (player.position.z >= GOAL_DISTANCE)
	{
		isGoal = true;

		// カメラを固定
		cameraMode = CameraMode::Goal;

		goalCameraFocus = player.position;
		goalCameraFocus.y += 2.0f;

		goalCameraPosition = camera_position; // 現在の位置を保存

		goalTimer += elapsedTime;

		//UpdateCamera(); // 固定カメラ or GOALカメラ

		if (goalTimer >= 5.0f)
		{
			SceneManager::instance().ChangeScene(new SceneEnd(fw_, gameTime, defeatedCount));
		}
		return;
	}

	// 攻撃処理
	InputAttack();

	// ステージ管理（無限生成）
	UpdateStages();

	// カメラ更新
	UpdateCamera();

	// 衝突判定
	CheckCollisions();

	// ライトパラメータ更新
}

void SceneGame::UpdatePlayer(float elaspedTime)
{
	if (player.knockbackTimer > 0.0f)
	{
		player.knockbackTimer -= elaspedTime;
		player.position.z += player.knockbackVelocityZ * elaspedTime;
		player.knockbackVelocityZ += 20.0f * elaspedTime;
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
			float moveStep = (diff > 0 ? 1.0f : -1.0f) * player.laneChangeSpeed * elaspedTime;
			if (fabsf(moveStep) > fabsf(diff)) player.position.x = targetX;
			else player.position.x += moveStep;
		}
		else
		{
			player.position.x = targetX;
		}

		player.position.z += player.moveSpeed * elaspedTime;

		if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && player.isGround && !attack_state)
		{
			player.velocity.y = player.jumpPower;
			player.isGround = false;
		}
	}
	else
	{
		player.wasLeftPressed = false;
		player.wasRightPressed = false;
	}

	player.velocity.y += player.gravity * elaspedTime;
	player.position.y += player.velocity.y * elaspedTime;

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

		player_anim_time += elaspedTime;

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
		if (!enemy.isAlive) continue;

		enemy.animationTime += elaspedTime;

		int frame = static_cast<int>(enemy.animationTime * anim.sampling_rate);

		if (frame >= static_cast<int>(anim.sequence.size()))
		{
			frame = 0;
			enemy.animationTime = 0.0f;
		}

		enemy.keyframe = anim.sequence[frame];
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
		}
		else if (nowRButton && !prevRButton)
		{
			attack_state = true;
			attack_hit_enable = true;
			attack_timer = 0;
			attack_type = AttackType::Right;
			miss_played = false;
			attack_c = { 0.0f, 0.0f, 1.0f, 1.0f };
		}
	}
	if (attack_hit_enable)
	{
		const float attackRange = 20.0f;
		const float laneThreshold = 2.5f;
		Boxes.length = attackRange;

		for (auto& enemy : enemies)
		{
			if (!enemy.isAlive) continue;

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
					enemy.isAlive = false;
					player.moveSpeed += 2.5f;
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
		if (attack_timer == 30) attack_hit_enable = false;
		if (attack_timer >= 60)
		{
			attack_state = false;
			attack_type = AttackType::None;
			attack_timer = 0;
		}
	}

	prevLButton = nowLButton;
	prevRButton = nowRButton;
}

void SceneGame::UpdateStages()
{
	float playerZ = player.position.z;

	if (!stages.empty())
	{
		StageObject& firstStage = stages.front();
		// プレイヤーより後ろになったら前に移動
		if (firstStage.position.z < playerZ - STAGE_TILE_LENGTH * 2)
		{
			StageObject& lastStage = stages.back();
			float nextZ = lastStage.position.z + STAGE_TILE_LENGTH;

			firstStage.position.z = nextZ;

			stages.push_back(firstStage);
			stages.erase(stages.begin());

			// 新しい敵の配置
			if (rand() % 3 == 0)
			{
				Enemy e;
				int lane = (rand() % 3) - 1;
				e.position = { lane * player.laneWidth, 0, nextZ };
				e.isAlive = true;
				// アニメーション初期化
				skinned_meshes[1]->update_animation(e.keyframe);
				// ランダムでタイプ決定 (0:赤, 1:青, 2:岩)
				e.type = rand() % 3;
				enemies.push_back(e);
			}
		}
	}

	// 通り過ぎた敵の削除
	auto it = enemies.begin();
	while (it != enemies.end())
	{
		if (it->position.z < playerZ - 10.0f || !it->isAlive)
		{
			it = enemies.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void SceneGame::UpdateCamera()
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
}

void SceneGame::CheckCollisions()
{
	if (player.knockbackTimer > 0.0f) return;

	const float hitRadius = 1.0f;
	const float hitHeight = 2.0f;

	for (auto& enemy : enemies)
	{
		if (!enemy.isAlive) continue;

		float dx = enemy.position.x - player.position.x;
		float dz = enemy.position.z - player.position.z;
		float distXZ = sqrtf(dx * dx + dz * dz);
		float dy = fabsf(player.position.y - enemy.position.y);

		if (distXZ < hitRadius && dy < hitHeight)
		{
			// 敵でも岩でもぶつかったらノックバック
			player.knockbackVelocityZ = -30.0f;
			player.knockbackTimer = 2.5f;
			player.moveSpeed = 15.0f;
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
	//fw_->data.camera_position = DirectX::XMFLOAT4(camera_position.x, camera_position.y, camera_position.z, 1.0f);

	fw_->immediate_context->UpdateSubresource(fw_->constant_buffers[0].Get(), 0, 0, &fw_->data, 0, 0);
	fw_->immediate_context->VSSetConstantBuffers(1, 1, fw_->constant_buffers[0].GetAddressOf());
	fw_->immediate_context->PSSetConstantBuffers(1, 1, fw_->constant_buffers[0].GetAddressOf());

	const DirectX::XMFLOAT4X4 coordinate_system_transforms[]{
{ -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },	// 0:RHS Y-UP
{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },		// 1:LHS Y-UP
{ -1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },	// 2:RHS Z-UP
{ 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },		// 3:LHS Z-UP
	};
	const float scale_factor = 1.0f; // To change the units from centimeters to meters, set 'scale_factor' to 0.01.

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
		skinned_meshes[0]->render(fw_->immediate_context.Get(), world, playerColor, &player.keyframe, true);
	}

	// -----------------------------------------------------------
	// 敵描画
	// -----------------------------------------------------------
	// 敵 & 岩 (ここで分岐)
	if (skinned_meshes[1] && skinned_meshes[4])
	{
		for (const auto& enemy : enemies)
		{
			if (!enemy.isAlive) continue;

			DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z);

			// 岩(2)か、敵(0,1)かで描画メッシュを変える
			if (enemy.type == 2)
			{
				// 岩の描画
				DirectX::XMMATRIX S = DirectX::XMMatrixScaling(rockScale.x, rockScale.y, rockScale.z);
				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, C * S * T); // 回転なし

				// グレー色で描画
				skinned_meshes[4]->render(fw_->immediate_context.Get(), world, { 0.4f, 0.4f, 0.4f, 1.0f }, nullptr, true);
			}
			else
			{
				// 敵の描画
				DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(180));
				DirectX::XMMATRIX S = DirectX::XMMatrixScaling(enemyScale.x, enemyScale.y, enemyScale.z);
				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, C * S * R * T);

				DirectX::XMFLOAT4 enemyColor = (enemy.type == 0) ? DirectX::XMFLOAT4(1, 0.5f, 0.5f, 1) : DirectX::XMFLOAT4(0.5f, 0.5f, 1, 1);
				skinned_meshes[1]->render(fw_->immediate_context.Get(), world, enemyColor, &enemy.keyframe);
			}
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
	if (skinned_meshes[3])
	{
		if (attack_state)
		{
			DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(player.position.x, player.position.y, player.position.z + 0.1);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(0.5f, 0.1f, Boxes.length);
			DirectX::XMFLOAT4X4 world;
			DirectX::XMStoreFloat4x4(&world, C * S * T);
			skinned_meshes[3]->render(fw_->immediate_context.Get(), world, attack_c, nullptr, true);
		}
	}

	fw_->framebuffers[0]->deactivate(fw_->immediate_context.Get());
	if (fw_->enable_bloom)
	{
		// Bloom 作成（抽出 + Blur 済みテクスチャ生成）
		fw_->bloomer->make(fw_->immediate_context.Get(), fw_->framebuffers[0]->shader_resource_views[0].Get());

		// Bloom 合成用フレームバッファ
		fw_->framebuffers[1]->clear(fw_->immediate_context.Get());
		fw_->framebuffers[1]->activate(fw_->immediate_context.Get());

		fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);

		fw_->immediate_context->RSSetState(fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get());

		fw_->immediate_context->OMSetBlendState(fw_->blend_states[static_cast<size_t>(BLEND_STATE::ALPHA)].Get(), nullptr, 0xFFFFFFFF);
		ID3D11ShaderResourceView* bloom_srvs[] =
		{
			fw_->framebuffers[0]->shader_resource_views[0].Get(), // Scene
			fw_->bloomer->shader_resource_view(),                  // Bloom
		};

		// FINAL PASS（Bloom 合成）
		fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), bloom_srvs, 0, 2, fw_->pixel_shaders[2].Get());
		fw_->framebuffers[1]->deactivate(fw_->immediate_context.Get());

		fw_->framebuffers[1]->deactivate(fw_->immediate_context.Get());
	}

	// Radial Blur
	if (fw_->enable_radial_blur)
	{
		fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);

		fw_->immediate_context->RSSetState(fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get());

		ID3D11ShaderResourceView* blur_srvs[] =
		{
			fw_->framebuffers[1]->shader_resource_views[0].Get() // Bloom 後
		};

		// BLUR PASS
		fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), blur_srvs, 0, 1, fw_->pixel_shaders[0].Get());
	}
	else
	{
		// Blur なし → Bloom 結果をそのまま出力
		fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), fw_->framebuffers[1]->shader_resource_views[0].GetAddressOf(), 0, 1, fw_->pixel_shaders[2].Get());
	}

	int speed = static_cast<int>(player.moveSpeed / P_ACCELE);
	//int speed = static_cast<int>(player.moveSpeed / 2.5f);

	DrawNumber(speed, 200, 30, 0.6f, fw_->immediate_context.Get());
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
void SceneGame::DrawNumber(int number, float x, float y, float scale, ID3D11DeviceContext* ctx)
{
	const float cellW = 256.0f;
	const float cellH = 303.33f;

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
			posX, y,         // 描画位置
			cellW * scale, cellH * scale,    // 描画サイズ
			1, 1, 1, 1,         // 色
			0.0f,            // 回転
			sx, sy,
			sw, sh   // 切り出し範囲 ←ここが重要!!
		);
		sprite_batches[0]->end(fw_->immediate_context.Get());

		posX += cellW / 2; // 次の桁へ移動
	}
}