#include "NmeaAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "NmeaAnalyzer.h"
#include "NmeaAnalyzerSettings.h"
#include <iostream>
#include <sstream>
#include <stdio.h>

NmeaAnalyzerResults::NmeaAnalyzerResults( NmeaAnalyzer* analyzer, NmeaAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

NmeaAnalyzerResults::~NmeaAnalyzerResults()
{
	/* Cleanup data backing pointer stored in mData1. */
	auto num_frames = GetNumFrames();
	for (auto frame_index = 0; frame_index < num_frames; frame_index++)
	{
		Frame frame = GetFrame(frame_index);
		auto type = static_cast<ResultType>(frame.mType);
		if (type == ResultType::SerialByte)
		{
			auto byte = reinterpret_cast<SerialByte*>(frame.mData1);
			delete byte;
		}
		else if (type == ResultType::NmeaData)
		{
			auto nmea = reinterpret_cast<NmeaData*>(frame.mData1);
			delete nmea;
		}
	}
}

void NmeaAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& /*channel*/, DisplayBase display_base )  //unrefereced vars commented out to remove warnings.
{
	U32 bits_per_transfer = mSettings->mBitsPerTransfer;

	//we only need to pay attention to 'channel' if we're making bubbles for more than one channel (as set by AddChannelBubblesWillAppearOn)
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	auto type = static_cast<ResultType>(frame.mType);

	if (type == ResultType::SerialByte)
	{
		auto byte = reinterpret_cast<SerialByte*>(frame.mData1);

		char number_str[128];
		AnalyzerHelpers::GetNumberString(byte->data, display_base, bits_per_transfer, number_str, 128);

		if (byte->error != SerialByte::SerialError::None)
		{
			char result_str[128];
			
			if (byte->error == SerialByte::SerialError::Framing)
				snprintf(result_str, sizeof(result_str), "%s (framing error)", number_str);
			else
				snprintf(result_str, sizeof(result_str), "%s (parity error)", number_str);

			AddResultString(result_str);
		}
		else
		{
			AddResultString(number_str);
		}
	}
	else if (type == ResultType::NmeaData)
	{
		auto nmea = reinterpret_cast<NmeaData*>(frame.mData1);

		AddResultString(nmea->nmea.c_str());
	}
}

void NmeaAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 /*export_type_user_id*/ )
{
	//export_type_user_id is only important if we have more than one export type.
	std::stringstream ss;

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();
	U64 num_frames = GetNumFrames();

	void* f = AnalyzerHelpers::StartFile( file );
	
	//Normal case -- not MP mode.
	ss << "Time [s],NMEA-0183" << std::endl;

	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
			
		auto type = static_cast<ResultType>(frame.mType);
		if (type == ResultType::NmeaData)
		{
			char time_str[128];
			AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);

			auto nmea = reinterpret_cast<NmeaData*>(frame.mData1);
			ss << time_str << "," << nmea->nmea;
			ss << std::endl;

			AnalyzerHelpers::AppendToFile((U8*)ss.str().c_str(), ss.str().length(), f);
			ss.str(std::string());
		}


		if(UpdateExportProgressAndCheckForCancel(i, num_frames ))
		{
			AnalyzerHelpers::EndFile( f );
			return;
		}
	}
	
	UpdateExportProgressAndCheckForCancel( num_frames, num_frames );
	AnalyzerHelpers::EndFile( f );
}

void NmeaAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	U32 bits_per_transfer = mSettings->mBitsPerTransfer;

	ClearTabularText();
	Frame frame = GetFrame(frame_index);

	auto type = static_cast<ResultType>(frame.mType);

	if (type == ResultType::SerialByte)
	{
	    // ... don't generate text for lone bytes, only for full NMEA strings ...
	}
	else if (type == ResultType::NmeaData)
	{
		auto nmea = reinterpret_cast<NmeaData*>(frame.mData1);

		AddTabularText(nmea->nmea.c_str());
	}
}

void NmeaAnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void NmeaAnalyzerResults::GenerateTransactionTabularText( U64 /*transaction_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}
