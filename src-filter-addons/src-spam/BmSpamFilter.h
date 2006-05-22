/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmSpamFilter_h
#define _BmSpamFilter_h

#include <Archivable.h>
#include <Autolock.h>

#include <deque>

#include "BmFilterAddon.h"
#include "BmFilterAddonPrefs.h"

#include "BmMemIO.h"

/*------------------------------------------------------------------------------*\
	BmSpamFilter 
		-	implements filtering through SIEVE
\*------------------------------------------------------------------------------*/
class BmSpamFilter : public BmFilterAddon {
	typedef BmFilterAddon inherited;
	
	friend class SpamTest;

	/*------------------------------------------------------------------------------*\
		OsbfClassifier is a C++-"port" of a classifier contained in CRM114 
		(the Controllable	Regex Mutilator, a language for filtering spam, 
		see crm114.sourceforge.net).
		I have simply ripped out the OSBF classifier (OSBF stands for 'orthogonal 
		sparse bigram, Fidelis style') which is used to classify the mails that
		Beam sees. It is a statistical filter, not quite Bayesian, but very similar.
		[the OSBF-classifier is copyright by Fidelis Assis]
		[crm114 is copyright William S. Yerazunis]
		Thanks to these guys and everyone on the crm114-lists!
	\*------------------------------------------------------------------------------*/
	class OsbfClassifier
	{
	public:
		OsbfClassifier();
		~OsbfClassifier();
		void Initialize();
		const BmString& ErrorString() const;
		bool LearnAsSpam( BmMsgContext* msgContext);
		bool LearnAsTofu( BmMsgContext* msgContext);
		bool Classify( BmMsgContext* msgContext);
		bool Reset( BmMsgContext* msgContext);
		bool Reload( BmMsgContext* msgContext);
		bool ResetStatistics( BmMsgContext* msgContext);
		bool GetStatistics( BmMsgContext* msgContext);
		void JobSpecs( const BMessage* jobSpecs)
													{ mJobSpecs = jobSpecs; }
													
#ifndef __MWERKS__
	private:
#endif
		static const BMessage* mJobSpecs;

		typedef struct
		{
			uint32 GetRawValue() const		{ return value; }
			uint32 GetValue() const			{ return value & ValueMask; }
			uint32 GetHash() const			{ return hash; }
			uint32 GetKey() const			{ return key; }
			void SetValue( uint32 val)		{ value = val; }
			void SetHash( uint32 val)		{ hash = val; }
			void SetKey( uint32 val)		{ key = val; }
			bool IsLocked() const			{ return value & LockedMask; }
			void Lock()							{ value |= LockedMask; }
			void Unlock()						{ value &= ~LockedMask; }
			bool InChain() const				{ return (value & ValueMask) != 0; }
			bool HashCompare( uint32 h, uint32 k) const
													{ return hash==h && key==k; }
		private:
			static const uint32 ValueMask;
			static const uint32 LockedMask;

			uint32 hash;
			uint32 key;
			uint32 value;

		} FeatureBucket;
		
		typedef struct
		{
			unsigned char version[4];
			unsigned long buckets;		/* number of buckets in the file */
			unsigned long learnings;	/* number of trainings executed */
			unsigned long classifications;		/* number of classifications */
			unsigned long mistakes;		/* number of wrong classifications */
		} Header;
		
		////////////////////////////////////////////////////////////////////
		//
		//     the hash coefficient table should be full of relatively
		//     prime numbers, and preferably superincreasing, though both of 
		//     those are not strict requirements.
		//
		static const long HashCoeff[20];
		
		static float FeatureWeight[6];

		static unsigned char FileVersion[4];
		
		/* max feature value */
		static const unsigned long WindowLen;

		/* max feature value */
		static const unsigned long FeatureBucketValueMax;
		/* max number of features */
		static const unsigned long DefaultFileLength;
		
		/* shall we try to do small cleanups automatically, if the hash-chains
		   get too long? */ 
		static const bool DoMicrogroom;
		/* max chain len - microgrooming is triggered after this, if enabled */ 
		static const unsigned long MicrogroomChainLength;
		/* maximum number of buckets groom-zeroed */
		static const unsigned long MicrogroomStopAfter;
		/* minimum ratio between max and min P(F|C) */
		static const unsigned long MinPmaxPminRatio;
	
	public:
		static void Microgroom( FeatureBucket* hash, Header* header,
										unsigned long hindex);
	private:
		static void PackData( Header* header, FeatureBucket* hash,
									 unsigned long packstart, unsigned long packlen);
		static void PackDataSeg( Header* header, FeatureBucket* hash,
										 unsigned long packstart, unsigned long packlen);

