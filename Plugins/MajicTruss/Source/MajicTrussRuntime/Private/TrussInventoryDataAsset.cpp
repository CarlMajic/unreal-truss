#include "TrussInventoryDataAsset.h"

UTrussInventoryDataAsset::UTrussInventoryDataAsset()
{
	Pieces = {
		{ETrussPieceType::TenFoot, 304.8f, nullptr},
		{ETrussPieceType::EightFoot, 243.84f, nullptr},
		{ETrussPieceType::FiveFoot, 152.4f, nullptr},
		{ETrussPieceType::FourFoot, 121.92f, nullptr},
		{ETrussPieceType::TwoFoot, 60.96f, nullptr}
	};
}

bool UTrussInventoryDataAsset::FindPiece(ETrussPieceType PieceType, FTrussPieceDefinition& OutPiece) const
{
	for (const FTrussPieceDefinition& Piece : Pieces)
	{
		if (Piece.PieceType == PieceType)
		{
			OutPiece = Piece;
			return true;
		}
	}

	return false;
}

