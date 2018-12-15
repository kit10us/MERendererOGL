// Copyright (c) 2003 - 2014, Evil Quail LLC
// All Rights Reserved

#include <meogl/IndexBuffer.h>
#include <me/exception/FailedToCreate.h>
#include <me/exception/FailedToLock.h>
#include <me/exception/OutOfBounds.h>
#include <me/exception/NotImplemented.h>

using namespace meogl;
using namespace me;

IndexBuffer::IndexBuffer( me::IRenderer * renderer )
	: m_renderer( dynamic_cast< Renderer * >(renderer) )
	, m_locked( false )
	, m_usage( BufferUsage::Default )
	, m_length( 0 )
	, m_stride( 0 )
{
}

IndexBuffer::IndexBuffer( me::IRenderer * renderer, IndexBufferParameters parameters )
	: m_renderer( dynamic_cast< Renderer * >(renderer) )
	, m_locked( false )
	, m_usage( BufferUsage::Default )
	, m_length( 0 )
	, m_stride( 0 )
{
	Create( parameters );
}


IndexBuffer::~IndexBuffer()
{
	Destroy();
}

void IndexBuffer::Create( IndexBufferParameters parameters )
{
	Destroy();

	auto dxDevice = m_renderer->GetDxDevice();

	HRESULT hr = S_OK;

	m_length = parameters.numIndices;

	// Fill in a buffer description.
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof( unsigned int ) * parameters.numIndices;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA initialData = D3D11_SUBRESOURCE_DATA();
	initialData.pSysMem = parameters.source;

	// Create the buffer with the device.
	hr = dxDevice->CreateBuffer( &bufferDesc, &initialData, &m_IB );
	if ( FAILED( hr ) )
	{
		throw exception::FailedToCreate( "Failed to create index buffer!" );
	}
}

void IndexBuffer::Resize( unsigned int numIndices )
{
	if ( GetUsage() != BufferUsage::Staging )
	{
		throw unify::Exception( "IndexBuffer's usage is not Staging! Cannot resize without CPU read and write access." );
	}

	/*
	TODO:

	unsigned int oldNumberOfIndices = GetLength();
	size_t oldSize = GetSize();
	ID3D11Buffer * oldIndexBuffer = m_IB;
	m_IB = 0;
	m_length = 0;

	Create( numIndices, this->GetFormat(), GetUsage(), m_createFlags );

	HRESULT hr;
	unsigned char * oldData;
	hr = oldIndexBuffer->Lock( 0, 0, (void**)&oldData, D3DLOCK_READONLY );
	if( FAILED(hr) )
	{
	throw exception::FailedToLock( "Failed to lock index buffer!" );
	}

	unsigned char * newData;
	hr = m_IB->Lock( 0, 0, (void**)&newData, D3DLOCK_DISCARD );
	if( FAILED(hr) )
	{
	oldIndexBuffer->Release();
	oldIndexBuffer = 0;
	throw exception::FailedToLock( "Failed to lock index buffer!" );
	}

	memcpy( newData, oldData, oldSize );
	m_IB->Unlock();
	oldIndexBuffer->Unlock();
	oldIndexBuffer->Release();
	oldIndexBuffer = 0;
	*/
}

size_t IndexBuffer::Append( const IndexBuffer & from, size_t vertexOffset  )
{
	size_t offset = GetLength();

	if ( from.GetLength() == 0 )
	{
		return offset;
	}

	if ( from.GetUsage() != BufferUsage::Staging )
	{
		throw unify::Exception( "From IndexBuffer's usage is not Staging!" );
	}

	if ( GetLength() == 0 )
	{
		assert( 0 ); // TODO:
		//Create( from.GetLength(), nullptr, from.GetUsage(), from.m_pimpl->m_createFlags );
	}
	else
	{
		if ( GetUsage() != BufferUsage::Staging )
		{
			throw unify::Exception( "IndexBuffer's usage is not Staging!" );
		}
		Resize( GetLength() + from.GetLength() );
	}

	// Copy vertices...
	IndexLock locksrc, lockdest;
	Lock( lockdest );

	from.LockReadOnly( locksrc );

	for ( size_t i = 0; i < locksrc.Count(); ++i )
	{
		size_t to = offset + i;
		const void * rawData = locksrc.GetItemReadOnly( i );
		if ( locksrc.Stride() == 2 )
		{
			unsigned short fromIndex = *static_cast< const short int * >(rawData);
			lockdest.SetIndex( to, fromIndex + vertexOffset );
		}
		else
		{
			unsigned int fromIndex = *static_cast< const unsigned int * >(rawData);
			lockdest.SetIndex( to, fromIndex + vertexOffset );
		}
	}

	from.UnlockReadOnly();
	Unlock();

	return offset;
}

void IndexBuffer::Destroy()
{
	m_IB = nullptr;
	m_length = 0;
}

void IndexBuffer::Lock( IndexLock & lock )
{
	throw exception::NotImplemented( "Locking not supported in DX11!" );
}

void IndexBuffer::LockReadOnly( IndexLock & lock ) const
{
	throw exception::NotImplemented( "Locking not supported in DX11!" );
}

void IndexBuffer::Unlock()
{
	throw exception::NotImplemented( "Locking not supported in DX11!" );
}

void IndexBuffer::UnlockReadOnly() const
{
	throw exception::NotImplemented( "Locking not supported in DX11!" );
}

bool IndexBuffer::Valid() const
{
	return m_IB != 0;
}

void IndexBuffer::Use() const
{
	if ( !m_IB )
	{
		return;
	}

	// Set the buffer.
	m_renderer->GetDxContext()->IASetIndexBuffer( m_IB, DXGI_FORMAT_R32_UINT, 0 );
}

bool IndexBuffer::Locked() const
{
	return m_locked;
}

BufferUsage::TYPE IndexBuffer::GetUsage() const
{
	return m_usage;
}

unsigned int IndexBuffer::GetStride() const
{
	return m_stride;
}

unsigned int IndexBuffer::GetLength() const
{
	return m_length;
}

unsigned int IndexBuffer::GetSize() const
{
	return m_stride * m_length;
}
