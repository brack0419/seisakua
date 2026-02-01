#pragma once

#include <windows.h>
#include <tchar.h>
#include <sstream>
#include "SceneManager.h"
#include "misc.h"
#include "high_resolution_timer.h"

#include <windowsx.h>

#ifdef USE_IMGUI
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern ImWchar glyphRangesJapanese[];
#endif

#include <d3d11.h>

#include "sprite.h"

#include "sprite_batch.h"

#include <wrl.h>

#include "geometric_primitive.h"

#include "static_mesh.h"

#include "skinned_mesh.h"

#include "framebuffer.h"
#include "fullscreen_quad.h"

// BLOOM
#include "bloom.h"

#include "Audio.h"
#include "AudioSource.h"

CONST LONG SCREEN_WIDTH{ 1920 };
CONST LONG SCREEN_HEIGHT{ 1080 };
CONST BOOL FULLSCREEN{ FALSE };
CONST LPWSTR APPLICATION_NAME{ L"自由" };

class framework
{
public:
	HCURSOR hoverCursor = nullptr;
	bool cursorSet = false;


	CONST HWND hwnd;

	// Direct3Dデバイスの取得
	Microsoft::WRL::ComPtr<ID3D11Device> get_device() const { return device; }

	// 即時コンテキストの取得
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> get_context() const { return immediate_context; }

	// スワップチェインの取得
	Microsoft::WRL::ComPtr<IDXGISwapChain> get_swapchain() const { return swap_chain; }

	// レンダーターゲットビューの取得
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> get_rtv() const { return render_target_view; }

	// 深度ステンシルビューの取得
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> get_dsv() const { return depth_stencil_view; }

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_stencil_view;

	std::unique_ptr<sprite> sprites[8];
	std::unique_ptr<sprite_batch> sprite_batches[8];

	enum class SAMPLER_STATE { POINT, LINEAR, ANISOTROPIC, LINEAR_BORDER_BLACK/*UNIT.32*/, LINEAR_BORDER_WHITE/*UNIT.32*/, LINEAR_CLAMP/*RADIAL_BLUR*/ };
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_states[6/*RADIAL_BLUR*/];

	enum class DEPTH_STATE { ZT_ON_ZW_ON, ZT_ON_ZW_OFF, ZT_OFF_ZW_ON, ZT_OFF_ZW_OFF };
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_stencil_states[4];

	enum class BLEND_STATE { NONE, ALPHA, ADD, MULTIPLY };
	Microsoft::WRL::ComPtr<ID3D11BlendState> blend_states[4];

	enum class RASTER_STATE { SOLID, WIREFRAME, CULL_NONE, WIREFRAME_CULL_NONE };
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_states[4];

	struct scene_constants
	{
		DirectX::XMFLOAT4X4 view_projection;
		DirectX::XMFLOAT4 light_direction;

		// DYNAMIC_TEXTURE
		float time = 0;
		float elapsed_time = 0;
		float pads[2];
	};

	struct parametric_constants
	{
		float extraction_threshold{ 0.8f };
		float gaussian_sigma{ 1.0f };
		float bloom_intensity{ 0.0f };
		float exposure{ 1.0f };
	};
	parametric_constants parametric_constants;

	// RADIAL_BLUR
	struct radial_blur_constants
	{
		DirectX::XMFLOAT2 blur_center = { 0.5, 0.5 }; // center point where the blur is applied
		float blur_strength = 0.0f; // blurring strength
		float blur_radius = 0.5f; // blurred radiu
		float blur_decay = 0.2f; // ratio of distance to decay to radius
		float pads[3];
	};
	radial_blur_constants radial_blur_data;

