// Copyright (c) 2003 - 2013, Evil Quail LLC
// All Rights Reserved
//

#include <DirectXTex.h>
#include <DDS.h>
#pragma comment( lib, "DirectXTex" )

#include <meogl/Texture.h>
#include <meogl/Renderer.h>
#include <qxml/Document.h>
#include <me/exception/NotImplemented.h>
#include <me/exception/FailedToLock.h>
#include <atlbase.h>

// MS agressive macros.
#ifdef LoadImage
#undef LoadImage
#endif

using namespace meogl;
using namespace me;


class Texture::Pimpl
{
	Texture & m_owner;
	Renderer * m_renderer;

	CComPtr< ID3D11Texture2D > m_texture;
	CComPtr< ID3D11SamplerState > m_colorMapSampler;
	CComPtr< ID3D11ShaderResourceView > m_colorMap;

	bool m_created;
	unify::Size< unsigned int > m_fileSize;
	unify::Size< unsigned int > m_imageSize;
	bool m_useColorKey;
	unify::Color m_colorKey;
	TextureParameters m_parameters;
	SpriteArrayMap m_spriteArrayMap;
	SpriteMasterList m_spriteMasterList;
	DirectX::ScratchImage m_scratch;	

public:
	Pimpl( Texture & owner, me::IRenderer * renderer, TextureParameters parameters )
		: m_owner( owner )
		, m_renderer( dynamic_cast<Renderer *>(renderer) )
		, m_useColorKey( false )
		, m_created( false )
		, m_parameters( parameters )
	{
	}

	Pimpl( Texture & owner, me::IRenderer * renderer, const unify::Path & filePath, TextureParameters parameters )
		: Pimpl( owner, renderer, parameters )
	{
		Create();
	}

	~Pimpl()
	{
		Destroy();
	}

	void Create()
	{
		// Load from a file.
		if ( m_parameters.source.Empty() )
		{
			throw unify::Exception( "Not enough information to load Texture!" );
		}

		LoadHeader();

		LoadImage( m_parameters.source );

		m_created = true;
	}

	// Destroyes data, keeps header intact - thus no graphical footprint, yet we can still use it's statistics/dimensions in calculations.
	void Destroy()
	{
		m_texture = nullptr;
		m_created = false; // TODO: Can solve this from m_texture.
		m_scratch.Release();
	}

	void LockRect( unsigned int level, TextureLock & lock, const unify::Rect< long > * rect, bool readonly )
	{
		if ( !m_created )
		{
			Create();
		}

		if ( m_parameters.lockable == false )
		{
			throw exception::FailedToLock( "Texture is not lockable!" );
		}

		// Allow us to convert from our flag type to DX.
		/*
		unsigned int dxFlags = readonly ? D3DLOCK_READONLY : 0;

		D3DLOCKED_RECT d3dLockedRect;
		if ( FAILED( m_texture->LockRect( level, &d3dLockedRect, (RECT*)rect, dxFlags ) ) )
		{
		lock.pBits = 0;
		lock.uStride = 0;
		throw unify::Exception( "Failed to lock texture: \"" + m_filePath.ToString() + "\"" );
		}
		*/

		lock.pBits = m_scratch.GetImage( level, 0, 0 )->pixels;
		lock.uStride = m_scratch.GetImage( level, 0, 0 )->rowPitch;
		lock.bpp = 4;
		if ( rect )
		{
			lock.uWidth = rect->right - rect->left;
			lock.uHeight = rect->bottom - rect->top;
		}
		else
		{
			lock.uWidth = m_imageSize.width;
			lock.uHeight = m_imageSize.height;
		}
		lock.totalBytes = lock.uHeight * lock.uStride;
	}

	void UnlockRect( unsigned int level )
	{

	}

