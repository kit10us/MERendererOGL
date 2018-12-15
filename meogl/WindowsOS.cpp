// Copyright (c) 2003 - 2013, Evil Quail LLC
// All Rights Reserved

#pragma once

#include <meogl/WindowsOS.h>
#include <meogl/Renderer.h>
#include <unify/Exception.h>
#include <unify/Path.h>
#include <me/exception/FailedToCreate.h>
#include <shellapi.h>

#include <meogl/OpenGL.h>

using namespace me;
using namespace meogl;

#ifdef GetCommandLine
#undef GetCommandLine
#endif

class WindowsOS::Pimpl
{
public:
	Pimpl( IGame * game )
		: m_game( game )
		, m_hasFocus{}
		, m_keyboard{}
		, m_mouse{}
	{
	}

	~Pimpl()
	{
	}

	IGame * m_game;

	std::string m_name;

	std::vector< std::string > m_commandLine;

	IInputSource * m_keyboard;
	IInputSource * m_mouse;

	bool m_hasFocus;
	std::list< HWND > m_childHandles; // Handles to be serviced.
	std::vector< Display > m_pendingDisplays;
	std::vector< IRenderer::ptr > m_renderers;
};


WindowsOS::WindowsOS( IGame * game )
: m_pimpl( new Pimpl( game ) )
{
	m_osParameters = game->GetOSParameters();
	/*
	if ( m_osParameters.hWnd == nullptr )
	{
		m_osParameters.hWnd = GetModuleHandle( 0 );
	}
	*/

	{
		using namespace std;
		char buffer[MAX_PATH];
		GetModuleFileNameA( NULL, buffer, MAX_PATH );
		string::size_type pos = string( buffer ).find_last_of( "\\/" );
		if( pos != string::npos ) 
		{
			m_pimpl->m_name = buffer;
		}
	}
		
	// TODO: We are also doing this in Game, shouldn't this be in once place?
	// Parse the commandline...
	std::string commandLineString( m_osParameters.lpszCmdLine );
	size_t l = 0;
	size_t r = 0;
	bool inQuote = false;
	std::string working;

	for( size_t l = 0, r = 0; r <= commandLineString.length(); ++r )
	{
		if ( ! inQuote && ( r == commandLineString.length()  || commandLineString.at( r ) == ' ' ) )
		{
			if ( l != r )
			{
 				working += commandLineString.substr( l, r - l );
			}
			if ( working.empty() == false )
			{
				m_pimpl->m_commandLine.push_back( working );
				working.clear();
			}
			l = r + 1;
		}
		else if ( commandLineString.at( r ) == '\"' )
		{
			// Include partial string...
			working += commandLineString.substr( l, r - l );
			l = r + 1; // One past the double quote.
			inQuote = !inQuote;
		}
	}
}

WindowsOS::~WindowsOS()
{
	m_pimpl->m_renderers.clear();
}

void * WindowsOS::Feed( std::string target, void * data )
{
	os::win::OSFood * food = (os::win::OSFood*)data;

	return (void*)WndProc( (HWND)food->handle, (UINT)food->message, (WPARAM)food->wParam, (LPARAM)food->lParam );
}

std::string WindowsOS::GetName() const
{
	return m_pimpl->m_name;
}

const std::vector< std::string > & WindowsOS::GetCommandLine() const
{
	return m_pimpl->m_commandLine;
}

void WindowsOS::AddDisplay( Display display )
{
	m_pimpl->m_pendingDisplays.push_back( display );
}

void WindowsOS::CreatePendingDisplays()
{
	if( m_pimpl->m_pendingDisplays.empty() )
	{
		return;
	}

	for( auto && display : m_pimpl->m_pendingDisplays )
	{
		CreateDisplay( display );
	}

	m_pimpl->m_pendingDisplays.clear();
}

