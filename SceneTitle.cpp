#include "System/Graphics.h"
#include "SceneTitle.h"
#include "System/Input.h"
#include "SceneGame.h"
#include "SceneLoading.h"
#include "SceneManager.h"
#include "framework.h"
#include "SceneTutorial.h"
SceneTitle::SceneTitle(framework* fw) : fw_(fw)
{
	if (!fw_) {
		MessageBox(nullptr, L"fw_ が nullptr です", L"Error", MB_OK);
		return;
	}
}

void SceneTitle::Initialize()
{
	HRESULT hr{ S_OK };

	skinned_meshes[0] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\title_text.cereal");
	skinned_meshes[1] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\base.cereal");
	skinned_meshes[2] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\neon6.cereal");
	skinned_meshes[3] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\idle.cereal");
	skinned_meshes[4] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\crown.cereal");
	skinned_meshes[5] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\cat.cereal");
	skinned_meshes[6] = std::make_unique<skinned_mesh>(fw_->device.Get(), ".\\resources\\sword.cereal");

	ShowCursor(true);
	fw_->light_direction = DirectX::XMFLOAT4{ -1.0f, -1.0f, 1.0f, 0.0f };

	fw_->distance = 20.0f;

	fw_->camera_position.x = 7.712f;
	fw_->camera_position.y = -0.130f;
	fw_->camera_position.z = -28.445f;

	fw_->translation_object3.x = 1.146f;
	fw_->translation_object3.y = -2.768f;
	fw_->translation_object3.z = -6.875f;

	fw_->radial_blur_data.blur_strength = 0.0f;
	fw_->radial_blur_data.blur_radius = 0.0f;
	fw_->radial_blur_data.blur_decay = 0.0f;

	fw_->bloomer->bloom_extraction_threshold = 0.2f;
	fw_->bloomer->bloom_intensity = 0.25f;

	text_base_y = text_object.y;
	camera_base_pos = fw_->camera_position;
	bloom_base_threshold = fw_->bloomer->bloom_extraction_threshold;
	bloom_base_intensity = fw_->bloomer->bloom_intensity;
	radial_blur_base = fw_->enable_radial_blur;
}

void SceneTitle::Finalize()
{
	skinned_meshes[0].reset();
	skinned_meshes[1].reset();
	skinned_meshes[2].reset();
	skinned_meshes[3].reset();
	skinned_meshes[4].reset();
	skinned_meshes[5].reset();
	skinned_meshes[6].reset();
}
DirectX::XMFLOAT3 HSVtoRGB(float h, float s, float v)
{
	float r, g, b;

	int i = int(h * 6.0f);
	float f = (h * 6.0f) - i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - f * s);
	float t = v * (1.0f - (1.0f - f) * s);

	switch (i % 6)
	{
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	case 5: r = v; g = p; b = q; break;
	}

	return { r, g, b };
}