	void LoadImage( unify::Path filePath )
	{
		auto dxDevice = m_renderer->GetDxDevice();

		// Release any previous texture
		Destroy();

		// Verify file exists
		if ( !filePath.Exists() )
		{
			throw unify::Exception( "Failed to load image, file not found! (" + filePath.ToString() + ")" );
		}

		HRESULT result = S_OK;

		//DirectX::Image sourceImages{};
		DirectX::TexMetadata texMetadata{};

		//DirectX::ScratchImagem_scratch{};

		if ( m_parameters.source.IsExtension( "DDS" ) )
		{
			result = DirectX::LoadFromDDSFile( unify::Cast< std::wstring >( m_parameters.source.ToString() ).c_str(), 0, &texMetadata, m_scratch );
		}
		else if ( m_parameters.source.IsExtension( "BMP" ) || m_parameters.source.IsExtension( "JPG" ) || m_parameters.source.IsExtension( "JPEG" ) || m_parameters.source.IsExtension( "TIFF" ) || m_parameters.source.IsExtension( "TIF" ) || m_parameters.source.IsExtension( "HDP" ) || m_parameters.source.IsExtension( "PNG" ) )
		{
			result = DirectX::LoadFromWICFile( unify::Cast< std::wstring >( m_parameters.source.ToString() ).c_str(), 0, &texMetadata, m_scratch );
		}
		else if ( m_parameters.source.IsExtension( "TGA" ) )
		{
			result = DirectX::LoadFromTGAFile( unify::Cast< std::wstring >( m_parameters.source.ToString() ).c_str(), &texMetadata, m_scratch );
		}
		else
		{
			throw unify::Exception( "File format for \"" + m_parameters.source.ToString() + "\" not supported!" );
		}

		if ( FAILED( result ) )
		{
			throw unify::Exception( "Failed to load image \"" + m_parameters.source.ToString() + "\"!" );
		}

		unsigned int width = m_scratch.GetImage( 0, 0, 0 )->width;
		unsigned int height = m_scratch.GetImage( 0, 0, 0 )->height;

		D3D11_SUBRESOURCE_DATA data{};
		data.pSysMem = m_scratch.GetImage( 0, 0, 0 )->pixels;
		data.SysMemPitch = m_scratch.GetImage( 0, 0, 0 )->rowPitch;
		data.SysMemSlicePitch = m_scratch.GetImage( 0, 0, 0 )->slicePitch;

		D3D11_TEXTURE2D_DESC textureDesc{};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = m_scratch.GetImage( 0, 0, 0 )->format;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DYNAMIC;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		textureDesc.MiscFlags = 0;
		result = dxDevice->CreateTexture2D( &textureDesc, &data, &m_texture );
		if ( FAILED( result ) )
		{
			Destroy();
			throw unify::Exception( "Failed to load image\"" + filePath.ToString() + "\"!" );
		}

		D3D11_SAMPLER_DESC colorMapDesc{};
		colorMapDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		colorMapDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		colorMapDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		colorMapDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		if ( m_parameters.min == Filtering::Point && m_parameters.mag == Filtering::Point && m_parameters.mip == Filtering::Point )
		{
			colorMapDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		}
		else if ( m_parameters.min == Filtering::Point && m_parameters.mag == Filtering::Point && m_parameters.mip == Filtering::Linear )
		{
			colorMapDesc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		}
		else if ( m_parameters.min == Filtering::Point && m_parameters.mag == Filtering::Linear && m_parameters.mip == Filtering::Point )
		{
			colorMapDesc.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		}
		else if ( m_parameters.min == Filtering::Point && m_parameters.mag == Filtering::Linear && m_parameters.mip == Filtering::Linear )
		{
			colorMapDesc.Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
		}
		else if ( m_parameters.min == Filtering::Linear && m_parameters.mag == Filtering::Point && m_parameters.mip == Filtering::Point )
		{
			colorMapDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		}
		else if ( m_parameters.min == Filtering::Linear && m_parameters.mag == Filtering::Point && m_parameters.mip == Filtering::Linear )
		{
			colorMapDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		}
		else if ( m_parameters.min == Filtering::Linear && m_parameters.mag == Filtering::Linear && m_parameters.mip == Filtering::Point )
		{
			colorMapDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		}
		else if ( m_parameters.min == Filtering::Linear && m_parameters.mag == Filtering::Linear && m_parameters.mip == Filtering::Linear )
		{
			colorMapDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		}
		else if ( m_parameters.min == Filtering::Anisotropic && m_parameters.mag == Filtering::Anisotropic && m_parameters.mip == Filtering::Anisotropic )
		{
			colorMapDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		}									
		else
		{
			throw unify::Exception( "Bad filtering combination!" );
		}

		colorMapDesc.MipLODBias = 0.0f;
		colorMapDesc.BorderColor[0] = 0;
		colorMapDesc.BorderColor[1] = 0;
		colorMapDesc.BorderColor[2] = 0;
		colorMapDesc.BorderColor[3] = 0;
		colorMapDesc.MinLOD = 0;
		colorMapDesc.MaxLOD = D3D11_FLOAT32_MAX;
		result = dxDevice->CreateSamplerState( &colorMapDesc, &m_colorMapSampler );
		assert( !FAILED( result ) );

		D3D11_SHADER_RESOURCE_VIEW_DESC textureResourceDesc{};
		textureResourceDesc.Format = textureDesc.Format;
		textureResourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		textureResourceDesc.Texture2D.MipLevels = 1;
		textureResourceDesc.Texture2D.MostDetailedMip = 0;

		result = dxDevice->CreateShaderResourceView( m_texture, &textureResourceDesc, &m_colorMap );
		assert( !FAILED( result ) );

		m_imageSize.width = width;
		m_imageSize.height = height;
	}