		/*------------------------------------------------------------------------------*\
			SpamRelevantMailtextSelector
				-	selects the mail-parts that (seemingly) are of most
					relevance to spam-filtering
		\*------------------------------------------------------------------------------*/
		class SpamRelevantMailtextSelector
		{
			class HtmlRemover : public BmMemFilter {
				typedef BmMemFilter inherited;
			
			public:
				HtmlRemover( BmMemIBuf* input, uint32 blockSize=nBlockSize);
			
			protected:
				// overrides of BmMailFilter base:
				void Filter( const char* srcBuf, uint32& srcLen, 
								 char* destBuf, uint32& destLen);
		
			private:	
				bool mInTag;
				bool mInQuot;
				bool mKeepATags;
				bool mKeepThisTagsContent;
			};
			
		public:
			SpamRelevantMailtextSelector(BmMail* mail);
			void operator() (BmStringIBuf& inBuf);
		
		private:
			void AddSpamRelevantBodyParts(BmStringIBuf& inBuf);
			BmBodyPart* FindBodyPartWithHighestSpamRelevance(BmBodyPart* parent);
			BmRef<BmMail> mMail;
			BmStringOBuf mDeHtmlBuf;
		};
		
		/*------------------------------------------------------------------------------*\
			FeatureFilter
				-	filters the mailtext for "features" (words)
		\*------------------------------------------------------------------------------*/
		class FeatureFilter : public BmMemFilter {
			typedef BmMemFilter inherited;
		
		public:
			FeatureFilter( BmMemIBuf* input, uint32 blockSize=nBlockSize);
		
		protected:
			// overrides of BmMailFilter base:
			void Filter( const char* srcBuf, uint32& srcLen, 
							 char* destBuf, uint32& destLen);
		};
	


		/*------------------------------------------------------------------------------*\
			FeatureLearner
				-	implements the learning of features into the given feature-class
		\*------------------------------------------------------------------------------*/
		struct FeatureLearner : public BmMemBufConsumer::Functor {
			FeatureLearner( FeatureBucket* hash, Header* header, bool revert);
			~FeatureLearner();

			status_t operator() (char* buf, uint32 bufLen);

			void Finalize();

			FeatureBucket* mHash;
			Header* mHeader;
			bool mRevert;
			deque<unsigned long> mHashpipe;
			status_t mStatus;
		};



		/*------------------------------------------------------------------------------*\
			FeatureClassifier
				-	implements the classifying of features into one of two (or more)
					feature-classes
		\*------------------------------------------------------------------------------*/
		struct FeatureClassifier : public BmMemBufConsumer::Functor {
			FeatureClassifier( FeatureBucket* spamHash, Header* spamHeader,
									 FeatureBucket* tofuHash, Header* tofuHeader);
			~FeatureClassifier();

			status_t operator() (char* buf, uint32 bufLen);
			
			void Finalize();

			static const uint32 MaxHash = 2;
					// we only have to hashes: spam and tofu

			FeatureBucket* mHash[MaxHash];
			Header* mHeader[MaxHash];

			char *mSeenFeatures[MaxHash];

			deque<unsigned long> mHashpipe;
			status_t mStatus;
			
			unsigned long mHits[MaxHash];
				// actual hits per feature per classifier
			unsigned long mTotalHits[MaxHash];
				// actual total hits per classifier
			unsigned long mLearnings[MaxHash];
				// total learnings per classifier
			unsigned long mTotalLearnings;
			unsigned long mTotalFeatures;
				//  total features
			unsigned long mUniqueFeatures[MaxHash];
				//  found features per class
			unsigned long mMissedFeatures[MaxHash];
				//  missed features per class
			double mPtc[MaxHash];	
				// current running probability of this class
			double mPltc[MaxHash];
				// current local probability of this class
			double mOverallPr;
			unsigned long mHashLen[MaxHash];
			const char *mHashName[MaxHash];
		};



		bool Learn( BmMsgContext* msgContext, bool learnAsSpam, bool revert);
		bool DoActualClassification( BmMsgContext* msgContext, double& overallPr);
		void Store();
		status_t CreateDataFile( const BmString& filename);
		status_t ReadDataFile( const BmString& filename, Header& header,
									  FeatureBucket*& hash);
		status_t WriteDataFile( const BmString& filename, Header& header,
										FeatureBucket*& hash);
		
		Header mSpamHeader;
		FeatureBucket* mSpamHash;
		bool mNeedToStoreSpam;
		Header mTofuHeader;
		FeatureBucket* mTofuHash;
		bool mNeedToStoreTofu;
		BLocker mLock;
		BmString mLastErr;
	};

public:
	BmSpamFilter( const BmString& name, const BMessage* archive);
	virtual ~BmSpamFilter();
	