void SceneTitle::Update(float elaspedTime)
{
	POINT mouse_client_pos{};
	GetCursorPos(&mouse_client_pos);

	HWND hwnd = FindWindow(APPLICATION_NAME, L"");
	ScreenToClient(hwnd, &mouse_client_pos);

	// --- Click Area : 演出トリガーのみ ---
	if (!text_falling)   // 落下中は無視
	{
		if (fw_->mouse_client_pos.x >= click_min.x &&
			fw_->mouse_client_pos.x <= click_max.x &&
			fw_->mouse_client_pos.y >= click_min.y &&
			fw_->mouse_client_pos.y <= click_max.y)
		{
			if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
			{
				// 落下演出スタート
				text_falling = true;
				text_object.y = text_base_y; // 初期位置に戻す（念のため）

				return;
			}
		}
	}

	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		SceneManager::instance().ChangeScene(new SceneLoading(new SceneGame(hwnd, fw_), fw_));
		return;
	}
	const float hue_speed = 0.1f;
	const float breathe_speed = 1.0f;
	const float fixed_pulse = 0.8f;

	const float t = fw_->data.time;

	const float hue = fmodf(t * hue_speed, 1.0f);

	const DirectX::XMFLOAT3 rgb = HSVtoRGB(hue, 1.0f, 1.0f);

	const float breathe = 0.5f * (sinf(t * breathe_speed) + 1.0f);

	const float brightness = 0.5f + 0.5f * breathe + fixed_pulse;

	const DirectX::XMFLOAT4 color{
		rgb.x * brightness,
		rgb.y * brightness,
		rgb.z * brightness,
		1.0f
	};

	fw_->material_color1 = color;
	fw_->material_color2 = color;

	if (!skinned_meshes[3]->animation_clips.empty())
	{
		animation& anim = skinned_meshes[3]->animation_clips[player_anim_index];

		player_anim_time += elaspedTime;

		int frame =
			static_cast<int>(player_anim_time * anim.sampling_rate);

		if (frame >= static_cast<int>(anim.sequence.size()))
		{
			frame = 0;
			player_anim_time = 0.0f;
		}

		player.keyframe = anim.sequence[frame];
	}
	if (!skinned_meshes[5]->animation_clips.empty())
	{
		animation& anim = skinned_meshes[5]->animation_clips[object_anim_index];

		object_anim_time += elaspedTime;

		int frame = static_cast<int>(object_anim_time * anim.sampling_rate);
		if (frame >= static_cast<int>(anim.sequence.size()))
		{
			frame = 0;
			object_anim_time = 0.0f;
		}

		object_keyframe = anim.sequence[frame];
	}
	if (!skinned_meshes[6]->animation_clips.empty())
	{
		animation& anim = skinned_meshes[6]->animation_clips[object2_anim_index];

		object2_anim_time += elaspedTime;

		int frame = static_cast<int>(object2_anim_time * anim.sampling_rate);
		if (frame >= static_cast<int>(anim.sequence.size()))
		{
			frame = 0;
			object2_anim_time = 0.0f;
		}

		object2_keyframe = anim.sequence[frame];
	}

	const float text_fall_speed = 0.5f;
	const float text_min_y = -2.0f;

	if (text_falling)
	{
		text_object.y -= text_fall_speed * elaspedTime;

		if (text_object.y <= text_min_y)
		{
			text_object.y = text_min_y;

			text_falling = false;
			text_float_base_y = text_object.y;
			text_float_time = 0.0f;

			landing_impact = true;
			impact_time = 0.0f;

			fw_->bloomer->bloom_extraction_threshold = 0.05f;
			fw_->bloomer->bloom_intensity = 4.0f;

			fw_->enable_radial_blur = true;
			fw_->radial_blur_data.blur_strength = 0.6f;
			fw_->radial_blur_data.blur_radius = 0.4f;

			flash_alpha = 1.0f;

			screen_shake = true;
			shake_time = 0.0f;
		}
	}
	else
	{
		const float float_amplitude = 0.15f;
		const float float_speed = 1.2f;

		text_float_time += elaspedTime;

		text_object.y =
			text_float_base_y +
			sinf(text_float_time * float_speed) * float_amplitude;
	}
	if (screen_shake)
	{
		shake_time += elaspedTime;

		float t = shake_time / shake_duration;
		if (t >= 1.0f)
		{
			screen_shake = false;
			fw_->camera_position = camera_base_pos;
		}
		else
		{
			float impact = powf(1.0f - t, 2.5f); // 最初が強い
			float strength = shake_power * impact;

			float offset_x =
				((rand() / (float)RAND_MAX) * 2.0f - 1.0f) * strength;
			float offset_y =
				((rand() / (float)RAND_MAX) * 2.0f - 1.0f) * strength;
			float offset_z =
				((rand() / (float)RAND_MAX) * 2.0f - 1.0f) * strength * 0.5f;

			fw_->camera_position.x = camera_base_pos.x + offset_x;
			fw_->camera_position.y = camera_base_pos.y + offset_y;
			fw_->camera_position.z = camera_base_pos.z + offset_z;
		}
	}

	if (landing_impact)
	{
		impact_time += elaspedTime;

		float t = impact_time / impact_duration;

		flash_alpha = 1.0f - t;

		if (t >= 1.0f)
		{
			landing_impact = false;

			fw_->bloomer->bloom_extraction_threshold = bloom_base_threshold;
			fw_->bloomer->bloom_intensity = bloom_base_intensity;

			fw_->enable_radial_blur = radial_blur_base;
			fw_->radial_blur_data.blur_strength = 0.0f;

			flash_alpha = 0.0f;
		}
	}
}

