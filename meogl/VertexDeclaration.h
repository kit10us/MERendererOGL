// Copyright (c) 2003 - 2013, Evil Quail LLC
// All Rights Reserved

#pragma once

#include <me/IRenderer.h>
#include <me/IVertexDeclaration.h>
#include <me/VertexElement.h>
#include <meogl/Renderer.h>
#include <unify/unify.h>
#include <unify/String.h>
#include <unify/DataLock.h>
#include <atlbase.h>
#include <map>

namespace meogl
{
	/// <summary> 
	/// Defines the structure of the data in a vertex. 
	/// </summary>
	class VertexDeclaration : public me::IVertexDeclaration
	{
	public:
		VertexDeclaration( me::IRenderer * renderer );
		VertexDeclaration( me::IRenderer * renderer, me::VertexDeclarationParameters parameters );

		~VertexDeclaration();

		void Build( const me::IVertexShader & vs );
		
		bool operator==( const VertexDeclaration & b ) const;
		bool operator!=( const VertexDeclaration & b ) const;

		void Destroy();
		
		size_t GetNumberOfElements() const;
		bool ElementExists( const std::string & name ) const;

		/// <summary>
		/// Get an element by name.
		/// </summary>
		bool GetElement( const std::string name, me::VertexElement & element ) const;

		/// <summary>
		/// Get an element by index.
		/// </summary>
		bool GetElement( size_t index, me::VertexElement & element ) const;

		/// <summary>
		/// Get an element that closely matches a specified element.
		/// </summary>
		bool GetElement( me::VertexElement toFind, me::VertexElement & element ) const;

		// Can throw, ensure ElementExists is tested before calling this function.
		size_t GetElementOffset( const std::string & name ) const;
		size_t GetElementSize( const std::string & name ) const;
		
		size_t GetSize( size_t slot = 0 ) const;

		size_t NumberOfSlots() const;

		std::vector< me::VertexElement > Elements() const override;

		void Use();

	private:
		Renderer * m_renderer;

		// ElementMap: first, std::string, is the element's name for lookup, and second, size_t, is the index into m_elements.
		typedef std::map< std::string, size_t, unify::CaseInsensitiveLessThanTest > ElementMap;

		std::vector< me::VertexElement > m_elements;
		ElementMap m_elementMap;

		CComPtr< ID3D11InputLayout > m_layout;
	};
}