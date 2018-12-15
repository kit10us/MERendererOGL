// Copyright (c) 2003 - 2013, Evil Quail LLC
// All Rights Reserved

#pragma once

#include <me/IVertexBuffer.h>
#include <me/IRenderer.h>
#include <me/DataBuffer.h>
#include <me/IVertexDeclaration.h>
#include <unify/BBox.h>

namespace meogl
{
	class VertexBuffer : public me::IVertexBuffer
	{
	public:
		VertexBuffer( me::IRenderer * renderer );
		VertexBuffer( me::IRenderer * renderer, me::VertexBufferParameters parameters );
		~VertexBuffer();

		void Create( me::VertexBufferParameters parameters ) override;
		void Destroy();

		void Lock( unify::DataLock & lock );
		void LockReadOnly( unify::DataLock & lock ) const;
		void Unlock();
		void Unlock() const;
		
		me::IVertexDeclaration::ptr GetVertexDeclaration() const override;
		size_t GetSlot() const;
		
		bool Valid() const;
		void Use() const;

		unify::BBox< float > & GetBBox() override;
		const unify::BBox< float > & GetBBox() const override;

		bool Locked() const;
		me::BufferUsage::TYPE GetUsage() const;
		unsigned int GetStride() const;
		unsigned int GetLength() const;
		unsigned int GetSize() const;

	protected:
		class Pimpl;
		std::shared_ptr< Pimpl > m_pimpl;

		me::IVertexDeclaration::ptr m_vertexDeclaration;
		size_t m_slot;

		unify::BBox< float > m_bbox;

		bool m_locked;
		me::BufferUsage::TYPE m_usage;
		unsigned int m_stride; // Size of each item in the buffer.
		unsigned int m_length; // Number of items we can store in the buffer.
	};
}