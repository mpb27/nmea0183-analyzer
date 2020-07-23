#include "NmeaAnalyzer.h"
#include "NmeaAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <bitset>


NmeaAnalyzer::NmeaAnalyzer()
:	Analyzer2(),  
	mSettings( new NmeaAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

NmeaAnalyzer::~NmeaAnalyzer()
{
	KillThread();
}

U32 NmeaAnalyzer::CalculateBitOffset()
{
	ClockGenerator clock_generator;
	clock_generator.Init( mSettings->mBitRate, GetSampleRate());
	return clock_generator.AdvanceByHalfPeriod();
}


void NmeaAnalyzer::SetupResults()
{
	//Unlike the worker thread, this function is called from the GUI thread
	//we need to reset the Results object here because it is exposed for direct access by the GUI, and it can't be deleted from the WorkerThread

	mResults.reset( new NmeaAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

SerialByte NmeaAnalyzer::SampleByte()
{
	auto ret = SerialByte{ 0, SerialByte::SerialError::None };

	// We're starting high - idle from previous byte.  (we'll assume that we're not in the middle of a byte). 
	mSerial->AdvanceToNextEdge();

	// We're now at the beginning of the start bit.  We can start collecting the data.
	ret.start_sample = mSerial->GetSampleNumber();

	// Advance into the center of the bit.
	mSerial->Advance(mBitOffset / 2);

	// Reconfirm that the start bit it still asserted.
	if (mSerial->GetBitState() != BitState::BIT_LOW)
	{
		ret.error = SerialByte::SerialError::Framing;
		return ret;
	}

	// Advance 1 bit at a time and sample the data bits.
	U32 data = 0;
	U32 num_bits = mSettings->mBitsPerTransfer;
	for (U32 i = 0; i < num_bits; i++)
	{
		mSerial->Advance(mBitOffset);
		U32 bit_value = mSerial->GetBitState() == BitState::BIT_LOW ? 0 : 1;
		data |= (bit_value << i);

		//if (mAddMarkersForBits) {
		//	mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel);
		//}
	}
	ret.data = static_cast<unsigned char>(data);

	// Handle parity bit if present.
	if (mSettings->mParity != AnalyzerEnums::None)
	{
		// Compute the parity of data and the parity bit by adding parity bit to the data.
		mSerial->Advance(mBitOffset);
		U32 bit_value = mSerial->GetBitState() == BitState::BIT_LOW ? 0 : 1;
		data |= (bit_value << num_bits);
		bool is_odd = std::bitset<32>(data).count() % 2 == 1;

		if (mSettings->mParity == AnalyzerEnums::Even && is_odd)
		{
			// Parity error:  For even parity, we expect {data,parity} to be even.
			ret.error = SerialByte::SerialError::Parity;
		}
		else if (mSettings->mParity == AnalyzerEnums::Odd && !is_odd)
		{
			// Parity error:  For odd parity, we expect {data,parity} to be odd.
			ret.error = SerialByte::SerialError::Parity;
		}

		//if (mAddMarkersForBits) {
		//	mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Square, mSettings->mInputChannel);
		//}		
	}

	// Handle the stop bit.
	// Move 1/2 bit time into the stop bit, and make sure the stop bit remains high from there to 0.5 bit times before the end of the top bit.
	// | STA |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  | PAR | STP |
	//                                                                ^      (for 1 stop bit make sure its high just at the center of the stop bit)
	//                                                                ^^     (for 1.5 stop bits make sure its high for 0.5 bit time)
	//                                                                ^^^    (for 2.0 stop bits make sure its high for 1 bit time)
	mSerial->Advance(mBitOffset);
	if (mSerial->GetBitState() == BitState::BIT_LOW)
	{
		// Framing error: Stop bit isn't high!
		ret.error = SerialByte::SerialError::Framing;

		//if (mAddMarkersForBits) {
		//	mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mInputChannel);
		//}		
	}

	U32 endOfStopBitOffset = static_cast<U32>(mBitOffset * (mSettings->mStopBits - 1.0) + 0.5);
	U32 num_edges = mSerial->Advance(endOfStopBitOffset);
	if (num_edges > 0)  // If num_edges is not zero then the stop bit changed to a low when it shouldn't have.
	{
		// Framing error: Stop bit changed state before it ends.
		ret.error = SerialByte::SerialError::Framing;

		//if (mAddMarkersForBits) {
		//	mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mInputChannel);
		//}		
	}

	// Record the end of the byte.
	ret.end_sample = mSerial->GetSampleNumber();

	// If there was a framing error advance until the line goes high.
	if (mSerial->GetBitState() == BitState::BIT_LOW)
		mSerial->AdvanceToNextEdge();

	return ret;
}

[[noreturn]] void NmeaAnalyzer::WorkerThread()
{
	mBitOffset = CalculateBitOffset();	
	mSerial = GetAnalyzerChannelData(mSettings->mInputChannel);
	mSerial->TrackMinimumPulseWidth(); // Tracks minimum pulse width for autobaudrate feature.
	
	if( mSerial->GetBitState() == BIT_LOW )
		mSerial->AdvanceToNextEdge();

	enum class State { FindStart, RecvSentence, RecvCrc1, RecvCrc2 };

	State state = State::FindStart;
	auto nmea_data = std::unique_ptr<NmeaData>();

	auto create_byte_frame = [](const SerialByte& byte) {
		auto frame = Frame();
		frame.mStartingSampleInclusive = byte.start_sample;
		frame.mEndingSampleInclusive = byte.end_sample;
		frame.mType = static_cast<U8>(ResultType::SerialByte);
		frame.mData1 = reinterpret_cast<U64>(new SerialByte(byte));
		return frame;
	};




	for( ; ; )
	{
		auto data = this->SampleByte();

		if (data.error != SerialByte::SerialError::None)
		{
			if (state != State::FindStart)
			{
				// ... complete active data / partial sentence ...
				auto incomplete_frame = Frame();
				incomplete_frame.mStartingSampleInclusive = nmea_data->start_sample;
				incomplete_frame.mEndingSampleInclusive = nmea_data->end_sample;
				incomplete_frame.mType = static_cast<U8>(ResultType::NmeaData);
				incomplete_frame.mData1 = reinterpret_cast<U64>(nmea_data.release());
				mResults->AddFrame(incomplete_frame);
			}

			auto error_frame = create_byte_frame(data);			
			mResults->AddFrame(error_frame);
			mResults->CommitResults();
		}
		else
		{
			if (state == State::RecvSentence)
			{
				nmea_data->nmea.push_back(data.data);
				nmea_data->end_sample = data.end_sample;
				if (data.data == '*') 
					state = State::RecvCrc1;				
			}
			else if (state == State::RecvCrc1)
			{
				nmea_data->nmea.push_back(data.data);
				nmea_data->end_sample = data.end_sample;
				state = State::RecvCrc2;
			}
			else if (state == State::RecvCrc2)
			{
				nmea_data->nmea.push_back(data.data);
				nmea_data->end_sample = data.end_sample;
				auto complete_frame = Frame();				
				complete_frame.mStartingSampleInclusive = nmea_data->start_sample;
				complete_frame.mEndingSampleInclusive = nmea_data->end_sample;
				complete_frame.mType = static_cast<U8>(ResultType::NmeaData);
				complete_frame.mData1 = reinterpret_cast<U64>(nmea_data.release());
				mResults->AddFrame(complete_frame);
				mResults->CommitResults();
				state = State::FindStart;
			}
			else if (state == State::FindStart)
			{
				if (data.data == '$')
				{
					nmea_data = std::make_unique<NmeaData>();
					nmea_data->nmea.push_back(data.data);
					nmea_data->start_sample = data.start_sample;
					nmea_data->end_sample = data.end_sample;
					state = State::RecvSentence;
				}
				else
				{
					auto byte_frame = create_byte_frame(data);
					mResults->AddFrame(byte_frame);
					mResults->CommitResults();
				}
			}
		}

		

		//mResults->CommitResults();

		ReportProgress( mSerial->GetSampleNumber() );
		CheckIfThreadShouldExit();  // not sure how the thread exists ? forced ? but it can leak one NmeaData since the unique_ptr might not get cleared up.

		if(data.error == SerialByte::SerialError::Framing)  //if we're still low, let's fix that for the next round. // this is also done in SampleByte
		{
			if( mSerial->GetBitState() == BIT_LOW )
				mSerial->AdvanceToNextEdge();
		}
	}
}

bool NmeaAnalyzer::NeedsRerun()
{
	if(!mSettings->mUseAutobaud)
		return false;

	//ok, lets see if we should change the bit rate, base on mShortestActivePulse

	U64 shortest_pulse = mSerial->GetMinimumPulseWidthSoFar();

	if( shortest_pulse == 0 )
		AnalyzerHelpers::Assert( "Alg problem, shortest_pulse was 0" );

	U32 computed_bit_rate = U32( double( GetSampleRate() ) / double( shortest_pulse ) );

	if( computed_bit_rate > GetSampleRate() )
		AnalyzerHelpers::Assert( "Alg problem, computed_bit_rate is higer than sample rate" );  //just checking the obvious...

	if( computed_bit_rate > (GetSampleRate() / 4) )
		return false; //the baud rate is too fast.
	if( computed_bit_rate == 0 )
	{
		//bad result, this is not good data, don't bother to re-run.
		return false;
	}

	U32 specified_bit_rate = mSettings->mBitRate;

	double error = double( AnalyzerHelpers::Diff32( computed_bit_rate, specified_bit_rate ) ) / double( specified_bit_rate );

	if( error > 0.1 )
	{
		mSettings->mBitRate = computed_bit_rate;
		mSettings->UpdateInterfacesFromSettings();
		return true;
	}else
	{
		return false;
	}
}

U32 NmeaAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if(!mSimulationInitilized)
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 NmeaAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings->mBitRate * 4;
}

const char* NmeaAnalyzer::GetAnalyzerName() const
{
	return "NMEA-0183";
}

const char* GetAnalyzerName()
{
	return "NMEA-0183";
}

Analyzer* CreateAnalyzer()
{
	return new NmeaAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
