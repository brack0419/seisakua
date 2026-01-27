#include "System/Graphics.h"
#include "SceneTitle.h"
#include "SceneEnd.h"
#include "System/Input.h"
#include "SceneGame.h"
#include "SceneLoading.h"
#include "SceneManager.h"
#include "framework.h"
#include "sprite.h"

extern float saveSpeed;

SceneEnd::SceneEnd(framework* fw, float time, int enemyCount)
	: fw_(fw), clearTime(time), defeatedEnemies(enemyCount)
{
	if (!fw_) {
		MessageBox(nullptr, L"fw_ が nullptr です", L"Error", MB_OK);
		return;
	}
}

//初期化
void SceneEnd::Initialize()
{
	HRESULT hr{ S_OK };

	//skinned_meshes[0] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\base.cereal");
	skinned_meshes[0] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\End_building.cereal");
	skinned_meshes[1] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\End_building2.cereal");
	//skinned_meshes[2] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\neon6.cereal");

	sprite_end = std::make_unique<sprite>(fw_->device.Get(), L".\\resources\\owari.png");
	font_batch = std::make_unique<sprite_batch>(fw_->device.Get(), L".\\resources\\fontS.png", 1);
	//fw_->framebuffers[0] = std::make_unique<framebuffer>(fw_->device.Get(), 1920, 1080);
	//fw_->framebuffers[1] = std::make_unique<framebuffer>(fw_->device.Get(), 1920, 1080);
}

//終了化
void SceneEnd::Finalize()
{
	/*skinned_meshes[0].reset();
	skinned_meshes[1].reset();
	skinned_meshes[2].reset();*/
	sprite_end.reset();
}

//更新処理
void SceneEnd::Update(float elaspedTime)
{
	fw_->light_direction = DirectX::XMFLOAT4{ -1.0f, -1.0f, 1.0f, 0.0f };

	fw_->distance = 20.0f;

	//fw_->camera_position.x = 7.712f;
	//fw_->camera_position.y = -0.130f;
	//fw_->camera_position.z = -28.445f;

	//fw_->translation_object3.x = 1.146f;
	//fw_->translation_object3.y = -2.768f;
	//fw_->translation_object3.z = -6.875f;

	// -------------------------
	// マウス座標取得
	// -------------------------
	//POINT mouse_client_pos{};
	//GetCursorPos(&mouse_client_pos);

	// クライアント座標へ変換
	HWND hwnd = FindWindow(APPLICATION_NAME, L"");
	//ScreenToClient(hwnd, &mouse_client_pos);

	// -------------------------
	// 元々の SPACE キー遷移
	// -------------------------
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		SceneManager::instance().ChangeScene(new SceneTitle(fw_));
		return;
	}
}

void SceneEnd::DrawNumber(int number, float x, float y, float scale)
{
	const float cellW = 256.0f;
	const float cellH = 303.33f;

	std::string str = std::to_string(number);
	float posX = x;

	for (char c : str)
	{
		int digit = c - '0';

		// 文字が数字以外(マイナス等)の場合の安全策
		if (digit < 0 || digit > 9) continue;

		int col = digit % 4;
		int row = digit / 4;

		float sx = col * cellW;
		float sy = row * cellH;
		float sw = cellW;
		float sh = cellH;

		// font_batch を使用
		font_batch->begin(fw_->immediate_context.Get());
		font_batch->render(
			fw_->immediate_context.Get(), // 引数を渡すか、fw_から取得
			posX, y,
			cellW * scale, cellH * scale,
			1, 1, 1, 1,
			0.0f,
			sx, sy,
			sw, sh
		);
		font_batch->end(fw_->immediate_context.Get());

		posX += (cellW * scale) * 0.7f; // 文字間隔を調整（少し詰めるなら0.7倍など）
	}
}

