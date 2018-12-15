// Copyright (c) 2003 - 2013, Evil Quail LLC
// All Rights Reserved

#include <meogl/VertexShader.h>
#include <meogl/Renderer.h>
#include <me/exception/NotImplemented.h>
#include <atlbase.h>

using namespace meogl;
using namespace me;

class VertexShader::Pimpl
{
	VertexShader & m_owner;
	Renderer * m_renderer;

	// TODO: Unify::Matrix
	struct ConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
		DirectX::XMMATRIX viewMatrix;
		DirectX::XMMATRIX projectionMatrix;
	} m_vertexShaderConstants;

	CComPtr< ID3D11VertexShader > m_vertexShader;
	CComPtr< ID3D10Blob > m_vertexShaderBuffer;
	//CComPtr< ID3D11ShaderReflection > m_vertexShaderReflection;

	// TODO: Need a standard interface for updating constant data that is cross renderer support.
	CComPtr< ID3D11Buffer > m_vertexShaderConstantBuffer;

public:
	Pimpl( VertexShader & owner, me::IRenderer * renderer )
		: m_owner( owner )
		, m_renderer( dynamic_cast< Renderer * >(renderer) )
	{
	}

	~Pimpl()
	{
		Destroy();
	}

	void Destroy()
	{
		m_vertexShader = nullptr;
		m_vertexShaderBuffer = nullptr;
	}

	void Create()
	{
		bool debug = false;
#if defined( DEBUG ) || defined( _DEBUG )
		debug = true;
#endif

		HRESULT result = S_OK;
		CComPtr< ID3D10Blob > errorBlob; // Generic buffer for error data.
		D3D_SHADER_MACRO * shaderMacros = 0;
		unsigned int flags1 = D3DCOMPILE_ENABLE_STRICTNESS;
		if ( debug )
		{
			flags1 |= D3DCOMPILE_DEBUG;
		}
		unsigned int flags2 = 0; // Only used for effect compilation.
		result = D3DCompileFromFile( unify::Cast< std::wstring >( m_owner.m_filePath.ToString() ).c_str(), shaderMacros, D3D_COMPILE_STANDARD_FILE_INCLUDE, m_owner.m_entryPointName.c_str(), m_owner.m_profile.c_str(), flags1, flags2, &m_vertexShaderBuffer, &errorBlob );
		if ( FAILED( result ) )
		{
			OutputDebugStringA( (char*)errorBlob->GetBufferPointer() );
			assert( !FAILED( result ) );
		}
		CreateThisShader();
	}

	void CreateThisShader()
	{
		HRESULT result = S_OK;

		auto dxDevice = m_renderer->GetDxDevice();
		ID3D11ClassLinkage * classLinkage = nullptr;
		result = dxDevice->CreateVertexShader( m_vertexShaderBuffer->GetBufferPointer(), m_vertexShaderBuffer->GetBufferSize(), classLinkage, &m_vertexShader );
		assert( !FAILED( result ) );

		/*
		result = D3DReflect( m_vertexShaderBuffer->GetBufferPointer(), m_vertexShaderBuffer->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&m_vertexShaderReflection );
		assert( !FAILED( result ) );
		*/

		using namespace DirectX;

		m_vertexShaderConstants.worldMatrix = XMMatrixIdentity();

		XMVECTOR eye = XMVectorSet( 0.0f, 1.0f, -50.0f, 0.0f );
		XMVECTOR at = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
		XMVECTOR up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
		m_vertexShaderConstants.viewMatrix = XMMatrixLookAtLH( eye, at, up );

		m_vertexShaderConstants.projectionMatrix = XMMatrixPerspectiveFovLH( DirectX::XM_PIDIV4, 800 / 640, 0.01f, 100.0f );


		D3D11_BUFFER_DESC constantBufferDesc = D3D11_BUFFER_DESC();
		constantBufferDesc.ByteWidth = sizeof( m_vertexShaderConstants );
		constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		D3D11_SUBRESOURCE_DATA constantBufferSourceData = D3D11_SUBRESOURCE_DATA();
		constantBufferSourceData.pSysMem = &m_vertexShaderConstants;
		result = dxDevice->CreateBuffer( &constantBufferDesc, &constantBufferSourceData, &m_vertexShaderConstantBuffer );
		assert( !FAILED( result ) );
		
		m_owner.m_vertexDeclaration->Build( m_owner );
		m_owner.m_created = true;
	}

	void Use( const RenderInfo & renderInfo )
	{
		auto dxDevice = m_renderer->GetDxDevice();
		auto dxContext = m_renderer->GetDxContext();

		using namespace DirectX;
		D3D11_MAPPED_SUBRESOURCE subResource = D3D11_MAPPED_SUBRESOURCE();
		HRESULT result = dxContext->Map( m_vertexShaderConstantBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &subResource );
		if ( FAILED( result ) )
		{
			throw unify::Exception( "Failed to set vertex shader!" );
		}

		memcpy( &m_vertexShaderConstants.worldMatrix.r[0], &renderInfo.GetWorldMatrix().m, sizeof( float ) * 4 * 4 );
		memcpy( &m_vertexShaderConstants.viewMatrix.r[0], &renderInfo.GetViewMatrix().m, sizeof( float ) * 4 * 4 );
		memcpy( &m_vertexShaderConstants.projectionMatrix.r[0], &renderInfo.GetProjectionMatrix().m, sizeof( float ) * 4 * 4 );

		ConstantBuffer * target = (ConstantBuffer*)subResource.pData;
		*target = m_vertexShaderConstants;

		dxContext->Unmap( m_vertexShaderConstantBuffer, 0 );

		dxContext->VSSetConstantBuffers( 0, 1, &m_vertexShaderConstantBuffer.p );

		dxContext->VSSetShader( m_vertexShader, nullptr, 0 );
	}

	const void * GetBytecode() const
	{
		return m_vertexShaderBuffer->GetBufferPointer();
	}

	size_t GetBytecodeLength() const
	{
		return m_vertexShaderBuffer->GetBufferSize();
	}
};


