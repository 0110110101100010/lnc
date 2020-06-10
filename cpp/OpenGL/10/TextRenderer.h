#pragma once 

#include "Renderer.h"

class TextRenderer : public Renderer {
public:
	TextRenderer(uint32_t width, uint32_t height) : Renderer(width,height) {}
	virtual ~TextRenderer() {}
	virtual void render(const std::array<Vec2i,4>& tetrominoPos, const Vec3& currentColor, 
						const std::array<Vec2i,4>& nextTetrominoPos, const Vec3& nextColor,
						const std::array<Vec2i,4>& targerTetrominoPos,
						const std::vector<Vec3>& colorData, float time);
	
	virtual void clearRows(const std::vector<uint32_t>& ) {}
	virtual void actionCam(const std::array<Vec2i,4>& source, const std::array<Vec2i,4>& target) {}

private:
	static void setColor(const Vec3& color);
	
};
	
	
