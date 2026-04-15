// Copyright (c) 2025 Rafael Valoto. All Rights Reserved.
// Project: GamepadCore
// Description: Cross-platform library for DualSense and generic gamepad input support.
// Targets: Windows, Linux, macOS.
#pragma once
#include "GCore/Types/DSCoreTypes.h"
#include "GCore/Types/Structs/Context/DeviceContext.h"

namespace FDualSenseTriggerComposer
{
	/**
	 * Disables the trigger functionality for the specified hand or hands on the
	 * provided device context.
	 *
	 * @param Context A pointer to the device context which holds the trigger state
	 * to be modified.
	 * @param Hand An enumeration specifying which hand's trigger functionality to
	 * disable (Left, Right, or AnyHand).
	 */
	inline void Off(FDeviceContext* Context, const EDSGamepadHand& Hand)
	{
		if (Hand == EDSGamepadHand::Left || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.LeftTrigger.Mode = 0x0;
		}

		if (Hand == EDSGamepadHand::Right || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.RightTrigger.Mode = 0x0;
		}
	}

	/**
	 * Configures resistance for the trigger functionality on the specified hand or
	 * hands in the given device context.
	 *
	 * @param Context A pointer to the device context that holds the trigger
	 * settings to be modified.
	 * @param StartZones The starting position of the resistance zone for the
	 * trigger.
	 * @param Strength The level of resistance to be applied within the defined
	 * zone.
	 * @param Hand An enumeration specifying which hand's trigger functionality to
	 * configure (Left, Right, or AnyHand).
	 */
	inline void Resistance(FDeviceContext* Context, std::uint8_t StartZones,
	                       std::uint8_t Strength, const EDSGamepadHand& Hand)
	{
		if (Hand == EDSGamepadHand::Left || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.LeftTrigger.Mode = 0x01;
			Context->Output.LeftTrigger.Strengths.Compose[0] = StartZones;
			Context->Output.LeftTrigger.Strengths.Compose[1] = Strength;
		}
		if (Hand == EDSGamepadHand::Right || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.RightTrigger.Mode = 0x01;
			Context->Output.RightTrigger.Strengths.Compose[0] = StartZones;
			Context->Output.RightTrigger.Strengths.Compose[1] = Strength;
		}
	}

	/**
	 * Configures the trigger functionality on the specified hand or hands to
	 * emulate a GameCube-style resistance.
	 *
	 * @param Context A pointer to the device context that holds the trigger
	 * settings to be modified.
	 * @param Hand An enumeration specifying which hand's trigger functionality to
	 * configure (Left, Right, or AnyHand).
	 */
	inline void GameCube(FDeviceContext* Context, const EDSGamepadHand& Hand)
	{
		if (Hand == EDSGamepadHand::Left || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.LeftTrigger.Mode = 0x02;
			Context->Output.LeftTrigger.Strengths.Compose[0] = 0x90;
			Context->Output.LeftTrigger.Strengths.Compose[1] = 0x0a;
			Context->Output.LeftTrigger.Strengths.Compose[2] = 0xff;
		}

		if (Hand == EDSGamepadHand::Right || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.RightTrigger.Mode = 0x02;
			Context->Output.RightTrigger.Strengths.Compose[0] = 0x90;
			Context->Output.RightTrigger.Strengths.Compose[1] = 0x0a;
			Context->Output.RightTrigger.Strengths.Compose[2] = 0xff;
		}
	}

	inline void Bow22(FDeviceContext* Context, std::uint8_t StartZone,
	                  std::uint8_t SnapBack, const EDSGamepadHand& Hand)
	{
		if (Hand == EDSGamepadHand::Left || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.LeftTrigger.Mode = 0x22;
			Context->Output.LeftTrigger.Strengths.Compose[0] = StartZone;
			Context->Output.LeftTrigger.Strengths.Compose[1] = 0x01;
			Context->Output.LeftTrigger.Strengths.Compose[2] = SnapBack;
		}

		if (Hand == EDSGamepadHand::Right || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.RightTrigger.Mode = 0x22;
			Context->Output.RightTrigger.Strengths.Compose[0] = StartZone;
			Context->Output.RightTrigger.Strengths.Compose[1] = 0x01;
			Context->Output.RightTrigger.Strengths.Compose[2] = SnapBack;
		}
	}

