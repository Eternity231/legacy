#pragma once
#include "includes.h"

enum BoneSetupFlags {
	None = 0,
	UseInterpolatedOrigin = (1 << 0),
	UseCustomOutput = (1 << 1),
	ForceInvalidateBoneCache = (1 << 2),
	AttachmentHelper = (1 << 3),
};

class Bones {

public:
	bool m_running;

public:
	bool setup(Player* player, BoneArray* out, IWSRecord* record);
	bool BuildBones(Player* target, int mask, BoneArray* out, IWSRecord* record);

	bool SetupBones(Player* entity, BoneArray* pBoneMatrix, int nBoneCount, int boneMask, float time, int flags);
	bool SetupBonesRebuild(Player* entity, BoneArray* pBoneMatrix, int nBoneCount, int boneMask, float time, int flags);
};

extern Bones g_bones;
