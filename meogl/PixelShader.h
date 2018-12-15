// Copyright (c) 2003 - 2013, Evil Quail LLC
// All Rights Reserved

#pragma once

#include <me/RenderInfo.h>
#include <me/IRenderer.h>
#include <meogl/Renderer.h>
#include <unify/Path.h>
#include <memory>
#include <atlbase.h>

namespace meogl
{
	class PixelShader : public me::IPixelShader
	{
	public:
		PixelShader( me::IRenderer * renderer );
		PixelShader( me::IRenderer * renderer, me::PixelShaderParameters parameters );

		~PixelShader();

		void Destroy();

		void Create( me::PixelShaderParameters parameters );

		void Use( const me::RenderInfo & renderInfo ) const override;

		void SetTrans( bool bTrans ) override;

		bool IsTrans() override;

		std::string GetError();

	protected:
		me::PixelShaderParameters m_parameters;
		bool m_assembly;
		bool m_created;

		bool m_isTrans;	// Does this pixel shader turn the render into transparent (in part or entire)

		Renderer * m_renderer;
		CComPtr< ID3D11PixelShader > m_pixelShader;
		CComPtr< ID3D10Blob > m_pixelShaderBuffer;
	};
}