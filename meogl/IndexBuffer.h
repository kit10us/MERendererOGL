// Copyright (c) 2003 - 2014, Evil Quail LLC
// All Rights Reserved

#pragma once

#include <meogl/Renderer.h>
#include <me/IIndexBuffer.h>
#include <me/DataBuffer.h>
#include <me/IndexLock.h>
#include <me/DataBuffer.h>
#include <unify/unify.h>
#include <unify/Flags.h>
#include <atlbase.h>
#include <memory>

namespace meogl
{
	class IndexBuffer : public me::IIndexBuffer
	{
	public:
		struct CreateFlags
		{
			enum TYPE
			{
				DoNotClip = FLAG01,
				Dynamic = FLAG02,
				NPatches = FLAG03,
				Points = FLAG04,
				RTPatches = FLAG05,
				SoftwareProcessing = FLAG06,
				WriteOnly = FLAG07,
			};
		};

		IndexBuffer( me::IRenderer * renderer );
		IndexBuffer( me::IRenderer * renderer, me::IndexBufferParameters parameters );
		~IndexBuffer();

		void Create( me::IndexBufferParameters parameters );
		void Resize( unsigned int numIndices );

		/// <summary>
		/// Append to our existing indices, the indices from ib, adding a vertex offset to each new index.
		/// Returns the index offset for the first index from 'from'.
		/// </summary>
		size_t Append( const IndexBuffer & from, size_t vertexOffset = 0 );
		void Destroy();

		void Lock( me::IndexLock & lock );
		void LockReadOnly( me::IndexLock & lock ) const;
		void Unlock();
		void UnlockReadOnly() const;
		unsigned int GetCreateFlags() const;
		bool Valid() const;
		
		void Use() const override;

		bool Locked() const;
		me::BufferUsage::TYPE GetUsage() const;
		unsigned int GetStride() const;
		unsigned int GetLength() const;
		unsigned int GetSize() const;

	protected:
		Renderer * m_renderer;
		unsigned int m_createFlags;
		CComPtr< ID3D11Buffer > m_IB;
		bool m_locked;
		me::BufferUsage::TYPE m_usage;
		unsigned int m_stride; // Size of each item in the buffer.
		unsigned int m_length; // Number of items we can store in the buffer.
	};
}