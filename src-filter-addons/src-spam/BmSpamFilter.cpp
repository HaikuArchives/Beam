/*
	BmSpamFilter.cpp

		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


#include <Alert.h>
#include <File.h>
#include <Message.h>

#include <MButton.h>
#include <Space.h>

#include <cctype>

#include "BubbleHelper.h"

#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmRosterBase.h"
#include "BmSpamFilter.h"


//     strnhash - generate the hash of a string of length N
//     goals - fast, works well with short vars includng 
//     letter pairs and palindromes, not crypto strong, generates
//     hashes that tend toward relative primality against common
//     hash table lengths (so taking the output of this function
//     modulo the hash table length gives a relatively uniform distribution
//
//     In timing tests, this hash function can hash over 10 megabytes
//     per second (using as text the full 2.4.9 linux kernel source)
//     hashing individual whitespace-delimited tokens, on a Transmeta
//     666 MHz.
unsigned long strnhash (char *str, long len)
{
  long i;
  // unsigned long hval;
  long hval;
  unsigned long tmp;

  // initialize hval
  hval= len;

  //  for each character in the incoming text:
  for ( i = 0; i < len; i++)
    {
      //    xor in the current byte against each byte of hval
      //    (which alone gaurantees that every bit of input will have
      //    an effect on the output)

      tmp = str[i];
      tmp = tmp | (tmp << 8) | (tmp << 16) | (tmp << 24);
      hval ^= tmp;

      //    add some bits out of the middle as low order bits.
      hval = hval + (( hval >> 12) & 0x0000ffff) ;

      //     swap most and min significative bytes 
      tmp = (hval << 24) | ((hval >> 24) & 0xff);
      hval &= 0x00ffff00;           // zero most and min significative bytes
      hval |= tmp;                  // OR with swapped bytes

      //    rotate hval 3 bits to the left (thereby making the
      //    3rd msb of the above mess the hsb of the output hash)
      hval = (hval << 3) + (hval >> 29);
    }
  return (hval);
}



// standard logfile-name for this file:
#undef BM_LOGNAME
#define BM_LOGNAME "Filter"

const char* FILTER_SPAM 			= "Spam";

extern "C" __declspec(dllexport)
BmFilterAddon* InstantiateFilter( const BmString& name, 
											 const BMessage* archive,
											 const BmString& kind);

extern "C" __declspec(dllexport) 
BmFilterAddonPrefsView* InstantiateFilterPrefs( float minx, float miny,
																float maxx, float maxy,
																const BmString& kind);

extern "C" __declspec(dllexport) 
const char* FilterKinds[] = {
	FILTER_SPAM,
	NULL
};

extern "C" __declspec(dllexport) 
const char* DefaultFilterName = "<<< SPAM-filter >>>";

/********************************************************************************\
	BmSpamFilter::OsbfClassifier::FeatureFilter
\********************************************************************************/
// #pragma mark --- FeatureFilter ---

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSpamFilter::OsbfClassifier
::FeatureFilter::FeatureFilter( BmMemIBuf* input, uint32 blockSize)
	:	inherited( input, blockSize, nTagImmediatePassOn)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilter::OsbfClassifier
::FeatureFilter::Filter( const char* srcBuf, uint32& srcLen, char* destBuf, 
								 uint32& destLen)
{
	BM_LOG3( BM_LogFilter, 
				BmString("starting to filter features of ") 
					<< srcLen << " bytes");
	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;
	char* dest = destBuf;
	char* destEnd = destBuf+destLen;

	bool haveFeature = false;
	char c;
	for( ; src<srcEnd && dest<destEnd; ++src) {
		c = *src;
		if (isalnum(c)) {
			*dest++ = c;
			haveFeature = true;
		} else {
			if (haveFeature)
				break;
		}
	}

	srcLen = src-srcBuf;
	destLen = dest-destBuf;
	BM_LOG3( BM_LogFilter, "feature-filter: done");
}



