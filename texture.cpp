#include "texture.h"
#include "misc.h"

#include <WICTextureLoader.h>
using namespace DirectX;

#include <wrl.h>
using namespace Microsoft::WRL;

#include <string>
#include <map>
using namespace std;

#include <sstream>
#include <iomanip>

#include <filesystem>
#include <DDSTextureLoader.h>


HRESULT make_dummy_texture(ID3D11Device* device, ID3D11ShaderResourceView** shader_resource_view, DWORD value, UINT dimension);

static map<wstring, ComPtr<ID3D11ShaderResourceView>> resources;

HRESULT load_texture_from_file(ID3D11Device* device, const wchar_t* filename, ID3D11ShaderResourceView** shader_resource_view, D3D11_TEXTURE2D_DESC* texture2d_desc)
{
	HRESULT hr{ S_OK };
	ComPtr<ID3D11Resource> resource;

	auto it = resources.find(filename);
	if (it != resources.end())
	{
		*shader_resource_view = it->second.Get();
		(*shader_resource_view)->AddRef();
		(*shader_resource_view)->GetResource(resource.GetAddressOf());
	}
	else
	{
		std::filesystem::path dds_filename(filename);
		dds_filename.replace_extension("dds");
		if (std::filesystem::exists(dds_filename.c_str()))
		{
			Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context;
			device->GetImmediateContext(immediate_context.GetAddressOf());
			hr = DirectX::CreateDDSTextureFromFile(device, immediate_context.Get(), dds_filename.c_str(), resource.GetAddressOf(), shader_resource_view);
		}
		else
		{
			// 1. 通常の読み込みを試行
			hr = CreateWICTextureFromFile(device, filename, resource.GetAddressOf(), shader_resource_view);

			// 2. 失敗した場合、resourcesフォルダ内を探す
			if (FAILED(hr))
			{
				std::filesystem::path p(filename);
				std::wstring simpleFilename = p.filename().wstring();
				std::wstring newPath = L".\\resources\\" + simpleFilename;

				hr = CreateWICTextureFromFile(device, newPath.c_str(), resource.GetAddressOf(), shader_resource_view);

				// 成功したら、元のパス名で登録しておく（次回から高速化）
				if (SUCCEEDED(hr))
				{
					resources.insert(make_pair(filename, *shader_resource_view));
				}
			}

			// 3. それでも見つからない場合、ダミーテクスチャ（マゼンタ色）を作成して代用する
			if (FAILED(hr))
			{
				OutputDebugStringW(L"\n[Warning] Texture Not Found. Using Dummy: ");
				OutputDebugStringW(filename);
				OutputDebugStringW(L"\n");

				// エラーで停止させないため、ピンク色の1x1テクスチャを作る
				hr = make_dummy_texture(device, shader_resource_view, 0xFF00FFFF, 16);

				// ダミー作成に成功したら、それをリソースとして登録
				if (SUCCEEDED(hr))
				{
					(*shader_resource_view)->GetResource(resource.GetAddressOf());
					resources.insert(make_pair(filename, *shader_resource_view));
				}
			}
		}
	}

	// 最終確認: ここで hr が失敗状態だと Assert で止まる
	// ダミー生成が成功していれば hr は S_OK になっているはず
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	if (SUCCEEDED(hr))
	{
		ComPtr<ID3D11Texture2D> texture2d;
		hr = resource.Get()->QueryInterface<ID3D11Texture2D>(texture2d.GetAddressOf());
		if (SUCCEEDED(hr))
		{
			texture2d->GetDesc(texture2d_desc);
		}
	}

	return hr;
}

void release_all_textures()
{
	resources.clear();
}

HRESULT make_dummy_texture(ID3D11Device* device, ID3D11ShaderResourceView** shader_resource_view, DWORD value, UINT dimension)
{
	HRESULT hr{ S_OK };

	wstringstream keyname;
	keyname << setw(8) << setfill(L'0') << hex << uppercase << value << L"." << dec << dimension;
	map<wstring, ComPtr<ID3D11ShaderResourceView>>::iterator it = resources.find(keyname.str().c_str());
	if (it != resources.end())
	{
		*shader_resource_view = it->second.Get();
		(*shader_resource_view)->AddRef();
	}
	else
	{
		D3D11_TEXTURE2D_DESC texture2d_desc{};
		texture2d_desc.Width = dimension;
		texture2d_desc.Height = dimension;
		texture2d_desc.MipLevels = 1;
		texture2d_desc.ArraySize = 1;
		texture2d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texture2d_desc.SampleDesc.Count = 1;
		texture2d_desc.SampleDesc.Quality = 0;
		texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		texture2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texture2d_desc.CPUAccessFlags = 0;
		texture2d_desc.MiscFlags = 0;

		size_t texels = static_cast<size_t>(dimension) * dimension;
		unique_ptr<DWORD[]> sysmem{ make_unique< DWORD[]>(texels) };
		for (size_t i = 0; i < texels; ++i)
		{
			sysmem[i] = value;
		}

		D3D11_SUBRESOURCE_DATA subresource_data{};
		subresource_data.pSysMem = sysmem.get();
		subresource_data.SysMemPitch = sizeof(DWORD) * dimension;
		subresource_data.SysMemSlicePitch = 0;

		ComPtr<ID3D11Texture2D> texture2d;
		hr = device->CreateTexture2D(&texture2d_desc, &subresource_data, &texture2d);
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc{};
		shader_resource_view_desc.Format = texture2d_desc.Format;
		shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MipLevels = 1;

		hr = device->CreateShaderResourceView(texture2d.Get(), &shader_resource_view_desc, shader_resource_view);
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		resources.insert(std::make_pair(keyname.str().c_str(), *shader_resource_view));
	}
	return hr;
}