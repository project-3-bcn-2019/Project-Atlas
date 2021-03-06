#include "ResourceAnimation.h"
#include "Application.h"
#include "Globals.h"
#include "ModuleTimeManager.h"
#include "Applog.h"

ResourceAnimation::ResourceAnimation(resource_deff deff):Resource(deff)
{
	Parent3dObject = deff.Parent3dObject;
}

ResourceAnimation::~ResourceAnimation()
{
}

void ResourceAnimation::LoadToMemory()
{
	LoadAnimation();
}

void ResourceAnimation::UnloadFromMemory()
{

}

bool ResourceAnimation::LoadAnimation()
{
	bool ret = false;

	//Get the buffer
	char* buffer = App->fs.ImportFile(binary.c_str());
	char* cursor = buffer;

	//Load ranges
	uint ranges[3];
	uint bytes = sizeof(ranges);
	memcpy(ranges, cursor, bytes);
	cursor += bytes;

	ticks = ranges[0];
	ticksXsecond = ranges[1];
	numBones = ranges[2];

	if (numBones > 0)
	{
		boneTransformations = new BoneTransform[numBones];

		//Loading Bones
		for (int i = 0; i < numBones; i++)
		{
			//Loading Ranges
			uint boneRanges[4];
			uint bytes = sizeof(boneRanges);
			memcpy(boneRanges, cursor, bytes);
			cursor += bytes;

			boneTransformations[i].numPosKeys = boneRanges[0];
			boneTransformations[i].numScaleKeys = boneRanges[1];
			boneTransformations[i].numRotKeys = boneRanges[2];

			//Loading Name
			bytes = sizeof(char)*boneRanges[3];
			char* auxName = new char[boneRanges[3]];
			memcpy(auxName, cursor, bytes);
			boneTransformations[i].NodeName = auxName;
			boneTransformations[i].NodeName = boneTransformations[i].NodeName.substr(0, bytes);
			RELEASE_ARRAY(auxName);
			cursor += bytes;

			//Loading Pos Time
			bytes = boneTransformations[i].numPosKeys * sizeof(double);
			boneTransformations[i].PosKeysTimes = new double[boneTransformations[i].numPosKeys];
			memcpy(boneTransformations[i].PosKeysTimes, cursor, bytes);
			cursor += bytes;
			//Loading Pos Values
			bytes = boneTransformations[i].numPosKeys * sizeof(float) * 3;
			boneTransformations[i].PosKeysValues = new float[boneTransformations[i].numPosKeys * 3];
			memcpy(boneTransformations[i].PosKeysValues, cursor, bytes);
			cursor += bytes;

			//Loading Scale Time
			bytes = boneTransformations[i].numScaleKeys * sizeof(double);
			boneTransformations[i].ScaleKeysTimes = new double[boneTransformations[i].numScaleKeys];
			memcpy(boneTransformations[i].ScaleKeysTimes, cursor, bytes);
			cursor += bytes;
			//Loading Scale Values
			bytes = boneTransformations[i].numScaleKeys * sizeof(float) * 3;
			boneTransformations[i].ScaleKeysValues = new float[boneTransformations[i].numScaleKeys * 3];
			memcpy(boneTransformations[i].ScaleKeysValues, cursor, bytes);
			cursor += bytes;

			//Loading Rotation Time
			bytes = boneTransformations[i].numRotKeys * sizeof(double);
			boneTransformations[i].RotKeysTimes = new double[boneTransformations[i].numRotKeys];
			memcpy(boneTransformations[i].RotKeysTimes, cursor, bytes);
			cursor += bytes;
			//Loading Rotation Values
			bytes = boneTransformations[i].numRotKeys * sizeof(float) * 4;
			boneTransformations[i].RotKeysValues = new float[boneTransformations[i].numRotKeys * 4];
			memcpy(boneTransformations[i].RotKeysValues, cursor, bytes);
			cursor += bytes;
		}

		ret = true;
	}

	RELEASE_ARRAY(buffer);

	return ret;
}