	// implementations for abstract BmFilterAddon-methods:
	bool Execute( BmMsgContext* msgContext, 
					  const BMessage* jobSpecs = NULL);
	bool SanityCheck( BmString& complaint, BmString& fieldName);
	status_t Archive( BMessage* archive, bool deep = true) const;
	BmString ErrorString() const;
	void Initialize()							{ nClassifier.Initialize(); }

	// getters:
	inline const BmString &Name() const	{ return mName; }

	// setters:

	// archivable components:
	static const char* const MSG_VERSION;
	static const char* const MSG_FILE_SPAM;
	static const char* const MSG_FILE_LEARNED_SPAM;
	static const char* const MSG_FILE_LEARNED_TOFU;
	static const char* const MSG_MARK_SPAM_AS_READ;
	static const char* const MSG_SPAM_THRESHOLD;
	static const char* const MSG_TOFU_THRESHOLD;
	static const char* const MSG_PROTECT_KNOWN;
	static const char* const MSG_FILE_UNSURE;
	static const char* const MSG_UNSURE_THRESHOLD;
	static const char* const MSG_DE_HTML;
	static const char* const MSG_KEEP_A_TAGS;
	static const int16 nArchiveVersion;

	struct Data {
		Data();
		bool mActionFileSpam;
		bool mActionFileLearnedSpam;
		bool mActionMarkSpamAsRead;
		bool mActionFileLearnedTofu;
		int8 mSpamThreshold;
		int8 mTofuThreshold;
		bool mProtectKnownAddrs;
		bool mActionFileUnsure;
		int8 mUnsureThreshold;
		bool mDeHtml;
		bool mKeepATags;
	};
	static Data D;

protected:
	BmString mName;
							// the name of this filter-implementation
	static OsbfClassifier nClassifier;
	
private:
	BmSpamFilter();									// hide default constructor
	// Hide copy-constructor and assignment:
	BmSpamFilter( const BmSpamFilter&);
	BmSpamFilter operator=( const BmSpamFilter&);

};



/*------------------------------------------------------------------------------*\
	BmSpamFilterPrefs
		-	
\*------------------------------------------------------------------------------*/

class BmCheckControl;
class MButton;
class MSlider;
class BStatusBar;

enum {
	BM_SHOW_STATISTICS			= 'bmTa',
	BM_FILESPAM_CHANGED			= 'bmTb',
	BM_FILELEARNEDSPAM_CHANGED	= 'bmTc',
	BM_FILELEARNEDTOFU_CHANGED	= 'bmTd',
	BM_THRESHOLD_CHANGED			= 'bmTe',
	BM_PROTECTMYTOFU_CHANGED	= 'bmTf',
	BM_RESET_STATISTICS			= 'bmTg',
	BM_MARKSPAM_CHANGED			= 'bmTh',
	BM_TRAIN_FILTER				= 'bmTi',
	BM_FILEUNSURESPAM_CHANGED	= 'bmTj',
	BM_UNSURE_THRESHOLD_CHANGED= 'bmTk',
	BM_PROTECTKNOWN_CHANGED		= 'bmTl'
};


class BmSpamFilterPrefs : public BmFilterAddonPrefsView {
	typedef BmFilterAddonPrefsView inherited;

public:
	BmSpamFilterPrefs( minimax minmax);
	virtual ~BmSpamFilterPrefs();
	
	// native methods:

	// implementations for abstract base-class methods:
	const char *Kind() const;
	void ShowFilter( BmFilterAddon* addon);
	void Initialize();
	void Activate();

	// BView overrides:
	void MessageReceived( BMessage* msg);

private:

	bool UpdateState( bool force);

	MButton* mShowStatisticsButton;
	MButton* mResetStatisticsButton;
	MButton* mTrainButton;
	MSlider* mThresholdControl;
	MSlider* mProtectMyTofuControl;
	MSlider* mUnsureThresholdControl;
	BStatusBar* mSpamThresholdBar;
	BStatusBar* mTofuThresholdBar;
	BmCheckControl* mFileSpamControl;
	BmCheckControl* mMarkSpamAsReadControl;
	BmCheckControl* mFileLearnedSpamControl;
	BmCheckControl* mFileLearnedTofuControl;
	BmCheckControl* mFileUnsureSpamControl;
	BmCheckControl* mProtectKnownAddrsControl;

	BmSpamFilter* mCurrFilterAddon;
	
	static int32 StartSpamometer( void* data);
	static bool nSpamometerRunning;

	// Hide copy-constructor and assignment:
	BmSpamFilterPrefs( const BmSpamFilterPrefs&);
	BmSpamFilterPrefs operator=( const BmSpamFilterPrefs&);
};



#endif