void SceneTitle::Render()
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

	const XMFLOAT4X4 coordinate_system_transforms[] =
	{
		{ -1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 },
		{  1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 },
		{ -1, 0, 0, 0,  0, 0,-1, 0,  0, 1, 0, 0,  0, 0, 0, 1 },
		{  1, 0, 0, 0,  0, 0, 1, 0,  0, 1, 0, 0,  0, 0, 0, 1 },
	};

	const float scale_factor = 1.0f;
	XMMATRIX C =
		XMLoadFloat4x4(&coordinate_system_transforms[0]) *
		XMMatrixScaling(scale_factor, scale_factor, scale_factor);

	{
		XMMATRIX T = XMMatrixTranslation(text_object.x, text_object.y, text_object.z);
		XMMATRIX S = XMMatrixScaling(0.7f, 0.7f, 0.7f);
		XMMATRIX R = XMMatrixRotationRollPitchYaw(
			fw_->rotation_object3.x,
			fw_->rotation_object3.y,
			fw_->rotation_object3.z
		);

		XMFLOAT4X4 world;
		XMStoreFloat4x4(&world, C * S * R * T);

		skinned_meshes[0]->render(
			fw_->immediate_context.Get(),
			world,
			fw_->material_color1,
			nullptr,
			true
		);
	}

	if (!text_falling)
	{
		{
			XMMATRIX T = XMMatrixTranslation(
				fw_->translation_object3.x,
				fw_->translation_object3.y,
				fw_->translation_object3.z
			);
			XMMATRIX S = XMMatrixScaling(1, 1, 1);
			XMMATRIX R = XMMatrixRotationRollPitchYaw(
				fw_->rotation_object3.x,
				fw_->rotation_object3.y,
				fw_->rotation_object3.z
			);

			XMFLOAT4X4 world;
			XMStoreFloat4x4(&world, C * S * R * T);

			skinned_meshes[1]->render(
				fw_->immediate_context.Get(),
				world,
				fw_->material_color,
				nullptr,
				false
			);

			XMFLOAT4 sword_color
			{
				fw_->material_color.x * sword_bloom_boost,
				fw_->material_color.y * sword_bloom_boost,
				fw_->material_color.z * sword_bloom_boost,
				fw_->material_color.w
			};

			skinned_meshes[4]->render(
				fw_->immediate_context.Get(),
				world,
				sword_color,
				nullptr,
				false
			);

			skinned_meshes[6]->render(
				fw_->immediate_context.Get(),
				world,
				player_color,
				&object2_keyframe,
				false
			);
		}

		{
			XMMATRIX T = XMMatrixTranslation(
				fw_->translation_object3.x,
				fw_->translation_object3.y,
				fw_->translation_object3.z
			);
			XMMATRIX S = XMMatrixScaling(1, 1, 1);
			XMMATRIX R = XMMatrixRotationRollPitchYaw(
				fw_->rotation_object3.x,
				fw_->rotation_object3.y,
				fw_->rotation_object3.z
			);

			XMFLOAT4X4 world;
			XMStoreFloat4x4(&world, C * S * R * T);
		}

		{
			XMMATRIX T = XMMatrixTranslation(static_pos.x, static_pos.y, static_pos.z);
			XMMATRIX R = XMMatrixRotationRollPitchYaw(static_rot.x, static_rot.y, static_rot.z);
			XMMATRIX S = XMMatrixScaling(static_scale, static_scale, static_scale);

			XMFLOAT4X4 world;
			XMStoreFloat4x4(&world, C * S * R * T);

			skinned_meshes[3]->render(
				fw_->immediate_context.Get(),
				world,
				player_color,
				&player.keyframe,
				true
			);
		}

		{
			XMMATRIX T = XMMatrixTranslation(effect_pos.x, effect_pos.y, effect_pos.z);
			XMMATRIX R = XMMatrixRotationRollPitchYaw(effect_rot.x, effect_rot.y, effect_rot.z);
			XMMATRIX S = XMMatrixScaling(effect_scale, effect_scale, effect_scale);

			XMFLOAT4X4 world;
			XMStoreFloat4x4(&world, C * S * R * T);

			skinned_meshes[5]->render(
				fw_->immediate_context.Get(),
				world,
				player_color,
				&object_keyframe,
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
	else
	{
		fw_->bit_block_transfer->blit(
			fw_->immediate_context.Get(),
			fw_->framebuffers[1]->shader_resource_views[0].GetAddressOf(),
			0,
			1,
			fw_->pixel_shaders[2].Get()
		);
	}
}

void SceneTitle::DrawGUI()
{
#ifdef USE_IMGUI
	ImGui::Begin("ImGUI");

	// -------------------------------
	// Click Area Visualization
	// -------------------------------
	bool is_hover =
		fw_->mouse_client_pos.x >= click_min.x &&
		fw_->mouse_client_pos.x <= click_max.x &&
		fw_->mouse_client_pos.y >= click_min.y &&
		fw_->mouse_client_pos.y <= click_max.y;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// 画面左上基準に変換
	ImVec2 offset = ImGui::GetWindowPos();

	ImU32 fill_col = is_hover
		? IM_COL32(0, 255, 0, 80)
		: IM_COL32(255, 0, 0, 80);

	ImU32 border_col = is_hover
		? IM_COL32(0, 255, 0, 200)
		: IM_COL32(255, 0, 0, 200);

	// 矩形（塗り）
	draw_list->AddRectFilled(
		ImVec2(offset.x + click_min.x, offset.y + click_min.y),
		ImVec2(offset.x + click_max.x, offset.y + click_max.y),
		fill_col
	);

	// 枠線
	draw_list->AddRect(
		ImVec2(offset.x + click_min.x, offset.y + click_min.y),
		ImVec2(offset.x + click_max.x, offset.y + click_max.y),
		border_col,
		0.0f,
		0,
		2.0f
	);
	ImGui::NewLine();
	ImGui::NewLine();
	ImGui::NewLine();
	ImGui::NewLine();
	ImGui::NewLine();
	ImGui::NewLine();

	ImGui::Text("Click Area");

	ImGui::SliderFloat("Min X", &click_min.x, 0.0f, 1920.0f);
	ImGui::SliderFloat("Min Y", &click_min.y, 0.0f, 1080.0f);
	ImGui::SliderFloat("Max X", &click_max.x, 0.0f, 1920.0f);
	ImGui::SliderFloat("Max Y", &click_max.y, 0.0f, 1080.0f);

	ImGui::DragFloat3(
		"Position",
		&static_pos.x,
		0.01f,
		-50.0f,
		50.0f
	);

	static float rot_deg[3]{
		DirectX::XMConvertToDegrees(static_rot.x),
		DirectX::XMConvertToDegrees(static_rot.y),
		DirectX::XMConvertToDegrees(static_rot.z)
	};

	if (ImGui::DragFloat3("Rotation (deg)", rot_deg, 1.0f, -180.0f, 180.0f))
	{
		static_rot.x = DirectX::XMConvertToRadians(rot_deg[0]);
		static_rot.y = DirectX::XMConvertToRadians(rot_deg[1]);
		static_rot.z = DirectX::XMConvertToRadians(rot_deg[2]);
	}

	ImGui::SliderFloat(
		"Scale",
		&static_scale,
		0.01f,
		1
	);
	ImGui::SliderFloat(
		"Obj2 Scale",
		&obj2_scale,
		0.01f,
		10.0f
	);

	ImGui::SliderFloat(
		"Sword Bloom Boost",
		&sword_bloom_boost,
		1.0f,
		100.0f
	);
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

	ImGui::DragFloat2("blur_center", &fw_->radial_blur_data.blur_center.x, 0.01f);
	ImGui::SliderFloat("blur_strength", &fw_->radial_blur_data.blur_strength, +0.0f, +1.0f);
	ImGui::SliderFloat("blur_radius", &fw_->radial_blur_data.blur_radius, +0.0f, +1.0f);
	ImGui::SliderFloat("blur_decay", &fw_->radial_blur_data.blur_decay, +0.0f, +1.0f);

	ImGui::SliderFloat("bloom_extraction_threshold", &fw_->bloomer->bloom_extraction_threshold, +0.0f, +1.0f);
	ImGui::SliderFloat("bloom_intensity", &fw_->bloomer->bloom_intensity, +0.0f, +5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
	ImGui::SliderFloat("Camera Distance", &fw_->distance, 1.0f, 20.0f);
	ImGui::PopStyleVar();

	ImGui::Text("Stage Rotation");
	ImGui::SliderFloat("Pos X##obj3", &text_object.x, -10.0f, 10.0f);
	if (ImGui::DragFloat(
		"Text Base Y",
		&text_base_y,
		0.01f,
		-10.0f,
		10.0f))
	{
		text_object.y = text_base_y;
	}

	ImGui::SliderFloat("Pos Z##obj3", &text_object.z, -10.0f, 10.0f);

	ImGui::DragFloat3(
		"Text Position",
		&text_object.x,
		0.01f,
		-50.0f,
		50.0f
	);

	ImGui::Separator();
	ImGui::Text("Effect (static_mesh5)");

	ImGui::DragFloat3(
		"Effect Position",
		&effect_pos.x,
		0.01f,
		-50.0f,
		50.0f
	);

	static float effect_rot_deg[3]{
		DirectX::XMConvertToDegrees(effect_rot.x),
		DirectX::XMConvertToDegrees(effect_rot.y),
		DirectX::XMConvertToDegrees(effect_rot.z)
	};

	if (ImGui::DragFloat3("Effect Rotation (deg)", effect_rot_deg, 1.0f, -180, 180))
	{
		effect_rot.x = DirectX::XMConvertToRadians(effect_rot_deg[0]);
		effect_rot.y = DirectX::XMConvertToRadians(effect_rot_deg[1]);
		effect_rot.z = DirectX::XMConvertToRadians(effect_rot_deg[2]);
	}

	ImGui::SliderFloat(
		"Effect Scale",
		&effect_scale,
		0.01f,
		1.0f
	);

	ImGui::End();
#endif
}