void WindowsOS::CreateDisplay( Display display )
{
	bool isPrimary = m_pimpl->m_renderers.empty() ? true : false; // Note that this is VERY explicit - we are actual spelling out our intention (even though it looks redundant).

	std::shared_ptr< Renderer > renderer;

	// If we have no handle, create a window...
	if ( display.GetHandle() == 0 )
	{
		// Regardless of windowed or not, we need a window...
		WNDCLASS wc;
		memset( &wc, 0, sizeof( wc ) );
		wc.style = 0;
		wc.lpfnWndProc = (WNDPROC)m_osParameters.wndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetHInstance();
		wc.hIcon = LoadIcon( (HINSTANCE)NULL, IDI_APPLICATION );
		wc.hCursor = LoadCursor( (HINSTANCE)NULL, IDC_ARROW );

		if( isPrimary )
		{
			wc.lpszMenuName = L"MainMenu";
			wc.lpszClassName = L"MainWndClass";
		}
		else		    // TODO: This worked, yet is dodgy...
		{
			wc.lpszMenuName = 0;
			wc.lpszClassName = L"SecondWndClass";
		}

		if( !RegisterClass( &wc ) )
		{
			throw std::exception( "Failed to register window class!" );
		}

		int x = static_cast< int >(display.GetPosition().x);
		int y = static_cast< int >(display.GetPosition().y);
		int width = static_cast< int >(display.GetSize().width);
		int height = static_cast< int >(display.GetSize().height);
		HWND parentHandle = (HWND)display.GetParentHandle();
		HINSTANCE hInstance = GetHInstance();
		HWND handle = CreateWindowA( "MainWndClass", "An Empty DirectX Project", WS_OVERLAPPEDWINDOW, x, y, width, height,
			parentHandle, (HMENU)NULL, hInstance, (LPVOID)NULL );

		if( !handle )
		{
			throw exception::FailedToCreate( "Failed to create window!" );
		}

		{ // Resize window to ensure exact pixel match...
			RECT windowRect;
			GetWindowRect( handle, &windowRect );

			RECT clientRect;
			GetClientRect( handle, &clientRect );

			long newWindowWidth = (windowRect.right - windowRect.left) + width - clientRect.right;
			long newWindowHeight = (windowRect.bottom - windowRect.top) + height - clientRect.bottom;
			MoveWindow( handle, windowRect.left, windowRect.top, newWindowWidth, newWindowHeight, false );
		}

		ShowWindow( handle, m_osParameters.nCmdShow );
		UpdateWindow( handle );
		display.SetHandle( handle );
	}

	renderer.reset( new Renderer( this, display, m_pimpl->m_renderers.size() ) );
	m_pimpl->m_renderers.push_back( renderer );
}

int WindowsOS::RendererCount() const
{
	return m_pimpl->m_renderers.size();
}

me::IRenderer * WindowsOS::GetRenderer( int index) const
{
	return m_pimpl->m_renderers[ index ].get();
}

void WindowsOS::SetHasFocus( bool hasFocus )
{
	m_pimpl->m_hasFocus = hasFocus;
}

bool WindowsOS::GetHasFocus() const
{
	return m_pimpl->m_hasFocus;
}

HINSTANCE WindowsOS::GetHInstance() const
{
	return (HINSTANCE)m_osParameters.hInstance;
}

HWND WindowsOS::GetHandle() const
{
	if ( m_osParameters.hWnd )
	{
		return (HWND)m_osParameters.hWnd;
	}
	else
	{
		return (HWND)m_pimpl->m_renderers[0]->GetDisplay().GetHandle();
	}
}

void WindowsOS::BuildRenderers()
{		 
	CreatePendingDisplays();

	// TODO: DragAcceptFiles( this->GetHWnd(), true );
}

void WindowsOS::Startup()
{
	auto keyboardItr = m_pimpl->m_game->GetInputManager()->Find( "Keyboard" );
	if ( keyboardItr )
	{
		m_pimpl->m_keyboard = keyboardItr.get();
	}

	auto mouseItr = m_pimpl->m_game->GetInputManager()->Find( "Mouse" );
	if ( mouseItr )
	{
		m_pimpl->m_mouse = mouseItr.get();
	}
}

void WindowsOS::Shutdown()
{
}

void WindowsOS::DebugWrite( const std::string & text )
{
	OutputDebugStringA( text.c_str() );
}

void WindowsOS::DebugWriteLine( const std::string & line )
{
	DebugWrite( line + "\n" );
}