//描画処理
void SceneEnd::Render()
{
	// DYNAMIC_TEXTURE
	fw_->data.elapsed_time = elapsed_time;
	fw_->data.time += elapsed_time;

	fw_->immediate_context->UpdateSubresource(fw_->constant_buffers[0].Get(), 0, 0, &fw_->data, 0, 0);
	fw_->immediate_context->VSSetConstantBuffers(1, 1, fw_->constant_buffers[0].GetAddressOf());

	fw_->immediate_context->PSSetConstantBuffers(1, 1, fw_->constant_buffers[0].GetAddressOf());

	fw_->immediate_context->UpdateSubresource(fw_->constant_buffers[1].Get(), 0, 0, &fw_->parametric_constants, 0, 0);
	fw_->immediate_context->PSSetConstantBuffers(2, 1, fw_->constant_buffers[1].GetAddressOf());

	// DYNAMIC_TEXTURE
	if (fw_->enable_dynamic_shader)
	{
		fw_->dynamic_texture->clear(fw_->immediate_context.Get());
		fw_->dynamic_texture->activate(fw_->immediate_context.Get());
		fw_->immediate_context->OMSetDepthStencilState(depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
		fw_->immediate_context->RSSetState(rasterizer_states[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get());
		fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), nullptr, 0, 0, effect_shaders[0].Get());
		fw_->dynamic_texture->deactivate(fw_->immediate_context.Get());
		fw_->immediate_context->PSSetShaderResources(15, 1, fw_->dynamic_texture->shader_resource_views[0].GetAddressOf());
	}
	else
	{
		ID3D11ShaderResourceView* null_srv[] = { nullptr };
		fw_->immediate_context->PSSetShaderResources(15, 1, null_srv);
	}

	// RADIAL_BLUR
	if (fw_->enable_radial_blur)
	{
		fw_->immediate_context->UpdateSubresource(fw_->constant_buffers[1].Get(), 0, 0, &fw_->radial_blur_data, 0, 0);
		fw_->immediate_context->PSSetConstantBuffers(2, 1, fw_->constant_buffers[1].GetAddressOf());
	}

	fw_->framebuffers[0]->clear(fw_->immediate_context.Get());
	fw_->framebuffers[0]->activate(fw_->immediate_context.Get());

	// DYNAMIC_BACKGROUND
	if (fw_->enable_dynamic_background)
	{
		fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
		fw_->immediate_context->RSSetState(fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get());
		fw_->immediate_context->OMSetBlendState(fw_->blend_states[static_cast<size_t>(BLEND_STATE::NONE)].Get(), nullptr, 0xFFFFFFFF);
		fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), nullptr, 0, 0, fw_->effect_shaders[1].Get());
	}

	fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_ON_ZW_ON)].Get(), 0);
	fw_->immediate_context->RSSetState(fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::SOLID)].Get());

	fw_->immediate_context->OMSetBlendState(fw_->blend_states[static_cast<size_t>(BLEND_STATE::ALPHA)].Get(), nullptr, 0xFFFFFFFF);

	const DirectX::XMFLOAT4X4 coordinate_system_transforms[]{
{ -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },	// 0:RHS Y-UP
{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },		// 1:LHS Y-UP
{ -1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },	// 2:RHS Z-UP
{ 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },		// 3:LHS Z-UP
	};

	const float scale_factor = 1.0f; // To change the units from centimeters to meters, set 'scale_factor' to 0.01.

	const float spacing = 2.0f;

	DirectX::XMMATRIX C{ DirectX::XMLoadFloat4x4(&coordinate_system_transforms[0]) * DirectX::XMMatrixScaling(scale_factor, scale_factor, scale_factor) };

	//DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z) };
	//DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) };
	//DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z) };
	DirectX::XMFLOAT4X4 world;

	{
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
			translation_object3.x,
			translation_object3.y,
			translation_object3.z
		);
		DirectX::XMMATRIX Scale = DirectX::XMMatrixScaling(1, 1, 1);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
			rotation_object3.x,
			rotation_object3.y,
			rotation_object3.z
		);
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, C * Scale * R * T);
		bool prev_flat = flat_shading;
		flat_shading = true; // Object3 はフラット描画
		skinned_meshes[0]->render(fw_->immediate_context.Get(), world, material_color, nullptr, flat_shading);
		flat_shading = prev_flat;
	}
	{
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
			translation_object4.x,
			translation_object4.y,
			translation_object4.z
		);
		DirectX::XMMATRIX Scale = DirectX::XMMatrixScaling(1, 1, 1);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
			rotation_object4.x,
			rotation_object4.y,
			rotation_object4.z
		);
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, C * Scale * R * T);
		bool prev_flat = flat_shading;
		flat_shading = true; // Object3 はフラット描画
		skinned_meshes[1]->render(fw_->immediate_context.Get(), world, material_color, nullptr, flat_shading);
		flat_shading = prev_flat;
	}
	fw_->framebuffers[0]->deactivate(fw_->immediate_context.Get());

	//if (fw_->enable_bloom)
	//{
	//	// BLOOM
	//	fw_->bloomer->make(fw_->immediate_context.Get(), fw_->framebuffers[0]->shader_resource_views[0].Get());

	//	fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
	//	fw_->immediate_context->RSSetState(fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get());
	//	fw_->immediate_context->OMSetBlendState(fw_->blend_states[static_cast<size_t>(BLEND_STATE::ALPHA)].Get(), nullptr, 0xFFFFFFFF);
	//	ID3D11ShaderResourceView* shader_resource_views[] =
	//	{
	//		fw_->framebuffers[0]->shader_resource_views[0].Get(),
	//		fw_->bloomer->shader_resource_view(),
	//	};
	//	fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), shader_resource_views, 0, 2, fw_->pixel_shaders[0].Get());
	//}

	if (fw_->enable_radial_blur)
	{
		// RADIAL_BLUR
		fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
		fw_->immediate_context->RSSetState(fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get());
		ID3D11ShaderResourceView* shader_resource_views[]{ fw_->framebuffers[0]->shader_resource_views[0].Get() };
		fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), shader_resource_views, 0, _countof(shader_resource_views), fw_->pixel_shaders[0].Get());
	}

	fw_->framebuffers[1]->clear(fw_->immediate_context.Get());
	fw_->framebuffers[1]->activate(fw_->immediate_context.Get());
	fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
	fw_->immediate_context->RSSetState(fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get());
	fw_->bit_block_transfer->blit(fw_->immediate_context.Get(), fw_->framebuffers[0]->shader_resource_views[0].GetAddressOf(), 0, 1, fw_->pixel_shaders[0].Get());
	fw_->framebuffers[1]->deactivate(fw_->immediate_context.Get());

	//if (sprite_end)
	//{
	//	fw_->immediate_context->OMSetDepthStencilState(fw_->depth_stencil_states[static_cast<size_t>(DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
	//	fw_->immediate_context->RSSetState(fw_->rasterizer_states[static_cast<size_t>(RASTER_STATE::CULL_NONE)].Get());

	//	sprite_end->render(fw_->immediate_context.Get(), 0, 0, 1920, 1080);
	//}
	DrawNumber(static_cast<int>(clearTime), timePosition.x, timePosition.y, timeScale);

	//saveSpeed

	int maxSpeed = static_cast<int>(saveSpeed / P_ACCELE);

	int seconds = static_cast<int>(clearTime);
	int decimals = static_cast<int>((clearTime - seconds) * 100);
	DrawNumber(decimals, timePosition.x + 150.0f, timePosition.y, timeScale);

	DrawNumber(static_cast<int>(maxSpeed), speedPos.x, speedPos.y, speedScale);

	DrawNumber(defeatedEnemies, scorePosition.x, scorePosition.y, scoreScale);
}