void ResourceAnimation::LoadDuration()
{
	//Get the buffer
	char* buffer = App->fs.ImportFile(binary.c_str());
	char* cursor = buffer;

	//Load ranges
	uint ranges[3];
	uint bytes = sizeof(ranges);
	memcpy(ranges, cursor, bytes);
	cursor += bytes;

	ticks = ranges[0];
	ticksXsecond = ranges[1];

	RELEASE_ARRAY(buffer);
}

void ResourceAnimation::resetFrames()
{
	for (int i = 0; i < numBones; i++)
	{
		boneTransformations[i].currentPosIndex = 0;
		boneTransformations[i].currentRotIndex = 0;
		boneTransformations[i].currentScaleIndex = 0;
	}
}

float ResourceAnimation::getDuration() const
{
	return ticks / ticksXsecond;
}

BoneTransform* ResourceAnimation::FindBone(std::string& check)
{
	BoneTransform* ret = nullptr;
	for (int i = 0; i < numBones && ret == nullptr; ++i)
		ret = (check.compare(boneTransformations[i].NodeName) == 0) ? &boneTransformations[i] : nullptr;

	return ret;
}


BoneTransform::~BoneTransform()
{
	RELEASE_ARRAY(PosKeysValues);
	RELEASE_ARRAY(PosKeysTimes);

	RELEASE_ARRAY(ScaleKeysValues);
	RELEASE_ARRAY(ScaleKeysTimes);

	RELEASE_ARRAY(RotKeysValues);
	RELEASE_ARRAY(RotKeysTimes);
}

bool BoneTransform::calcCurrentIndex(float time, bool test)
{
	bool ret = false;

	if ((App->time->getGameState() != GameState::PLAYING && !test) || currentPosIndex == -1 || currentRotIndex == -1 || currentScaleIndex == -1 ||
		nextPosIndex == -1 || nextRotIndex == -1 || nextScaleIndex == -1)
	{
		currentPosIndex = currentRotIndex = currentScaleIndex = 0;
		nextPosIndex = nextRotIndex = nextScaleIndex = 1;
		return true;
	}

	for (int i = 0; i < numPosKeys; i++)
	{
		if (PosKeysTimes[i] <= time && (i + 1 >= numPosKeys || PosKeysTimes[i + 1] > time))
		{
			currentPosIndex = i;

			nextPosIndex = currentPosIndex + 1;

			if (nextPosIndex >= numPosKeys && numPosKeys > 1)
			{
				nextPosIndex = 0;
			}


			ret = true;
			break;
		}
	}
	for (int i = 0; i < numRotKeys; i++)
	{
		if (RotKeysTimes[i] <= time && (i + 1 >= numRotKeys || RotKeysTimes[i + 1] > time))
		{
			currentRotIndex = i;

			nextRotIndex = currentRotIndex + 1;

			if (nextRotIndex >= numRotKeys && numRotKeys > 1)
			{
				nextRotIndex = 0;
			}

			ret = true;
			break;
		}
	}
	for (int i = 0; i < numScaleKeys; i++)
	{
		if (ScaleKeysTimes[i] <= time && (i + 1 >= numScaleKeys || ScaleKeysTimes[i + 1] > time))
		{
			currentScaleIndex = i;

			nextScaleIndex = currentScaleIndex + 1;

			if (nextScaleIndex >= numScaleKeys && numScaleKeys > 1)
			{
				nextScaleIndex = 0;
			}

			ret = true;
			break;
		}
	}

	return ret;
}

