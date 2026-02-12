// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#define ENGINE_UPGRADE_NOTICE(MinorVersion, Message) static_assert(!(ENGINE_MINOR_VERSION >= MinorVersion), Message);
