// BLOOM
#pragma once

#include <memory>
#include <d3d11.h>
#include <wrl.h>

#include "framebuffer.h"
#include "fullscreen_quad.h"

class bloom
{
public:
	bloom(ID3D11Device* device, uint32_t width, uint32_t height);
	~bloom() = default;
	bloom(const bloom&) = delete;
	bloom& operator =(const bloom&) = delete;
	bloom(bloom&&) noexcept = delete;
	bloom& operator =(bloom&&) noexcept = delete;

	void bloom::make(ID3D11DeviceContext* immediate_context, ID3D11ShaderResourceView* color_map);
	ID3D11ShaderResourceView* shader_resource_view() const
	{
		return glow_extraction->shader_resource_views[0].Get();
	}

public:
	float bloom_extraction_threshold = 0.0f;
	float bloom_intensity = 0.1f;

private:
	std::unique_ptr<fullscreen_quad> bit_block_transfer;
	std::unique_ptr<framebuffer> glow_extraction;

	static const size_t downsampled_count = 6;
	std::unique_ptr<framebuffer> gaussian_blur[downsampled_count][2];

	Microsoft::WRL::ComPtr<ID3D11PixelShader> glow_extraction_ps;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> gaussian_blur_downsampling_ps;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> gaussian_blur_horizontal_ps;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> gaussian_blur_vertical_ps;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> gaussian_blur_upsampling_ps;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_stencil_state;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_state;
	Microsoft::WRL::ComPtr<ID3D11BlendState> blend_state;

	struct bloom_constants
	{
		float bloom_extraction_threshold;
		float bloom_intensity;
		float something[2];
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;
};
