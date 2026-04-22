#include "TrussMathLibrary.h"

float UTrussMathLibrary::FeetToCentimeters(float Feet)
{
	return Feet * 30.48f;
}

float UTrussMathLibrary::CentimetersToFeet(float Centimeters)
{
	return Centimeters / 30.48f;
}

FTrussCombinationResult UTrussMathLibrary::FindBestTrussCombination(float TargetLengthCm, float ToleranceCm)
{
	FTrussCombinationResult Result;

	const TArray<ETrussPieceType> Fillers = {
		ETrussPieceType::EightFoot,
		ETrussPieceType::FiveFoot,
		ETrussPieceType::FourFoot,
		ETrussPieceType::TwoFoot
	};

	const TArray<ETrussPieceType> PieceOrder = {
		ETrussPieceType::TenFoot,
		ETrussPieceType::EightFoot,
		ETrussPieceType::FiveFoot,
		ETrussPieceType::FourFoot,
		ETrussPieceType::TwoFoot
	};

	const float TenFootCm = GetDefaultPieceLengthCm(ETrussPieceType::TenFoot);
	const int32 MaxTens = FMath::FloorToInt(TargetLengthCm / FMath::Max(TenFootCm - ToleranceCm, 0.001f));

	for (int32 NumTens = MaxTens; NumTens >= 0; --NumTens)
	{
		const float Remainder = TargetLengthCm - (NumTens * TenFootCm);
		if (Remainder < -ToleranceCm)
		{
			continue;
		}

		TArray<ETrussPieceType> Exact;
		if (FindExact(Remainder, Fillers, ToleranceCm, Exact))
		{
			for (int32 Index = 0; Index < NumTens; ++Index)
			{
				Result.Pieces.Add(ETrussPieceType::TenFoot);
				Result.ActualLengthCm += TenFootCm;
			}

			for (ETrussPieceType PieceType : Exact)
			{
				Result.Pieces.Add(PieceType);
				Result.ActualLengthCm += GetDefaultPieceLengthCm(PieceType);
			}

			return Result;
		}
	}

	float RemainingCm = TargetLengthCm;
	for (ETrussPieceType PieceType : PieceOrder)
	{
		const float PieceLengthCm = GetDefaultPieceLengthCm(PieceType);
		while (RemainingCm >= PieceLengthCm - ToleranceCm)
		{
			Result.Pieces.Add(PieceType);
			Result.ActualLengthCm += PieceLengthCm;
			RemainingCm -= PieceLengthCm;
		}
	}

	return Result;
}

FString UTrussMathLibrary::PieceTypeToLabel(ETrussPieceType PieceType)
{
	switch (PieceType)
	{
	case ETrussPieceType::TenFoot:
		return TEXT("10 ft Truss");
	case ETrussPieceType::EightFoot:
		return TEXT("8 ft Truss");
	case ETrussPieceType::FiveFoot:
		return TEXT("5 ft Truss");
	case ETrussPieceType::FourFoot:
		return TEXT("4 ft Truss");
	case ETrussPieceType::TwoFoot:
		return TEXT("2 ft Truss");
	case ETrussPieceType::CornerBlock:
		return TEXT("Corner Block");
	case ETrussPieceType::Base:
		return TEXT("Base");
	case ETrussPieceType::Hinge:
		return TEXT("Hinge");
	default:
		return TEXT("Unknown");
	}
}

bool UTrussMathLibrary::FindExact(float TargetLengthCm, const TArray<ETrussPieceType>& CandidatePieces, float ToleranceCm, TArray<ETrussPieceType>& OutPieces)
{
	if (FMath::Abs(TargetLengthCm) <= ToleranceCm)
	{
		OutPieces.Reset();
		return true;
	}

	for (ETrussPieceType PieceType : CandidatePieces)
	{
		const float PieceLengthCm = GetDefaultPieceLengthCm(PieceType);
		if (PieceLengthCm <= TargetLengthCm + ToleranceCm)
		{
			TArray<ETrussPieceType> RemainderPieces;
			if (FindExact(TargetLengthCm - PieceLengthCm, CandidatePieces, ToleranceCm, RemainderPieces))
			{
				OutPieces.Reset();
				OutPieces.Add(PieceType);
				OutPieces.Append(RemainderPieces);
				return true;
			}
		}
	}

	return false;
}

float UTrussMathLibrary::GetDefaultPieceLengthCm(ETrussPieceType PieceType)
{
	switch (PieceType)
	{
	case ETrussPieceType::TenFoot:
		return 304.8f;
	case ETrussPieceType::EightFoot:
		return 243.84f;
	case ETrussPieceType::FiveFoot:
		return 152.4f;
	case ETrussPieceType::FourFoot:
		return 121.92f;
	case ETrussPieceType::TwoFoot:
		return 60.96f;
	default:
		return 0.0f;
	}
}

