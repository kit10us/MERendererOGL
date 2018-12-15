// Copyright (c) 2002 - 2013, Evil Quail LLC
// All Rights Reserved

#pragma once
#include <me/IRenderer.h>
#include <me/TextureLock.h>
#include <unify/Rect.h>
#include <unify/Color.h>
#include <unify/TexArea.h>
#include <unify/Path.h>
#include <string>
#include <memory>
#include <map>

// MS agressive macros.
#ifdef LoadImage
#undef LoadImage
#endif

#ifdef min
#undef min
#endif

namespace meogl
{

	//////////////////////////////////
	// Font definitions...

#define FONT_BOLD        0x0001
#define FONT_ITALIC      0x0002
#define FONT_ZENABLE     0x0004

	// Font rendering flags
#define FONT_CENTERED_X  0x0001
#define FONT_CENTERED_Y  0x0002
#define FONT_TWOSIDED    0x0004
#define FONT_FILTERED    0x0008

#define SPRITEANIMLOOP_NONE			0
#define SPRITEANIMLOOP_REPEAT		1	// 1 loop period
#define SPRITEANIMLOOP_FORWARDBACK	2	// 2 loop periods

	class Texture : public me::ITexture
	{
	public:
		typedef std::shared_ptr< Texture > ptr;

		struct SpriteArray
		{
			SpriteArray() {}
			SpriteArray( unsigned int head, unsigned int tail ) : head( head ), tail( tail ) {}
			unsigned int head;
			unsigned int tail;
		};

		typedef std::map< const std::string, SpriteArray > SpriteArrayMap;
		typedef std::vector< unify::TexArea > SpriteMasterList;

		static bool s_allowTextureUses;

		Texture( me::IRenderer * renderer, me::TextureParameters parameters = me::TextureParameters() );
		virtual ~Texture();

		// ::Resource...
		void Preload();
		void Create();
		void Destroy();

		void ClearHeader();

		const unsigned int FileWidth() const;
	
		const unsigned int FileHeight() const;

		const unify::Size< unsigned int > & ImageSize() const;

		bool Use( unsigned int stage );

		bool GetRenderable() const;

		bool GetLockable() const;

		void LockRect( unsigned int level, me::TextureLock & lock, const unify::Rect< long > * rect, bool readonly );
		void UnlockRect( unsigned int level );

		bool HasSpriteArray( const std::string & name ) const;
		const Texture::SpriteArray & FindSpriteArray( const std::string & name ) const;
		const Texture::SpriteArray & GetSpriteArray( unsigned int index ) const;
		const unify::TexArea & GetSprite( const std::string & arrayName, unsigned int index ) const;
		unsigned int SpriteArrayCount() const;
		const unify::TexArea & GetSprite( unsigned int index ) const;
		unsigned int SpriteCount() const;

	protected:
		void LoadImage( const unify::Path & filePath );
		void LoadHeader();

		class Pimpl;
		std::shared_ptr< class Pimpl > m_pimpl;
	};
}