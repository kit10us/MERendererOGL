// Copyright (c) 2003 - 2013, Evil Quail LLC
// All Rights Reserved

#pragma once

#include <me/RenderInfo.h>
#include <me/IRenderer.h>
#include <me/IVertexShader.h>
#include <me/IVertexDeclaration.h>
#include <unify/Path.h>

namespace meogl
{
	class VertexShader : public me::IVertexShader
	{
	public:
		VertexShader( me::IRenderer * renderer );
		VertexShader( me::IRenderer * renderer, me::VertexShaderParameters parameters );

		~VertexShader();

		void Destroy();

		void Create( me::VertexShaderParameters parameters );

		void SetVertexDeclaration( me::IVertexDeclaration::ptr vertexDeclaration );

		me::IVertexDeclaration::ptr GetVertexDeclaration() const override;

		const void * GetBytecode() const;

		size_t GetBytecodeLength() const;

		void Use( const me::RenderInfo & renderInfo );

		std::string GetError();

	protected:	   
		unify::Path m_filePath;
		std::string m_code;
		bool m_assembly;
		std::string m_entryPointName;
		std::string m_profile;
		std::string m_errorMessage;
		bool m_created;
		me::IVertexDeclaration::ptr m_vertexDeclaration;

		class Pimpl;
		std::shared_ptr< Pimpl > m_pimpl;
	};
}