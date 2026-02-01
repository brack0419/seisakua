#include "SceneTutorial.h"
#include "SceneTitle.h"
#include "SceneManager.h"
#include "shader.h"
#include "texture.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

#undef min
#undef max

static const float TUTORIAL_STAGE_LENGTH = 180.0f;

static int tutorialDefeatedRed = 0;
static int tutorialDefeatedBlue = 0;

SceneTutorial::SceneTutorial(HWND hwnd, framework* fw) : hwnd(hwnd), fw_(fw)
{
}

void SceneTutorial::Initialize()
{
	HRESULT hr{ S_OK };
	ShowCursor(FALSE);

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

	sprite_batches[0] = std::make_unique<sprite_batch>(fw_->device.Get(), L".\\resources\\fontS.png", 1);
	sprite_batches[1] = std::make_unique<sprite_batch>(fw_->device.Get(), L".\\resources\\tutorial\\text.dds", 1);
	sprite_batches[2] = std::make_unique<sprite_batch>(fw_->device.Get(), L".\\resources\\tutorial\\text1.dds", 1);
	sprite_batches[3] = std::make_unique<sprite_batch>(fw_->device.Get(), L".\\resources\\tutorial\\text2.dds", 1);
	sprite_batches[4] = std::make_unique<sprite_batch>(fw_->device.Get(), L".\\resources\\tutorial\\text3.dds", 1);
	sprite_batches[5] = std::make_unique<sprite_batch>(fw_->device.Get(), L".\\resources\\tutorial\\text4.dds", 1);

	SE_PANCHI = Audio::Instance().LoadAudioSource(".\\resources\\panchi.wav");
	SE_KICK = Audio::Instance().LoadAudioSource(".\\resources\\kick.wav");
	SE_MISS = Audio::Instance().LoadAudioSource(".\\resources\\miss.wav");

	playerScale = { 0.02f, 0.02f, 0.02f };
	rockScale = { 0.12f, 0.12f, 0.12f };

	player.position = { 0, 0, 0 };
	player.currentLane = 1;
	player.moveSpeed = 15.0f;
	player.isGround = true;
	player.knockbackTimer = 0.0f;

	fw_->bloomer->bloom_extraction_threshold = 0.2f;
	fw_->bloomer->bloom_intensity = 0.25f;

	stages.clear();
	for (int i = 0; i < MAX_STAGE_TILES; ++i)
	{
		StageObject s;
		s.position = { 0, 0, i * TUTORIAL_STAGE_LENGTH };
		stages.push_back(s);
	}

	fw_->light_direction = DirectX::XMFLOAT4{ 0.0f, -1.0f, 1.0f, 0.0f };

	currentStep = TutorialStep::Welcome;
	stepTimer = 0.0f;
	actionDone = false;
	isSlide = false;
	isAttackAnim = false;
	isJumpAnim = false;
	knockState = KnockState::None;

	knockBlurTimer = 0.0f;
	shakeTimer = 0.0f;
	shakePower = 0.0f;
	cameraKickBack = 0.0f;

	tutorialDefeatedRed = 0;
	tutorialDefeatedBlue = 0;

	fw_->radial_blur_data.blur_strength = 0.0f;
	fw_->radial_blur_data.blur_radius = 0.0f;
	fw_->radial_blur_data.blur_decay = 0.0f;
	UpdateCamera(0.0f);
}

void SceneTutorial::Finalize()
{
	ShowCursor(TRUE);
	for (auto& m : skinned_meshes) m.reset();
	sprite_batches[0].reset();
	sprite_batches[1].reset();
	sprite_batches[2].reset();
	sprite_batches[3].reset();
	sprite_batches[4].reset();
	sprite_batches[5].reset();

	if (SE_PANCHI) { delete SE_PANCHI; SE_PANCHI = nullptr; }
	if (SE_KICK) { delete SE_KICK; SE_KICK = nullptr; }
	if (SE_MISS) { delete SE_MISS; SE_MISS = nullptr; }
}

