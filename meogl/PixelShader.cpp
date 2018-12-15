// Copyright (c) 2003 - 2013, Evil Quail LLC
// All Rights Reserved

#include <meogl/PixelShader.h>
#include <me/exception/NotImplemented.h>
#include <me/exception/FailedToCreate.h>

using namespace meogl;
using namespace me;

PixelShader::PixelShader( me::IRenderer * renderer )
	: m_renderer( dynamic_cast< Renderer * >(renderer) )
	, m_assembly( false )
	, m_created( false )
	, m_isTrans( false )
{
}

PixelShader::PixelShader( me::IRenderer * renderer, PixelShaderParameters parameters )
	: PixelShader( renderer )
{
	Create( parameters );
}

PixelShader::~PixelShader()
{
	Destroy();
}

void PixelShader::Destroy()
{
	m_pixelShader = nullptr;
	m_pixelShaderBuffer = nullptr;
}

void PixelShader::Create( PixelShaderParameters parameters )
{
	Destroy();

	m_parameters = parameters;

	bool debug =
#if defined( DEBUG ) || defined( _DEBUG )
		true;
#else
		false;
#endif

	HRESULT result;
	CComPtr< ID3D10Blob > errorBlob; // Generic buffer for error data.
	D3D_SHADER_MACRO * shaderMacros = 0;
	unsigned int flags1 = D3DCOMPILE_ENABLE_STRICTNESS;
	if ( debug )
	{
		flags1 |= D3DCOMPILE_DEBUG;
	}

	unsigned int flags2 = 0; // Only used for effect compilation.

	if ( ! m_parameters.code.empty() )
	{
		result = D3DCompile( m_parameters.code.c_str(), m_parameters.code.length(), nullptr, nullptr, nullptr, m_parameters.entryPointName.c_str(), m_parameters.profile.c_str(), flags1, flags2, &m_pixelShaderBuffer, &errorBlob );
	}
	else if ( ! m_parameters.path.Empty() )
	{
		result = D3DCompileFromFile( unify::Cast< std::wstring >( m_parameters.path.ToString() ).c_str(), shaderMacros, D3D_COMPILE_STANDARD_FILE_INCLUDE, m_parameters.entryPointName.c_str(), m_parameters.profile.c_str(), flags1, flags2, &m_pixelShaderBuffer, &errorBlob );
	}
	else
	{
		throw exception::FailedToCreate( "Failed to create shader, neither code nor file path specified!" );
	}

	if ( FAILED( result ) )
	{
		OutputDebugStringA( (char*)errorBlob->GetBufferPointer() );
		throw exception::FailedToCreate( std::string( "Failed to create shader \"" ) + m_parameters.path.ToString() + "\": " +  std::string( (char*)errorBlob->GetBufferPointer() ) );
	}

	ID3D11ClassLinkage * classLinkage = nullptr;
	result = m_renderer->GetDxDevice()->CreatePixelShader( m_pixelShaderBuffer->GetBufferPointer(), m_pixelShaderBuffer->GetBufferSize(), classLinkage, &m_pixelShader );
	if ( FAILED( result ) )
	{
		throw exception::FailedToCreate( "Failed to create shader!" );
	}
}

void PixelShader::Use( const RenderInfo & renderInfo ) const
{
	m_renderer->GetDxContext()->PSSetShader( m_pixelShader, nullptr, 0 );
}

void PixelShader::SetTrans( bool bTrans )
{
	m_isTrans = bTrans;
}

bool PixelShader::IsTrans()
{
	return(this && m_isTrans) ? true : FALSE;
}
