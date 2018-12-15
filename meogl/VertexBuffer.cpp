// Copyright (c) 2003 - 2013, Evil Quail LLC
// All Rights Reserved

#include <meogl/VertexBuffer.h>
#include <meogl/Renderer.h>
#include <me/exception/FailedToCreate.h>
#include <me/exception/FailedToLock.h>
#include <me/exception/NotImplemented.h>
#include <me/exception/Exception.h>
#include <atlbase.h>

using namespace meogl;
using namespace me;

class VertexBuffer::Pimpl
{
public:
	Pimpl( VertexBuffer & owner, me::IRenderer * renderer )
		: m_owner( owner )
		, m_renderer( dynamic_cast< Renderer * >(renderer) )
	{
	}

	~Pimpl()
	{
	}

	void Destroy()
	{
		m_VB = nullptr;
	}

	void Create( VertexBufferParameters parameters )
	{
		auto dxDevice = m_renderer->GetDxDevice();

		// Ensure that if we are BufferUsage::Immutable, then source is not null.
		if ( BufferUsage::Immutable && parameters.source == nullptr )
		{
			throw exception::FailedToCreate( "Buffer is immutable, yet source is null!" );
		}

		D3D11_USAGE usageDX{};
		switch ( m_owner.m_usage )
		{
		case BufferUsage::Default:
			usageDX = D3D11_USAGE_DEFAULT;
			break;
		case BufferUsage::Immutable:
			usageDX = D3D11_USAGE_IMMUTABLE;
			break;
		case BufferUsage::Dynamic:
			usageDX = D3D11_USAGE_DYNAMIC;
			break;
		case BufferUsage::Staging:
			usageDX = D3D11_USAGE_STAGING;
			break;
		}

		D3D11_BUFFER_DESC vertexBufferDesc{};
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.ByteWidth = parameters.vertexDeclaration->GetSize( 0 ) * parameters.numVertices;
		vertexBufferDesc.Usage = usageDX;

		HRESULT result;
		if ( parameters.source != nullptr )
		{
			D3D11_SUBRESOURCE_DATA initialData{};
			initialData.pSysMem = parameters.source;
			if ( parameters.usage == BufferUsage::Dynamic )
			{
				vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // TODO: make not hard coded.
			}
			result = dxDevice->CreateBuffer( &vertexBufferDesc, &initialData, &m_VB );
		}
		else
		{
			result = dxDevice->CreateBuffer( &vertexBufferDesc, nullptr, &m_VB );
		}
		OnFailedThrow( result, "Failed to create vertex buffer!" );
	}

	void Lock( unify::DataLock & lock )
	{
		throw exception::NotImplemented();
	}

	void LockReadOnly( unify::DataLock & lock ) const
	{
		throw exception::NotImplemented();
	}

	void Unlock()
	{
		throw exception::NotImplemented();
	}

	void Unlock() const
	{
		throw exception::NotImplemented();
	}

	bool Valid() const
	{
		return m_VB != 0;
	}

	void Use() const
	{
		unsigned int streamNumber = 0;
		unsigned int stride = m_owner.GetStride();
		unsigned int offsetInBytes = 0;
		auto dxContext = m_renderer->GetDxContext();
		dxContext->IASetVertexBuffers( streamNumber, 1, &m_VB.p, &stride, &offsetInBytes );
	}

	VertexBuffer & m_owner;
	Renderer * m_renderer;
	CComPtr< ID3D11Buffer > m_VB;
};

VertexBuffer::VertexBuffer( me::IRenderer * renderer )
	: m_pimpl( new Pimpl( *this, renderer ) )
	, m_slot( 0 )
	, m_locked( false )
	, m_usage( BufferUsage::Default )
	, m_length( 0 )
	, m_stride( 0 )
{
}

VertexBuffer::VertexBuffer( me::IRenderer * renderer, me::VertexBufferParameters parameters )
	: m_pimpl( new Pimpl( *this, renderer ) )
	, m_slot( 0 )
	, m_locked( false )
	, m_usage( BufferUsage::Default )
	, m_length( 0 )
	, m_stride( 0 )
{
	Create( parameters );
}


VertexBuffer::~VertexBuffer()
{
	Destroy();
}

void VertexBuffer::Create( VertexBufferParameters parameters )
{
	Destroy();

	m_vertexDeclaration = parameters.vertexDeclaration;
	m_slot = parameters.slot;
	m_stride = m_vertexDeclaration->GetSize( parameters.slot );
	m_length = parameters.numVertices;
	m_usage = parameters.usage;
	m_bbox = parameters.bbox;

	// Ensure we have some sort of idea what we need to be...
	if( GetSize() == 0 )
	{
		throw exception::FailedToCreate( "Not a valid vertex buffer size!" );
	}

	m_pimpl->Create( parameters );
}

void VertexBuffer::Destroy()
{
	m_pimpl->Destroy();
	m_length = 0;
	m_stride = 0;
}

void VertexBuffer::Lock( unify::DataLock & lock )
{
	m_pimpl->Lock( lock );
}

void VertexBuffer::LockReadOnly( unify::DataLock & lock ) const
{
	m_pimpl->LockReadOnly( lock );
}

void VertexBuffer::Unlock()
{
	m_pimpl->Unlock();
}

void VertexBuffer::Unlock() const
{
	m_pimpl->Unlock();
}

IVertexDeclaration::ptr VertexBuffer::GetVertexDeclaration() const
{
	return m_vertexDeclaration;
}

size_t VertexBuffer::GetSlot() const
{
	return m_slot;
}

bool VertexBuffer::Valid() const
{
	return m_pimpl->Valid();
}

void VertexBuffer::Use() const
{
	m_pimpl->Use();
}

unify::BBox< float > & VertexBuffer::GetBBox()
{
	return m_bbox;
}

const unify::BBox< float > & VertexBuffer::GetBBox() const
{
	return m_bbox;
}

bool VertexBuffer::Locked() const
{
	return m_locked;
}

BufferUsage::TYPE VertexBuffer::GetUsage() const
{
	return m_usage;
}

unsigned int VertexBuffer::GetStride() const
{
	return m_stride;
}

unsigned int VertexBuffer::GetLength() const
{
	return m_length;
}

unsigned int VertexBuffer::GetSize() const
{
	return m_stride * m_length;
}