	bool Use( unsigned int stage )
	{
		auto dxContext = m_renderer->GetDxContext();
		dxContext->PSSetShaderResources( 0, 1, &m_colorMap.p );
		dxContext->PSSetSamplers( 0, 1, &m_colorMapSampler.p );
		return true;
	}

	void ClearHeader()
	{
		m_useColorKey = FALSE;
		m_colorKey = unify::Color::ColorZero();
		m_spriteMasterList.clear();
		m_spriteArrayMap.clear();
	}

	const unsigned int FileWidth() const
	{
		return m_fileSize.width;
	}

	const unsigned int FileHeight() const
	{
		return m_fileSize.height;
	}

	const unify::Size< unsigned int > & ImageSize() const
	{
		return m_imageSize;
	}

	bool GetRenderable() const
	{
		return m_parameters.renderable;
	}

	bool GetLockable() const
	{
		return m_parameters.lockable;
	}

	void Preload()
	{
		// Clear the header file
		ClearHeader();

		// Load any new image header file if there
		LoadHeader();

		// Attempt to load the image for information
		Create();

		// Destroy our texture.
		Destroy();
	}

	void LoadHeader()
	{
		unify::Path imageHeaderFilepath( m_parameters.source.ChangeExtension( ".xml" ) );

		// Check if the file exists...
		if ( !imageHeaderFilepath.Exists() )
		{
			return;	// It's ok to not have an IHF file.
		}

		qxml::Document document;
		document.Load( imageHeaderFilepath );

		for ( const auto sheet : document.GetRoot()->Children( "addsheet" ) )
		{
			std::string name = sheet.GetAttributeElse< std::string >( "name", "" );
			unify::TexCoords arrayUL( sheet.GetAttributeElse< float >( "ulu", 0 ), sheet.GetAttributeElse< float >( "ulv", 0 ) );
			unify::Size< float > size;
			unify::RowColumn< unsigned int > arrayRC{ qxml::AttributeCast< unify::RowColumn< unsigned int >, unsigned int >( sheet, "r", "c" ) };
			assert( arrayRC.row && arrayRC.column );

			if ( sheet.HasAttributes( "sizeu,sizev" ) )
			{
				size.width = sheet.GetAttribute( "sizeu" )->Get< float >();
				size.height = sheet.GetAttribute( "sizev" )->Get< float >();
			}
			else
			{
				size = unify::Size< float >( (1 - arrayUL.u) / arrayRC.column, (1 - arrayUL.v) / arrayRC.column );
			}

			if ( !name.empty() )
			{
				unsigned int head = static_cast< unsigned int >(m_spriteMasterList.size());
				m_spriteArrayMap[name] = SpriteArray( head, head + arrayRC.row * arrayRC.column );
			}

			unify::RowColumn< unsigned int > rc;
			for ( rc.row = 0; rc.row < arrayRC.row; ++rc.row )
			{
				for ( rc.column = 0; rc.column < arrayRC.column; ++rc.column )
				{
					unify::TexCoords ul( arrayUL.u + rc.column * size.width, arrayUL.v + rc.row * size.height );
					unify::TexCoords dr( ul + unify::TexCoords( size.width, size.height ) );
					m_spriteMasterList.push_back( unify::TexArea( ul, dr ) );
				}
			}
		}
	}

	bool HasSpriteArray( const std::string & name ) const
	{
		SpriteArrayMap::const_iterator itr = m_spriteArrayMap.find( name );
		return itr == m_spriteArrayMap.end() ? false : true;
	}