void SceneTutorial::Update(float elapsed_time)
{
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

	UpdatePlayer(elapsed_time);
	InputAttack();

	for (auto& enemy : enemies)
	{
		if (enemy.isBlownAway)
		{
			enemy.blowVelocity.y -= 40.0f * elapsed_time;

			enemy.position.x += enemy.blowVelocity.x * elapsed_time;
			enemy.position.y += enemy.blowVelocity.y * elapsed_time;
			enemy.position.z += enemy.blowVelocity.z * elapsed_time;

			enemy.blowAngleX += 720.0f * elapsed_time;
			enemy.blowAngleY += 360.0f * elapsed_time;

			if (enemy.position.y < -20.0f)
			{
				enemy.isAlive = false;
			}
		}
	}

	UpdateTutorialFlow(elapsed_time);
	UpdateCamera(elapsed_time);

	if (player.knockbackTimer <= 0.0f && knockState == KnockState::None)
	{
		for (auto& enemy : enemies)
		{
			if (!enemy.isAlive || enemy.isBlownAway) continue;

			float dx = enemy.position.x - player.position.x;
			float dz = enemy.position.z - player.position.z;
			float distXZ = sqrtf(dx * dx + dz * dz);
			float dy = fabsf(player.position.y - enemy.position.y);

			if (distXZ < 1.0f && dy < 2.0f)
			{
				player.knockbackVelocityZ = -30.0f;
				player.knockbackTimer = 2.5f;

				knockState = KnockState::Knocked;
				knocked_anim_time = 0.0f;

				if (enemy.type == 0)
				{
					enemy.animState = EnemyAnimState::Kick;
					enemy.animationTime = 0.0f;
				}
			}
		}
	}
}

