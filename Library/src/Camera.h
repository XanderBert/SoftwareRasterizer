#pragma once
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"
#include "DataTypes.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{}

		
		
		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};


		float nearPlane{ 1.f };
		float farPlane{ 1000.f };

		
		void Initialize(float aspectRatio, float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);
			origin = _origin;

			CalculateViewMatrix();
			CalculateProjectionMatrix(aspectRatio);
		}


		static bool IsOutsideFrustum(const Vector4& vector)
		{			
			return vector.x < -1.f || vector.x > 1.f
				|| vector.y < -1.f || vector.y > 1.f
				|| vector.z < -1.f || vector.z > 1.f;
		}
		
		void CalculateViewMatrix()
		{
			//ONB => invViewMatrix
			//Inverse(ONB) => ViewMatrix

			//TODO -> Calculate ViewMatrix in Matrix class
			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
			
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right);

			invViewMatrix = Matrix
			{
				right,
				up,
				forward,
				origin
			};

			viewMatrix = invViewMatrix.Inverse();
		}

		void CalculateProjectionMatrix(const float aspectRatio)
		{
			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio,nearPlane,farPlane);	
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
						const float deltaTime = pTimer->GetElapsed();

			// Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			// Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			// Speed and limit constants
			constexpr float keyboardMovementSpeed{ 10.0f };
			constexpr float mouseMovementSpeed{ 2.0f };
			constexpr float angularSpeed{ 50.0f * TO_RADIANS };

			// The total movement of this frame
			Vector3 direction{};

			// Calculate new position with keyboard inputs
			direction += (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_Z]) * forward * keyboardMovementSpeed * deltaTime;
			direction -= pKeyboardState[SDL_SCANCODE_S] * forward * keyboardMovementSpeed * deltaTime;
			direction -= (pKeyboardState[SDL_SCANCODE_Q] || pKeyboardState[SDL_SCANCODE_A]) * right * keyboardMovementSpeed * deltaTime;
			direction += pKeyboardState[SDL_SCANCODE_D] * right * keyboardMovementSpeed * deltaTime;

			// Calculate new position and rotation with mouse inputs
			switch (mouseState)
			{
			case SDL_BUTTON_LMASK: // LEFT CLICK
				direction -= forward * (mouseY * mouseMovementSpeed * deltaTime);
				totalYaw += mouseX * angularSpeed * deltaTime;
				break;
			case SDL_BUTTON_RMASK: // RIGHT CLICK
				totalYaw += mouseX * angularSpeed * deltaTime;
				totalPitch -= mouseY * angularSpeed * deltaTime;
				break;
			case SDL_BUTTON_X2: // BOTH CLICK
				direction.y -= mouseY * mouseMovementSpeed * deltaTime;
				break;
			}
			totalPitch = std::clamp(totalPitch, -89.f * TO_RADIANS, 89.f * TO_RADIANS);


			//Movement calculations
			constexpr float shiftSpeed{ 4.0f };
			direction *= 1.0f + pKeyboardState[SDL_SCANCODE_LSHIFT] * (shiftSpeed - 1.0f);

			origin += direction;

			// Calculate the rotation matrix with the new pitch and yaw
			const Matrix rotationMatrix = Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw);

			// Calculate the new forward vector with the new pitch and yaw
			forward = rotationMatrix.TransformVector(Vector3::UnitZ);

			//Update Matrices
			CalculateViewMatrix();
			//CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