VertexShader::VertexShader( me::IRenderer * renderer )
	: m_pimpl( new Pimpl( *this, renderer ) )
	, m_assembly( false )
	, m_created( false )
{
}

VertexShader::VertexShader( me::IRenderer * renderer, VertexShaderParameters parameters )
	: m_pimpl( new Pimpl( *this, renderer ) )
	, m_assembly( false )
	, m_created( false )
	, m_vertexDeclaration( parameters.vertexDeclaration )
{
	Create( parameters );
}

VertexShader::~VertexShader()
{
	Destroy();
}

void VertexShader::Destroy()
{
	m_pimpl->Destroy();
}

void VertexShader::Create( VertexShaderParameters parameters )
{
	Destroy();

	if( parameters.path.Empty() == false )
	{
		m_filePath = parameters.path;
	}
	else
	{
		m_code = parameters.code;
	}

	m_entryPointName = parameters.entryPointName;
	m_profile = parameters.profile;

	m_pimpl->Create();
}

void VertexShader::SetVertexDeclaration( IVertexDeclaration::ptr vertexDeclaration )
{
	m_vertexDeclaration = vertexDeclaration;
}

IVertexDeclaration::ptr VertexShader::GetVertexDeclaration() const
{
	return m_vertexDeclaration;
}

const void * VertexShader::GetBytecode() const
{
	return m_pimpl->GetBytecode();
}

size_t VertexShader::GetBytecodeLength() const
{
	return m_pimpl->GetBytecodeLength();
}

void VertexShader::Use( const RenderInfo & renderInfo )
{
	m_vertexDeclaration->Use();
	m_pimpl->Use( renderInfo );
}

std::string VertexShader::GetError()
{
	return m_errorMessage;
}