void SceneTutorial::UpdateTutorialFlow(float elapsed_time)
{
	stepTimer += elapsed_time;
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
		if (!actionDone && player.currentLane != 1)
		{
			actionDone = true;
			stepTimer = 0.0f;
		}
		if (actionDone && stepTimer > 2.0f)
		{
			currentStep = TutorialStep::Jump;
			stepTimer = 0.0f;
			actionDone = false;
			SpawnObstacle();
		}
		break;

	case TutorialStep::Jump:
		if (player.position.y > 1.0f) actionDone = true;
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
		if (tutorialDefeatedRed >= 3 && tutorialDefeatedBlue >= 3)
		{
			actionDone = true;
		}

		if (!actionDone)
		{
			bool existsActiveEnemy = false;
			for (const auto& e : enemies)
			{
				if (e.isAlive && !e.isBlownAway)
				{
					if (e.position.z > player.position.z - 5.0f)
					{
						existsActiveEnemy = true;
						break;
					}
				}
			}

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
	if (currentStep != prevStep)
	{
		tutorialTextTimer = 0.0f;
		tutorialTextScale = 0.8f;
		prevStep = currentStep;
	}

	tutorialTextTimer += elapsed_time;

	// スケールイン（イージング）
	float t = std::min(tutorialTextTimer / 0.4f, 1.0f);
	tutorialTextScale = 0.8f + (1.0f - 0.8f) * (1.0f - powf(1.0f - t, 3));
}

void SceneTutorial::SpawnObstacle()
{
	for (int i = -1; i <= 1; ++i)
	{
		Enemy e;
		e.position = { i * player.laneWidth, 0, player.position.z + 40.0f };
		e.type = 2;
		e.isAlive = true;
		e.animState = EnemyAnimState::Run;
		enemies.push_back(e);
	}
}

void SceneTutorial::SpawnDummyEnemy()
{
	for (int i = -1; i <= 1; ++i)
	{
		Enemy e;
		e.position = { i * player.laneWidth, 0, player.position.z + 40.0f };
		e.type = rand() % 2;
		e.isAlive = true;
		e.animState = EnemyAnimState::Run;

		if (e.type == 0 && skinned_meshes[1]) e.keyframe = skinned_meshes[1]->animation_clips[0].sequence[0];
		if (e.type == 1 && skinned_meshes[5]) e.keyframe = skinned_meshes[5]->animation_clips[0].sequence[0];

		enemies.push_back(e);
	}
}

void SceneTutorial::UpdatePlayer(float elapsed_time)
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

	if (player.position.y <= 0.0f)
	{
		player.position.y = 0.0f;
		player.velocity.y = 0.0f;
		player.isGround = true;
	}

	if (!skinned_meshes[0]->animation_clips.empty())
	{
		animation& anim = skinned_meshes[0]->animation_clips[player_anim_index];

		player_anim_time += elapsed_time;

		int frame =
			static_cast<int>(player_anim_time * anim.sampling_rate);

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

		if (enemy.animState == EnemyAnimState::Kick)
		{
			mesh = skinned_meshes[11].get();
			time = &enemy.kickAnimTime;
		}
		else
		{
			mesh = skinned_meshes[enemy.type == 0 ? 1 : 5].get();
			time = &enemy.runAnimTime;
		}

		if (!mesh || mesh->animation_clips.empty()) continue;

		animation& anim = mesh->animation_clips[0];

		*time += elapsed_time;
		int frame = static_cast<int>(*time * anim.sampling_rate);

		if (frame >= (int)anim.sequence.size())
		{
			if (enemy.animState == EnemyAnimState::Kick)
			{
				enemy.animState = EnemyAnimState::Run;
				enemy.kickAnimTime = 0.0f;
				enemy.runAnimTime = 0.0f;
				continue;
			}
			else
			{
				*time = 0.0f;
				frame = 0;
			}
		}

		enemy.keyframe = anim.sequence[frame];
	}

	if (isSlide && skinned_meshes[6] && !skinned_meshes[6]->animation_clips.empty())
	{
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

		const float SLIDE_DURATION = 0.75f;

		if (slideTimer >= SLIDE_DURATION)
		{
			isSlide = false;
			slideTimer = 0.0f;
			slide_anim_time = 0.0f;
		}

		return;
	}

	blurFade -= elapsed_time * BLUR_FADE_SPEED;
	blurFade = std::clamp(blurFade, 0.0f, 1.0f);

	float eased = 1.0f - powf(1.0f - blurFade, 2.0f);

	fw_->radial_blur_data.blur_strength = 0.2f * eased;
	fw_->radial_blur_data.blur_radius = 2.0f * eased;
	fw_->radial_blur_data.blur_decay = 0.0f;

	if (isAttackAnim && skinned_meshes[7] && !skinned_meshes[7]->animation_clips.empty())
	{
		attackAnimTimer += elapsed_time;

		animation& anim = skinned_meshes[7]->animation_clips[0];

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
	if (knockState != KnockState::None)
	{
		if (knockState == KnockState::Knocked &&
			skinned_meshes[8] &&
			!skinned_meshes[8]->animation_clips.empty())
		{
			animation& anim = skinned_meshes[8]->animation_clips[0];

			knocked_anim_time += elapsed_time * KNOCKED_ANIM_SPEED;
			int frame = static_cast<int>(knocked_anim_time * anim.sampling_rate);

			if (frame >= static_cast<int>(anim.sequence.size()) - 1)
			{
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

		float k = knockBlurTimer / 0.3f;
		fw_->radial_blur_data.blur_strength = 0.35f * k;
		fw_->radial_blur_data.blur_radius = 1.5f * k;
		fw_->radial_blur_data.blur_decay = 0.0f;
	}
	if (isJumpAnim && skinned_meshes[10] && !skinned_meshes[10]->animation_clips.empty())
	{
		animation& anim = skinned_meshes[10]->animation_clips[0];

		const float JUMP_ANIM_DURATION = 1.0f;

		jump_anim_time += elapsed_time;

		float t = jump_anim_time / JUMP_ANIM_DURATION;
		t = std::clamp(t, 0.0f, 1.0f);

		int frame = static_cast<int>(t * anim.sequence.size());
		if (frame >= static_cast<int>(anim.sequence.size()))
		{
			frame = static_cast<int>(anim.sequence.size()) - 1;
		}

		player.keyframe = anim.sequence[frame];

		if (player.isGround || t >= 1.0f)
		{
			isJumpAnim = false;
			jump_anim_time = 0.0f;
		}

		return;
	}
}

void SceneTutorial::InputAttack()
{
	bool nowLButton = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
	bool nowRButton = (GetKeyState(VK_RBUTTON) & 0x8000) != 0;

	if (!attack_state && player.isGround && knockState == KnockState::None)
	{
		if (nowLButton && !prevLButton)
		{
			attack_state = true;
			attack_hit_enable = true;
			attack_timer = 0;
			attack_type = 1;
			attack_c = { 1,0,0,1 };

			isAttackAnim = true;
			attackAnimTimer = 0.0f;
			attack_anim_time = 0.0f;
		}
		else if (nowRButton && !prevRButton)
		{
			attack_state = true;
			attack_hit_enable = true;
			attack_timer = 0;
			attack_type = 2;
			attack_c = { 0,0,1,1 };

			isSlide = true;
			slideTimer = 0.0f;
			slide_anim_time = 0.0f;
		}
	}

	if (attack_hit_enable)
	{
		Boxes.length = 20.0f;
		for (auto& enemy : enemies)
		{
			if (!enemy.isAlive || enemy.isBlownAway || enemy.type == 2) continue;

			float distZ = enemy.position.z - player.position.z;
			if (distZ > 0.0f && distZ < 20.0f && fabsf(enemy.position.x - player.position.x) < 2.5f)
			{
				if ((attack_type == 1 && enemy.type == 0) || (attack_type == 2 && enemy.type == 1))
				{
					enemy.isBlownAway = true;

					float blowUp = 15.0f;
					float blowForward = 25.0f;
					float blowSide = (attack_type == 2) ? 5.0f : -5.0f;

					enemy.blowVelocity = { blowSide, blowUp, blowForward };

					if (enemy.type == 0) tutorialDefeatedRed++;
					if (enemy.type == 1) tutorialDefeatedBlue++;

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
		if (attack_timer == 20) attack_hit_enable = false;
		if (attack_timer >= 25) { attack_state = false; attack_timer = 0; }
	}
	prevLButton = nowLButton;
	prevRButton = nowRButton;
}

void SceneTutorial::UpdateCamera(float elapsed_time)
{
	camera_focus = player.position;
	camera_focus.y += 2.0f;
	DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&camera_focus);
	float sx = sinf(rotateX), cx = cosf(rotateX);
	float sy = sinf(rotateY), cy = cosf(rotateY);
	DirectX::XMVECTOR Offset = DirectX::XMVectorScale(DirectX::XMVectorSet(cx * sy, -sx, cx * cy, 0.0f), distance);

	DirectX::XMStoreFloat3(&camera_position, DirectX::XMVectorSubtract(Focus, Offset));

	if (shakeTimer > 0.0f)
	{
		shakeTimer -= elapsed_time;

		float t = shakeTimer * 20.0f;
		float shakeX = (sinf(t * 12.3f)) * shakePower;
		float shakeY = (cosf(t * 9.7f)) * shakePower;

		camera_position.x += shakeX;
		camera_position.y += shakeY;
	}
	if (cameraKickBack > 0.0f)
	{
		cameraKickBack -= elapsed_time * 8.0f;
	}
	cameraKickBack = (cameraKickBack < 0.0f) ? 0.0f : cameraKickBack;

	Offset = DirectX::XMVectorScale(DirectX::XMVectorSet(cx * sy, -sx, cx * cy, 0.0f), distance + cameraKickBack);
	DirectX::XMStoreFloat3(&camera_position, DirectX::XMVectorSubtract(Focus, Offset));
}

void SceneTutorial::Render()
{
	fw_->framebuffers[0]->clear(fw_->immediate_context.Get());
	fw_->framebuffers[0]->activate(fw_->immediate_context.Get());

	fw_->immediate_context->OMSetBlendState(fw_->blend_states[static_cast<size_t>(BLEND_STATE::ALPHA)].Get(), nullptr, 0xFFFFFFFF);
	fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_ON_ZW_ON)].Get(), 0);
	fw_->immediate_context->RSSetState(fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::SOLID)].Get());

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

	DirectX::XMMATRIX C = DirectX::XMMatrixIdentity();
	C.r[0].m128_f32[0] = -1.0f;

	if (skinned_meshes[0])
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, C * DirectX::XMMatrixScaling(playerScale.x, playerScale.y, playerScale.z) * DirectX::XMMatrixTranslation(player.position.x, player.position.y, player.position.z));

		int meshIndex = 0;
		if (knockState == KnockState::Knocked)      meshIndex = 8;
		else if (knockState == KnockState::Recover) meshIndex = 9;
		else if (isSlide)                           meshIndex = 6;
		else if (isAttackAnim)                      meshIndex = 7;
		else if (isJumpAnim)                        meshIndex = 10;

		if (skinned_meshes[meshIndex])
			skinned_meshes[meshIndex]->render(fw_->immediate_context.Get(), world, playerColor, &player.keyframe, false);
	}

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

	if (skinned_meshes[2])
	{
		for (const auto& s : stages)
		{
			DirectX::XMFLOAT4X4 world;
			DirectX::XMStoreFloat4x4(&world, C * DirectX::XMMatrixScaling(0.3f, 0.3f, 0.3f) * DirectX::XMMatrixTranslation(s.position.x, s.position.y, s.position.z));
			skinned_meshes[2]->render(fw_->immediate_context.Get(), world, { 1,1,1,1 }, nullptr, false);
		}
	}
	if (attack_state && skinned_meshes[3])
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, C * DirectX::XMMatrixScaling(0.5f, 0.1f, Boxes.length) * DirectX::XMMatrixTranslation(player.position.x, player.position.y, player.position.z + 0.1f));
		skinned_meshes[3]->render(fw_->immediate_context.Get(), world, attack_c, nullptr, true);
	}

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
	DrawNumber((int)currentStep + 1, 100, 30, 0.5f, fw_->immediate_context.Get());
}