void BoneTransform::calcTransfrom(float time, bool interpolation, float duration, int tickxs)
{
	float tp, ts, tr;

	tp = ts = tr = 0.0f;

	float3 position_1 = { PosKeysValues[currentPosIndex * 3], PosKeysValues[currentPosIndex * 3 + 1], PosKeysValues[currentPosIndex * 3 + 2] };
	Quat rotation_1 = { RotKeysValues[currentRotIndex * 4], RotKeysValues[currentRotIndex * 4 + 1], RotKeysValues[currentRotIndex * 4 + 2], RotKeysValues[currentRotIndex * 4 + 3] };
	float3 scale_1 = { ScaleKeysValues[currentScaleIndex * 3], ScaleKeysValues[currentScaleIndex * 3 + 1], ScaleKeysValues[currentScaleIndex * 3 + 2] };

	if (!interpolation)
	{
		lastTransform.Set(float4x4::FromTRS(position_1, rotation_1, scale_1));
		return;
	}

	float3 position_2 = (numPosKeys > 1) ?  float3(PosKeysValues[nextPosIndex * 3], PosKeysValues[nextPosIndex * 3 + 1], PosKeysValues[nextPosIndex * 3 + 2]) : position_1;
	Quat rotation_2 = (numRotKeys > 1) ? Quat(RotKeysValues[nextRotIndex * 4], RotKeysValues[nextRotIndex * 4 + 1], RotKeysValues[nextRotIndex * 4 + 2], RotKeysValues[nextRotIndex * 4 + 3]) : rotation_1;
	float3 scale_2 = (numScaleKeys > 1) ? float3(ScaleKeysValues[nextScaleIndex * 3], ScaleKeysValues[nextScaleIndex * 3 + 1], ScaleKeysValues[nextScaleIndex * 3 + 2]) : scale_1;

	float nextpostime = (numPosKeys > 1) ? PosKeysTimes[nextPosIndex] : PosKeysTimes[currentPosIndex];
	float nextrottime = (numRotKeys > 1) ? RotKeysTimes[nextRotIndex] : RotKeysTimes[currentRotIndex];
	float nextscaletime = (numScaleKeys > 1) ? ScaleKeysTimes[nextScaleIndex] : ScaleKeysTimes[currentScaleIndex];

	tp = ((time - PosKeysTimes[currentPosIndex]) / (nextpostime - PosKeysTimes[currentPosIndex]));
	tr = ((time - RotKeysTimes[currentRotIndex]) / (nextrottime - RotKeysTimes[currentRotIndex]));
	ts = ((time - ScaleKeysTimes[currentScaleIndex]) / (nextscaletime - ScaleKeysTimes[currentScaleIndex]));

	if (tp < 0) tp = ((time - PosKeysTimes[currentPosIndex]) / (nextpostime - PosKeysTimes[currentPosIndex] + duration));
	if (tr < 0) tr = ((time - RotKeysTimes[currentRotIndex]) / (nextrottime - RotKeysTimes[currentRotIndex] + duration));
	if (ts < 0) ts = ((time - ScaleKeysTimes[currentScaleIndex]) / (nextscaletime - ScaleKeysTimes[currentScaleIndex] + duration));

	if(nextPosIndex < numPosKeys)
		position_1 = position_1.Lerp(position_2, tp);
	if (nextRotIndex < numRotKeys)
		rotation_1 = rotation_1.Slerp(rotation_2, tr);
	if (nextScaleIndex < numScaleKeys)
		scale_1 = scale_1.Lerp(scale_2, ts);

	lastTransform.Set(float4x4::FromTRS(position_1, rotation_1, scale_1));
}

void BoneTransform::smoothBlending(const float4x4& blendtrans, float time)
{
	float3 position_1;
	Quat rotation_1;
	float3 scale_1;

	lastTransform.Decompose(position_1, rotation_1, scale_1);

	float3 position_2;
	Quat rotation_2;
	float3 scale_2;

	blendtrans.Decompose(position_2, rotation_2, scale_2);

	time = (time > 1) ? 1 : time;
	time = (time < 0) ? 0 : time;

	float3 finalpos = position_1.Lerp(position_2, (1 -time));
	Quat finalrot = rotation_1.Slerp(rotation_2, (1 - time));
	float3 finalscale = scale_1.Lerp(scale_2, (1 - time));

	lastTransform.Set(float4x4::FromTRS(finalpos, finalrot, finalscale));
}