	struct Transform {
		DirectX::XMFLOAT3 translation{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 rotation{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 scaling{ 1.0f, 1.0f, 1.0f };
	};
	Transform mesh_transforms[5]; // skinned_meshes[0]～[4] 用

	//////////////////////////////////////////////
	struct Patty {
		DirectX::XMFLOAT3 pos;          // 中心
		DirectX::XMFLOAT3 vel;
		DirectX::XMFLOAT3 half_extents; // AABB の半径 (hx, hy, hz)
		float mass;
		float restitution;

		bool isColliding = false; // ← 追加
		bool isStuck = false; // ← 串に刺さって固定されているか
	};

	std::vector<Patty> patties; // Patty 一覧（将来 N 体）

	// グローバル物理パラメータ
	bool patty_sim_enabled = true;
	float patty_gravity_y = -0.8f; // 下向き重力（単位系に応じて微調整）
	float patty_ground_y = 0.235f; // 地面/テーブルの Y（シーンに合わせて）
	float patty_default_radius = 0.02f; // Patty の見た目に合わせて調整
	DirectX::XMFLOAT3 patty_default_half_extents{ 0.035f, 0.012f, 0.020f };
	float patty_default_mass = 1.0f;
	float patty_default_restitution = 0.0f; // Patty 同士の最小反発係数
	// 生成時のX
	float patty_spawn_x = -0.3f;

	// 起動時 1 体だけ自動スポーンするためのフラグ
	bool patty_spawned_initial = false;

	// ユーティリティ（メンバ関数宣言）
	void add_patty(const DirectX::XMFLOAT3& pos,
		const DirectX::XMFLOAT3& half_extents,
		float mass,
		float restitution);
	void simulate_patties(float elapsed_time);
	void resolve_pair(int ia, int ib);
	//////////////////////////////////

	float ground_z_center = -10.0f;
	float ground_z_half_range = 0.010f;  //地面
	bool  ground_range_visible = true;
	DirectX::XMFLOAT4 ground_range_color{ 0.2f, 0.6f, 1.0f, 0.20f };
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffers[8];

	DirectX::XMFLOAT3 camera_position{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 light_direction{ -1.0, -0.8f, -1.0f, 0.0f };
	DirectX::XMFLOAT3 camera_focus{ 0.0f, 0.283f, -10.0f };
	float rotateX{ DirectX::XMConvertToRadians(5) };
	float rotateY{ DirectX::XMConvertToRadians(90) };
	POINT cursor_position;
	float wheel{ 0 };
	float distance{ 0.1f };

	DirectX::XMFLOAT3 center_of_rotation;
	DirectX::XMFLOAT3 translation{ 0, 0, 0 };
	DirectX::XMFLOAT3 scaling{ 1, 1, 1 };
	DirectX::XMFLOAT3 rotation{ 0, 0.643f, 0 };
	DirectX::XMFLOAT4 material_color{ 1 ,1, 1, 1 };
	DirectX::XMFLOAT4 material_color1{ 1 ,1, 1, 1 };
	DirectX::XMFLOAT4 material_color2{ 1 ,1, 1, 1 };
	DirectX::XMFLOAT3 rotation_object3{ 0.0f, 3.115f, 0.0f };
	DirectX::XMFLOAT3 rotation_object4{ 0.0f, 3.13f, 0.0f };
	DirectX::XMFLOAT3 translation_object3{ -0.254f, 0.0f, -9.391f };
	DirectX::XMFLOAT3 translation_object4{ -0.254f, 0.0f, -9.391f };
	DirectX::XMFLOAT3 model_position_man{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 model_position_walk{ 0.0f, 0.0f, 0.0f };

	std::unique_ptr<static_mesh> static_meshes[8];

	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shaders[8];

	std::unique_ptr<skinned_mesh> skinned_meshes[11];

	float factors[4]{ 0.0f, 121.438332f };

	std::unique_ptr<framebuffer> framebuffers[8];
	std::unique_ptr<fullscreen_quad> bit_block_transfer;

	// DYNAMIC_TEXTURE
	std::unique_ptr<framebuffer> dynamic_texture;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> dynamic_texture_ps;

	// DYNAMIC_BACKGROUND
	std::unique_ptr<framebuffer> dynamic_background;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> dynamic_background_ps;
	scene_constants data{};

	// BLOOM
	std::unique_ptr<bloom> bloomer;

	//ImGui追加設定
		// ピクセルシェーダー格納用配列
	Microsoft::WRL::ComPtr<ID3D11PixelShader> effect_shaders[2];
	bool flat_shading = false;
	bool enable_bloom = true;
	bool enable_dynamic_shader = false;
	bool enable_dynamic_background = false;
	bool enable_radial_blur = true;
	int current_effect = 0; // 0 = テクスチャ表示, 1 = 背景アニメーション

	struct Bun {
		DirectX::XMFLOAT3 pos{ -0.3f, 1.3f, 0.0f };
		DirectX::XMFLOAT3 vel{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 half_extents{ 0.020f, 0.011f, 0.020f };
		float mass{ 1.0f };
		float restitution{ 0.0f };
		bool exists{ false };
	};
	Bun bun;
	bool bun_spawned_initial = false;
	bool bun_collider_visible = true;
	DirectX::XMFLOAT4 bun_collider_color{ 0.2f, 1.0f, 0.2f, 0.35f };

	// 1体だけの簡易シミュレーション（重力＆床Y）
	void simulate_bun(float elapsed_time);

	// マウス追加
	POINT mouse_client_pos{ 960, 540 };
	DirectX::XMFLOAT3 pate_position{ -0.4f, 3.0f, 0.0f };
	DirectX::XMFLOAT3 pate_target{ -0.4f, 3.0f, 0.0f };
	float pate_follow_smooth = 0.25f;
	float pate_fixed_x = -0.3f;
	float pate_fixed_z = 0.0f; //
	float pate_control_z = 0.0f; // ヒットするZ平面（固定Z）
	////////////////////////////////////////
		//串
	struct Kusi
	{
		DirectX::XMFLOAT3 pos{ -0.3f, 1.0f, -10.0f };
		DirectX::XMFLOAT3 half_extents{ 0.01f, 0.01f, 0.01f };

		bool move = false;
	};
	Kusi kusi;

	//DirectX::XMFLOAT3 kusi_position{ -0.3f, 1.0f, -10.0f };
	//DirectX::XMFLOAT3 kusi_half{ 0.01f, 0.01f, 0.01f };
	//bool kusi = false;
	//bool kusi_collider_visible = true;
	///////////////////////////////////////
	bool pate_collider_enabled = true; // 衝突を有効化
	bool patty_collider_visible = true;
	DirectX::XMFLOAT4 patty_collider_color{ 1.0f, 0.2f, 0.2f, 0.35f };
	// コライダー中心は pate_position（既存）を使用
	DirectX::XMFLOAT3 pate_half_extents{ 0.060f, 0.025f, 0.015f }; // X=幅/2, Y=厚み/2, Z=奥行/2
	float pate_restitution = 0.0f; // 反発
	float pate_plane_y = 0.373f; // マウス追従に使う平面の高さ（pate_position.y に反映）
	//////////////////////////
	float patty_spawn_z = -10.0f; // 新規スポーン時に使うZ
	float patties_z_offset = 0.0f; // 全体シフト用の現在値
	float patties_z_offset_prev = 0.0f; // 前フレーム値（差分適用に利用）
	int patty_selected_index = -1; // 選択中（-1: なし）
	/////////////////////////////
	// 1体の Patty（球）と pate（AABB）の衝突解決
	void resolve_pate_collision(struct Patty& S);

	void CheckKusiPattyCollision();

	//void framework::resolve_kusi_collision(Patty& S);
	// フレーム時間履歴用（60フレーム分）
	static constexpr int FPS_HISTORY_COUNT = 120;
	float frame_times[FPS_HISTORY_COUNT] = {};
	int frame_time_index = 0;
	bool frame_times_filled = false;

	framework(HWND hwnd);
	~framework();

	framework(const framework&) = delete;
	framework& operator=(const framework&) = delete;
	framework(framework&&) noexcept = delete;
	framework& operator=(framework&&) noexcept = delete;

	int run()
	{
		MSG msg{};

		if (!initialize())
		{
			return 0;
		}

#ifdef USE_IMGUI
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 14.0f, nullptr, glyphRangesJapanese);
		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX11_Init(device.Get(), immediate_context.Get());
		ImGui::StyleColorsDark();
#endif

		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				tictoc.tick();
				calculate_frame_stats();
				update(tictoc.time_interval());
				render(tictoc.time_interval());
			}
		}

#ifdef USE_IMGUI
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
#endif

		BOOL fullscreen{};
		swap_chain->GetFullscreenState(&fullscreen, 0);
		if (fullscreen)
		{
			swap_chain->SetFullscreenState(FALSE, 0);
		}

		return uninitialize() ? static_cast<int>(msg.wParam) : 0;
	}

	LRESULT CALLBACK handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
#ifdef USE_IMGUI
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) { return true; }
#endif
		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);