	inline void Galloping23(FDeviceContext* Context, std::uint8_t StartPosition,
	                        std::uint8_t EndPosition, std::uint8_t FirstFoot,
	                        std::uint8_t SecondFoot, std::uint8_t Frequency,
	                        const EDSGamepadHand& Hand)
	{
		const std::uint8_t FirstFootNib = static_cast<std::uint8_t>(std::clamp(
		    static_cast<int>(std::lround((FirstFoot / 8) * 15)), 1, 15));
		const std::uint8_t SecondFootNib = static_cast<std::uint8_t>(std::clamp(
		    static_cast<int>(std::lround((SecondFoot / 8) * 15)), 1, 15));
		const std::uint16_t PositionMask = (1 << StartPosition) | (1 << EndPosition);
		if (Hand == EDSGamepadHand::Left || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.LeftTrigger.Mode = 0x23;
			Context->Output.LeftTrigger.Strengths.Compose[0] = PositionMask & 0xFF;
			Context->Output.LeftTrigger.Strengths.Compose[1] =
			    (PositionMask >> 8) & 0xFF;
			Context->Output.LeftTrigger.Strengths.Compose[2] =
			    ((FirstFootNib & 0x0F) << 4) | (SecondFootNib & 0x0F);
			Context->Output.LeftTrigger.Strengths.Compose[3] = Frequency;
		}

		if (Hand == EDSGamepadHand::Right || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.RightTrigger.Mode = 0x23;
			Context->Output.RightTrigger.Strengths.Compose[0] = PositionMask & 0xFF;
			Context->Output.RightTrigger.Strengths.Compose[1] =
			    (PositionMask >> 8) & 0xFF;
			Context->Output.RightTrigger.Strengths.Compose[2] =
			    ((FirstFootNib & 0x0F) << 4) | (SecondFootNib & 0x0F);
			Context->Output.RightTrigger.Strengths.Compose[3] = Frequency;
		}
	}

	// === OPENCHAOS-PATCH BEGIN: Weapon25 packing rewrite ===
	// Local patch by OpenChaos project. Original Weapon25 packed bytes as
	// `(StartZone << 4) | Amplitude` + Behavior + Trigger, which does NOT
	// match the reverse-engineered Sony Weapon (0x25) effect layout. The
	// real layout (per Nielk1 gist
	// https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db) is:
	//
	//   byte[0] = 0x25
	//   byte[1] = low  8 bits of startAndStopZones
	//   byte[2] = high 8 bits of startAndStopZones
	//   byte[3] = strength - 1
	//
	// where `startAndStopZones` is a 16-bit bitmap with bits set at the
	// `startPosition` and `endPosition` indices (zone indices 2..7 /
	// start+1..8 per the Limited_Weapon constraints; values outside still
	// encode but may not produce meaningful effects).
	//
	// The old signature parameters are reinterpreted as:
	//   StartZone -> startPosition (zone where the click zone begins)
	//   Amplitude -> endPosition   (zone where the click zone ends)
	//   Behavior  -> strength      (0..8, mapped to strength-1 per spec)
	//   Trigger   -> unused        (kept for ABI compat)
	//
	// See devlog `new_game_devlog/dualsense_lib_pr_notes.md` for rationale
	// and the PR plan to upstream this fix.
	inline void Weapon25(FDeviceContext* Context, std::uint8_t StartZone,
	                     std::uint8_t Amplitude, std::uint8_t Behavior,
	                     std::uint8_t /*Trigger*/, const EDSGamepadHand& Hand)
	{
		const std::uint8_t startPosition = StartZone;
		const std::uint8_t endPosition   = Amplitude;
		const std::uint8_t strength      = Behavior;

		const std::uint16_t startAndStopZones =
		    static_cast<std::uint16_t>((1u << startPosition) | (1u << endPosition));
		const std::uint8_t lo  = static_cast<std::uint8_t>(startAndStopZones & 0xff);
		const std::uint8_t hi  = static_cast<std::uint8_t>((startAndStopZones >> 8) & 0xff);
		const std::uint8_t str = static_cast<std::uint8_t>(strength > 0 ? (strength - 1) : 0);

		if (Hand == EDSGamepadHand::Left || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.LeftTrigger.Mode = 0x25;
			Context->Output.LeftTrigger.Strengths.Compose[0] = lo;
			Context->Output.LeftTrigger.Strengths.Compose[1] = hi;
			Context->Output.LeftTrigger.Strengths.Compose[2] = str;
		}

		if (Hand == EDSGamepadHand::Right || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.RightTrigger.Mode = 0x25;
			Context->Output.RightTrigger.Strengths.Compose[0] = lo;
			Context->Output.RightTrigger.Strengths.Compose[1] = hi;
			Context->Output.RightTrigger.Strengths.Compose[2] = str;
		}
	}
	// === OPENCHAOS-PATCH END ===

