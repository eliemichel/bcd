// This file is part of the reference implementation for the paper 
//   Bayesian Collaborative Denoising for Monte-Carlo Rendering
//   Malik Boughida and Tamy Boubekeur.
//   Computer Graphics Forum (Proc. EGSR 2017), vol. 36, no. 4, p. 137-153, 2017
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.txt file.

#include "Utils.h"

#include "DeepImage.h"

#include <cstring>
#include <cassert>

using namespace std;

namespace bcd
{

	bool Utils::separateNbOfSamplesFromHistogram(
			Deepimf& o_rHistoImage,
			Deepimf& o_rNbOfSamplesImage,
			const Deepimf& i_rHistoAndNbOfSamplesImage)
	{
		int w = i_rHistoAndNbOfSamplesImage.getWidth();
		int h = i_rHistoAndNbOfSamplesImage.getHeight();
		int d = i_rHistoAndNbOfSamplesImage.getDepth() - 1;

		o_rHistoImage.resize(w, h, d);
		o_rNbOfSamplesImage.resize(w, h, 1);

		size_t histoDataSize = d * sizeof(float);
		ImfIt histoIt = o_rHistoImage.begin();
		ImfIt nbOfSamplesIt = o_rNbOfSamplesImage.begin();
		ImfConstIt histoAndNbOfSamplesIt = i_rHistoAndNbOfSamplesImage.begin();
		ImfConstIt histoAndNbOfSamplesItEnd = i_rHistoAndNbOfSamplesImage.end();

		for( ; histoAndNbOfSamplesIt != histoAndNbOfSamplesItEnd; ++histoIt, ++nbOfSamplesIt, ++histoAndNbOfSamplesIt)
		{
			memcpy(*histoIt, *histoAndNbOfSamplesIt, histoDataSize);
			nbOfSamplesIt[0] = histoAndNbOfSamplesIt[d];
		}

		return true;
	}

	bool Utils::separateNbOfSamplesFromBlenderHistogram(
		Deepimf& o_rHistoImage,
		Deepimf& o_rNbOfSamplesImage,
		const Deepimf& i_rHistoAndNbOfSamplesImage)
	{
		int w = i_rHistoAndNbOfSamplesImage.getWidth();
		int h = i_rHistoAndNbOfSamplesImage.getHeight();
		int d = i_rHistoAndNbOfSamplesImage.getDepth() - 1;

		int nbLayers = d / 4;
		o_rHistoImage.resize(w, h, 3 * nbLayers); // Keep RGB but not A
		o_rNbOfSamplesImage.resize(w, h, 1);

		ImfIt histoIt = o_rHistoImage.begin();
		ImfIt nbOfSamplesIt = o_rNbOfSamplesImage.begin();
		ImfConstIt histoAndNbOfSamplesIt = i_rHistoAndNbOfSamplesImage.begin();
		ImfConstIt histoAndNbOfSamplesItEnd = i_rHistoAndNbOfSamplesImage.end();

		size_t destStride = 3 * sizeof(float);
		size_t sourceStride = 4 * sizeof(float);
		size_t offset = 1 * sizeof(float); // Channels are ordered ABGR so we skip the first one

		for (; histoAndNbOfSamplesIt != histoAndNbOfSamplesItEnd; ++histoIt, ++nbOfSamplesIt, ++histoAndNbOfSamplesIt)
		{
			for (int i = 0; i < nbLayers; ++i)
			{
				memcpy(*histoIt + i * destStride + 0 * offset, *histoAndNbOfSamplesIt + i * sourceStride + 3 * offset, destStride); // R
				memcpy(*histoIt + i * destStride + 1 * offset, *histoAndNbOfSamplesIt + i * sourceStride + 2 * offset, destStride); // G
				memcpy(*histoIt + i * destStride + 2 * offset, *histoAndNbOfSamplesIt + i * sourceStride + 1 * offset, destStride); // B
			}
			nbOfSamplesIt[0] = histoAndNbOfSamplesIt[d];
		}

		return true;
	}

