// Copyright (c) 2003 - 2014, Evil Quail LLC
// All Rights Reserved

#include <meogl/WindowsOS.h>
#include <memory.h>
#include <me/IGame.h>

using namespace meogl;

void Deleter( WindowsOS * factory )
{
	delete factory;
}

extern "C" __declspec(dllexport) bool DXILoader( me::IGame * game, const qxml::Document * doc );

__declspec(dllexport) bool DXILoader( me::IGame * game, const qxml::Document * doc )
{
	game->SetOS( me::IOS::ptr( new WindowsOS( game ) ) );

	return true;
}

