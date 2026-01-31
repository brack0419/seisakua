#include "System/Graphics.h"
#include "System/Input.h"
#include "SceneLoading.h"
#include "SceneManager.h"
#include "framework.h"

SceneLoading::SceneLoading(Scene* nextScene, framework* fw) : nextScene(nextScene), fw_(fw)
{
}

//初期化
void SceneLoading::Initialize()
{
	HRESULT hr{ S_OK };

	//スプライト初期化
	skinned_meshes[1] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\cat.cereal");

	//スレッド開始
	thread = new std::thread(LoadingThread, this);

	ShowCursor(false);
	fw_->distance = 20.0f;

	fw_->camera_position.x = 7.712f;
	fw_->camera_position.y = -0.130f;
	fw_->camera_position.z = -28.445f;
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

	skinned_meshes[1].reset();
}

//更新処理
void SceneLoading::Update(float elapsedTime)
{
	constexpr float speed = 180;
	angle += speed * elapsedTime;

	//次のシーンの準備が完了したらシーンを切り替える
	if (nextScene->IsReady())
	{
		SceneManager::instance().ChangeScene(nextScene);
		nextScene = nullptr;
	}

	if (!skinned_meshes[1]->animation_clips.empty())
	{
		animation& anim = skinned_meshes[1]->animation_clips[object1_anim_index];

		object1_anim_time += elapsedTime;

		int frame = static_cast<int>(object1_anim_time * anim.sampling_rate);
		if (frame >= static_cast<int>(anim.sequence.size()))
		{
			frame = 0;
			object1_anim_time = 0.0f;
		}

		object1_keyframe = anim.sequence[frame];
	}
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

	{
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
			fw_->translation_object3.x,
			fw_->translation_object3.y,
			fw_->translation_object3.z
		);
		DirectX::XMMATRIX Scale = DirectX::XMMatrixScaling(1, 1, 1);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
			fw_->rotation_object3.x,
			fw_->rotation_object3.y,
			fw_->rotation_object3.z
		);
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, C * Scale * R * T);
		bool prev_flat = fw_->flat_shading;
		fw_->flat_shading = true; // Object3 はフラット描画
		skinned_meshes[1]->render(fw_->immediate_context.Get(), world, fw_->material_color, &object1_keyframe, fw_->flat_shading);
		fw_->flat_shading = prev_flat;
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
void SceneLoading::DrawGUI()
{
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