LRESULT WindowsOS::WndProc( HWND handle, UINT message, WPARAM wParam, LPARAM lParam )
{
	IGame & game = *m_pimpl->m_game;
	static bool trackingMouse = false;

	switch ( message )
	{
	case WM_CLOSE: // Fall through to WM_DESTROY...
	case WM_DESTROY:
		game.Quit();
		return 0;

	case WM_MOUSELEAVE:
	{
		if ( m_pimpl->m_mouse == nullptr ) break;

		for( int renderer = 0; renderer < RendererCount(); ++renderer )
		{
			if ( GetRenderer( renderer )->GetHandle() == handle )
			{
				trackingMouse = false;
				m_pimpl->m_mouse->SetState( renderer, "MouseAvailable", "Available", false );
				break;
			}
		}
	}
	break;

	case WM_LBUTTONDOWN:
	{
		if ( m_pimpl->m_mouse == nullptr ) break;

		for ( int renderer = 0; renderer < RendererCount(); ++renderer )
		{
			if ( GetRenderer( renderer )->GetHandle() == handle )
			{
				trackingMouse = false;
				m_pimpl->m_mouse->SetState( renderer, "LeftButton", "Down", true );
				break;
			}
		}
	}
	break;

	case WM_LBUTTONUP:
	{
		if ( m_pimpl->m_mouse == nullptr ) break;

		for ( int renderer = 0; renderer < RendererCount(); ++renderer )
		{
			if ( GetRenderer( renderer )->GetHandle() == handle )
			{
				trackingMouse = false;
				m_pimpl->m_mouse->SetState( renderer, "LeftButton", "Down", false );
				break;
			}
		}
	}
	break;

	case WM_RBUTTONDOWN:
	{
		if ( m_pimpl->m_mouse == nullptr ) break;

		for ( int renderer = 0; renderer < RendererCount(); ++renderer )
		{
			if ( GetRenderer( renderer )->GetHandle() == handle )
			{
				trackingMouse = false;
				m_pimpl->m_mouse->SetState( renderer, "RightButton", "Down", true );
				break;
			}
		}
	}
	break;

	case WM_RBUTTONUP:
	{
		if ( m_pimpl->m_mouse == nullptr ) break;

		for ( int renderer = 0; renderer < RendererCount(); ++renderer )
		{
			if ( GetRenderer( renderer )->GetHandle() == handle )
			{
				trackingMouse = false;
				m_pimpl->m_mouse->SetState( renderer, "RightButton", "Down", false );
				break;
			}
		}
	}
	break;

	case WM_MOUSEWHEEL:
	{
		if ( m_pimpl->m_mouse == nullptr ) break;

		short zDelta = GET_WHEEL_DELTA_WPARAM( wParam );
		for ( int renderer = 0; renderer < RendererCount(); ++renderer )
		{
			if ( GetRenderer( renderer )->GetHandle() == handle )
			{
				trackingMouse = false;
				m_pimpl->m_mouse->SetValue( renderer, "MouseWheel", zDelta / (float)WHEEL_DELTA );
				break;
			}
		}
	}
	break;

	case WM_MOUSEMOVE:
	{
		if ( m_pimpl->m_mouse == nullptr ) break;

		// TODO:
		// Enable tracking when the mouse leaves the client area...
		if ( !trackingMouse )
		{
			TRACKMOUSEEVENT trackMouseEvent = TRACKMOUSEEVENT();
			trackMouseEvent.cbSize = sizeof TRACKMOUSEEVENT;
			trackMouseEvent.dwFlags = TME_LEAVE;
			trackMouseEvent.hwndTrack = handle;
			trackMouseEvent.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent( &trackMouseEvent );
		}


		RECT clientRect;
		GetClientRect( handle, &clientRect );

		for ( int renderer = 0; renderer < RendererCount(); ++renderer )
		{
			if ( GetRenderer( renderer )->GetHandle() == handle )
			{
				trackingMouse = false;				
								
				float width = static_cast<float>(GetRenderer( renderer )->GetViewport().GetSize().width);
				float height = static_cast< float >(GetRenderer( renderer )->GetViewport().GetSize().height);
				float clientWidth = static_cast< float >(clientRect.right);
				float clientHeight = static_cast< float >(clientRect.bottom);

				unify::V2< int > mousePosition( static_cast< int >(LOWORD( lParam )), static_cast< int >(HIWORD( lParam )) );
				mousePosition.x *= static_cast< int >(width / clientWidth);
				mousePosition.y *= static_cast< int >(height / clientHeight);

				m_pimpl->m_mouse->SetValue( renderer, "PositionX", (float)mousePosition.x );
				m_pimpl->m_mouse->SetValue( renderer, "PositionY", (float)mousePosition.y );
				break;
			}
		}
	}
	break;

	case WM_ACTIVATE:
	{
		WORD lowOrder = wParam & 0x0000FFFF;
		bool minimized = (wParam & 0xFFFF0000) != 0;
		switch ( lowOrder )
		{
		case WA_ACTIVE:
		case WA_CLICKACTIVE:
			SetHasFocus( true );
			break;
		case WA_INACTIVE:
			SetHasFocus( false );
			break;

		default:
			assert( 0 && "Invalid activity state!" );
		}
	}
	break;
	/*

	case WM_DROPFILES:
	{
		HDROP drop = reinterpret_cast< HDROP >(wParam);

		std::vector< unify::Path > files;
		unify::V2< float > point;

		size_t numberOfFiles = DragQueryFile( drop, 0xFFFFFFFF, 0, 0 );
		for ( size_t file = 0; file < numberOfFiles; ++file )
		{
			char filePath[260];
			DragQueryFileA( drop, file, filePath, 260 );
			files.push_back( filePath );
		}

		POINT pt;
		DragQueryPoint( drop, &pt );
		point.x = static_cast< float >(pt.x);
		point.y = static_cast< float >(pt.y);

		game.OnDragDrop( files, point );
	}
	break;
	*/
	}

	return DefWindowProc( handle, message, wParam, lParam );
}