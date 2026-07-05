#pragma once
// Play field on the XY plane (top-down). X = scoring axis (left/right), Y = paddle axis (up/down).
namespace PongField
{
	static constexpr float HalfWidth  = 700.f;  // X extent (goals at +/- HalfWidth)
	static constexpr float HalfHeight = 400.f;  // Y extent (walls at +/- HalfHeight)
	static constexpr float PaddleX    = 640.f;  // paddle distance from centre along X
	static constexpr float PaddleHalfLen = 90.f; // paddle half-size along Y
	static constexpr float BallRadius = 20.f;
	static constexpr float BallSpeed  = 900.f;
}