	bool Utils::convertFromABGR(
		Deepimf& o_rDestImage,
		const Deepimf& i_rSourceImage)
	{
		int w = i_rSourceImage.getWidth();
		int h = i_rSourceImage.getHeight();
		int d = i_rSourceImage.getDepth();

		int nbLayers = d / 4;
		o_rDestImage.resize(w, h, 3 * nbLayers); // Keep RGB but not A

		ImfIt destIt = o_rDestImage.begin();
		ImfConstIt sourceIt = i_rSourceImage.begin();
		ImfConstIt sourceItEnd = i_rSourceImage.end();

		size_t destStride = 3 * sizeof(float);
		size_t sourceStride = 4 * sizeof(float);
		size_t offset = 1 * sizeof(float); // Channels are ordered ABGR so we skip the first one

		for (; sourceIt != sourceItEnd; ++destIt, ++sourceIt)
		{
			for (int i = 0; i < nbLayers; ++i)
			{
				memcpy(*destIt + i * destStride + 0 * offset, *sourceIt + i * sourceStride + 3 * offset, destStride); // R
				memcpy(*destIt + i * destStride + 1 * offset, *sourceIt + i * sourceStride + 2 * offset, destStride); // G
				memcpy(*destIt + i * destStride + 2 * offset, *sourceIt + i * sourceStride + 1 * offset, destStride); // B
			}
		}

		return true;
	}

	Deepimf Utils::mergeHistogramAndNbOfSamples(
			const Deepimf& i_rHistoImage,
			const Deepimf& i_rNbOfSamplesImage)
	{
		Deepimf histoAndNbOfSamplesImage;

		int w = i_rHistoImage.getWidth();
		int h = i_rHistoImage.getHeight();
		int d = i_rHistoImage.getDepth();

		assert(i_rNbOfSamplesImage.getWidth() == w);
		assert(i_rNbOfSamplesImage.getHeight() == h);
		assert(i_rNbOfSamplesImage.getDepth() == 1);

		histoAndNbOfSamplesImage.resize(w, h, d + 1);

		size_t histoDataSize = d * sizeof(float);
		ImfConstIt histoIt = i_rHistoImage.begin();
		ImfConstIt nbOfSamplesIt = i_rNbOfSamplesImage.begin();
		ImfIt histoAndNbOfSamplesIt = histoAndNbOfSamplesImage.begin();
		ImfIt histoAndNbOfSamplesItEnd = histoAndNbOfSamplesImage.end();

		for( ; histoAndNbOfSamplesIt != histoAndNbOfSamplesItEnd; ++histoIt, ++nbOfSamplesIt, ++histoAndNbOfSamplesIt)
		{
			memcpy(*histoAndNbOfSamplesIt, *histoIt, histoDataSize);
			histoAndNbOfSamplesIt[d] = nbOfSamplesIt[0];
		}

		return histoAndNbOfSamplesImage;
	}


	string Utils::extractFolderPath(const string& i_rFilePath)
	{
		cout << "extracting folder path from file: '" << i_rFilePath << "': ";
		const char sep = '/';
		size_t pos = i_rFilePath.rfind(sep);
		if(pos == string::npos)
			return "";
		return i_rFilePath.substr(0, pos + 1);
	}

	string Utils::getRelativePathFromFolder(const string& i_rFileAbsolutePath, const string& i_rFolderAbsolutePath)
	{
		const char sep = '/';
		size_t l1 = i_rFileAbsolutePath.length();
		size_t l2 = i_rFolderAbsolutePath.length();
		size_t l = (l1 > l2 ? l2 : l1);

		size_t posAfterLastCommonSep = 0;
		for(size_t i = 0; i < l; ++i)
		{
			char c = i_rFileAbsolutePath[i];
			if(c != i_rFolderAbsolutePath[i])
				break;
			if(c == sep)
				posAfterLastCommonSep = i + 1;
		}

		string relativePath = "";
		for(size_t i = posAfterLastCommonSep; i < l2; ++i)
			if(i_rFolderAbsolutePath[i] == sep)
				relativePath += "../";

		relativePath += i_rFileAbsolutePath.substr(posAfterLastCommonSep);

		return relativePath;
	}

}
