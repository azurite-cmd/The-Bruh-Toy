#pragma once
#include <memory>
#include "common/String.h"
#include "common/Vec2.h"
#include "graphics/Pixel.h"
class VideoBuffer;

struct wall_type
{
	pixel colour;
	pixel eglow; // if emap set, add this to fire glow
	int drawstyle;
	std::unique_ptr<VideoBuffer> (*textureGen)(int, Vec2<int>);
	String name;
	ByteString identifier;
	String descs;
};
