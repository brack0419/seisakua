#include "System/Graphics.h"
#include "System/Input.h"
#include "SceneLoading.h"
#include "SceneManager.h"
#include "framework.h"
#include <random>

SceneLoading::SceneLoading(Scene* nextScene, framework* fw) : nextScene(nextScene), fw_(fw)
{
}

//初期化
void SceneLoading::Initialize()
{
	HRESULT hr{ S_OK };

	//スプライト初期化
	skinned_meshes[0] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\load.cereal");
	skinned_meshes[1] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\load1.cereal");
	skinned_meshes[2] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\load2.cereal");
	skinned_meshes[3] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\load4.cereal");
	skinned_meshes[4] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\load5.cereal");
	skinned_meshes[5] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\player_idle.cereal");
	skinned_meshes[6] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\player_idle2.cereal");
	skinned_meshes[7] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\player_idle3.cereal");
	skinned_meshes[8] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\loading.cereal");

	ShowCursor(TRUE);
	//スレッド開始
	thread = new std::thread(LoadingThread, this);

	ShowCursor(false);
	fw_->distance = 20.0f;

	fw_->camera_position.x = 7.712f;
	fw_->camera_position.y = -0.130f;
	fw_->camera_position.z = -28.445f;
	for (int i = 0; i < MESH_COUNT; ++i)
	{
		anim_times[i] = 0.0f;
		anim_indices[i] = 0;
	}

	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_int_distribution<int> meshRand(0, MESH_COUNT - 1);

	std::uniform_real_distribution<float> posX(-40.0f, 40.0f);
	std::uniform_real_distribution<float> posY(-22.0f, 22.0f);


	std::uniform_real_distribution<float> posZ(5.0f, 25.0f);


	std::uniform_real_distribution<float> vel(-0.1f, 0.1f);
	std::uniform_real_distribution<float> phase(0.0f, DirectX::XM_2PI);

	for (int i = 0; i < INSTANCE_COUNT; ++i)
	{
		instances[i].meshIndex = meshRand(gen);

		instances[i].centerPos = {
			posX(gen),
			posY(gen),
			posZ(gen)
		};

		instances[i].velocity = {
			vel(gen),
			vel(gen),
			0.0f
		};

		instances[i].phase = phase(gen);
	}



}

//終了化
void SceneLoading::Finalize()
{
	//スレッド終了化
	if (thread != nullptr)
	{
		thread->join();
		delete thread;
		thread = nullptr;
	}

	//スプライト終了化
	//if (sprite != nullptr)
	//{
	//	delete sprite;
	//	sprite = nullptr;
	//}
	ShowCursor(FALSE);
	skinned_meshes[1].reset();
}