	inline void MachineGun26(FDeviceContext* Context, std::uint8_t StartZone,
	                         std::uint8_t Behavior, std::uint8_t Amplitude,
	                         std::uint8_t Frequency, const EDSGamepadHand& Hand)
	{
		if (Hand == EDSGamepadHand::Left || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.LeftTrigger.Mode = 0x26;
			Context->Output.LeftTrigger.Strengths.Compose[0] = StartZone;
			Context->Output.LeftTrigger.Strengths.Compose[1] =
			    Behavior > 0 ? 0x03 : 0x00;
			Context->Output.LeftTrigger.Strengths.Compose[2] = 0x00;
			Context->Output.LeftTrigger.Strengths.Compose[3] = 0x00;
			Context->Output.LeftTrigger.Strengths.Compose[4] =
			    Amplitude == 1 ? 0x8F : 0x8a;
			Context->Output.LeftTrigger.Strengths.Compose[5] =
			    Amplitude == 2 ? 0x3F : 0x1F;
			Context->Output.LeftTrigger.Strengths.Compose[9] = Frequency;
		}

		if (Hand == EDSGamepadHand::Right || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.RightTrigger.Mode = 0x26;
			Context->Output.RightTrigger.Strengths.Compose[0] = 0xf8;
			Context->Output.RightTrigger.Strengths.Compose[1] =
			    Behavior > 0 ? 0x03 : 0x00;
			Context->Output.RightTrigger.Strengths.Compose[2] = 0x00;
			Context->Output.RightTrigger.Strengths.Compose[3] = 0x00;
			Context->Output.RightTrigger.Strengths.Compose[4] =
			    Amplitude == 1 ? 0x8F : 0x8a;
			Context->Output.RightTrigger.Strengths.Compose[5] =
			    Amplitude == 2 ? 0x3F : 0x1F;
			Context->Output.RightTrigger.Strengths.Compose[9] = Frequency;
		}
	}

	inline void Machine27(FDeviceContext* Context, std::uint8_t StartZone,
	                      std::uint8_t BehaviorFlag, std::uint8_t Force,
	                      std::uint8_t Amplitude, std::uint8_t Period,
	                      std::uint8_t Frequency, const EDSGamepadHand& Hand)
	{
		if (Hand == EDSGamepadHand::Left || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.LeftTrigger.Mode = 0x27;
			Context->Output.LeftTrigger.Strengths.Compose[0] = StartZone;
			Context->Output.LeftTrigger.Strengths.Compose[1] =
			    BehaviorFlag > 0 ? 0x02 : 0x01;
			Context->Output.LeftTrigger.Strengths.Compose[2] =
			    Force << 4 | (Amplitude & 0x0F);
			;
			Context->Output.LeftTrigger.Strengths.Compose[3] = Period;
			Context->Output.LeftTrigger.Strengths.Compose[4] = Frequency;
		}

		if (Hand == EDSGamepadHand::Right || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.RightTrigger.Mode = 0x27;
			Context->Output.RightTrigger.Strengths.Compose[0] = StartZone;
			Context->Output.RightTrigger.Strengths.Compose[1] =
			    BehaviorFlag > 0 ? 0x02 : 0x00;
			Context->Output.RightTrigger.Strengths.Compose[2] =
			    Force << 4 | (Amplitude & 0x0F);
			Context->Output.RightTrigger.Strengths.Compose[3] = Period;
			Context->Output.RightTrigger.Strengths.Compose[4] = Frequency;
		}
	}

	inline void CustomTrigger(FDeviceContext* Context, const EDSGamepadHand& Hand,
	                          const std::vector<std::uint8_t>& HexBytes)
	{
		switch (HexBytes[0])
		{
			case 0x01:
			case 0x02:
			case 0x21:
			case 0x22:
			case 0x23:
			case 0x25:
			case 0x26:
			case 0x27:
				break;
			default:
				return;
		}

		if (Hand == EDSGamepadHand::Left || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.LeftTrigger.Mode = 0xFF;
			std::memcpy(Context->Output.LeftTrigger.Strengths.Compose, HexBytes.data(), 10);
		}

		if (Hand == EDSGamepadHand::Right || Hand == EDSGamepadHand::AnyHand)
		{
			Context->Output.RightTrigger.Mode = 0xFF;
			std::memcpy(Context->Output.RightTrigger.Strengths.Compose, HexBytes.data(), 10);
		}
	}

} // namespace FDualSenseTriggerComposer
