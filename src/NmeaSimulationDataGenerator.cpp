#include "NmeaSimulationDataGenerator.h"
#include "NmeaAnalyzerSettings.h"

NmeaSimulationDataGenerator::NmeaSimulationDataGenerator()
{
}

NmeaSimulationDataGenerator::~NmeaSimulationDataGenerator()
{
}

void NmeaSimulationDataGenerator::Initialize( U32 simulation_sample_rate, NmeaAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mClockGenerator.Init( mSettings->mBitRate, simulation_sample_rate );
	mNmeaSimulationData.SetChannel( mSettings->mInputChannel );
	mNmeaSimulationData.SetSampleRate( simulation_sample_rate );


	mBitLow = BIT_LOW;
	mBitHigh = BIT_HIGH;

	mNmeaSimulationData.SetInitialBitState( mBitHigh );
	mNmeaSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

	mValue = 0;

	mMpModeAddressMask = 0;
	mMpModeDataMask = 0;
	mNumBitsMask = 0;

	U32 num_bits = mSettings->mBitsPerTransfer;
	for( U32 i = 0; i < num_bits; i++ )
	{
		mNumBitsMask <<= 1;
		mNumBitsMask |= 0x1;
	}
}

U32 NmeaSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mNmeaSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
	CreateNmeaByte( mValue++ );

	mNmeaSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}

	*simulation_channels = &mNmeaSimulationData;



	return 1;  // we are retuning the size of the SimulationChannelDescriptor array.  In our case, the "array" is length 1.
}

void NmeaSimulationDataGenerator::CreateNmeaByte( U64 value )
{
	//assume we start high

	mNmeaSimulationData.Transition();  //low-going edge for start bit
	mNmeaSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod() );  //add start bit time

	U32 num_bits = mSettings->mBitsPerTransfer;

	BitExtractor bit_extractor( value, AnalyzerEnums::ShiftOrder::LsbFirst, num_bits );

	for( U32 i=0; i<num_bits; i++ )
	{
		mNmeaSimulationData.TransitionIfNeeded( bit_extractor.GetNextBit() );
		mNmeaSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod() );
	}

	if( mSettings->mParity == AnalyzerEnums::Even )
	{

		if(AnalyzerHelpers::IsEven(AnalyzerHelpers::GetOnesCount(value ) ))
			mNmeaSimulationData.TransitionIfNeeded( mBitLow ); //we want to add a zero bit
		else
			mNmeaSimulationData.TransitionIfNeeded( mBitHigh ); //we want to add a one bit

		mNmeaSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod() );

	}else
	if( mSettings->mParity == AnalyzerEnums::Odd )
	{

		if(AnalyzerHelpers::IsOdd(AnalyzerHelpers::GetOnesCount(value ) ))
			mNmeaSimulationData.TransitionIfNeeded( mBitLow ); //we want to add a zero bit
		else
			mNmeaSimulationData.TransitionIfNeeded( mBitHigh );

		mNmeaSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod() );

	}
	
	mNmeaSimulationData.TransitionIfNeeded( mBitHigh ); //we need to end high

	//lets pad the end a bit for the stop bit:
	mNmeaSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( mSettings->mStopBits ) );
}
