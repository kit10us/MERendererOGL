// Copyright (c) 2003 - 2014, Evil Quail LLC
// All Rights Reserved

#include <meogl/Renderer.h>
#include <meogl/WindowsOS.h>
#include <meogl/VertexBuffer.h>
#include <meogl/IndexBuffer.h>
#include <meogl/VertexShader.h>
#include <meogl/PixelShader.h>
#include <meogl/VertexDeclaration.h>
#include <meogl/Texture.h>
#include <me/RenderMethod.h>
#include <me/exception/FailedToCreate.h>
#include <me/exception/NotImplemented.h>
#include <atlbase.h>
#include <cassert>

using namespace meogl;
using namespace me;

class Renderer::Pimpl
{
public:
	Renderer & m_owner;
	WindowsOS * m_OS;
	me::Display m_display;
	size_t m_index;

	CComPtr< ID3D11Device > m_dxDevice;
	CComPtr< ID3D11DeviceContext > m_dxContext;
	DXGI_SWAP_CHAIN_DESC m_swapChainDesc;
	CComPtr< IDXGISwapChain > m_swapChain;
	CComPtr< ID3D11RenderTargetView > m_renderTargetView;
	CComPtr< ID3D11Texture2D > m_depthStencilBuffer;
	CComPtr< ID3D11DepthStencilView > m_depthStencilView;
	CComPtr< ID3D11RasterizerState > m_rasterizerState;