//更新処理
void SceneLoading::Update(float elapsedTime)
{
	constexpr float speed = 180;
	angle += speed * elapsedTime;

	for (int i = 0; i < MESH_COUNT; ++i)
	{
		if (!skinned_meshes[i]) continue;
		if (skinned_meshes[i]->animation_clips.empty()) continue;

		animation& anim = skinned_meshes[i]->animation_clips[anim_indices[i]];

		anim_times[i] += elapsedTime;

		int frame = static_cast<int>(anim_times[i] * anim.sampling_rate);
		if (frame >= static_cast<int>(anim.sequence.size()))
		{
			frame = 0;
			anim_times[i] = 0.0f;
		}

		keyframes[i] = anim.sequence[frame];
	}

	if (nextScene->IsReady())
	{
		SceneManager::instance().ChangeScene(nextScene);
		nextScene = nullptr;
	}
	for (int i = 0; i < MESH_COUNT; ++i)
	{
		modelPos[i].x += modelVel[i].x * elapsedTime;
		modelPos[i].y += modelVel[i].y * elapsedTime;

		floatPhase[i] += elapsedTime;

		// ごく弱い無重力ゆらぎ
		modelPos[i].y += sinf(floatPhase[i]) * 0.004f;
		modelPos[i].x += cosf(floatPhase[i]) * 0.003f;
	}
	constexpr float FLOAT_RADIUS = 3.0f;

	for (int i = 0; i < INSTANCE_COUNT; ++i)
	{
		auto& inst = instances[i];

		inst.phase += elapsedTime;

		// 中心をゆっくり動かす
		inst.centerPos.x += inst.velocity.x * elapsedTime;
		inst.centerPos.y += inst.velocity.y * elapsedTime;

		// 外に行きすぎたら少し戻す（超重要）
		if (fabs(inst.centerPos.x) > 55.0f)
			inst.velocity.x *= -1.0f;

		if (fabs(inst.centerPos.y) > 32.0f)
			inst.velocity.y *= -1.0f;

	}

	POINT mouse;
	GetCursorPos(&mouse);
	ScreenToClient(fw_->hwnd, &mouse);

	// 画面 → 疑似ワールド変換（超ざっくりでOK）
	float mouseX = (mouse.x - 990.0f) * 0.06f;   // 1980/2
	float mouseY = -(mouse.y - 540.0f) * 0.06f;  // 1080/2

	// 1. マウスの状態を取得
	bool currentMouseState = (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
	static bool prevMouseState = false; // 前のフレームの状態を保持

	// 2. 「今押されていて、かつ前は押されていなかった」瞬間を判定
	bool isClicked = currentMouseState && !prevMouseState;

	// ★重要：次のフレームのために今の状態を保存（これをループの前か後に入れる）
	prevMouseState = currentMouseState;

	for (int i = 0; i < INSTANCE_COUNT; ++i)
	{
		auto& inst = instances[i];
		float dx = mouseX - inst.centerPos.x;
		float dy = mouseY - inst.centerPos.y;
		float dist = sqrtf(dx * dx + dy * dy) + 0.001f;

		const float influenceRadius = 10.0f;

		// isClicked が true の時（クリックした瞬間）だけこの中に入る
		if (isClicked && dist < influenceRadius)
		{
			float blastFactor = (1.0f - dist / influenceRadius);
			float baseAngle = atan2f(-dy, -dx);
			float randomOffset = ((float)rand() / RAND_MAX - 0.5f) * (DirectX::XM_PI * 1.5f);
			float finalAngle = baseAngle + randomOffset;

			// 弱めに設定（前回の調整値）
			float clickPower = 1.5f * blastFactor;

			inst.velocity.x = cosf(finalAngle) * clickPower;
			inst.velocity.y = sinf(finalAngle) * clickPower;
		}
		else
		{
			// 慣性でゆっくり止まる
			inst.velocity.x *= 0.95f;
			inst.velocity.y *= 0.95f;
		}

		// 座標更新
		inst.centerPos.x += inst.velocity.x;
		inst.centerPos.y += inst.velocity.y;


		// --- 画面外に出すぎた場合の安全策（任意） ---
		// あまりに遠くへ行き過ぎたら、中心方向へ微弱な力を加えるか
		// 画面端で跳ね返る処理を入れると、ローディング中に全消えするのを防げます
		if (fabs(inst.centerPos.x) > 60.0f) inst.velocity.x *= -0.8f;
		if (fabs(inst.centerPos.y) > 35.0f) inst.velocity.y *= -0.8f;
	}
	// mesh8 (Index 8) のアニメーション更新を確実に行う
	if (skinned_meshes[8] && !skinned_meshes[8]->animation_clips.empty())
	{
		// 0番目のアニメーションを使用
		animation& anim = skinned_meshes[8]->animation_clips[0];

		// 経過時間でフレームを進める
		mesh8AnimTime += elapsedTime;

		int frame = static_cast<int>(mesh8AnimTime * anim.sampling_rate);
		if (frame >= static_cast<int>(anim.sequence.size()))
		{
			frame = 0;
			mesh8AnimTime = 0.0f; // ループ
		}

		mesh8Keyframe = anim.sequence[frame];
	}



	// 任意で浮かせたい場合
	mesh8FloatPhase += elapsedTime;


}

//描画処理
void SceneLoading::Render()
{
	using namespace DirectX;

	fw_->framebuffers[0]->clear(fw_->immediate_context.Get());
	fw_->framebuffers[0]->activate(fw_->immediate_context.Get());

	fw_->immediate_context->OMSetDepthStencilState(
		fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_ON_ZW_ON)].Get(), 0
	);
	fw_->immediate_context->RSSetState(
		fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::SOLID)].Get()
	);
	fw_->immediate_context->OMSetBlendState(
		fw_->blend_states[static_cast<size_t>(BLEND_STATE::ALPHA)].Get(),
		nullptr,
		0xFFFFFFFF
	);

	const DirectX::XMFLOAT4X4 coordinate_system_transforms[]{
{ -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },	// 0:RHS Y-UP
{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },		// 1:LHS Y-UP
{ -1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },	// 2:RHS Z-UP
{ 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },		// 3:LHS Z-UP
	};

	const float scale_factor = 1.0f; // To change the units from centimeters to meters, set 'scale_factor' to 0.01.

	const float spacing = 2.0f;

	DirectX::XMMATRIX C{ DirectX::XMLoadFloat4x4(&coordinate_system_transforms[0]) * DirectX::XMMatrixScaling(scale_factor, scale_factor, scale_factor) };

	DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(fw_->scaling.x, fw_->scaling.y, fw_->scaling.z) };
	DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(fw_->rotation.x, fw_->rotation.y, fw_->rotation.z) };
	DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(fw_->translation.x, fw_->translation.y, fw_->translation.z) };
	DirectX::XMFLOAT4X4 world;

	for (int i = 0; i < INSTANCE_COUNT; ++i)
	{
		auto& inst = instances[i];
		auto& mesh = skinned_meshes[inst.meshIndex];
		if (!mesh) continue;

		float x = inst.centerPos.x + cosf(inst.phase) * 2.0f;
		float y = inst.centerPos.y + sinf(inst.phase) * 2.0f;
		float z = inst.centerPos.z;

		XMMATRIX T = XMMatrixTranslation(x, y, z);

		// Renderループ内のスケール計算部分
		float baseScale = 0.25f;
		float depthScale = 1.0f / (1.0f + z * 0.05f);

		// 反応しているときのスケール変化を「滑らか」にする
		float targetExtraScale = 1.0f;
		if (dist < 20.0f) {
			// 1.0倍から1.4倍まで、距離に応じてスムーズに変化
			targetExtraScale = 1.0f + (1.0f - dist / 20.0f) * 0.4f;
		}
		// 前のフレームのスケールを保持しているなら線形補完(Lerp)するとさらに綺麗ですが、
		// まずはこれだけで十分自然に見えます。
		float s = baseScale * modelScale * depthScale * targetExtraScale;

		XMMATRIX Scale = XMMatrixScaling(s, s, s);

		XMMATRIX R = XMMatrixRotationRollPitchYaw(
			inst.phase * 0.3f,
			inst.phase * 0.2f,
			inst.phase * 0.4f
		);

		XMFLOAT4X4 world;
		XMStoreFloat4x4(&world, C * Scale * R * T);

		mesh->render(
			fw_->immediate_context.Get(),
			world,
			fw_->material_color,
			&keyframes[inst.meshIndex],
			false
		);

		// --- mesh[8]描画をループ外に独立させる ---
		if (skinned_meshes[8])
		{
			// 上下ふわふわ
			float floatY = sinf(mesh8FloatPhase * 1.5f) * 0.8f;

			// ImGuiで変更可能にする変数例
			// mesh8Pos = XMFLOAT3, mesh8Scale = float, mesh8Rotation = XMFLOAT3
			XMMATRIX T = XMMatrixTranslation(
				mesh8Pos.x,
				mesh8Pos.y + floatY,
				mesh8Pos.z
			);

			XMMATRIX S = XMMatrixScaling(
				mesh8Scale,
				mesh8Scale,
				mesh8Scale
			);

			// 回転を ImGui で調整
			XMMATRIX R = XMMatrixRotationRollPitchYaw(
				mesh8Rotation.x,  // Pitch
				mesh8Rotation.y,  // Yaw
				mesh8Rotation.z   // Roll
			);
			XMFLOAT4 loading_color
			{
				fw_->material_color.x * loading_bloom_boost,
				fw_->material_color.y * loading_bloom_boost,
				fw_->material_color.z * loading_bloom_boost,
				fw_->material_color.w
			};
			// ワールド行列
			XMFLOAT4X4 world;
			XMStoreFloat4x4(&world, C * S * R * T);

			skinned_meshes[8]->render(
				fw_->immediate_context.Get(),
				world,
				loading_color,
				&mesh8Keyframe,  // 0フレーム固定
				false
			);
		}




	}



	fw_->framebuffers[0]->deactivate(fw_->immediate_context.Get());

	if (fw_->enable_bloom)
	{
		fw_->bloomer->make(
			fw_->immediate_context.Get(),
			fw_->framebuffers[0]->shader_resource_views[0].Get()
		);

		fw_->framebuffers[1]->clear(fw_->immediate_context.Get());
		fw_->framebuffers[1]->activate(fw_->immediate_context.Get());

		fw_->immediate_context->OMSetDepthStencilState(
			fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0
		);
		fw_->immediate_context->RSSetState(
			fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get()
		);
		fw_->immediate_context->OMSetBlendState(
			fw_->blend_states[static_cast<size_t>(BLEND_STATE::ALPHA)].Get(),
			nullptr,
			0xFFFFFFFF
		);

		ID3D11ShaderResourceView* bloom_srvs[] =
		{
			fw_->framebuffers[0]->shader_resource_views[0].Get(),
			fw_->bloomer->shader_resource_view()
		};

		fw_->bit_block_transfer->blit(
			fw_->immediate_context.Get(),
			bloom_srvs,
			0,
			2,
			fw_->pixel_shaders[2].Get()
		);

		fw_->framebuffers[1]->deactivate(fw_->immediate_context.Get());
	}

	if (fw_->enable_radial_blur)
	{
		fw_->immediate_context->OMSetDepthStencilState(
			fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0
		);
		fw_->immediate_context->RSSetState(
			fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get()
		);

		ID3D11ShaderResourceView* blur_srvs[] =
		{
			fw_->framebuffers[1]->shader_resource_views[0].Get()
		};

		fw_->bit_block_transfer->blit(
			fw_->immediate_context.Get(),
			blur_srvs,
			0,
			1,
			fw_->pixel_shaders[0].Get()
		);
	}
}

//GUI描画
void SceneLoading::DrawGUI() {
#ifdef USE_IMGUI
#ifdef USE_IMGUI
	ImGui::Begin("Loading Scene");
	ImGui::SliderFloat(
		"Sword Bloom Boost",
		&loading_bloom_boost,
		1.0f,
		100.0f
	);
	ImGui::DragFloat3("Position", &mesh8Pos.x, 0.1f);
	ImGui::SliderFloat("Scale", &mesh8Scale, 0.01f, 10.0f);
	ImGui::DragFloat3("Rotation (Pitch/Yaw/Roll)", &mesh8Rotation.x, 0.01f);

	ImGui::End();
#endif

#endif
}
//ローディングスレッド
void SceneLoading::LoadingThread(SceneLoading* scene)
{
	//COM関連の初期化でスレッド毎に呼ぶ必要がある
	CoInitialize(nullptr);

	//次のシーンの初期化を行う
	scene->nextScene->Initialize();

	//スレッドが終わる前にCOM関連の終了化
	CoUninitialize();

	//次のシーンの準備完了設定
	scene->nextScene->SetReady();
}