	const Texture::SpriteArray & FindSpriteArray( const std::string & name ) const
	{
		SpriteArrayMap::const_iterator itr = m_spriteArrayMap.find( name );
		if ( itr == m_spriteArrayMap.end() )
		{
			throw unify::Exception( "Sprite array not found!" );
		}
		return itr->second;
	}

	const Texture::SpriteArray & GetSpriteArray( unsigned int index ) const
	{
		assert( index < m_spriteArrayMap.size() );

		unsigned int currentIndex = 0;
		SpriteArrayMap::const_iterator itr = m_spriteArrayMap.begin();
		for ( itr; itr != m_spriteArrayMap.end() && currentIndex != index; ++itr, ++currentIndex )
		{
			// Empty.
		}
		return itr->second;
	}

	unsigned int SpriteArrayCount() const
	{
		return static_cast< unsigned int >(m_spriteArrayMap.size());
	}

	const unify::TexArea & GetSprite( unsigned int index ) const
	{
		assert( index < m_spriteMasterList.size() );
		return m_spriteMasterList[index];
	}

	const unify::TexArea & GetSprite( const std::string & arrayName, unsigned int index ) const
	{
		const SpriteArrayMap::const_iterator itr = m_spriteArrayMap.find( arrayName );
		if ( itr == m_spriteArrayMap.end() )
		{
			throw unify::Exception( "Sprite array not found!" );
		}

		assert( index < itr->second.tail );

		return m_spriteMasterList[itr->second.head + index];
	}

	unsigned int SpriteCount() const
	{
		return static_cast< unsigned int >(m_spriteMasterList.size());
	}
};


Texture::Texture( me::IRenderer * renderer, TextureParameters parameters )
	: m_pimpl( new Pimpl( *this, renderer, parameters ) )
{
	Create();
}

Texture::~Texture()
{
}

void Texture::Create()
{
	m_pimpl->Create();
}

// Destroyes data, keeps header intact - thus no graphical footprint, yet we can still use it's statistics/dimensions in calculations.
void Texture::Destroy()
{
	m_pimpl->Destroy();
}

void Texture::ClearHeader()
{
	m_pimpl->ClearHeader();
}

const unsigned int Texture::FileWidth() const
{
	return m_pimpl->FileWidth();
}

const unsigned int Texture::FileHeight() const
{
	return m_pimpl->FileHeight();
}

const unify::Size< unsigned int > & Texture::ImageSize() const
{
	return m_pimpl->ImageSize();
}

bool Texture::GetRenderable() const
{
	return m_pimpl->GetRenderable();
}

bool Texture::GetLockable() const
{
	return m_pimpl->GetLockable();
}

void Texture::LockRect( unsigned int level, TextureLock & lock, const unify::Rect< long > * rect, bool readonly )
{
	m_pimpl->LockRect( level, lock, rect, readonly );
}

void Texture::UnlockRect( unsigned int level )
{
	m_pimpl->UnlockRect( level );
}

bool Texture::Use( unsigned int stage )
{
	return m_pimpl->Use( stage );
}

// Load all possible info (short of bits) about the texture
void Texture::Preload()
{
	m_pimpl->Preload();
}

void Texture::LoadHeader()
{
	m_pimpl->LoadHeader();
}

// Load the actual image file...
void Texture::LoadImage( const unify::Path & filePath )
{
	m_pimpl->LoadImage( filePath );
}

bool Texture::HasSpriteArray( const std::string & name ) const
{
	return m_pimpl->HasSpriteArray( name );
}

const Texture::SpriteArray & Texture::FindSpriteArray( const std::string & name ) const
{
	return m_pimpl->FindSpriteArray( name );
}

const Texture::SpriteArray & Texture::GetSpriteArray( unsigned int index ) const
{
	return m_pimpl->GetSpriteArray( index );
}

unsigned int Texture::SpriteArrayCount() const
{
	return m_pimpl->SpriteArrayCount();
}

const unify::TexArea & Texture::GetSprite( unsigned int index ) const
{
	return m_pimpl->GetSprite( index );
}

const unify::TexArea & Texture::GetSprite( const std::string & arrayName, unsigned int index ) const
{
	return m_pimpl->GetSprite( arrayName, index );
}

unsigned int Texture::SpriteCount() const
{
	return m_pimpl->SpriteCount();
}								  