	Pimpl( Renderer & owner, WindowsOS * os, me::Display display, size_t index )
		: m_owner( owner )
		, m_OS( os )
		, m_display( display )
		, m_swapChainDesc{}
		, m_index{ index }
	{
		bool debug =
#if defined( DEBUG ) || defined( _DEBUG )
			true;
#else
			false;
#endif

		HRESULT result = S_OK;

		HWND hWnd = (HWND)display.GetHandle();

		m_swapChainDesc.BufferCount = 1;
		m_swapChainDesc.BufferDesc.Width = (unsigned int )display.GetSize().width;
		m_swapChainDesc.BufferDesc.Height = (unsigned int)display.GetSize().height;
		m_swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		m_swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		m_swapChainDesc.OutputWindow = hWnd;
		m_swapChainDesc.SampleDesc.Count = 1;
		m_swapChainDesc.SampleDesc.Quality = 0;
		m_swapChainDesc.Windowed = 1;

		unsigned int flags = debug ? D3D11_CREATE_DEVICE_DEBUG : 0;
		D3D_FEATURE_LEVEL featureLevelsRequested[] = { D3D_FEATURE_LEVEL_11_0 };
		D3D_FEATURE_LEVEL featureLevelSupported;
		result = D3D11CreateDeviceAndSwapChain( 0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, featureLevelsRequested, sizeof( featureLevelsRequested ) / sizeof( D3D_FEATURE_LEVEL ), D3D11_SDK_VERSION, &m_swapChainDesc, &m_swapChain, &m_dxDevice, &featureLevelSupported, &m_dxContext );
		if ( FAILED( result ) )
		{
			throw me::exception::FailedToCreate( "Failed to create Direct-X 11!" );
		}

		{
			// Create the back buffer...

			CComPtr< ID3D11Texture2D > backBuffer;
			result = m_swapChain->GetBuffer( 0, __uuidof(ID3D11Texture2D), (void**)&backBuffer );
			if ( FAILED( result ) )
			{
				m_swapChain = 0;
				m_dxDevice = 0;
				m_dxContext = 0;
				throw exception::FailedToCreate( "Failed to get backbuffer during device creation!" );
			}

			result = m_dxDevice->CreateRenderTargetView( backBuffer, 0, &m_renderTargetView );
			backBuffer = 0;
			if ( FAILED( result ) )
			{
				m_swapChain = 0;
				m_dxDevice = 0;
				m_dxContext = 0;
				throw exception::FailedToCreate( "Failed to create render target view during device creation!" );
			}
		}

		{
			D3D11_TEXTURE2D_DESC depthStencilDesc = D3D11_TEXTURE2D_DESC();
			depthStencilDesc.Width = (unsigned int)display.GetSize().width;
			depthStencilDesc.Height = (unsigned int)display.GetSize().height;
			depthStencilDesc.MipLevels = 1;
			depthStencilDesc.ArraySize = 1;
			depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthStencilDesc.SampleDesc.Count = 1;
			depthStencilDesc.SampleDesc.Quality = 0;
			depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
			depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			depthStencilDesc.CPUAccessFlags = 0;
			depthStencilDesc.MiscFlags = 0;
			result = m_dxDevice->CreateTexture2D( &depthStencilDesc, nullptr, &m_depthStencilBuffer );
			assert( !FAILED( result ) );

			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = D3D11_DEPTH_STENCIL_VIEW_DESC();
			depthStencilViewDesc.Format = depthStencilDesc.Format;
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			depthStencilViewDesc.Texture2D.MipSlice = 0;

			result = m_dxDevice->CreateDepthStencilView( m_depthStencilBuffer, &depthStencilViewDesc, &m_depthStencilView );
			assert( !FAILED( result ) );

			m_dxContext->OMSetRenderTargets( 1, &m_renderTargetView.p, m_depthStencilView.p );
		}

		D3D11_VIEWPORT viewport = D3D11_VIEWPORT();
		{
			viewport.Width = static_cast< float >(m_swapChainDesc.BufferDesc.Width);
			viewport.Height = static_cast< float >(m_swapChainDesc.BufferDesc.Height);
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			m_dxContext->RSSetViewports( 1, &viewport );
		}

		{
			D3D11_RASTERIZER_DESC rasterizerDesc = D3D11_RASTERIZER_DESC();
			rasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
			rasterizerDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE; // default: D3D11_CULL_BACK
			rasterizerDesc.FrontCounterClockwise = false;
			rasterizerDesc.DepthBias = 0;
			rasterizerDesc.DepthBiasClamp = 0.0f;
			rasterizerDesc.SlopeScaledDepthBias = 0.0f;
			rasterizerDesc.DepthClipEnable = true;
			rasterizerDesc.ScissorEnable = false;
			rasterizerDesc.MultisampleEnable = false;
			rasterizerDesc.AntialiasedLineEnable = false;
			m_dxDevice->CreateRasterizerState( &rasterizerDesc, &m_rasterizerState );
		}
		m_dxContext->RSSetState( m_rasterizerState );
	}

	const me::Display & GetDisplay() const
	{
		return m_display;
	}

	ID3D11Device * GetDxDevice() const
	{
		return m_dxDevice;
	}

	ID3D11DeviceContext * GetDxContext() const
	{
		return m_dxContext;
	}

	IDXGISwapChain * GetSwapChain() const
	{
		return m_swapChain;
	}					   