void SceneEnd::DrawGUI()
{
#ifdef USE_IMGUI
	ImGui::Begin("RESULT");

	ImGui::SetWindowFontScale(2.0f);
	ImGui::Text("CLEAR TIME : %.2f sec", clearTime);
	ImGui::SetWindowFontScale(1.0f);

	if (ImGui::CollapsingHeader("Layout Adjustment", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("--- Time Display ---");
		ImGui::DragFloat2("Time Pos", &timePosition.x, 1.0f);
		ImGui::DragFloat("Time Scale", &timeScale, 0.01f);

		ImGui::Separator();

		ImGui::Text("--- Enemy Count ---");
		ImGui::DragFloat2("Score Pos", &scorePosition.x, 1.0f);
		ImGui::DragFloat("Score Scale", &scoreScale, 0.01f);

		ImGui::Separator();

		ImGui::Text("--- speed Display ---");
		ImGui::DragFloat2("speed Pos", &speedPos.x, 1.0f);
		ImGui::DragFloat("speed Scale", &speedScale, 0.01f);
	}

	ImGui::SliderFloat("translation_object3.x", &translation_object4.x, -100.0f, +100.0f);
	ImGui::SliderFloat("translation_object3.y", &translation_object4.y, -100.0f, +100.0f);
	ImGui::SliderFloat("translation_object3.z", &translation_object4.z, -100.0f, +100.0f);

	ImGui::Separator();
	ImGui::SliderFloat("rotation_object3.y", &rotation_object4.y, -100.0f, +100.0f);

	ImGui::Separator();
	ImGui::Text("Press SPACE to Return Title");

	ImGui::End();
#endif
}