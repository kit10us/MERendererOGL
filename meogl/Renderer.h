// Copyright (c) 2003 - 2014, Evil Quail LLC
// All Rights Reserved

#pragma once

#include <me/render/IRenderer.h>
#include <me/render/Display.h>
#include <meogl/OpenGL.h>
#include <memory>

namespace meogl
{
	class WindowsOS;

	class Renderer : public me::render::IRenderer
	{
	public:
		Renderer( WindowsOS * os, me::render::Display display, size_t index );
		virtual ~Renderer();																															   
		const me::render::Display & GetDisplay() const;

		/*
		ID3D11Device * GetDxDevice() const;
		ID3D11DeviceContext * GetDxContext() const;
		IDXGISwapChain * GetSwapChain() const;
		*/

		void SetCullMode( me::render::CullMode::TYPE mode ) override;

		void BeforeRender() override;
		void AfterRender() override;

		me::render::Viewport GetViewport() const override;

		bool IsFullscreen() const override;

		size_t GetIndex() const override;

		void* GetHandle() const override;


		void Render( me::RenderMethod ) const override;

		me::render::IVertexBuffer::ptr ProduceVB( me::render::VertexBufferParameters parameters ) override;
		me::render::IIndexBuffer::ptr ProduceIB( me::render::IndexBufferParameters parameters ) override;
		me::render::IVertexShader::ptr ProduceVS( me::render::VertexShaderParameters parameters ) override;
		me::render::IPixelShader::ptr ProducePS( me::render::PixelShaderParameters parameters ) override;
		me::render::IVertexDeclaration::ptr ProduceVD( me::render::VertexDeclarationParameters parameters ) override;
		me::render::ITexture::ptr ProduceT( me::render::TextureParameters parameters ) override;

	private:
		class Pimpl;
		std::shared_ptr< Pimpl > m_pimpl;
	};
}