/********************************************************************************\
	BmSpamFilter::OsbfClassifier::FeatureLearner
\********************************************************************************/
// #pragma mark --- FeatureLearner ---

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSpamFilter::OsbfClassifier
::FeatureLearner::FeatureLearner( FeatureBucket* hash, Header* header, 	
											 bool revert)
	:	mHash( hash)
	,	mHeader( header)
	,	mRevert( revert)
	,	mSeenFeatures( new char [header->buckets])
	,	mStatus( B_OK)
{
   //  init the hashpipe with 0xDEADBEEF 
   for (uint32 h = 0; h < WindowLen; h++)
      mHashpipe.push_back( 0xDEADBEEF);
   memset( mSeenFeatures, 0, header->buckets);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSpamFilter::OsbfClassifier
::FeatureLearner::~FeatureLearner()
{
	delete [] mSeenFeatures;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmSpamFilter::OsbfClassifier
::FeatureLearner::operator()( char* buf, uint32 bufLen)
{
	if (!buf || !bufLen || !mHeader->buckets) {
		mStatus = B_BAD_VALUE;
		return mStatus;
	}

#if 0
	printf("<%*.*s>\n", (int)bufLen, (int)bufLen, buf);
#endif

   // Shift hash value of feature into pipe
   mHashpipe.pop_front();
   mHashpipe.push_back( strnhash( buf, bufLen));

	int sense = mRevert ? -1 : 1;

	unsigned long hindex;
	unsigned long h1, h2;
	unsigned long incrs;
	
	//
	//     old Hash polynomial: h0 + 3h1 + 5h2 +11h3 +23h4
	//     (coefficients chosen by requiring superincreasing,
	//     as well as prime)
	//
	for (uint32 j = 1; j < WindowLen; j++) {
		h1 = mHashpipe[0] * HashCoeff[0] + mHashpipe[j] * HashCoeff[j << 1];
		h2 = mHashpipe[0] * HashCoeff[1] + mHashpipe[j] * HashCoeff[(j << 1)-1];
		
		hindex = h1 % mHeader->buckets;
		
		//
		//  we now look at both the primary (h1) and 
		//  crosscut (h2) indexes to see if we've got
		//  the right bucket or if we need to look further
		//
		incrs = 0;
		while (mHash[hindex].value != 0 
				&& (mHash[hindex].hash != h1 || mHash[hindex].key != h2)) 
		{
			incrs++;
			// 
			//        If microgrooming is enabled, and we've found a 
			//        chain that's too long, we groom it down.
			//
			if (DoMicrogroom && (incrs > MicrogroomChainLength)) {
				//     set the random number generator up...
				//     note that this is repeatable for a
				//     particular test set, yet dynamic.  That
				//     way, we don't always autogroom away the
				//     same feature; we depend on the previous
				//     feature's key.
				srand ((unsigned int) h2);
				//
				//  and do the groom.
				// second argument is not necessary any more... fix it!
				Microgroom (mHash, mHeader, hindex);
				// since things may have moved after a
				// microgroom, restart our search
				hindex = h1 % mHeader->buckets;
				incrs = 0;
			};

			//       check to see if we've incremented ourself all the
			//       way around the .css file.   If so, we're full, and
			//       can hold no more features (this is unrecoverable)
			if (incrs > mHeader->buckets - 3) {
				BM_LOGERR("Your program is stuffing too many "
                      "features into this data-file.   "
                      "Adding any more features is "
                      "impossible in this file."
                      "You are advised to build a larger "
                      "data file and merge your data into "
                      "it.");
				mStatus = B_ERROR;
				return mStatus;
			}
			hindex++;
			if (hindex >= mHeader->buckets)
			   hindex = 0;
		};

      if (mHash[hindex].value == 0)
			BM_LOG3( BM_LogFilter, 
						BmString("New feature at ") << hindex);
		else
			BM_LOG3( BM_LogFilter, 
						BmString("Old feature at ") << hindex);

		//    always rewrite hash and key, as they may be incorrect
		//    (on a reused bucket) or zero (on a fresh one)
		//
		mHash[hindex].hash = h1;
		mHash[hindex].key = h2;
		
		//       watch out - sense may be both + or -, so check before 
		//       adding it...
		//
		if (mSeenFeatures[hindex] == 0) {
		   if (sense > 0 
		   && mHash[hindex].value + sense >=	FeatureBucketValueMax - 1)
		      mHash[hindex].value = FeatureBucketValueMax - 1;
		   else if (sense < 0 && mHash[hindex].value <= (uint32)-sense)
		      mHash[hindex].value = 0;
		   else
		      mHash[hindex].value += sense;
			mSeenFeatures[hindex] = 1;
		}
	}
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilter::OsbfClassifier
::FeatureLearner::Finalize()
{
   // update the number of learnings
	int sense = mRevert ? -1 : 1;
	if (sense > 0) {
		mHeader->learnings += sense;
		if (mHeader->learnings >= FeatureBucketValueMax-1) {
			mHeader->learnings >>= 1;
			for (uint32 i = 0; i < mHeader->buckets; i++)
				mHash[i].value >>= 1;
			BM_LOG( BM_LogFilter, 
						"You have managed to LEARN so many documents that"
						" you have forced rescaling of the entire database."
						" If you are the first person to do this, Fidelis "
						" owes you a bottle of good singlemalt scotch");
		}
   } else if (mHeader->learnings + sense > 0)
	   mHeader->learnings += sense;
}



/********************************************************************************\
	BmSpamFilter::OsbfClassifier::FeatureClassifier
\********************************************************************************/
// #pragma mark --- FeatureClassifier ---

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSpamFilter::OsbfClassifier
::FeatureClassifier::FeatureClassifier( FeatureBucket* spamHash, 
													 Header* spamHeader, 
													 FeatureBucket* tofuHash, 
													 Header* tofuHeader)
	:	mStatus( B_OK)
	,	mClassifiedAsSpam( false)
	,	mClassifiedAsTofu( false)
	,	mTotalLearnings( 0)
	,	mTotalFeatures( 0)
{
	mHash[0] = spamHash;
	mHash[1] = tofuHash;
	mHeader[0] = spamHeader;
	mHeader[1] = tofuHeader;

   //  init the hashpipe with 0xDEADBEEF 
   for (uint32 h = 0; h < WindowLen; h++)
      mHashpipe.push_back( 0xDEADBEEF);
      
	// init basic arrays
	for (uint32 i = 0; i < MaxHash; i++) {
		mLearnings[i] = mHeader[i]->learnings;
		//
		//   increment learnings to avoid division by 0
		if (mLearnings[i] == 0)
			mLearnings[i]++;

		// update total learnings
		mTotalLearnings += mLearnings[i];
		
		//  set this hashlens to the length in features instead
		//  of the length in bytes.
		mHashLen[i] = mHeader[i]->buckets;
		mHashName[i] = (i == 0 ? "Spam" : "Tofu");
	}

	// init all data arrays
	for (uint32 i = 0; i < MaxHash; i++) {
		mSeenFeatures[i] = new char [mHeader[i]->buckets];
	   memset( mSeenFeatures[i], 0, mHeader[i]->buckets);
      mHits[i] = 0;				// absolute hit counts 
      mTotalHits[i] = 0;		// absolute hit counts 
      mUniqueFeatures[i] = 0;	// features counted per class
      mMissedFeatures[i] = 0;	// missed features per class
      mPtc[i] = (double) mLearnings[i] / mTotalLearnings;	
      									// a priori probability
      // mPtc[i] = 0.5;          // (another) a priori probability
      mPltc[i] = 0.5;		// local probability
    }
      
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSpamFilter::OsbfClassifier
::FeatureClassifier::~FeatureClassifier()
{
	delete [] mSeenFeatures[1];
	delete [] mSeenFeatures[0];
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmSpamFilter::OsbfClassifier
::FeatureClassifier::operator()( char* buf, uint32 bufLen)
{
	if (!buf || !bufLen || !mHeader[0]->buckets || !mHeader[1]->buckets) {
		mStatus = B_BAD_VALUE;
		return mStatus;
	}

#if 0
	printf("<%*.*s>\n", (int)bufLen, (int)bufLen, buf);
#endif

   // Shift hash value of feature into pipe
   mHashpipe.pop_front();
   mHashpipe.push_back( strnhash( buf, bufLen));

	uint32 j, k;
	unsigned long hindex;
	unsigned long h1, h2;
	// remember indexes of classes with min and max local probabilities
	uint32 i_min_p, i_max_p;
	// remember min and max local probabilities of a feature
	double min_local_p, max_local_p;
	double htf;

	//
	for (j = 1; j < WindowLen; j++) {
		h1 = mHashpipe[0] * HashCoeff[0] + mHashpipe[j] * HashCoeff[j << 1];
		h2 = mHashpipe[0] * HashCoeff[1] + mHashpipe[j] * HashCoeff[(j << 1)-1];
		
		hindex = h1;
		
		//
		//    Note - a strict interpretation of Bayesian
		//    chain probabilities should use 0 as the initial
		//    state.  However, because we rapidly run out of
		//    significant digits, we use a much less strong
		//    initial state.   Note also that any nonzero
		//    positive value prevents divide-by-zero
		//
		//       Zero out "Hits This Feature"
		htf = 0;
		mTotalFeatures++;
		//
		//    calculate the precursors to the local probabilities;
		//    these are the hits[k] array, and the htf total.
		//
		min_local_p = 1.0;
		max_local_p = 0;
		i_min_p = i_max_p = 0;
		for (k = 0; k < MaxHash; k++) {
			uint32 lh, lh0;
			float p_feat = 0;
			
			lh = hindex % mHashLen[k];
			lh0 = lh;
			mHits[k] = 0;
			
			// look for feature hashes h1 and h2
			while (mHash[k][lh].value != 0
					&& (mHash[k][lh].hash != h1 || mHash[k][lh].key != h2))
			{
				lh++;
				if (lh >= mHashLen[k])
					lh = 0;
				if (lh == lh0)
					break;	// wraparound
			};
	
			// if the feature wasn't found in the class, the index lh
			// points to the first empty bucket after the chain and its
			// value is 0.
			
			if (mSeenFeatures[k][lh] == 0) {
				// only not previously seen features are considered
				mUniqueFeatures[k] += 1;	// count unique features used
				if (mHash[k][lh].value != 0) {
					mHits[k] = mHash[k][lh].value;
					mTotalHits[k] += mHits[k];	// remember totalhits
					htf += mHits[k];	// and hits-this-feature
					p_feat = mHits[k] / mLearnings[k];
					// find class with minimum P(F)
					if (p_feat <= min_local_p) {
					    i_min_p = k;
					    min_local_p = p_feat;
					}
					// find class with maximum P(F)
					if (p_feat >= max_local_p) {
					    i_max_p = k;
					    max_local_p = p_feat;
					}
					// mark the feature as seen
					mSeenFeatures[k][lh] = 1;
				} else {
					// a feature that wasn't found can't be marked as already
					// seen in the doc because the index lh doesn't refer to it,
					// but to the first empty bucket after the chain, which
					// is common to all not-found features in the same chain.
					// This is not a problem though, because if the feature is
					// found in another class, it'll be marked as seen on that
					// class, which is enough to mark it as seen. If it's not
					// found in any class, it will have zero count on all classes
					// and will be ignored as well. So, only found features
					// are marked as seen.
					i_min_p = k;
					min_local_p = p_feat;
					// for statistics only (for now...)
					mMissedFeatures[k] += 1;
				}
			} else {
				// ignore already seen features
				min_local_p = max_local_p = 0;
				break;
			}
		}
	
		//=======================================================
		// Update the probabilities using Bayes:
		//
		//                      P(F|S) P(S)
		//     P(S|F) = -------------------------------
		//               P(F|S) P(S) +  P(F|NS) P(NS)
		//
		// S = class spam; NS = class nonspam; F = feature
		//
		// Here we adopt a different method for estimating
		// P(F|S). Instead of estimating P(F|S) as (hits[S][F] /
		// (hits[S][F] + hits[NS][F])), like in the original
		// code, we use (hits[S][F] / learnings[S]) which is the
		// ratio between the number of messages of the class S
		// where the feature F was observed during learnings and
		// the total number of learnings of that class. Both
		// values are kept in the respective .css file, the
		// number of learnings in the header and the number of
		// occurrences of the feature F as the value of its
		// feature bucket.
		//
		// It's worth noting another important difference here:
		// as we want to estimate the *number of messages* of a
		// given class where a certain feature F occurs, we
		// count only the first ocurrence of each feature in a
		// message (repetitions are ignored), both when learning
		// and when classifying.
		// 
		// Advantages of this method, compared to the original:
		//
		// - First of all, and the most important: accuracy is
		// really much better, at about the same speed! With
		// this higher accuracy, it's also possible to increase
		// the speed, at the cost of a low decrease in accuracy,
		// using smaller .css files;
		//
		// - It is not affected by different sized classes
		// because the numerator and the denominator belong to
		// the same class;
		//
		// - It allows a simple and fast pruning method that
		// seems to introduce little noise: just zero features
		// with lower count in a overflowed chain, zeroing first
		// those in their right places, to increase the chances
		// of deleting older ones.
		//
		// Disadvantages:
		//
		// - It breaks compatibility with previous css file
		// format because of different header structure and
		// meaning of the counts.
		//
		// Confidence factors
		//
		// The motivation for confidence factors is to reduce
		// the noise introduced by features with small counts
		// and/or low significance. This is an attempt to mimic
		// what we do when inspecting a message to tell if it is
		// spam or not. We intuitively consider only a few
		// tokens, those which carry strong indications,
		// according to what we've learned and remember, and
		// discard the ones that may occur (approximately)
		// equally in both classes.
		//
		// Once P(Feature|Class) is estimated as above, the
		// calculated value is adjusted using the following
		// formula:
		//
		//  CP(Feature|Class) = 0.5 + 
		//             CF(Feature) * (P(Feature|Class) - 0.5)
		//
		// Where CF(Feature) is the confidence factor and
		// CP(Feature|Class) is the adjusted estimate for the
		// probability.
		//
		// CF(Feature) is calculated taking into account the
		// weight, the max and the min frequency of the feature
		// over the classes, using the empirical formula:
		//
		//     (((Hmax - Hmin)^2 + Hmax*Hmin - K1/SH) / SH^2) ^ K2
		// CF(Feature) = ------------------------------------------
		//                    1 +  K3 / (SH * Weight)
		//
		// Hmax  - Number of documents with the feature "F" on
		// the class with max local probability;
		// Hmin  - Number of documents with the feature "F" on
		// the class with min local probability;
		// SH - Sum of Hmax and Hmin
		// K1, K2, K3 - Empirical constants
		//
		// OBS: - Hmax and Hmin are normalized to the max number
		//  of learnings of the 2 classes involved.
		//  - Besides modulating the estimated P(Feature|Class),
		//  reducing the noise, 0 <= CF < 1 is also used to
		//  restrict the probability range, avoiding the
		//  certainty falsely implied by a 0 count for a given
		//  class.
		//
		// -- Fidelis Assis
		//=========================================================
		
		// ignore less significant features (confidence factor = 0)
		if ((max_local_p - min_local_p) < 0.02)
			continue;
		// testing speed-up...
		if (min_local_p > 0 && max_local_p / min_local_p < MinPmaxPminRatio)
			continue;
		
		// code under testing....
		// calculate confidence_factor
		//
		// hmmm, unsigned long gives better precision than float...
		//float hits_max_p, hits_min_p, sum_hits, diff_hits;
		unsigned long hits_max_p, hits_min_p, sum_hits, diff_hits;
		float K1, K2, K3;
		float confidence_factor;
		
		hits_min_p = mHits[i_min_p];
		hits_max_p = mHits[i_max_p];
		
		// normalize hits to max learnings
		if (mLearnings[i_min_p] < mLearnings[i_max_p])
			hits_min_p *= mLearnings[i_max_p] / mLearnings[i_min_p];
		else
			hits_max_p *= mLearnings[i_min_p] / mLearnings[i_max_p];
		
		sum_hits = hits_max_p + hits_min_p;
		diff_hits = hits_max_p - hits_min_p;
		if (diff_hits < 0)
			diff_hits = -diff_hits;
		
		// constants used in the CF formula above
		// K1 = 0.25; K2 = 10; K3 = 8;
		K1 = 0.25;
		K2 = 10;
		K3 = 8;
		
		// calculate confidence factor (CF)
		if (min_local_p > 0)
			confidence_factor = 1;
		else
			confidence_factor =
				pow ((diff_hits * diff_hits + hits_max_p * hits_min_p -
						K1 / sum_hits) / (sum_hits * sum_hits),
						K2) / (1.0 + K3 / (sum_hits * FeatureWeight[j]));
		
		BM_LOG3( BM_LogFilter,
					BmString("CF:") << confidence_factor 
						<< " max_hits:" << hits_max_p
						<<" min_hits:" << hits_min_p
						<< " weight:"<< FeatureWeight[j]);
	
		// calculate the numerators  - P(F|C) * P(C)
		// and the denominator (sum of numerators)
		double bayes_denominator = 0.0;
		for (k = 0; k < MaxHash; k++) {
			// P(C) = learnings[k] / total_learnings
			// P(F|C) = hits[k]/learnings[k], adjusted with confidence factors
			// 
			// the ratio between unique and total features improves final
			// accuracy, reduces the reinforcement threshold to around 10
			// and reduces the number of reinforcements required to get
			// final accuracy.
			mPltc[k] = (double) mLearnings[k] / mTotalLearnings *
									(0.5 + confidence_factor *
								   ((double) mUniqueFeatures[k] / mTotalFeatures) *
									(mHits[k] / mLearnings[k] - 0.5));
			
			BM_LOG3( BM_LogFilter, 
						BmString("CF:") << confidence_factor 
							<< " totalhits[k]:" << mTotalHits[k]
							<< " missedfeatures[k]:" << mMissedFeatures[k]
							<< " uniquefeatures[k]:" << mUniqueFeatures[k]
							<< " totalfeatures:" << mTotalFeatures
							<< " weight:" << FeatureWeight[j]);
	
			bayes_denominator += mPltc[k];
		}
	
	   double renorm = 0.0;
		// divide by Bayes' denominator
		for (k = 0; k < MaxHash; k++) {
			//   now calculate the updated per class probabilities
			mPtc[k] = mPtc[k] * mPltc[k] / bayes_denominator;
			
			//   if we have underflow (any probability == 0.0 ) then
			//   bump the probability back up to 10^-308, or
			//   whatever a small multiple of the minimum double
			//   precision value is on the current platform.
			if (mPtc[k] < 10 * DBL_MIN)
				mPtc[k] = 10 * DBL_MIN;
			renorm += mPtc[k];
		}
	
		// renormalize probabilities
		for (k = 0; k < MaxHash; k++)
			mPtc[k] = mPtc[k] / renorm;
	
	}

	return B_OK;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilter::OsbfClassifier
::FeatureClassifier::Finalize()
{
	uint32 k;
	//   force probabilities into the non-stuck zone
	for (k = 0; k < MaxHash; k++)
		if (mPtc[k] < 10 * DBL_MIN)
	   	mPtc[k] = 10 * DBL_MIN;
	
   for (k = 0; k < MaxHash; k++)
		BM_LOG2( BM_LogFilter,
					BmString("Probability of match for file ") << mHashName[k]
						<< " = " << mPtc[k]);

	// determine best (well...better) match
	uint32 bestseen = 0;
   for (k = 0; k < MaxHash; k++)
		if (mPtc[k] > mPtc[bestseen])
			bestseen = k;
	double remainder = 10 * DBL_MIN;
   for (k = 0; k < MaxHash; k++)
		if (bestseen != k)
			remainder = remainder + mPtc[k];
	
	mClassifiedAsSpam = bestseen == 0;
	mClassifiedAsTofu = bestseen == 1;

	BM_LOG2( BM_LogFilter,
				BmString("Best match to file ") << mHashName[bestseen]
					<< " prob:" << mPtc[bestseen] 
					<< " pR:" << (log10 (mPtc[bestseen]) - log10 (remainder)));
}



/********************************************************************************\
	BmSpamFilter::OsbfClassifier
\********************************************************************************/
// #pragma mark --- OsbfClassifier ---

BmSpamFilter::OsbfClassifier 
BmSpamFilter::nClassifier = BmSpamFilter::OsbfClassifier();

unsigned char 
BmSpamFilter::OsbfClassifier
::FileVersion[4] = { 'C', 'F', 'C', '1' };

////////////////////////////////////////////////////////////////////
//
//     the hash coefficient table should be full of relatively
//     prime numbers, and preferably superincreasing, though both of 
//     those are not strict requirements.
//
const long BmSpamFilter::OsbfClassifier
::HashCoeff[20] = {
	1, 7,
	3, 13,
	5, 29,
	11, 51,
	23, 101,
	47, 203,
	97, 407,
	197, 817,
	397, 1637,
	797, 3277
};

float BmSpamFilter::OsbfClassifier
::FeatureWeight[6] = { 0, 3125, 277, 27, 4, 1 };

/* max feature value */
const unsigned long BmSpamFilter::OsbfClassifier
::WindowLen = 5;

/* max feature value */
const unsigned long BmSpamFilter::OsbfClassifier
::FeatureBucketValueMax = 65535;

/* max number of features */
const unsigned long BmSpamFilter::OsbfClassifier
::DefaultFileLength = 94321;

/* shall we try to do small cleanups automatically, if the hash-chains
   get too long? */ 
const bool BmSpamFilter::OsbfClassifier
::DoMicrogroom = true;

/* max chain len - microgrooming is triggered after this, if enabled */ 
const unsigned long BmSpamFilter::OsbfClassifier
::MicrogroomChainLength = 29;

/* maximum number of buckets groom-zeroed */
const unsigned long BmSpamFilter::OsbfClassifier
::MicrogroomStopAfter = 128;

/* minimum ratio between max and min P(F|C) */
const unsigned long BmSpamFilter::OsbfClassifier
::MinPmaxPminRatio = 9;
	
/*------------------------------------------------------------------------------*\
	OsbfClassifier()
		-	
\*------------------------------------------------------------------------------*/
BmSpamFilter::OsbfClassifier::OsbfClassifier()
	:	mSpamHash( NULL)
	,	mTofuHash( NULL)
	,	mNeedToStoreSpam( false)
	,	mNeedToStoreTofu( false)
	,	mLock( "SpamClassifierLock", true)
{
}

/*------------------------------------------------------------------------------*\
	~OsbfClassifier()
		-	
\*------------------------------------------------------------------------------*/
BmSpamFilter::OsbfClassifier::~OsbfClassifier()
{
	Store();
	// ToDo: need to store the datafiles here!
	delete [] mTofuHash;
	delete [] mSpamHash;
}

/*------------------------------------------------------------------------------*\
	Initialize()
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilter::OsbfClassifier::Initialize()
{
	if (!mSpamHash) {
		BmString spamFilename 
			= BmString( BeamRoster->SettingsPath()) << "/Spam.data";
		ReadDataFile( spamFilename, mSpamHeader, mSpamHash);
	}
	if (!mTofuHash) {
		BmString tofuFilename 
			= BmString( BeamRoster->SettingsPath()) << "/Tofu.data";
		ReadDataFile( tofuFilename, mTofuHeader, mTofuHash);
	}
}

/*------------------------------------------------------------------------------*\
	Store()
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilter::OsbfClassifier::Store()
{
	if (mSpamHash && mNeedToStoreSpam) {
		BmString spamFilename 
			= BmString( BeamRoster->SettingsPath()) << "/Spam.data";
		WriteDataFile( spamFilename, mSpamHeader, mSpamHash);
	}
	if (mTofuHash && mNeedToStoreTofu) {
		BmString tofuFilename 
			= BmString( BeamRoster->SettingsPath()) << "/Tofu.data";
		WriteDataFile( tofuFilename, mTofuHeader, mTofuHash);
	}
}

/*------------------------------------------------------------------------------*\
	LearnAsSpam()
		-	
\*------------------------------------------------------------------------------*/
bool 
BmSpamFilter::OsbfClassifier::LearnAsSpam( BmMsgContext* msgContext)
{
	if (msgContext->mail->IsMarkedAsTofu()) {
		// unlearn this mail as tofu, since it's not:
		if (!Learn(msgContext, false, true))
			return false;
	}
	// learn this mail as spam:
	return Learn(msgContext, true, false);
}

/*------------------------------------------------------------------------------*\
	Learn()
		-	
\*------------------------------------------------------------------------------*/
bool BmSpamFilter::OsbfClassifier::LearnAsTofu( BmMsgContext* msgContext)
{
	if (msgContext->mail->IsMarkedAsSpam()) {
		// unlearn this mail as spam, since it's not:
		if (!Learn(msgContext, true, true))
			return false;
	}
	// learn this mail as tofu:
	return Learn(msgContext, false, false);
}

/*------------------------------------------------------------------------------*\
	Learn()
		-	
\*------------------------------------------------------------------------------*/
bool BmSpamFilter::OsbfClassifier::Learn( BmMsgContext* msgContext, 
														bool learnAsSpam, bool revert)
{
	// set lock to serialize OSBF-calls:
	BAutolock lock( &mLock);
	if (!lock.IsLocked()) {
		mLastErr = "Unable to get SPAM-lock";
		return false;
	}
	
	Header* header = learnAsSpam ? &mSpamHeader : &mTofuHeader;
	FeatureBucket* hash = learnAsSpam ? mSpamHash : mTofuHash;
	if (learnAsSpam)
		mNeedToStoreSpam = true;
	else
		mNeedToStoreTofu = true;

	BmStringIBuf text(msgContext->mail->RawText());
	FeatureFilter filter( &text);
	BmMemBufConsumer consumer( 4096);
	FeatureLearner learner( hash, header, revert);
	consumer.Consume(&filter, &learner);

	if (learner.mStatus == B_OK)
		learner.Finalize();
	
	return learner.mStatus == B_OK;
}

/*------------------------------------------------------------------------------*\
	Learn()
		-	
\*------------------------------------------------------------------------------*/
bool BmSpamFilter::OsbfClassifier::Classify( BmMsgContext* msgContext)
{
	// set lock to serialize OSBF-calls:
	BAutolock lock( &mLock);
	if (!lock.IsLocked()) {
		mLastErr = "Unable to get SPAM-lock";
		return false;
	}

	if (!msgContext || !msgContext->mail) {
		mLastErr = "Illegal msg-context!";
		return false;
	}
	
	BmStringIBuf text(msgContext->mail->RawText());
	FeatureFilter filter( &text);
	BmMemBufConsumer consumer( 4096);
	FeatureClassifier classifier( mSpamHash, &mSpamHeader, 
											mTofuHash, &mTofuHeader);
	consumer.Consume(&filter, &classifier);

	if (classifier.mStatus == B_OK)
		classifier.Finalize();
	
	return classifier.mStatus == B_OK;
}

/*------------------------------------------------------------------------------*\
	Microgroom()
		-	
\*------------------------------------------------------------------------------*/
//
//     How to microgroom a .css file that's getting full
//
//     NOTA BENE NOTA BENE NOTA BENE NOTA BENE
//      
//         This whole section of code is under intense develoment; right now
//         it "works" but not any better than nothing at all.  Be warned
//         that any patches issued on it may well never see the light of
//         day, as intense testing and comparison may show that the current
//         algorithms are, well, suckful.
//
//
//     There are two steps to microgrooming - first, since we know we're
//     already too full, we execute a 'zero unity bins'.
//
void BmSpamFilter::OsbfClassifier::Microgroom ( FeatureBucket* hash,
																Header* header,
																unsigned long hindex)
{
	uint32 i, j, k;
	uint32 packstart;
	uint32 packlen;
	uint32 zeroed_countdown;
	uint32 min_value, distance, max_distance;
	
	zeroed_countdown = MicrogroomStopAfter;
	k = 0;
	
	//   micropack - start at initial chain start, move to back of 
	//   chain that overflowed, then scale just that chain.
	i = j = hindex % header->buckets;
	min_value = hash[i].value;
	while (hash[i].value != 0) {
		if (hash[i].value < min_value)
			min_value = hash[i].value;
		if (i == 0)
			i = header->buckets - 1;
		else
			i--;
		if (i == j)
			break;			// don't hang if we have a 100% full .css file
	}
	
	//	  now, move our index to the first bucket in this chain.
	i++;
	if (i >= header->buckets)
		i = 0;
	packstart = i;

	i = j = hindex % header->buckets;
	while (hash[i].value != 0) {
		i++;
	   if (i == header->buckets)
			i = 0;
		if (i == j)
			break;			// don't hang if we have a 100% full .css file
	}

	//     now, our index is right after the last bucket in this chain.
	packlen = i - packstart;
	if (packlen < 0)
		packlen += header->buckets;
	
	//   This pruning method zeroes buckets with minimum count in the chain.
	//   It tries first buckets with minimum distance to their right position,
	//   to increase the chance of zeroing older buckets first. If none with
	//   distance 0 is found, the distance is increased until at least one
	//   bucket is zeroed.
	//
	//   We keep track of how many buckets we've zeroed and we stop
	//   zeroing additional buckets after that point.   NO!  BUG!  That 
	//   messes up the tail length, and if we don't repack the tail, then
	//   features in the tail can become permanently inaccessible!   Therefore,
	//   we really can't stop in the middle of the tail (well, we could 
	//   stop zeroing, but we need to pass the full length of the tail in.
	// 
	//   Note that we can't do this "adaptively" in packcss, because zeroes
	//   there aren't necessarily overflow chain terminators (because -we-
	//   might have inserted them here.
	//
	
	// try features in their right place first
	max_distance = 1;
	// while no bucket is zeroed...
	while (zeroed_countdown == MicrogroomStopAfter) {
		// printf("Start: %ld, stop_after: %ld, max_distance: %ld\n", packstart, microgroom_stop_after, max_distance);
		i = packstart;
		while (hash[i].value != 0 && zeroed_countdown > 0) {
			// check if it's a candidate
			if (hash[i].value == min_value) {
				// if it is, check the distance
				distance = i - hash[i].hash % header->buckets;
				if (distance < 0)
					distance += header->buckets;
				if (distance < max_distance) {
					hash[i].value = 0;
					zeroed_countdown--;
				}
			}
			i++;
			if (i >= header->buckets)
				i = 0;
		}
		//  if none was zeroed, increase the allowed distance between the
		//  candidade's position and its right place.
		if (zeroed_countdown == MicrogroomStopAfter)
			max_distance++;
	}
	
	BM_LOG3( BM_LogFilter, 
				BmString("Leaving microgroom: ") 
				<< MicrogroomStopAfter - zeroed_countdown
				<< " buckets with value " << hash[i].value
				<< " zeroed at distance " << max_distance - 1);
	
	//   now we pack the buckets
	PackData(header, hash, packstart, packlen);
}

/*------------------------------------------------------------------------------*\
	PackData()
		-	
\*------------------------------------------------------------------------------*/
//    How we pack...
//   
//    We look at each bucket, and attempt to reinsert it at the "best"
//    place.  We know at worst it will end up where it already is, and
//    at best it will end up lower (at a lower index) in the file, except
//    if it's in wraparound mode, in which case we know it will not get
//    back up past us (since the file must contain at least one empty)
//    and so it's still below us in the file.
void BmSpamFilter::OsbfClassifier::PackData( Header* header, 
														 	FeatureBucket* hash,
															unsigned long packstart, 
															unsigned long packlen)
{
	if (packstart + packlen <= header->buckets) {
		//  no wraparound in this case
		PackDataSeg(header, hash, packstart, packlen);
	} else {
		//  wraparound mode - do it as two separate repacks
		PackDataSeg(header, hash, packstart, (header->buckets - packstart));
		PackDataSeg(header, hash, 0, (packlen - (header->buckets - packstart)));
	}
}

/*------------------------------------------------------------------------------*\
	PackDataSeg()
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilter::OsbfClassifier::PackDataSeg( Header* header, 
															 	FeatureBucket* hash,
																unsigned long packstart, 
																unsigned long packlen)
{
	unsigned long ifrom, ito;
	unsigned long thash, tkey, tvalue;

	// Our slot values are now somewhat in disorder because empty
	// buckets may now have been inserted into a chain where there used
	// to be placeholder buckets.  We need to re-insert slot data in a
	// bucket where it will be found.
	
	for (ifrom = packstart; ifrom < packstart + packlen; ifrom++) {
		//    Now find the next bucket to place somewhere 
		thash = hash[ifrom].hash;
		tkey = hash[ifrom].key;
		tvalue = hash[ifrom].value;
	
		if (tvalue != 0) {
			ito = thash % header->buckets;
			while (!((hash[ito].value == 0)
		   		|| (hash[ito].hash == thash && hash[ito].key == tkey))) 
		   {
				ito++;
				if (ito >= header->buckets)
					ito = 0;
			}
	
			//   found an empty slot, put this value there, and zero the
			//   original one.  Sometimes this is a noop.  We don't care.
			if (ito != ifrom) {
				hash[ifrom].hash = 0;
				hash[ifrom].key = 0;
				hash[ifrom].value = 0;
				
				hash[ito].hash = thash;
				hash[ito].key = tkey;
				hash[ito].value = tvalue;
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	ErrorString()
		-	
\*------------------------------------------------------------------------------*/
const BmString& BmSpamFilter::OsbfClassifier::ErrorString() const {
	return mLastErr;
}

/*------------------------------------------------------------------------------*\
	CreateDataFile()
		-	
\*------------------------------------------------------------------------------*/
status_t BmSpamFilter::OsbfClassifier::CreateDataFile( const BmString& filename)
{
	status_t err;
	BFile file;
	BM_LOG( BM_LogFilter, 
			  BmString("Spam-Addon: trying to create datafile ") << filename);
	err = file.SetTo( filename.String(), B_READ_WRITE | B_CREATE_FILE);
	if (err == B_OK) {
		// Set the header.
		Header h;
		unsigned long bucketCount = DefaultFileLength;
		h.version = FileVersion;
		h.learnings = 0;
		h.buckets = bucketCount;
		// Write header
		if ((err=file.Write(&h, sizeof (h))) != sizeof(h))
     		return err < 0 ? err : B_IO_ERROR;

		//  Initialize hashes - zero all buckets
		FeatureBucket feature = { 0, 0, 0 };
		for (unsigned long i = 0; i < bucketCount; i++)
		{
      	// Write buckets
      	if ((err=file.Write(&feature, sizeof(feature))) != sizeof(feature))
      		return err < 0 ? err : B_IO_ERROR;
		}
		BM_LOG( BM_LogFilter, 
				  BmString("Spam-Addon: ok, done creating datafile ") << filename);
	}
	if (err < B_OK)
		BM_LOGERR( BmString("Couldn't create spam/tofu datafile ")
						<< filename << " -> " << strerror(err));
	return err;
}

/*------------------------------------------------------------------------------*\
	ReadDataFile()
		-	
\*------------------------------------------------------------------------------*/
status_t BmSpamFilter::OsbfClassifier::ReadDataFile( const BmString& filename,
												 					  Header& header,
												 					  FeatureBucket*& hash)
{
	BFile file;
	// try to open data-file...
	status_t err;
	BM_LOG( BM_LogFilter, 
			  BmString("Spam-Addon: trying to read datafile ") << filename);
	if ((err = file.SetTo( filename.String(), B_READ_ONLY)) != B_OK) {
		err = CreateDataFile( filename);
		if (err != B_OK)
			return err;
		err = file.SetTo( filename.String(), B_READ_ONLY);
		if (err != B_OK) {
			BM_LOGERR( BmString("Giving up on spam/tofu datafile ") << filename);
			return err;
		}
	}
	// read header
	if ((err = file.Read(&header, sizeof(header))) < (int32)sizeof(header))
		return err < 0 ? err : B_IO_ERROR;
	BM_LOG2( BM_LogFilter, 
			   BmString("Spam-Addon: file has room for ") << header.buckets
			   	<< " features, number of learnings is " << header.learnings);
	// check version
	if (memcmp(header.version, FileVersion, sizeof(header.version)) != 0) {
		BM_LOGERR( BmString("Wrong version of spam/tofu datafile ") << filename);
		return B_MISMATCHED_VALUES;
	}
	// read hash
	hash = new FeatureBucket [header.buckets];
	ssize_t sz = header.buckets * sizeof(FeatureBucket);
	if ((err = file.Read(hash, sz)) < sz) {
		BM_LOGERR( BmString("Not enough data in datafile ") << filename);
		delete hash;
		hash = NULL;
  		return err < 0 ? err : B_IO_ERROR;
	}
	BM_LOG( BM_LogFilter, 
			  BmString("Spam-Addon: ok, done reading datafile ") << filename);
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	WriteDataFile()
		-	
\*------------------------------------------------------------------------------*/
status_t BmSpamFilter::OsbfClassifier::WriteDataFile( const BmString& filename,
												 						Header& header,
												 						FeatureBucket*& hash)
{
	BFile file;
	// try to open data-file...
	status_t err;
	BM_LOG( BM_LogFilter, 
			  BmString("Spam-Addon: trying to write datafile ") << filename);
	if ((err = file.SetTo( filename.String(), B_READ_WRITE)) != B_OK)
		return err;

	// write header
	if ((err = file.Write(&header, sizeof(header))) < (int32)sizeof(header))
		return err < 0 ? err : B_IO_ERROR;
	// write hash
	ssize_t sz = header.buckets * sizeof(FeatureBucket);
	if ((err = file.Write(hash, sz)) < sz) {
		BM_LOGERR( BmString("Couldn't write data to file ") << filename);
  		return err < 0 ? err : B_IO_ERROR;
	}
	BM_LOG( BM_LogFilter, 
			  BmString("Spam-Addon: ok, done writing datafile ") << filename);
	return B_OK;
}



/********************************************************************************\
	BmSpamFilter
\********************************************************************************/
// #pragma mark --- BmSpamFilter ---

const char* const BmSpamFilter::MSG_VERSION = 		"bm:version";
const int16 BmSpamFilter::nArchiveVersion = 1;

/*------------------------------------------------------------------------------*\
	BmSpamFilter( archive)
		-	c'tor
		-	constructs a BmSpamFilter from a BMessage
\*------------------------------------------------------------------------------*/
BmSpamFilter::BmSpamFilter( const BmString& name, const BMessage* archive) 
	:	mName( name)
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
}

/*------------------------------------------------------------------------------*\
	~BmSpamFilter()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmSpamFilter::~BmSpamFilter() {
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmSpamFilter into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t 
BmSpamFilter::Archive( BMessage* archive, bool) const 
{
	status_t ret = (archive->AddInt16( MSG_VERSION, nArchiveVersion));
	return ret;
}

/*------------------------------------------------------------------------------*\
	Execute()
		-	
\*------------------------------------------------------------------------------*/
bool 
BmSpamFilter::Execute( BmMsgContext* msgContext, const BmString& jobSpecifier) 
{
	BmString mailId;
	if (msgContext)
		mailId = msgContext->mail->Name();
	BM_LOG2( BM_LogFilter, BmString("Spam-Addon: asked to execute filter <") 
									<< Name() 
									<< "> on mail with Id <" << mailId << ">");
	if (!jobSpecifier.ICompare("LearnAsSpam")) {
		BM_LOG2( BM_LogFilter, "Spam-Addon: starting LearnAsSpam job...");
		nClassifier.LearnAsSpam( msgContext);
	} else if (!jobSpecifier.ICompare("LearnAsTofu")) {
		BM_LOG2( BM_LogFilter, "Spam-Addon: starting LearnAsTofu job...");
		nClassifier.LearnAsTofu( msgContext);
	} else {
		BM_LOG2( BM_LogFilter, "Spam-Addon: starting Classify job...");
		nClassifier.Classify( msgContext);
	}
	BM_LOG2( BM_LogFilter, "Spam-Addon: done.");
	return true;
}

/*------------------------------------------------------------------------------*\
	SanityCheck()
		-	checks if the current values make sense and returns error-info through
			given out-params
		-	returns true if values are ok, false (and error-info) if not
\*------------------------------------------------------------------------------*/
bool BmSpamFilter::SanityCheck( BmString& complaint, BmString& fieldName) {
	return true;
}

/*------------------------------------------------------------------------------*\
	ErrorString()
		-	
\*------------------------------------------------------------------------------*/
BmString BmSpamFilter::ErrorString() const {
	return nClassifier.ErrorString();
}



/*------------------------------------------------------------------------------*\
	InstantiateFilter()
		-	
\*------------------------------------------------------------------------------*/
extern "C" __declspec(dllexport)
BmFilterAddon* InstantiateFilter( const BmString& name, 
											 const BMessage* archive,
											 const BmString& kind) {
	if (!kind.ICompare( FILTER_SPAM))
		return new BmSpamFilter( name, archive);
	else
		return NULL;
}



/********************************************************************************\
	BmSpamFilterPrefs
\********************************************************************************/
// #pragma mark --- BmSpamFilterPrefs ---

/*------------------------------------------------------------------------------*\
	BmSpamFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
BmSpamFilterPrefs::BmSpamFilterPrefs( minimax minmax)
	:	inherited( minmax.mini.x, minmax.mini.y, minmax.maxi.x, minmax.maxi.y)
	,	mCurrFilterAddon( NULL)
{
	VGroup* vgroup = 
		new VGroup( 
			mStatisticsButton = new MButton( 
				"Show Statistics...", 
				new BMessage( BM_SHOW_STATISTICS),
				this, minimax(-1,-1,-1,-1)
			),
			new Space(minimax(0,0,1E5,1E5)),
			0
		);
	
	AddChild( dynamic_cast<BView*>( vgroup));
}

/*------------------------------------------------------------------------------*\
	~BmSpamFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
BmSpamFilterPrefs::~BmSpamFilterPrefs() {
	TheBubbleHelper->SetHelp( mStatisticsButton, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilterPrefs::Initialize() {
	TheBubbleHelper->SetHelp( 
		mStatisticsButton, 
		""
	);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilterPrefs::Activate() {
}

/*------------------------------------------------------------------------------*\
	Kind()
		-	
\*------------------------------------------------------------------------------*/
const char* BmSpamFilterPrefs::Kind() const { 
	return FILTER_SPAM;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilterPrefs::MessageReceived( BMessage* msg) {
	switch( msg->what) {
		case BM_SHOW_STATISTICS: {
			break;
		}
		default:
			inherited::MessageReceived( msg);
	}
}

/*------------------------------------------------------------------------------*\
	ShowFilter()
		-	
\*------------------------------------------------------------------------------*/
void BmSpamFilterPrefs::ShowFilter( BmFilterAddon* addon) {
}



/*------------------------------------------------------------------------------*\
	InstantiateFilterPrefs()
		-	
\*------------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) 
BmFilterAddonPrefsView* InstantiateFilterPrefs( float minx, float miny,
																float maxx, float maxy,
																const BmString& kind) {
	if (!kind.ICompare( FILTER_SPAM))
		return new BmSpamFilterPrefs( minimax( minx, miny, maxx, maxy));
	else
		return NULL;
}