	void BeforeRender()
	{
		float clearColor[] = { 0.1f, 0.0f, 0.2f, 1.0f };
		m_dxContext->ClearRenderTargetView( m_renderTargetView, clearColor );
		m_dxContext->ClearDepthStencilView( m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

		m_dxContext->OMSetDepthStencilState( 0, 0 );
		float blendFactors[] = { 0, 0, 0, 0 };
		m_dxContext->OMSetBlendState( 0, blendFactors, std::numeric_limits< unsigned int >::max() );
	}

	void AfterRender()
	{
		m_swapChain->Present( 0, 0 );
	}

	void SetCullMode( CullMode::TYPE mode )
	{
		// TODO: throw exception::NotImplemented( "DX11" );
	}

	Viewport GetViewport() const
	{
		return Viewport( 0, 0, GetDisplay().GetSize().width, GetDisplay().GetSize().height, GetDisplay().GetNearZ(), GetDisplay().GetFarZ() );
	}

	bool IsFullscreen() const
	{
		return m_display.IsFullscreen();
	}

	size_t GetIndex() const
	{
		return m_index;
	}

	HWND GetHandle() const
	{
		return  (HWND)m_display.GetHandle();
	}
};


Renderer::Renderer( WindowsOS * os, me::Display display, size_t index )
	: m_pimpl( new Pimpl( *this, os, display, index ) )
{
}

Renderer::~Renderer()
{
}

const me::Display & Renderer::GetDisplay() const
{
	return m_pimpl->GetDisplay();
}

ID3D11Device * Renderer::GetDxDevice() const
{
	return m_pimpl->GetDxDevice();
}

ID3D11DeviceContext * Renderer::GetDxContext() const
{
	return m_pimpl->GetDxContext();
}

IDXGISwapChain * Renderer::GetSwapChain() const
{
	return m_pimpl->GetSwapChain();
}
	   
void Renderer::BeforeRender()
{
	m_pimpl->BeforeRender();
}

void Renderer::AfterRender()
{
	m_pimpl->AfterRender();
}

void Renderer::SetCullMode( me::CullMode::TYPE mode )
{
	m_pimpl->SetCullMode( mode );
}

me::Viewport Renderer::GetViewport() const
{
	return Viewport( 0, 0, GetDisplay().GetSize().width, GetDisplay().GetSize().height, GetDisplay().GetNearZ(), GetDisplay().GetFarZ() );
}

bool Renderer::IsFullscreen() const
{
	return m_pimpl->GetDisplay().IsFullscreen();
}

size_t Renderer::GetIndex() const
{
	return m_pimpl->GetIndex();
}

void* Renderer::GetHandle() const
{
	return m_pimpl->GetDisplay().GetHandle();
}

void Renderer::Render( RenderMethod method ) const
{
	auto dxRenderer = this;
	auto dxDevice = dxRenderer->GetDxDevice();
	auto dxContext = dxRenderer->GetDxContext();
	
	D3D11_PRIMITIVE_TOPOLOGY topology{};
	switch( method.primitiveType )
	{
	case PrimitiveType::PointList: 
		topology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST; 
		break;
	case PrimitiveType::LineList: 
		topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST; 
		break;
	case PrimitiveType::LineStrip: 
		topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP; 
		break;
	case PrimitiveType::TriangleList: 
		topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;	
		break;
	case PrimitiveType::TriangleStrip: 
		topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;  
		break;
	}
	dxContext->IASetPrimitiveTopology( topology );

	if( method.useIB == false )
	{
		dxContext->Draw( method.vertexCount,  method.startVertex );
	}
	else
	{
		dxContext->DrawIndexed( method.indexCount, method.startIndex, method.baseVertexIndex );
	}
}

me::IVertexBuffer::ptr Renderer::ProduceVB( VertexBufferParameters parameters )
{
	return me::IVertexBuffer::ptr( new VertexBuffer( this, parameters ) );
}

me::IIndexBuffer::ptr Renderer::ProduceIB( IndexBufferParameters parameters )
{
	return me::IIndexBuffer::ptr( new IndexBuffer( this, parameters ) );
}

me::IVertexShader::ptr Renderer::ProduceVS( VertexShaderParameters parameters )
{
	return me::IVertexShader::ptr( new VertexShader( this, parameters ) );
}

me::IPixelShader::ptr Renderer::ProducePS( PixelShaderParameters parameters )
{
	return me::IPixelShader::ptr( new PixelShader( this, parameters ) );
}

me::IVertexDeclaration::ptr Renderer::ProduceVD( VertexDeclarationParameters parameters )
{
	return me::IVertexDeclaration::ptr( new VertexDeclaration( this, parameters ) );
}

me::ITexture::ptr Renderer::ProduceT( TextureParameters parameters )
{
	return me::ITexture::ptr( new Texture( this, parameters ) );
}