void SceneTutorial::DrawNumber(int number, float x, float y, float scale, ID3D11DeviceContext* ctx)
{
	const float BASE_W = 2000.0;
	const float BASE_H = 2000.0;

	int spriteIndex = -1;

	switch (currentStep)
	{
	case TutorialStep::Welcome: spriteIndex = 1; break;
	case TutorialStep::Move:    spriteIndex = 2; break;
	case TutorialStep::Jump:    spriteIndex = 3; break;
	case TutorialStep::Attack:  spriteIndex = 4; break;
	case TutorialStep::Complete:  spriteIndex = 5; break;
	default: break;
	}

	float floatY = sinf(stepTimer * 1.6f) * 20.0f;

	float pop = 0.0f;
	if (tutorialTextTimer < 0.15f)
	{
		float p = tutorialTextTimer / 0.15f;
		pop = (1.0f - p) * 0.2f; // 最大 +20%
	}
	if (tutorialTextTimer < 0.2f)
	{
		fw_->radial_blur_data.blur_strength = 0.15f;
	}
	if (spriteIndex != -1 && sprite_batches[spriteIndex])
	{
		const float BASE_W = 2000.0f;
		const float BASE_H = 2000.0f;

		sprite_batches[spriteIndex]->begin(ctx);

		sprite_batches[spriteIndex]->render(
			ctx,
			spritePos[spriteIndex - 1].x,
			spritePos[spriteIndex - 1].y + floatY,
			BASE_W * tutorialSpriteScale * tutorialTextScale * (1.0f + pop),
			BASE_H * tutorialSpriteScale * tutorialTextScale * (1.0f + pop)
		);

		sprite_batches[spriteIndex]->end(ctx);
	}
}

void SceneTutorial::DrawGUI()
{
#ifdef USE_IMGUI
	ImGui::Begin("Game Settings");
	ImGui::End();
#endif
}