			EndPaint(hwnd, &ps);
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_CREATE:
			break;
		case WM_KEYDOWN:
			if (wparam == VK_ESCAPE)
			{
				PostMessage(hwnd, WM_CLOSE, 0, 0);
			}
			break;
		case WM_ENTERSIZEMOVE:
			tictoc.stop();
			break;
		case WM_EXITSIZEMOVE:
			tictoc.start();
			break;
			///////////////////////////
		case WM_MOUSEMOVE:
		{
			mouse_client_pos.x = GET_X_LPARAM(lparam);
			mouse_client_pos.y = GET_Y_LPARAM(lparam);
		}
		break;
		//////////////////////////

		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		return 0;
	}

private:
	bool initialize();
	void update(float elapsed_time/*Elapsed seconds from last frame*/);
	void render(float elapsed_time/*Elapsed seconds from last frame*/);
	bool uninitialize();

private:
	high_resolution_timer tictoc;
	uint32_t frames{ 0 };
	float elapsed_time{ 0.0f };
	void calculate_frame_stats()
	{
		if (++frames, (tictoc.time_stamp() - elapsed_time) >= 1.0f)
		{
			float fps = static_cast<float>(frames);
			std::wostringstream outs;
			outs.precision(6);
			outs << APPLICATION_NAME << L" : FPS : " << fps << L" / " << L"Frame Time : " << 1000.0f / fps << L" (ms)";
			SetWindowTextW(hwnd, outs.str().c_str());

			frames = 0;
			elapsed_time += 1.0f;
		}
	}
};
